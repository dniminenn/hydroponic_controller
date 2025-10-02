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

class TcpServer {
public:
    TcpServer(SensorManager* sensor_manager, 
              LightsController* lights_controller,
              PumpController* pump_controller,
              HeaterController* heater_controller,
              FanController* fan_controller);
    ~TcpServer();
    
    bool start();
    void stop();
    void handleClients();
    
private:
    // TCP callbacks
    static err_t tcp_accept_callback(void* arg, struct tcp_pcb* newpcb, err_t err);
    static err_t tcp_recv_callback(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
    static void tcp_err_callback(void* arg, err_t err);
    
    err_t tcpAccept(struct tcp_pcb* newpcb, err_t err);
    err_t tcpRecv(struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
    void tcpErr(err_t err);
    
    // Command processing
    void processTcpCommand(const char* command);
    void sendTcpResponse(const char* message);
    
    // Command handlers
    void processLightsCommand(const char* args);
    void processPumpCommand(const char* args);
    void processHeaterCommand(const char* args);
    void processHumidityCommand(const char* args);
    void processModeCommand(const char* args);
    void processMinRunCommand(const char* args);
    void processMinOffCommand(const char* args);
    void processMaxOffCommand(const char* args);
    void processFanCommand(const char* args);
    void processStatusCommand();
    void processSaveCommand();
    void processLoadCommand();
    void processHelpCommand();
    void processTempCommand();
    void processHumidCommand();
    void processUploadCommand(const char* args);
    void processDataCommand(const char* args);
    void processListCommand();
    
    // Component references
    SensorManager* sensor_manager_;
    LightsController* lights_controller_;
    PumpController* pump_controller_;
    HeaterController* heater_controller_;
    FanController* fan_controller_;
    
    // TCP state
    struct tcp_pcb* tcp_server_pcb_;
    struct tcp_pcb* tcp_client_pcb_;
    char tcp_command_buffer_[256];
    uint16_t tcp_command_len_;
    
    // Upload state
    bool upload_in_progress_;
    char upload_path_[64];
    uint32_t upload_size_;
    uint32_t upload_received_;
    uint8_t* upload_buffer_;
};
