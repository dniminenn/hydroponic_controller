#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lwip/tcp.h"
#include "lwip/pbuf.h"

class SensorManager;
class LightsController;
class PumpController;
class HeaterController;
class FanController;

// HTTP request structure
struct HttpRequest {
    char method[8];
    char path[128];
    char query[256];
    char body[512];
    char content_type[64];
    uint16_t content_length;
};

// HTTP response structure
struct HttpResponse {
    int status_code;
    char content_type[64];
    char* body;
    size_t body_length;
    bool free_body;
};

class WebServer {
public:
    WebServer(SensorManager* sensor_manager, 
              LightsController* lights_controller,
              PumpController* pump_controller,
              HeaterController* heater_controller,
              FanController* fan_controller);
    
    bool start();
    void stop();
    void handleClients();
    
private:
    // Web server callbacks
    static err_t web_accept_callback(void* arg, struct tcp_pcb* newpcb, err_t err);
    static err_t web_recv_callback(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
    static void web_err_callback(void* arg, err_t err);
    
    err_t webAccept(struct tcp_pcb* newpcb, err_t err);
    err_t webRecv(struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
    void webErr(err_t err);
    
    // HTTP parsing and handling
    bool parseHttpRequest(const char* raw_request, HttpRequest* request);
    void handleHttpRequest(struct tcp_pcb* tpcb, const HttpRequest* request);
    void sendHttpResponse(struct tcp_pcb* tpcb, const HttpResponse* response);
    void sendHttpError(struct tcp_pcb* tpcb, int code, const char* message);
    
    // API endpoints
    void handleApiStatus(struct tcp_pcb* tpcb, const HttpRequest* request);
    void handleApiConfig(struct tcp_pcb* tpcb, const HttpRequest* request);
    void handleApiLights(struct tcp_pcb* tpcb, const HttpRequest* request);
    void handleApiPump(struct tcp_pcb* tpcb, const HttpRequest* request);
    void handleApiHeater(struct tcp_pcb* tpcb, const HttpRequest* request);
    void handleApiFan(struct tcp_pcb* tpcb, const HttpRequest* request);
    void handleApiHumidity(struct tcp_pcb* tpcb, const HttpRequest* request);
    void handleApiSave(struct tcp_pcb* tpcb, const HttpRequest* request);
    
    // Static file serving
    void serveStaticFile(struct tcp_pcb* tpcb, const char* filename, const char* content_type);
    void serveMainPage(struct tcp_pcb* tpcb);
    void serveCss(struct tcp_pcb* tpcb);
    void serveJs(struct tcp_pcb* tpcb);
    void serveFavicon(struct tcp_pcb* tpcb);
    
    // JSON generation
    char* generateStatusJson();
    char* generateConfigJson();
    
    // Utility functions
    void parseQueryParams(const char* query, char* buffer, size_t buffer_size, const char* param);
    void urlDecode(char* str);
    char* createJsonResponse(const char* json_data);
    uint32_t parseTimeToSeconds(const char* time_str);
    
    // Component references
    SensorManager* sensor_manager_;
    LightsController* lights_controller_;
    PumpController* pump_controller_;
    HeaterController* heater_controller_;
    FanController* fan_controller_;
    
    // Web server state
    struct tcp_pcb* web_server_pcb_;
    struct tcp_pcb* web_client_pcb_;
    
    // Request buffer for multi-packet requests
    char request_buffer_[2048];
    size_t request_buffer_pos_;
};
