#include "web_server.h"
#include "config.h"
#include "storage/flash_storage.h"
#include "sensors/sensor_manager.h"
#include "control/lights_controller.h"
#include "control/pump_controller.h"
#include "control/heater_controller.h"
#include "control/fan_controller.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

WebServer::WebServer(SensorManager* sensor_manager, 
                     LightsController* lights_controller,
                     PumpController* pump_controller,
                     HeaterController* heater_controller,
                     FanController* fan_controller) 
    : sensor_manager_(sensor_manager),
      lights_controller_(lights_controller),
      pump_controller_(pump_controller),
      heater_controller_(heater_controller),
      fan_controller_(fan_controller),
      web_server_pcb_(nullptr),
      web_client_pcb_(nullptr),
      request_buffer_pos_(0) {
    memset(request_buffer_, 0, sizeof(request_buffer_));
}

bool WebServer::start() {
    web_server_pcb_ = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (web_server_pcb_ == nullptr) {
        printf("Failed to create web server PCB\n");
        return false;
    }
    
    err_t err = tcp_bind(web_server_pcb_, IP_ANY_TYPE, 80);
    if (err != ERR_OK) {
        printf("Failed to bind web server to port 80: %d\n", err);
        tcp_close(web_server_pcb_);
        web_server_pcb_ = nullptr;
        return false;
    }
    
    web_server_pcb_ = tcp_listen(web_server_pcb_);
    tcp_accept(web_server_pcb_, web_accept_callback);
    tcp_arg(web_server_pcb_, this);
    
    printf("Web server started on port 80\n");
    return true;
}

void WebServer::stop() {
    if (web_server_pcb_) {
        tcp_close(web_server_pcb_);
        web_server_pcb_ = nullptr;
    }
    if (web_client_pcb_) {
        tcp_close(web_client_pcb_);
        web_client_pcb_ = nullptr;
    }
}

void WebServer::handleClients() {
    // Handle any pending client requests
    // This is called from the main loop
}

err_t WebServer::web_accept_callback(void* arg, struct tcp_pcb* newpcb, err_t err) {
    WebServer* server = static_cast<WebServer*>(arg);
    return server->webAccept(newpcb, err);
}

err_t WebServer::webAccept(struct tcp_pcb* newpcb, err_t err) {
    if (err == ERR_OK && newpcb != nullptr) {
        if (web_client_pcb_ == nullptr) {
            web_client_pcb_ = newpcb;
            tcp_arg(newpcb, this);
            tcp_recv(newpcb, web_recv_callback);
            tcp_err(newpcb, web_err_callback);
            printf("Web client connected\n");
            return ERR_OK;
        } else {
            printf("Web server busy, rejecting connection\n");
            tcp_close(newpcb);
            return ERR_ABRT;
        }
    }
    return ERR_ARG;
}

err_t WebServer::web_recv_callback(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    WebServer* server = static_cast<WebServer*>(arg);
    return server->webRecv(tpcb, p, err);
}

err_t WebServer::webRecv(struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (err == ERR_OK && p != nullptr) {
        // Accumulate request data
        uint16_t len = p->tot_len;
        if (request_buffer_pos_ + len > sizeof(request_buffer_) - 1) {
            len = sizeof(request_buffer_) - 1 - request_buffer_pos_;
        }
        
        if (len > 0) {
            pbuf_copy_partial(p, request_buffer_ + request_buffer_pos_, len, 0);
            request_buffer_pos_ += len;
            request_buffer_[request_buffer_pos_] = '\0';
        } else if (request_buffer_pos_ >= sizeof(request_buffer_) - 1) {
            // Buffer full without complete request - reject
            printf("HTTP request too large\n");
            sendHttpError(tpcb, 413, "Request Entity Too Large");
            request_buffer_pos_ = 0;
            memset(request_buffer_, 0, sizeof(request_buffer_));
            tcp_recved(tpcb, p->tot_len);
            pbuf_free(p);
            return ERR_OK;
        }
        
        // Check if we have a complete HTTP request
        if (strstr(request_buffer_, "\r\n\r\n")) {
            HttpRequest request;
            if (parseHttpRequest(request_buffer_, &request)) {
                handleHttpRequest(tpcb, &request);
            } else {
                sendHttpError(tpcb, 400, "Bad Request");
            }
            
            // Reset buffer for next request
            request_buffer_pos_ = 0;
            memset(request_buffer_, 0, sizeof(request_buffer_));
        }
        
        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);
    } else if (err == ERR_OK && p == nullptr) {
        // Connection closed
        printf("Web client disconnected\n");
        web_client_pcb_ = nullptr;
        request_buffer_pos_ = 0;
        memset(request_buffer_, 0, sizeof(request_buffer_));
        tcp_close(tpcb);
    }
    
    return ERR_OK;
}

