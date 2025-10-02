#include "tcp_server.h"
#include "../config.h"
#include "../utils/time_utils.h"
#include "../sensors/sensor_manager.h"
#include "../control/lights_controller.h"
#include "../control/pump_controller.h"
#include "../control/heater_controller.h"
#include "../control/fan_controller.h"
#include "../storage/flash_storage.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

TcpServer::TcpServer(SensorManager* sensor_manager, 
                     LightsController* lights_controller,
                     PumpController* pump_controller,
                     HeaterController* heater_controller,
                     FanController* fan_controller)
    : sensor_manager_(sensor_manager),
      lights_controller_(lights_controller),
      pump_controller_(pump_controller),
      heater_controller_(heater_controller),
      fan_controller_(fan_controller),
      tcp_server_pcb_(nullptr),
      tcp_client_pcb_(nullptr),
      tcp_command_len_(0),
      upload_in_progress_(false),
      upload_size_(0),
      upload_received_(0),
      upload_buffer_(nullptr) {
}

TcpServer::~TcpServer() {
    stop();
    if (upload_buffer_) {
        free(upload_buffer_);
        upload_buffer_ = nullptr;
    }
}

bool TcpServer::start() {
    if (tcp_server_pcb_) return true;
    
    tcp_server_pcb_ = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!tcp_server_pcb_) {
        printf("Failed to create TCP PCB\n");
        return false;
    }
    
    err_t err = tcp_bind(tcp_server_pcb_, IP_ANY_TYPE, TCP_PORT);
    if (err != ERR_OK) {
        printf("Failed to bind TCP server: %d\n", err);
        tcp_close(tcp_server_pcb_);
        tcp_server_pcb_ = nullptr;
        return false;
    }
    
    tcp_server_pcb_ = tcp_listen(tcp_server_pcb_);
    tcp_accept(tcp_server_pcb_, tcp_accept_callback);
    
    printf("TCP server started on port %d\n", TCP_PORT);
    return true;
}

void TcpServer::stop() {
    if (tcp_server_pcb_) {
        tcp_close(tcp_server_pcb_);
        tcp_server_pcb_ = nullptr;
    }
    if (tcp_client_pcb_) {
        tcp_close(tcp_client_pcb_);
        tcp_client_pcb_ = nullptr;
    }
}

void TcpServer::handleClients() {
    // TCP handling is done in callbacks
}

// Static callback implementations
err_t TcpServer::tcp_accept_callback(void* arg, struct tcp_pcb* newpcb, err_t err) {
    TcpServer* server = (TcpServer*)arg;
    return server->tcpAccept(newpcb, err);
}

err_t TcpServer::tcp_recv_callback(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    TcpServer* server = (TcpServer*)arg;
    return server->tcpRecv(tpcb, p, err);
}

void TcpServer::tcp_err_callback(void* arg, err_t err) {
    TcpServer* server = (TcpServer*)arg;
    server->tcpErr(err);
}

err_t TcpServer::tcpAccept(struct tcp_pcb* newpcb, err_t err) {
    if (err != ERR_OK || newpcb == nullptr) {
        return ERR_VAL;
    }
    
    printf("TCP client connected\n");
    
    tcp_client_pcb_ = newpcb;
    tcp_arg(newpcb, this);
    tcp_recv(newpcb, tcp_recv_callback);
    tcp_err(newpcb, tcp_err_callback);
    
    // Send welcome message
    sendTcpResponse("=== Pico 2 W Hydroponic Controller ===");
    sendTcpResponse("Type 'help' for available commands");
    
    return ERR_OK;
}

