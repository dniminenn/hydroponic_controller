#pragma once

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
class SensorManager;
class NetworkManager;
class TcpServer;
class WebServer;
class LightsController;
class PumpController;
class HeaterController;
class FanController;

class HydroponicController {
public:
    HydroponicController();
    ~HydroponicController();

    void begin();
    void loop();
    
    // Core 1 entry point (network and servers)
    void core1Entry();
    
    // Status reporting
    void printStatusTable();

private:
    // Component initialization
    void initializeComponents();
    
    // Core 0 loop (control and sensors)
    void core0Loop();
    
    // Core 1 loop (network and servers)
    void core1Loop();
    
    // Component references
    SensorManager* sensor_manager_;
    NetworkManager* network_manager_;
    TcpServer* tcp_server_;
    WebServer* web_server_;
    LightsController* lights_controller_;
    PumpController* pump_controller_;
    HeaterController* heater_controller_;
    FanController* fan_controller_;
    
    // Status printing timing
    uint32_t last_status_print_ms_;
    static const uint32_t STATUS_INTERVAL_MS = 5000UL;
    
    // Core synchronization
    volatile bool core1_initialized_;
};
