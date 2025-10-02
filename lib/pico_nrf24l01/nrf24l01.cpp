#include "nrf24l01.h"
#include <string.h>
#include "pico/stdlib.h"

NRF24L01::NRF24L01(spi_inst_t* spi, uint8_t csn_pin, uint8_t ce_pin)
    : spi_(spi), csn_pin_(csn_pin), ce_pin_(ce_pin), payload_size_(32) {
}

bool NRF24L01::init() {
    // Setup GPIO pins
    gpio_init(csn_pin_);
    gpio_set_dir(csn_pin_, GPIO_OUT);
    gpio_init(ce_pin_);
    gpio_set_dir(ce_pin_, GPIO_OUT);
    
    csn_high();
    ce_low();
    
    sleep_ms(5);  // Power on reset
    
    // Flush buffers
    flushRx();
    flushTx();
    
    // Clear status flags
    writeRegister(NRF_REG_STATUS, 0x70);
    
    // Set address width to 5 bytes
    writeRegister(NRF_REG_SETUP_AW, 0x03);
    
    // Set auto retransmit delay and count (for TX mode if needed)
    writeRegister(NRF_REG_SETUP_RETR, 0x5F);
    
    // Enable auto acknowledgment on pipe 0 and 1
    writeRegister(NRF_REG_EN_AA, 0x03);
    
    // Enable RX addresses for pipe 0 and 1
    writeRegister(NRF_REG_EN_RXADDR, 0x03);
    
    // Default channel 76
    setChannel(76);
    
    // Default 1Mbps data rate
    setDataRate(DataRate::DR_1MBPS);
    
    // Default HIGH power for stability
    setPowerLevel(PowerLevel::PWR_HIGH);
    
    // Enable dynamic payloads and ACK payloads
    writeRegister(NRF_REG_FEATURE, 0x07);  // EN_DPL | EN_ACK_PAY | EN_DYN_ACK
    writeRegister(NRF_REG_DYNPD, 0x03);    // Enable dynamic payload on pipe 0 and 1
    
    // Power up and set to RX mode
    writeRegister(NRF_REG_CONFIG, NRF_CONFIG_EN_CRC | NRF_CONFIG_CRCO | NRF_CONFIG_PWR_UP | NRF_CONFIG_PRIM_RX);
    
    sleep_ms(5);  // Wait for power up
    
    return true;
}

void NRF24L01::setChannel(uint8_t channel) {
    if (channel > 125) channel = 125;
    writeRegister(NRF_REG_RF_CH, channel);
}

void NRF24L01::setDataRate(DataRate rate) {
    uint8_t setup = readRegister(NRF_REG_RF_SETUP);
    setup &= ~(NRF_RF_SETUP_RF_DR_LOW | NRF_RF_SETUP_RF_DR_HIGH);
    
    switch (rate) {
        case DataRate::DR_250KBPS:
            setup |= NRF_RF_SETUP_RF_DR_LOW;
            break;
        case DataRate::DR_2MBPS:
            setup |= NRF_RF_SETUP_RF_DR_HIGH;
            break;
        default:  // 1MBPS
            break;
    }
    
    writeRegister(NRF_REG_RF_SETUP, setup);
}

void NRF24L01::setPowerLevel(PowerLevel level) {
    uint8_t setup = readRegister(NRF_REG_RF_SETUP);
    setup &= ~NRF_RF_SETUP_RF_PWR;
    setup |= ((uint8_t)level << 1);
    writeRegister(NRF_REG_RF_SETUP, setup);
}

void NRF24L01::setPayloadSize(uint8_t size) {
    payload_size_ = size > 32 ? 32 : size;
    writeRegister(NRF_REG_RX_PW_P0, payload_size_);
    writeRegister(NRF_REG_RX_PW_P1, payload_size_);
}

void NRF24L01::openReadingPipe(uint8_t pipe, const uint8_t* address, uint8_t addr_len) {
    if (pipe > 5) return;
    
    uint8_t reg = NRF_REG_RX_ADDR_P0 + pipe;
    writeRegisterN(reg, address, addr_len);
    
    // Enable this pipe
    uint8_t en_rxaddr = readRegister(NRF_REG_EN_RXADDR);
    en_rxaddr |= (1 << pipe);
    writeRegister(NRF_REG_EN_RXADDR, en_rxaddr);
}