err_t TcpServer::tcpRecv(struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (err == ERR_OK && p != nullptr) {
        // Copy data to command buffer
        uint16_t len = p->tot_len;
        if (tcp_command_len_ + len > sizeof(tcp_command_buffer_) - 1) {
            len = sizeof(tcp_command_buffer_) - 1 - tcp_command_len_;
        }
        
        if (len > 0) {
            pbuf_copy_partial(p, tcp_command_buffer_ + tcp_command_len_, len, 0);
            tcp_command_len_ += len;
            tcp_command_buffer_[tcp_command_len_] = '\0';
        } else if (tcp_command_len_ >= sizeof(tcp_command_buffer_) - 1) {
            // Buffer full without newline - discard and warn
            sendTcpResponse("ERROR: Command too long");
            tcp_command_len_ = 0;
            tcp_command_buffer_[0] = '\0';
        }
        
        // Process complete commands (ending with \n or \r)
        char* line_start = tcp_command_buffer_;
        char* line_end;
        
        while ((line_end = strchr(line_start, '\n')) || (line_end = strchr(line_start, '\r'))) {
            *line_end = '\0';
            
            if (strlen(line_start) > 0) {
                printf("TCP command: %s\n", line_start);
                processTcpCommand(line_start);
            }
            
            line_start = line_end + 1;
        }
        
        // Move remaining data to start of buffer
        if (line_start > tcp_command_buffer_) {
            tcp_command_len_ = strlen(line_start);
            memmove(tcp_command_buffer_, line_start, tcp_command_len_ + 1);
        }
        
        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);
    } else if (err == ERR_OK && p == nullptr) {
        // Connection closed by client
        printf("TCP client disconnected\n");
        tcp_client_pcb_ = nullptr;
        tcp_command_len_ = 0;
        tcp_close(tpcb);
    }
    
    return ERR_OK;
}

void TcpServer::tcpErr(err_t err) {
    printf("TCP error: %d\n", err);
    tcp_client_pcb_ = nullptr;
    tcp_command_len_ = 0;
}

void TcpServer::sendTcpResponse(const char* message) {
    if (!tcp_client_pcb_) return;
    
    size_t len = strlen(message);
    err_t err = tcp_write(tcp_client_pcb_, message, len, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        err = tcp_write(tcp_client_pcb_, "\n", 1, TCP_WRITE_FLAG_COPY);
        if (err == ERR_OK) {
            tcp_output(tcp_client_pcb_);
        }
    }
}

void TcpServer::processTcpCommand(const char* command) {
    if (!command || strlen(command) == 0) {
        sendTcpResponse("ERROR: Empty command");
        return;
    }
    
    // Find command and arguments
    char cmd_copy[256];
    strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';
    
    // Convert to lowercase
    for (char* p = cmd_copy; *p; p++) {
        *p = tolower(*p);
    }
    
    char* space_pos = strchr(cmd_copy, ' ');
    char* cmd_name = cmd_copy;
    char* cmd_args = nullptr;
    
    if (space_pos) {
        *space_pos = '\0';
        cmd_args = space_pos + 1;
    }
    
    // Process commands
    if (strcmp(cmd_name, "lights") == 0) {
        processLightsCommand(cmd_args);
    } else if (strcmp(cmd_name, "pump") == 0) {
        processPumpCommand(cmd_args);
    } else if (strcmp(cmd_name, "heater") == 0) {
        processHeaterCommand(cmd_args);
    } else if (strcmp(cmd_name, "humidity") == 0) {
        processHumidityCommand(cmd_args);
    } else if (strcmp(cmd_name, "mode") == 0) {
        processModeCommand(cmd_args);
    } else if (strcmp(cmd_name, "minrun") == 0) {
        processMinRunCommand(cmd_args);
    } else if (strcmp(cmd_name, "minoff") == 0) {
        processMinOffCommand(cmd_args);
    } else if (strcmp(cmd_name, "maxoff") == 0) {
        processMaxOffCommand(cmd_args);
    } else if (strcmp(cmd_name, "fan") == 0) {
        processFanCommand(cmd_args);
    } else if (strcmp(cmd_name, "status") == 0) {
        processStatusCommand();
    } else if (strcmp(cmd_name, "temp") == 0) {
        processTempCommand();
    } else if (strcmp(cmd_name, "humid") == 0) {
        processHumidCommand();
    } else if (strcmp(cmd_name, "save") == 0) {
        processSaveCommand();
    } else if (strcmp(cmd_name, "load") == 0) {
        processLoadCommand();
    } else if (strcmp(cmd_name, "help") == 0) {
        processHelpCommand();
    } else if (strcmp(cmd_name, "upload") == 0) {
        processUploadCommand(cmd_args);
    } else if (strcmp(cmd_name, "data") == 0) {
        processDataCommand(cmd_args);
    } else if (strcmp(cmd_name, "list") == 0) {
        processListCommand();
    } else {
        char response[128];
        snprintf(response, sizeof(response), "ERROR: Unknown command '%s'. Type 'help' for available commands.", cmd_name);
        sendTcpResponse(response);
    }
}