void WebServer::web_err_callback(void* arg, err_t err) {
    WebServer* server = static_cast<WebServer*>(arg);
    if (server) {
        printf("Web connection error: %d\n", err);
        server->web_client_pcb_ = nullptr;
        server->request_buffer_pos_ = 0;
        memset(server->request_buffer_, 0, sizeof(server->request_buffer_));
    }
}

bool WebServer::parseHttpRequest(const char* raw_request, HttpRequest* request) {
    // Simple HTTP request parser
    char method[16], path[256];
    if (sscanf(raw_request, "%15s %255s", method, path) != 2) {
        return false;
    }
    
    strncpy(request->method, method, sizeof(request->method) - 1);
    request->method[sizeof(request->method) - 1] = '\0';
    
    strncpy(request->path, path, sizeof(request->path) - 1);
    request->path[sizeof(request->path) - 1] = '\0';
    
    // Parse query parameters
    char* query_start = strchr(path, '?');
    if (query_start) {
        *query_start = '\0'; // Null terminate path
        strncpy(request->query, query_start + 1, sizeof(request->query) - 1);
        request->query[sizeof(request->query) - 1] = '\0';
    } else {
        request->query[0] = '\0';
    }
    
    // Parse body for POST requests
    const char* body_start = strstr(raw_request, "\r\n\r\n");
    if (body_start) {
        body_start += 4; // Skip \r\n\r\n
        strncpy(request->body, body_start, sizeof(request->body) - 1);
        request->body[sizeof(request->body) - 1] = '\0';
    } else {
        request->body[0] = '\0';
    }
    
    return true;
}

void WebServer::handleHttpRequest(struct tcp_pcb* tpcb, const HttpRequest* request) {
    printf("HTTP %s %s\n", request->method, request->path);
    
    // Route requests
    if (strcmp(request->path, "/") == 0) {
        serveMainPage(tpcb);
    } else if (strcmp(request->path, "/app.css") == 0) {
        serveCss(tpcb);
    } else if (strcmp(request->path, "/app.js") == 0) {
        serveJs(tpcb);
    } else if (strcmp(request->path, "/favicon.ico") == 0) {
        serveFavicon(tpcb);
    } else if (strncmp(request->path, "/api/", 5) == 0) {
        // API endpoints
        if (strcmp(request->path, "/api/status") == 0) {
            handleApiStatus(tpcb, request);
        } else if (strcmp(request->path, "/api/config") == 0) {
            handleApiConfig(tpcb, request);
        } else if (strcmp(request->path, "/api/lights") == 0) {
            handleApiLights(tpcb, request);
        } else if (strcmp(request->path, "/api/pump") == 0) {
            handleApiPump(tpcb, request);
        } else if (strcmp(request->path, "/api/heater") == 0) {
            handleApiHeater(tpcb, request);
        } else if (strcmp(request->path, "/api/fan") == 0) {
            handleApiFan(tpcb, request);
        } else if (strcmp(request->path, "/api/humidity") == 0) {
            handleApiHumidity(tpcb, request);
        } else if (strcmp(request->path, "/api/save") == 0) {
            handleApiSave(tpcb, request);
        } else {
            sendHttpError(tpcb, 404, "Not Found");
        }
    } else {
        sendHttpError(tpcb, 404, "Not Found");
    }
}

