#pragma once

#include <stdint.h>
#include <stddef.h>

// Flash storage layout for Pico 2 W (4MB flash)
// 0-1.5MB:    Program code and data (generous headroom)
// 1.5-4MB:    LittleFS partition (2.5MB for web files + config)
//
// LittleFS handles both web assets and config, leveraging built-in wear leveling

#define LITTLEFS_FLASH_OFFSET (1536 * 1024)      // 1.5MB offset
#define LITTLEFS_FLASH_SIZE   (2560 * 1024)      // 2.5MB for file system

class FlashStorage {
public:
    static FlashStorage& getInstance();
    
    bool init();
    bool getFile(const char* path, uint8_t** data, uint32_t* size, const char** mime_type);
    void freeFile(uint8_t* data);
    
    // File system utilities
    bool uploadFile(const char* path, const uint8_t* data, uint32_t size);
    bool deleteFile(const char* path);
    bool listFiles();
    
private:
    FlashStorage();
    ~FlashStorage();
    
    const char* getMimeType(const char* path);
    bool initialized_;
};