// Full command implementations
void TcpServer::processLightsCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: lights command requires two times (HH:MM HH:MM)");
        return;
    }
    
    char start_time[16], end_time[16];
    if (sscanf(args, "%15s %15s", start_time, end_time) != 2) {
        sendTcpResponse("ERROR: lights command requires two times (HH:MM HH:MM)");
        return;
    }
    
    uint32_t start_sec = TimeUtils::parseTimeToSeconds(start_time);
    uint32_t end_sec = TimeUtils::parseTimeToSeconds(end_time);
    
    if (start_sec == 0 && strcmp(start_time, "00:00") != 0) {
        sendTcpResponse("ERROR: Invalid start time format");
        return;
    }
    
    if (end_sec == 0 && strcmp(end_time, "00:00") != 0) {
        sendTcpResponse("ERROR: Invalid end time format");
        return;
    }
    
    uint32_t duration = (end_sec >= start_sec) ? (end_sec - start_sec) : (24U * 3600U - start_sec + end_sec);
    if (duration == 0) {
        sendTcpResponse("ERROR: Window duration cannot be zero");
        return;
    }
    
    lights_controller_->setSchedule(start_sec, end_sec);
    
    char response[128];
    snprintf(response, sizeof(response), "OK: Lights schedule updated to %s-%s", start_time, end_time);
    sendTcpResponse(response);
    printf("Lights schedule: %s-%s\n", start_time, end_time);
}

void TcpServer::processPumpCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: pump command requires two numbers (ON_SEC PERIOD_SEC)");
        return;
    }
    
    uint32_t on_sec, period_sec;
    if (sscanf(args, "%lu %lu", &on_sec, &period_sec) != 2) {
        sendTcpResponse("ERROR: pump command requires two numbers (ON_SEC PERIOD_SEC)");
        return;
    }
    
    if (on_sec == 0 || period_sec == 0 || on_sec >= period_sec) {
        sendTcpResponse("ERROR: Invalid pump timing (ON_SEC must be > 0 and < PERIOD_SEC)");
        return;
    }
    
    pump_controller_->setTiming(on_sec, period_sec);
    
    char response[128];
    snprintf(response, sizeof(response), "OK: Pump schedule updated to %lus ON, %lus period", on_sec, period_sec);
    sendTcpResponse(response);
    printf("Pump schedule: %lus ON, %lus period\n", on_sec, period_sec);
}

void TcpServer::processHeaterCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: heater command requires setpoint in °C (e.g. heater 20.5)");
        return;
    }
    
    float sp = atof(args);
    if (sp <= -40.0 || sp >= 80.0) {
        sendTcpResponse("ERROR: Setpoint out of range (-40..80)°C");
        return;
    }
    
    heater_controller_->setSetpoint(sp);
    
    char response[128];
    snprintf(response, sizeof(response), "OK: Heater setpoint set to %.1f°C", sp);
    sendTcpResponse(response);
    printf("Heater setpoint: %.1f°C\n", sp);
}