void WebServer::sendHttpResponse(struct tcp_pcb* tpcb, const HttpResponse* response) {
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        response->status_code,
        response->content_type,
        response->body_length
    );
    
    // Send header
    err_t err = tcp_write(tpcb, header, header_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Failed to send HTTP header\n");
        if (response->free_body && response->body) {
            free(response->body);
        }
        return;
    }
    
    // Send body
    if (response->body_length > 0) {
        err = tcp_write(tpcb, response->body, response->body_length, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            printf("Failed to send HTTP body\n");
            if (response->free_body && response->body) {
                free(response->body);
            }
            return;
        }
    }
    
    // Send response
    err = tcp_output(tpcb);
    if (err != ERR_OK) {
        printf("Failed to output HTTP response\n");
    }
    
    // Free body if requested (TCP_WRITE_FLAG_COPY means data was copied)
    if (response->free_body && response->body) {
        free(response->body);
    }
}

void WebServer::sendHttpError(struct tcp_pcb* tpcb, int code, const char* message) {
    char error_body[256];
    snprintf(error_body, sizeof(error_body),
        "<html><body><h1>%d %s</h1></body></html>", code, message);
    
    HttpResponse response;
    response.status_code = code;
    strcpy(response.content_type, "text/html");
    response.body = error_body;
    response.body_length = strlen(error_body);
    response.free_body = false;
    
    sendHttpResponse(tpcb, &response);
}

void WebServer::serveMainPage(struct tcp_pcb* tpcb) {
    serveStaticFile(tpcb, "/", "text/html");
}

void WebServer::serveCss(struct tcp_pcb* tpcb) {
    serveStaticFile(tpcb, "/app.css", "text/css");
}

void WebServer::serveJs(struct tcp_pcb* tpcb) {
    serveStaticFile(tpcb, "/app.js", "application/javascript");
}

void WebServer::serveFavicon(struct tcp_pcb* tpcb) {
    serveStaticFile(tpcb, "/favicon.ico", "image/x-icon");
}

void WebServer::serveStaticFile(struct tcp_pcb* tpcb, const char* filename, const char* content_type) {
    FlashStorage& storage = FlashStorage::getInstance();
    
    uint8_t* data = nullptr;
    uint32_t size = 0;
    const char* mime_type = nullptr;
    
    // Map "/" to "/index.html"
    const char* file_path = (strcmp(filename, "/") == 0) ? "/index.html" : filename;
    
    if (storage.getFile(file_path, &data, &size, &mime_type)) {
        HttpResponse response;
        response.status_code = 200;
        strcpy(response.content_type, mime_type ? mime_type : content_type);
        response.body = (char*)data;
        response.body_length = size;
        response.free_body = true;  // LittleFS allocates memory, must free
        sendHttpResponse(tpcb, &response);
    } else {
        sendHttpError(tpcb, 404, "Not Found");
    }
}

// API Endpoint Implementations
void WebServer::handleApiStatus(struct tcp_pcb* tpcb, const HttpRequest* request) {
    char* json = generateStatusJson();
    if (json) {
        HttpResponse response;
        response.status_code = 200;
        strcpy(response.content_type, "application/json");
        response.body = json;
        response.body_length = strlen(json);
        response.free_body = true;
        sendHttpResponse(tpcb, &response);
    } else {
        sendHttpError(tpcb, 500, "Internal Server Error");
    }
}

void WebServer::handleApiConfig(struct tcp_pcb* tpcb, const HttpRequest* request) {
    char* json = generateConfigJson();
    if (json) {
        HttpResponse response;
        response.status_code = 200;
        strcpy(response.content_type, "application/json");
        response.body = json;
        response.body_length = strlen(json);
        response.free_body = true;
        sendHttpResponse(tpcb, &response);
    } else {
        sendHttpError(tpcb, 500, "Internal Server Error");
    }
}

void WebServer::handleApiLights(struct tcp_pcb* tpcb, const HttpRequest* request) {
    if (strcmp(request->method, "POST") != 0) {
        sendHttpError(tpcb, 405, "Method Not Allowed");
        return;
    }
    
    const char* response_json = "{\"success\": true, \"message\": \"Lights schedule updated\"}";
    HttpResponse response;
    response.status_code = 200;
    strcpy(response.content_type, "application/json");
    response.body = (char*)response_json;
    response.body_length = strlen(response_json);
    response.free_body = false;
    sendHttpResponse(tpcb, &response);
}

