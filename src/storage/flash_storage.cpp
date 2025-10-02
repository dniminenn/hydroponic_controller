#include "flash_storage.h"
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lfs.h"

// LittleFS configuration
static lfs_t lfs;
static struct lfs_config lfs_cfg;
static bool lfs_mounted = false;

// Flash read/write/erase functions for LittleFS
static int lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    uint32_t addr = LITTLEFS_FLASH_OFFSET + (block * c->block_size) + off;
    memcpy(buffer, (void*)(XIP_BASE + addr), size);
    return 0;
}

static int lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = LITTLEFS_FLASH_OFFSET + (block * c->block_size) + off;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(addr, (const uint8_t*)buffer, size);
    restore_interrupts(ints);
    return 0;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = LITTLEFS_FLASH_OFFSET + (block * c->block_size);
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(addr, c->block_size);
    restore_interrupts(ints);
    return 0;
}

static int lfs_flash_sync(const struct lfs_config *c) {
    return 0;
}

FlashStorage& FlashStorage::getInstance() {
    static FlashStorage instance;
    return instance;
}

FlashStorage::FlashStorage() : initialized_(false) {
}

FlashStorage::~FlashStorage() {
    if (lfs_mounted) {
        lfs_unmount(&lfs);
    }
}

bool FlashStorage::init() {
    if (initialized_) return true;
    
    // Configure LittleFS with tuned wear leveling
    memset(&lfs_cfg, 0, sizeof(lfs_cfg));
    lfs_cfg.read = lfs_flash_read;
    lfs_cfg.prog = lfs_flash_prog;
    lfs_cfg.erase = lfs_flash_erase;
    lfs_cfg.sync = lfs_flash_sync;
    lfs_cfg.read_size = 1;
    lfs_cfg.prog_size = FLASH_PAGE_SIZE;
    lfs_cfg.block_size = FLASH_SECTOR_SIZE;
    lfs_cfg.block_count = LITTLEFS_FLASH_SIZE / FLASH_SECTOR_SIZE;
    lfs_cfg.cache_size = 256;
    lfs_cfg.lookahead_size = 128;  // Increased for better allocation over larger space
    lfs_cfg.block_cycles = 200;    // Lower value for more aggressive wear leveling
    
    // Try to mount existing file system
    int err = lfs_mount(&lfs, &lfs_cfg);
    
    // Format if mount fails
    if (err) {
        printf("No file system found, formatting...\n");
        err = lfs_format(&lfs, &lfs_cfg);
        if (err) {
            printf("Format failed: %d\n", err);
            return false;
        }
        
        err = lfs_mount(&lfs, &lfs_cfg);
        if (err) {
            printf("Mount after format failed: %d\n", err);
            return false;
        }
    }
    
    lfs_mounted = true;
    initialized_ = true;
    printf("LittleFS mounted successfully\n");
    return true;
}

bool FlashStorage::getFile(const char* path, uint8_t** data, uint32_t* size, const char** mime_type) {
    if (!initialized_ && !init()) {
        return false;
    }
    
    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (err) {
        printf("Failed to open %s: %d\n", path, err);
        return false;
    }
    
    // Get file size
    lfs_soff_t file_size = lfs_file_size(&lfs, &file);
    if (file_size < 0) {
        lfs_file_close(&lfs, &file);
        return false;
    }
    
    // Allocate buffer
    uint8_t* buffer = (uint8_t*)malloc(file_size);
    if (!buffer) {
        lfs_file_close(&lfs, &file);
        return false;
    }
    
    // Read file
    lfs_ssize_t bytes_read = lfs_file_read(&lfs, &file, buffer, file_size);
    lfs_file_close(&lfs, &file);
    
    if (bytes_read != file_size) {
        free(buffer);
        return false;
    }
    
    *data = buffer;
    *size = file_size;
    if (mime_type) {
        *mime_type = getMimeType(path);
    }
    
    return true;
}

void FlashStorage::freeFile(uint8_t* data) {
    if (data) {
        free(data);
    }
}

bool FlashStorage::uploadFile(const char* path, const uint8_t* data, uint32_t size) {
    if (!initialized_ && !init()) {
        return false;
    }
    
    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err) {
        printf("Failed to create %s: %d\n", path, err);
        return false;
    }
    
    lfs_ssize_t written = lfs_file_write(&lfs, &file, data, size);
    lfs_file_close(&lfs, &file);
    
    if (written != (lfs_ssize_t)size) {
        printf("Write incomplete: %d/%u\n", written, size);
        return false;
    }
    
    printf("Uploaded %s (%u bytes)\n", path, size);
    return true;
}

bool FlashStorage::deleteFile(const char* path) {
    if (!initialized_ && !init()) {
        return false;
    }
    
    int err = lfs_remove(&lfs, path);
    return (err == 0);
}

bool FlashStorage::listFiles() {
    if (!initialized_ && !init()) {
        return false;
    }
    
    lfs_dir_t dir;
    int err = lfs_dir_open(&lfs, &dir, "/");
    if (err) {
        return false;
    }
    
    printf("Files in flash:\n");
    struct lfs_info info;
    while (lfs_dir_read(&lfs, &dir, &info) > 0) {
        if (info.type == LFS_TYPE_REG) {
            printf("  %s (%d bytes)\n", info.name, info.size);
        }
    }
    
    lfs_dir_close(&lfs, &dir);
    return true;
}

const char* FlashStorage::getMimeType(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    
    return "application/octet-stream";
}