void TcpServer::processHumidityCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: humidity command requires threshold in % (e.g. humidity 60.0)");
        return;
    }
    
    float threshold = atof(args);
    if (threshold < 0.0 || threshold > 100.0) {
        sendTcpResponse("ERROR: Threshold out of range (0..100)%");
        return;
    }
    
    pump_controller_->setHumidityThreshold(threshold);
    
    char response[128];
    snprintf(response, sizeof(response), "OK: Humidity threshold set to %.1f%%", threshold);
    sendTcpResponse(response);
    printf("Humidity threshold: %.1f%%\n", threshold);
}

void TcpServer::processModeCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: mode command requires: timer or humidity");
        return;
    }
    
    char mode_copy[32];
    strncpy(mode_copy, args, sizeof(mode_copy) - 1);
    mode_copy[sizeof(mode_copy) - 1] = '\0';
    
    // Convert to lowercase
    for (char* p = mode_copy; *p; p++) {
        *p = tolower(*p);
    }
    
    if (strcmp(mode_copy, "timer") != 0 && strcmp(mode_copy, "humidity") != 0) {
        sendTcpResponse("ERROR: Mode must be 'timer' or 'humidity'");
        return;
    }
    
    bool new_mode = (strcmp(mode_copy, "humidity") == 0);
    pump_controller_->setHumidityMode(new_mode);
    
    char response[128];
    snprintf(response, sizeof(response), "OK: Pump mode set to %s", new_mode ? "humidity control" : "timer");
    sendTcpResponse(response);
    printf("Pump mode: %s\n", new_mode ? "humidity control" : "timer");
}

void TcpServer::processMinRunCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: minrun command requires time in seconds (e.g. minrun 45)");
        return;
    }
    
    uint32_t run_time = atoi(args);
    if (run_time < 5 || run_time > 300) {
        sendTcpResponse("ERROR: Run time out of range (5..300 seconds)");
        return;
    }
    
    pump_controller_->setMinRunTime(run_time);
    char response[128];
    snprintf(response, sizeof(response), "OK: Minimum pump run time set to %lu seconds", run_time);
    sendTcpResponse(response);
    printf("Min pump run time: %lus\n", run_time);
}

void TcpServer::processMinOffCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: minoff command requires time in seconds (e.g. minoff 600)");
        return;
    }
    
    uint32_t off_time = atoi(args);
    if (off_time < 60 || off_time > 3600) {
        sendTcpResponse("ERROR: Off time out of range (60..3600 seconds)");
        return;
    }
    
    pump_controller_->setMinOffTime(off_time);
    char response[128];
    snprintf(response, sizeof(response), "OK: Minimum pump off time set to %lu seconds", off_time);
    sendTcpResponse(response);
    printf("Min pump off time: %lus\n", off_time);
}

void TcpServer::processMaxOffCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: maxoff command requires time in seconds (e.g. maxoff 3600)");
        return;
    }
    
    uint32_t max_off_time = atoi(args);
    if (max_off_time < 300 || max_off_time > 7200) {
        sendTcpResponse("ERROR: Max off time out of range (300..7200 seconds)");
        return;
    }
    
    pump_controller_->setMaxOffTime(max_off_time);
    char response[128];
    snprintf(response, sizeof(response), "OK: Maximum pump off time set to %lu seconds", max_off_time);
    sendTcpResponse(response);
    printf("Max pump off time: %lus\n", max_off_time);
}

void TcpServer::processFanCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: fan command requires: on or off");
        return;
    }
    
    char fan_copy[16];
    strncpy(fan_copy, args, sizeof(fan_copy) - 1);
    fan_copy[sizeof(fan_copy) - 1] = '\0';
    
    // Convert to lowercase
    for (char* p = fan_copy; *p; p++) {
        *p = tolower(*p);
    }
    
    if (strcmp(fan_copy, "on") == 0) {
        fan_controller_->setManualControl(true);
        sendTcpResponse("OK: Fan turned ON (manual control)");
    } else if (strcmp(fan_copy, "off") == 0) {
        fan_controller_->setManualControl(false);
        sendTcpResponse("OK: Fan turned OFF (manual control)");
    } else {
        sendTcpResponse("ERROR: Fan command must be 'on' or 'off'");
    }
}