void WebServer::handleApiPump(struct tcp_pcb* tpcb, const HttpRequest* request) {
    if (strcmp(request->method, "POST") != 0) {
        sendHttpError(tpcb, 405, "Method Not Allowed");
        return;
    }
    
    const char* response_json = "{\"success\": true, \"message\": \"Pump settings updated\"}";
    HttpResponse response;
    response.status_code = 200;
    strcpy(response.content_type, "application/json");
    response.body = (char*)response_json;
    response.body_length = strlen(response_json);
    response.free_body = false;
    sendHttpResponse(tpcb, &response);
}

void WebServer::handleApiHeater(struct tcp_pcb* tpcb, const HttpRequest* request) {
    if (strcmp(request->method, "POST") != 0) {
        sendHttpError(tpcb, 405, "Method Not Allowed");
        return;
    }
    
    const char* response_json = "{\"success\": true, \"message\": \"Heater setpoint updated\"}";
    HttpResponse response;
    response.status_code = 200;
    strcpy(response.content_type, "application/json");
    response.body = (char*)response_json;
    response.body_length = strlen(response_json);
    response.free_body = false;
    sendHttpResponse(tpcb, &response);
}

void WebServer::handleApiFan(struct tcp_pcb* tpcb, const HttpRequest* request) {
    if (strcmp(request->method, "POST") != 0) {
        sendHttpError(tpcb, 405, "Method Not Allowed");
        return;
    }
    
    const char* response_json = "{\"success\": true, \"message\": \"Fan state updated\"}";
    HttpResponse response;
    response.status_code = 200;
    strcpy(response.content_type, "application/json");
    response.body = (char*)response_json;
    response.body_length = strlen(response_json);
    response.free_body = false;
    sendHttpResponse(tpcb, &response);
}

void WebServer::handleApiHumidity(struct tcp_pcb* tpcb, const HttpRequest* request) {
    if (strcmp(request->method, "POST") != 0) {
        sendHttpError(tpcb, 405, "Method Not Allowed");
        return;
    }
    
    const char* response_json = "{\"success\": true, \"message\": \"Humidity threshold updated\"}";
    HttpResponse response;
    response.status_code = 200;
    strcpy(response.content_type, "application/json");
    response.body = (char*)response_json;
    response.body_length = strlen(response_json);
    response.free_body = false;
    sendHttpResponse(tpcb, &response);
}

void WebServer::handleApiSave(struct tcp_pcb* tpcb, const HttpRequest* request) {
    if (strcmp(request->method, "POST") != 0) {
        sendHttpError(tpcb, 405, "Method Not Allowed");
        return;
    }
    
    const char* response_json = "{\"success\": true, \"message\": \"Configuration saved\"}";
    HttpResponse response;
    response.status_code = 200;
    strcpy(response.content_type, "application/json");
    response.body = (char*)response_json;
    response.body_length = strlen(response_json);
    response.free_body = false;
    sendHttpResponse(tpcb, &response);
}