void NRF24L01::startListening() {
    uint8_t config = readRegister(NRF_REG_CONFIG);
    config |= (NRF_CONFIG_PWR_UP | NRF_CONFIG_PRIM_RX);
    writeRegister(NRF_REG_CONFIG, config);
    writeRegister(NRF_REG_STATUS, 0x70);  // Clear flags
    
    ce_high();
    sleep_us(130);  // Settling time
}

void NRF24L01::stopListening() {
    ce_low();
    sleep_us(130);
    flushTx();
    flushRx();
}

bool NRF24L01::available() {
    uint8_t status = readStatus();
    
    // Check if RX FIFO is not empty
    if (status & NRF_STATUS_RX_DR) {
        writeRegister(NRF_REG_STATUS, NRF_STATUS_RX_DR);
        return true;
    }
    
    // Alternative check via FIFO status
    uint8_t fifo_status = readRegister(NRF_REG_FIFO_STATUS);
    return !(fifo_status & NRF_FIFO_RX_EMPTY);
}

bool NRF24L01::read(void* buffer, uint8_t len) {
    // Get actual payload width for dynamic payloads
    uint8_t payload_len = getPayloadWidth();
    if (payload_len == 0 || payload_len > 32) {
        flushRx();
        return false;
    }
    
    if (len > payload_len) len = payload_len;
    
    csn_low();
    spiTransfer(NRF_CMD_R_RX_PAYLOAD);
    
    uint8_t* buf = (uint8_t*)buffer;
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spiTransfer(0xFF);
    }
    
    csn_high();
    
    // Clear RX_DR flag
    writeRegister(NRF_REG_STATUS, NRF_STATUS_RX_DR);
    
    return true;
}

uint8_t NRF24L01::getPayloadWidth() {
    csn_low();
    spiTransfer(NRF_CMD_R_RX_PL_WID);
    uint8_t width = spiTransfer(0xFF);
    csn_high();
    return width;
}

uint8_t NRF24L01::readRegister(uint8_t reg) {
    csn_low();
    spiTransfer(NRF_CMD_R_REGISTER | (reg & 0x1F));
    uint8_t value = spiTransfer(0xFF);
    csn_high();
    return value;
}

void NRF24L01::writeRegister(uint8_t reg, uint8_t value) {
    csn_low();
    spiTransfer(NRF_CMD_W_REGISTER | (reg & 0x1F));
    spiTransfer(value);
    csn_high();
}

void NRF24L01::readRegisterN(uint8_t reg, uint8_t* buffer, uint8_t len) {
    csn_low();
    spiTransfer(NRF_CMD_R_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) {
        buffer[i] = spiTransfer(0xFF);
    }
    csn_high();
}

void NRF24L01::writeRegisterN(uint8_t reg, const uint8_t* buffer, uint8_t len) {
    csn_low();
    spiTransfer(NRF_CMD_W_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) {
        spiTransfer(buffer[i]);
    }
    csn_high();
}

void NRF24L01::csn_low() {
    gpio_put(csn_pin_, 0);
    sleep_us(1);
}

void NRF24L01::csn_high() {
    gpio_put(csn_pin_, 1);
    sleep_us(1);
}

void NRF24L01::ce_low() {
    gpio_put(ce_pin_, 0);
}

void NRF24L01::ce_high() {
    gpio_put(ce_pin_, 1);
}

uint8_t NRF24L01::spiTransfer(uint8_t data) {
    uint8_t rx_data;
    spi_write_read_blocking(spi_, &data, &rx_data, 1);
    return rx_data;
}

uint8_t NRF24L01::readStatus() {
    csn_low();
    uint8_t status = spiTransfer(NRF_CMD_NOP);
    csn_high();
    return status;
}

void NRF24L01::flushRx() {
    csn_low();
    spiTransfer(NRF_CMD_FLUSH_RX);
    csn_high();
}

void NRF24L01::flushTx() {
    csn_low();
    spiTransfer(NRF_CMD_FLUSH_TX);
    csn_high();
}