void TcpServer::processStatusCommand() {
    char response[1024];
    char time_str[64];
    
    time_t now = time(nullptr);
    if (now > 1600000000) {
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    } else {
        strcpy(time_str, "NOT SYNCED");
    }
    
    ConfigManager& config = ConfigManager::getInstance();
    
    // Build response with conditional humidity details
    char humidity_details[256] = "";
    if (config.getHumidityMode()) {
        snprintf(humidity_details, sizeof(humidity_details),
            "Humidity Threshold: %.1f%%\n"
            "Pump ON: when humidity < %.1f%%\n"
            "Pump OFF: when humidity >= %.1f%%\n"
            "Min Run Time: %lus\n"
            "Min Off Time: %lus\n"
            "Max Off Time: %lus (safety)\n",
            config.getHumidityThreshold(), config.getHumidityThreshold(), config.getHumidityThreshold(),
            config.getMinPumpRunSec(), config.getMinPumpOffSec(), config.getMaxPumpOffSec());
    }
    
    char temp_str[32], humidity_str[32], air_temp_str[32], air_humidity_str[32], ph_str[32], tds_str[32];
    if (sensor_manager_->isTemperatureValid()) {
        snprintf(temp_str, sizeof(temp_str), "%.1f°C", sensor_manager_->getLastTemperature());
    } else {
        strcpy(temp_str, "SENSOR FAILED!");
    }
    
    if (sensor_manager_->isHumidityValid()) {
        snprintf(humidity_str, sizeof(humidity_str), "%.1f%%", sensor_manager_->getLastHumidity());
    } else {
        strcpy(humidity_str, "SENSOR FAILED!");
    }
    
    if (sensor_manager_->isAirTempValid()) {
        snprintf(air_temp_str, sizeof(air_temp_str), "%.1f°C", sensor_manager_->getLastAirTemp());
    } else {
        strcpy(air_temp_str, "SENSOR FAILED!");
    }
    
    if (sensor_manager_->isAirHumidityValid()) {
        snprintf(air_humidity_str, sizeof(air_humidity_str), "%.1f%%", sensor_manager_->getLastAirHumidity());
    } else {
        strcpy(air_humidity_str, "SENSOR FAILED!");
    }
    
    if (sensor_manager_->isPHValid()) {
        snprintf(ph_str, sizeof(ph_str), "%.2f", sensor_manager_->getLastPH());
    } else {
        strcpy(ph_str, "N/A");
    }
    
    if (sensor_manager_->isTDSValid()) {
        snprintf(tds_str, sizeof(tds_str), "%.0f ppm", sensor_manager_->getLastTDS());
    } else {
        strcpy(tds_str, "N/A");
    }
    
    snprintf(response, sizeof(response),
        "=== HYDROPONIC CONTROLLER STATUS ===\n"
        "Current time: %s\n"
        "Lights: %s (window %02lu:%02lu-%02lu:%02lu)\n"
        "Pump: %s (%lus ON every %lus)\n"
        "Mode: %s\n"
        "Heater: %s (Setpoint: %.1f°C)\n"
        "Fan: %s (%.1f°C > %.1f°C ON, < %.1f°C OFF)\n"
        "Water Temp: %s\n"
        "Table Humidity: %s\n"
        "Room Air Temp: %s\n"
        "Room Air Humidity: %s\n"
        "pH: %s\n"
        "TDS: %s\n"
        "%s"
        "WiFi: Connected\n"
        "Time Sync: OK",
        time_str,
        lights_controller_->isOn() ? "ON" : "OFF",
        config.getLightsStartS() / 3600, (config.getLightsStartS() % 3600) / 60,
        config.getLightsEndS() / 3600, (config.getLightsEndS() % 3600) / 60,
        pump_controller_->isOn() ? "ON" : "OFF", config.getPumpOnSec(), config.getPumpPeriod(),
        config.getHumidityMode() ? "Humidity Control" : "Timer",
        heater_controller_->isOn() ? "ON" : "OFF", config.getHeaterSetpointC(),
        fan_controller_->isOn() ? "ON" : "OFF", 
        FanController::FAN_ON_TEMP_C, FanController::FAN_ON_TEMP_C, FanController::FAN_OFF_TEMP_C,
        temp_str, humidity_str, air_temp_str, air_humidity_str, ph_str, tds_str,
        humidity_details
    );
    
    sendTcpResponse(response);
}

