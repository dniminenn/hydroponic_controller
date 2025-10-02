#pragma once

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
struct tcp_pcb;

class NetworkManager {
public:
    static NetworkManager& getInstance();
    
    bool initialize();
    void update();
    void ensureConnected();
    
    bool isConnected() const { return wifi_connected_; }
    bool isTimeSynced() const { return time_synced_; }
    
    // Time management
    void syncTime();
    
private:
    NetworkManager();
    
    void initializeWiFi();
    void initializeNTP();
    
    bool wifi_connected_;
    bool time_synced_;
    uint32_t last_ntp_sync_;
    uint32_t last_wifi_attempt_;
};