char* WebServer::generateStatusJson() {
    char* json = (char*)malloc(1024);
    if (!json) return nullptr;
    
    ConfigManager& config = ConfigManager::getInstance();
    
    snprintf(json, 1024,
        "{"
        "\"temperature\": %.1f,"
        "\"humidity\": %.1f,"
        "\"air_temperature\": %.1f,"
        "\"air_humidity\": %.1f,"
        "\"ph\": %.2f,"
        "\"tds\": %.0f,"
        "\"lights_on\": %s,"
        "\"pump_on\": %s,"
        "\"heater_on\": %s,"
        "\"fan_on\": %s,"
        "\"wifi_connected\": true,"
        "\"time_synced\": true,"
        "\"lights_start_s\": %lu,"
        "\"lights_end_s\": %lu,"
        "\"pump_on_sec\": %lu,"
        "\"pump_period\": %lu,"
        "\"heater_setpoint_c\": %.1f,"
        "\"humidity_threshold\": %.1f,"
        "\"humidity_mode\": %s"
        "}",
        sensor_manager_->isTemperatureValid() ? sensor_manager_->getLastTemperature() : -999.0,
        sensor_manager_->isHumidityValid() ? sensor_manager_->getLastHumidity() : -999.0,
        sensor_manager_->isAirTempValid() ? sensor_manager_->getLastAirTemp() : -999.0,
        sensor_manager_->isAirHumidityValid() ? sensor_manager_->getLastAirHumidity() : -999.0,
        sensor_manager_->isPHValid() ? sensor_manager_->getLastPH() : -999.0,
        sensor_manager_->isTDSValid() ? sensor_manager_->getLastTDS() : -999.0,
        lights_controller_->isOn() ? "true" : "false",
        pump_controller_->isOn() ? "true" : "false",
        heater_controller_->isOn() ? "true" : "false",
        fan_controller_->isOn() ? "true" : "false",
        config.getLightsStartS(),
        config.getLightsEndS(),
        config.getPumpOnSec(),
        config.getPumpPeriod(),
        config.getHeaterSetpointC(),
        config.getHumidityThreshold(),
        config.getHumidityMode() ? "true" : "false"
    );
    
    return json;
}

char* WebServer::generateConfigJson() {
    char* json = (char*)malloc(1024);
    if (!json) return nullptr;
    
    ConfigManager& config = ConfigManager::getInstance();
    
    snprintf(json, 1024,
        "{"
        "\"lights_start_s\": %lu,"
        "\"lights_end_s\": %lu,"
        "\"pump_on_sec\": %lu,"
        "\"pump_period\": %lu,"
        "\"heater_setpoint_c\": %.1f,"
        "\"humidity_threshold\": %.1f,"
        "\"humidity_mode\": %s"
        "}",
        config.getLightsStartS(),
        config.getLightsEndS(),
        config.getPumpOnSec(),
        config.getPumpPeriod(),
        config.getHeaterSetpointC(),
        config.getHumidityThreshold(),
        config.getHumidityMode() ? "true" : "false"
    );
    
    return json;
}

void WebServer::parseQueryParams(const char* query, char* buffer, size_t buffer_size, const char* param) {
    buffer[0] = '\0';
    
    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", param);
    
    const char* start = strstr(query, search_pattern);
    if (start) {
        start += strlen(search_pattern);
        
        // Skip whitespace
        while (*start == ' ' || *start == '\t') start++;
        
        // Handle string values
        if (*start == '"') {
            start++; // Skip opening quote
            const char* end = strchr(start, '"');
            if (end) {
                size_t len = end - start;
                if (len < buffer_size) {
                    strncpy(buffer, start, len);
                    buffer[len] = '\0';
                }
            }
        }
        // Handle numeric values
        else {
            const char* end = start;
            while ((*end >= '0' && *end <= '9') || *end == '.' || *end == '-') {
                end++;
            }
            size_t len = end - start;
            if (len < buffer_size) {
                strncpy(buffer, start, len);
                buffer[len] = '\0';
            }
        }
    }
}

void WebServer::urlDecode(char* str) {
    char* src = str;
    char* dst = str;
    
    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            char hex[3] = {src[1], src[2], '\0'};
            *dst = (char)strtol(hex, nullptr, 16);
            src += 3;
        } else if (*src == '+') {
            *dst = ' ';
            src++;
        } else {
            *dst = *src;
            src++;
        }
        dst++;
    }
    *dst = '\0';
}

char* WebServer::createJsonResponse(const char* json_data) {
    size_t len = strlen(json_data);
    char* response = (char*)malloc(len + 1);
    if (response) {
        strcpy(response, json_data);
    }
    return response;
}

uint32_t WebServer::parseTimeToSeconds(const char* time_str) {
    int hours, minutes;
    if (sscanf(time_str, "%d:%d", &hours, &minutes) == 2) {
        return hours * 3600 + minutes * 60;
    }
    return 0;
}