void TcpServer::processTempCommand() {
    if (sensor_manager_->isTemperatureValid()) {
        char response[64];
        snprintf(response, sizeof(response), "Temperature: %.2f°C", sensor_manager_->getLastTemperature());
        sendTcpResponse(response);
    } else {
        sendTcpResponse("No temperature reading available");
    }
}

void TcpServer::processHumidCommand() {
    if (sensor_manager_->isHumidityValid()) {
        char response[64];
        snprintf(response, sizeof(response), "Humidity: %.1f%%", sensor_manager_->getLastHumidity());
        sendTcpResponse(response);
    } else {
        sendTcpResponse("No humidity reading available");
    }
}

void TcpServer::processSaveCommand() {
    ConfigManager& config = ConfigManager::getInstance();
    config.saveConfig();
    sendTcpResponse("OK: Configuration saved to flash");
}

void TcpServer::processLoadCommand() {
    ConfigManager& config = ConfigManager::getInstance();
    if (config.loadConfig()) {
        sendTcpResponse("OK: Configuration loaded from flash");
    } else {
        sendTcpResponse("ERROR: No valid configuration found in flash");
    }
}

void TcpServer::processHelpCommand() {
    char help[1024];
    snprintf(help, sizeof(help),
        "=== AVAILABLE COMMANDS ===\n"
        "lights HH:MM HH:MM    - Set lights window (e.g. lights 08:30 19:45)\n"
        "pump ON_SEC PERIOD_SEC - Set pump timing (e.g. pump 60 600)\n"
        "heater C              - Set heater setpoint in °C (e.g. heater 20.5)\n"
        "humidity C            - Set humidity threshold in %% (e.g. humidity 60.0)\n"
        "mode MODE             - Set pump mode: timer or humidity\n"
        "fan on|off            - Turn fan on or off (manual in 15-24°C zone)\n"
        "minrun SEC            - Set minimum pump run time in seconds (e.g. minrun 45)\n"
        "minoff SEC            - Set minimum pump off time in seconds (e.g. minoff 600)\n"
        "maxoff SEC            - Set maximum pump off time in seconds (e.g. maxoff 3600)\n"
        "status                 - Show current configuration and state\n"
        "temp                   - Get current temperature reading\n"
        "humid                  - Get current humidity reading\n"
        "save                   - Save current configuration to flash\n"
        "load                   - Load configuration from flash\n"
        "upload PATH SIZE       - Start file upload (e.g. upload /index.html 1024)\n"
        "data BASE64_DATA       - Send file data (base64 encoded)\n"
        "list                   - List files in flash storage\n"
        "help                   - Show this help message\n"
        "\nExample usage:\n"
        "  echo \"lights 09:00 21:00\" | nc IP_ADDRESS %d",
        TCP_PORT
    );
    
    sendTcpResponse(help);
}

