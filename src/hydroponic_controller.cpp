#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include <stdio.h>

#include "hydroponic_controller.h"
#include "sensors/sensor_manager.h"
#include "network/network_manager.h"
#include "network/tcp_server.h"
#include "network/web_server.h"
#include "control/lights_controller.h"
#include "control/pump_controller.h"
#include "control/heater_controller.h"
#include "control/fan_controller.h"
#include "utils/gpio_utils.h"
#include "utils/time_utils.h"
#include "config.h"

HydroponicController::HydroponicController() 
    : sensor_manager_(nullptr),
      network_manager_(nullptr),
      tcp_server_(nullptr),
      web_server_(nullptr),
      lights_controller_(nullptr),
      pump_controller_(nullptr),
      heater_controller_(nullptr),
      fan_controller_(nullptr),
      last_status_print_ms_(0),
      core1_initialized_(false) {
}

HydroponicController::~HydroponicController() {
    if (tcp_server_) {
        delete tcp_server_;
    }
    if (web_server_) {
        delete web_server_;
    }
    if (lights_controller_) {
        delete lights_controller_;
    }
    if (pump_controller_) {
        delete pump_controller_;
    }
    if (heater_controller_) {
        delete heater_controller_;
    }
    if (fan_controller_) {
        delete fan_controller_;
    }
    if (sensor_manager_) {
        delete sensor_manager_;
    }
}

