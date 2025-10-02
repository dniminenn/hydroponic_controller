#pragma once

#include <stdint.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"

// NRF24L01 register addresses
#define NRF_REG_CONFIG      0x00
#define NRF_REG_EN_AA       0x01
#define NRF_REG_EN_RXADDR   0x02
#define NRF_REG_SETUP_AW    0x03
#define NRF_REG_SETUP_RETR  0x04
#define NRF_REG_RF_CH       0x05
#define NRF_REG_RF_SETUP    0x06
#define NRF_REG_STATUS      0x07
#define NRF_REG_OBSERVE_TX  0x08
#define NRF_REG_RPD         0x09
#define NRF_REG_RX_ADDR_P0  0x0A
#define NRF_REG_RX_ADDR_P1  0x0B
#define NRF_REG_TX_ADDR     0x10
#define NRF_REG_RX_PW_P0    0x11
#define NRF_REG_RX_PW_P1    0x12
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_DYNPD       0x1C
#define NRF_REG_FEATURE     0x1D

// Commands
#define NRF_CMD_R_REGISTER    0x00
#define NRF_CMD_W_REGISTER    0x20
#define NRF_CMD_R_RX_PAYLOAD  0x61
#define NRF_CMD_W_TX_PAYLOAD  0xA0
#define NRF_CMD_FLUSH_TX      0xE1
#define NRF_CMD_FLUSH_RX      0xE2
#define NRF_CMD_REUSE_TX_PL   0xE3
#define NRF_CMD_R_RX_PL_WID   0x60
#define NRF_CMD_NOP           0xFF

// Config register bits
#define NRF_CONFIG_MASK_RX_DR  (1 << 6)
#define NRF_CONFIG_MASK_TX_DS  (1 << 5)
#define NRF_CONFIG_MASK_MAX_RT (1 << 4)
#define NRF_CONFIG_EN_CRC      (1 << 3)
#define NRF_CONFIG_CRCO        (1 << 2)
#define NRF_CONFIG_PWR_UP      (1 << 1)
#define NRF_CONFIG_PRIM_RX     (1 << 0)

// Status register bits
#define NRF_STATUS_RX_DR   (1 << 6)
#define NRF_STATUS_TX_DS   (1 << 5)
#define NRF_STATUS_MAX_RT  (1 << 4)
#define NRF_STATUS_RX_P_NO (0x07 << 1)
#define NRF_STATUS_TX_FULL (1 << 0)

// FIFO Status bits
#define NRF_FIFO_RX_EMPTY  (1 << 0)

// RF Setup bits
#define NRF_RF_SETUP_RF_DR_LOW  (1 << 5)
#define NRF_RF_SETUP_RF_DR_HIGH (1 << 3)
#define NRF_RF_SETUP_RF_PWR     (0x03 << 1)

enum class DataRate {
    DR_1MBPS = 0,
    DR_2MBPS = 1,
    DR_250KBPS = 2
};

enum class PowerLevel {
    PWR_MIN = 0,     // -18dBm
    PWR_LOW = 1,     // -12dBm
    PWR_HIGH = 2,    // -6dBm
    PWR_MAX = 3      // 0dBm
};

class NRF24L01 {
public:
    NRF24L01(spi_inst_t* spi, uint8_t csn_pin, uint8_t ce_pin);
    
    bool init();
    void setChannel(uint8_t channel);
    void setDataRate(DataRate rate);
    void setPowerLevel(PowerLevel level);
    void setPayloadSize(uint8_t size);
    void openReadingPipe(uint8_t pipe, const uint8_t* address, uint8_t addr_len = 5);
    void startListening();
    void stopListening();
    bool available();
    bool read(void* buffer, uint8_t len);
    uint8_t getPayloadWidth();
    
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);
    void readRegisterN(uint8_t reg, uint8_t* buffer, uint8_t len);
    void writeRegisterN(uint8_t reg, const uint8_t* buffer, uint8_t len);
    
private:
    spi_inst_t* spi_;
    uint8_t csn_pin_;
    uint8_t ce_pin_;
    uint8_t payload_size_;
    
    void csn_low();
    void csn_high();
    void ce_low();
    void ce_high();
    uint8_t spiTransfer(uint8_t data);
    uint8_t readStatus();
    void flushRx();
    void flushTx();
};