void TcpServer::processUploadCommand(const char* args) {
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: upload command requires path and size (e.g. upload /index.html 1024)");
        return;
    }
    
    if (upload_in_progress_) {
        sendTcpResponse("ERROR: Upload already in progress");
        return;
    }
    
    char path[64];
    uint32_t size;
    if (sscanf(args, "%63s %lu", path, &size) != 2) {
        sendTcpResponse("ERROR: upload command requires path and size (e.g. upload /index.html 1024)");
        return;
    }
    
    if (size == 0 || size > 1024 * 1024) {  // Max 1MB
        sendTcpResponse("ERROR: File size must be between 1 and 1048576 bytes");
        return;
    }
    
    // Allocate buffer for upload
    upload_buffer_ = (uint8_t*)malloc(size);
    if (!upload_buffer_) {
        sendTcpResponse("ERROR: Failed to allocate memory for upload");
        return;
    }
    
    // Initialize upload state
    strncpy(upload_path_, path, sizeof(upload_path_) - 1);
    upload_path_[sizeof(upload_path_) - 1] = '\0';
    upload_size_ = size;
    upload_received_ = 0;
    upload_in_progress_ = true;
    
    char response[128];
    snprintf(response, sizeof(response), "READY: Send %lu bytes of data using 'data' command", size);
    sendTcpResponse(response);
}

void TcpServer::processDataCommand(const char* args) {
    if (!upload_in_progress_) {
        sendTcpResponse("ERROR: No upload in progress");
        return;
    }
    
    if (!args || strlen(args) == 0) {
        sendTcpResponse("ERROR: data command requires base64 encoded data");
        return;
    }
    
    // Simple base64 decode (basic implementation)
    size_t data_len = strlen(args);
    size_t decoded_len = (data_len * 3) / 4;
    
    if (upload_received_ + decoded_len > upload_size_) {
        sendTcpResponse("ERROR: Data exceeds expected file size");
        free(upload_buffer_);
        upload_in_progress_ = false;
        return;
    }
    
    // Decode base64 data (simplified - assumes no padding for now)
    for (size_t i = 0; i < data_len && upload_received_ < upload_size_; i += 4) {
        if (i + 3 < data_len) {
            uint32_t chunk = 0;
            int valid_chars = 0;
            for (int j = 0; j < 4; j++) {
                char c = args[i + j];
                uint8_t val = 0;
                if (c >= 'A' && c <= 'Z') val = c - 'A';
                else if (c >= 'a' && c <= 'z') val = c - 'a' + 26;
                else if (c >= '0' && c <= '9') val = c - '0' + 52;
                else if (c == '+') val = 62;
                else if (c == '/') val = 63;
                else break;  // Invalid char - abort this chunk
                
                chunk = (chunk << 6) | val;
                valid_chars++;
            }
            
            // Only decode if we got all 4 valid chars
            if (valid_chars == 4) {
                // Extract 3 bytes from 4 base64 chars
                for (int j = 0; j < 3 && upload_received_ < upload_size_; j++) {
                    upload_buffer_[upload_received_++] = (chunk >> (16 - j * 8)) & 0xFF;
                }
            }
        }
    }
    
    // Check if upload is complete
    if (upload_received_ >= upload_size_) {
        // Save file to flash storage
        FlashStorage& storage = FlashStorage::getInstance();
        if (storage.uploadFile(upload_path_, upload_buffer_, upload_size_)) {
            char response[128];
            snprintf(response, sizeof(response), "OK: Uploaded %s (%lu bytes)", upload_path_, upload_size_);
            sendTcpResponse(response);
        } else {
            sendTcpResponse("ERROR: Failed to save file to flash storage");
        }
        
        // Clean up
        free(upload_buffer_);
        upload_in_progress_ = false;
    } else {
        char response[64];
        snprintf(response, sizeof(response), "RECEIVED: %lu/%lu bytes", upload_received_, upload_size_);
        sendTcpResponse(response);
    }
}

void TcpServer::processListCommand() {
    FlashStorage& storage = FlashStorage::getInstance();
    if (storage.listFiles()) {
        sendTcpResponse("Files listed above");
    } else {
        sendTcpResponse("ERROR: Failed to list files");
    }
}