void HydroponicController::begin() {
    stdio_init_all();
    printf("\n=== Pico 2 W Hydroponic Controller Starting ===\n");
    printf("=== Dual-Core Architecture Enabled ===\n");
    printf("Core 0: Control loop and sensors\n");
    printf("Core 1: Network servers and misc tasks\n\n");
    
    // Initialize GPIO
    GpioUtils::initializeGpioOutputs();
    GpioUtils::setAllRelaysOff();
    printf("GPIO initialized\n");
    
    // Initialize components
    initializeComponents();
    
    // Try to load config from flash
    ConfigManager& config = ConfigManager::getInstance();
    config.loadConfig();
    
    printf("Controller ready\n");
    
    // Print initial configuration
    printf("Lights schedule: %02lu:%02lu-%02lu:%02lu\n", 
           config.getLightsStartS() / 3600, (config.getLightsStartS() % 3600) / 60,
           config.getLightsEndS() / 3600, (config.getLightsEndS() % 3600) / 60);
    printf("Pump schedule: %lu s ON every %lu s\n", config.getPumpOnSec(), config.getPumpPeriod());
    printf("Heater setpoint: %.1f°C\n", config.getHeaterSetpointC());
    
    if (network_manager_->isConnected()) {
        printf("Web interface: http://%s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
        printf("TCP commands: echo \"help\" | nc %s %d\n", 
               ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
    }
}

void HydroponicController::loop() {
    core0Loop();
}

void HydroponicController::core0Loop() {
    // Core 0: Critical control loop and sensor reading
    
    // Read sensors
    sensor_manager_->readTemperature();
    sensor_manager_->readHumidity();
    sensor_manager_->readAirSensor();
    sensor_manager_->readNanoADCs();
    
    // Update control logic
    lights_controller_->update();
    pump_controller_->update();
    heater_controller_->update();
    fan_controller_->update();
    
    tight_loop_contents();
}

void HydroponicController::core1Entry() {
    printf("Core 1 started\n");
    core1_initialized_ = true;
    
    while (true) {
        core1Loop();
    }
}

void HydroponicController::core1Loop() {
    // Core 1: Network management and servers
    
    // Network management
    network_manager_->update();
    
    // Handle network clients
    if (network_manager_->isConnected()) {
        tcp_server_->handleClients();
        web_server_->handleClients();
    }
    
    // Print status periodically
    printStatusTable();
    
    tight_loop_contents();
}

void HydroponicController::initializeComponents() {
    // Initialize sensor manager
    sensor_manager_ = new SensorManager();
    sensor_manager_->initialize();
    
    // Initialize network manager
    network_manager_ = &NetworkManager::getInstance();
    network_manager_->initialize();
    
    // Initialize control components
    lights_controller_ = new LightsController();
    pump_controller_ = new PumpController(sensor_manager_);
    heater_controller_ = new HeaterController(sensor_manager_);
    fan_controller_ = new FanController(sensor_manager_);
    
    // Initialize network servers
    tcp_server_ = new TcpServer(sensor_manager_, lights_controller_, 
                                pump_controller_, heater_controller_, fan_controller_);
    web_server_ = new WebServer(sensor_manager_, lights_controller_, 
                                pump_controller_, heater_controller_, fan_controller_);
    
    if (network_manager_->isConnected()) {
        tcp_server_->start();
        web_server_->start();
    }
}

void HydroponicController::printStatusTable() {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_status_print_ms_ < STATUS_INTERVAL_MS) return;
    
    uint32_t current_seconds = TimeUtils::getSecondsFromMidnight();
    char timeStr[8];
    TimeUtils::secondsToTimeString(current_seconds, timeStr, sizeof(timeStr));
    
    ConfigManager& config = ConfigManager::getInstance();
    
    printf("\n┌─────────────────────────────────────────────────┐\n");
    printf("│              HYDRO CONTROLLER STATUS           │\n");
    printf("├─────────────────────────────────────────────────┤\n");
    printf("│ Time: %s (uptime)                        │\n", timeStr);
    printf("│ Lights: %s (window %02lu:%02lu-%02lu:%02lu)             │\n",
           lights_controller_->isOn() ? "ON " : "OFF",
           config.getLightsStartS() / 3600, (config.getLightsStartS() % 3600) / 60,
           config.getLightsEndS() / 3600, (config.getLightsEndS() % 3600) / 60);
    printf("│ Pump: %s (%lus ON every %lus)                │\n",
           pump_controller_->isOn() ? "ON " : "OFF", config.getPumpOnSec(), config.getPumpPeriod());
    printf("│ Mode: %s                                   │\n",
           config.getHumidityMode() ? "Humidity Control" : "Timer");
    printf("│ Heater: %s                                     │\n",
           heater_controller_->isOn() ? "ON " : "OFF");
    printf("│ Setpoint: %.1f°C                                  │\n", config.getHeaterSetpointC());
    printf("│ Fan: %s (>= %.1f°C: ON, <= %.1f°C: OFF)      │\n",
           fan_controller_->isOn() ? "ON " : "OFF", 
           FanController::FAN_ON_TEMP_C, FanController::FAN_OFF_TEMP_C);
    
    if (sensor_manager_->isTemperatureValid()) {
        printf("│ Temperature: %.1f°C                            │\n", sensor_manager_->getLastTemperature());
    } else {
        printf("│ Temperature: SENSOR FAILED!                    │\n");
    }
    
    if (sensor_manager_->isHumidityValid()) {
        printf("│ Humidity: %.1f%%                                  │\n", sensor_manager_->getLastHumidity());
    } else {
        printf("│ Humidity: SENSOR FAILED!                       │\n");
    }
    
    if (config.getHumidityMode()) {
        printf("│ Threshold: %.1f%% (ON: <%.1f%%, OFF: >=%.1f%%)     │\n", 
               config.getHumidityThreshold(), config.getHumidityThreshold(), config.getHumidityThreshold());
        printf("│ Min Run: %lus, Min Off: %lus, Max Off: %lus        │\n", 
               config.getMinPumpRunSec(), config.getMinPumpOffSec(), config.getMaxPumpOffSec());
    }
    
    printf("│ WiFi: %s                                  │\n",
           network_manager_->isConnected() ? "Connected" : "Disconnected");
    printf("│ Time Sync: %s                               │\n",
           network_manager_->isTimeSynced() ? "OK" : "FAILED");
    
    printf("└─────────────────────────────────────────────────┘\n");
    
    last_status_print_ms_ = now;
}
