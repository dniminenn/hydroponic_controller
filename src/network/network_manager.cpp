#include "network_manager.h"
#include "../config.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/apps/sntp.h"
#include <stdio.h>

NetworkManager& NetworkManager::getInstance() {
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager() 
    : wifi_connected_(false), time_synced_(false), 
      last_ntp_sync_(0), last_wifi_attempt_(0) {
}

bool NetworkManager::initialize() {
    printf("Initializing WiFi...\n");
    
    // Initialize cyw43 architecture
    if (cyw43_arch_init()) {
        printf("Failed to initialize cyw43_arch\n");
        return false;
    }
    
    // Enable station mode
    cyw43_arch_enable_sta_mode();
    
    // Connect to WiFi
    printf("Connecting to WiFi: %s\n", WIFI_SSID);
    int result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 20000);
    
    if (result == 0) {
        wifi_connected_ = true;
        printf("WiFi connected successfully\n");
        printf("IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
        
        // Initialize NTP
        initializeNTP();
        
        // Initial time sync
        printf("Performing initial time sync...\n");
        last_ntp_sync_ = 0;
        syncTime();
        
        return true;
    } else {
        printf("WiFi connection failed: %d\n", result);
        printf("Continuing without WiFi...\n");
        return false;
    }
}

void NetworkManager::update() {
    ensureConnected();
    if (wifi_connected_) {
        syncTime();
    }
}

void NetworkManager::ensureConnected() {
    static uint32_t last_check = 0;
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Check every 5 seconds
    if (now - last_check < 5000) return;
    last_check = now;
    
    if (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP) {
        if (!wifi_connected_) {
            wifi_connected_ = true;
            printf("WiFi reconnected: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
        }
        return;
    }
    
    if (wifi_connected_) {
        wifi_connected_ = false;
        printf("WiFi disconnected\n");
    }
    
    // Attempt reconnection every 10 seconds
    if (now - last_wifi_attempt_ > 10000) {
        printf("Reconnecting WiFi...\n");
        cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000);
        last_wifi_attempt_ = now;
    }
}

void NetworkManager::syncTime() {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Sync every 5 minutes
    if (last_ntp_sync_ != 0 && now - last_ntp_sync_ < 300000) return;
    
    if (!wifi_connected_) return;
    
    printf("Syncing time from %s...", NTP_SERVER);
    
    // Set timezone
    setenv("TZ", TZSTR, 1);
    tzset();
    
    // Initialize SNTP if not already done
    if (!sntp_enabled()) {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, NTP_SERVER);
        sntp_init();
    }
    
    // Wait for time sync (up to 10 seconds)
    time_t sync_time = time(nullptr);
    int attempts = 0;
    while (sync_time < 1600000000 && attempts < 20) {
        sleep_ms(500);
        sync_time = time(nullptr);
        attempts++;
    }
    
    if (sync_time > 1600000000) {
        if (!time_synced_) {
            time_synced_ = true;
            printf(" SUCCESS\n");
            printf("Time synced: %s", ctime(&sync_time));
        }
    } else {
        time_synced_ = false;
        printf(" FAILED\n");
        printf("Time sync failed - check NTP server\n");
    }
    
    last_ntp_sync_ = now;
}

void NetworkManager::initializeNTP() {
    // Set timezone
    setenv("TZ", TZSTR, 1);
    tzset();
    
    // Initialize SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTP_SERVER);
    sntp_init();
    
    printf("NTP initialized with server: %s\n", NTP_SERVER);
    printf("Timezone: %s\n", TZSTR);
}
