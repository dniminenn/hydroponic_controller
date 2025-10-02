/*
 * NRF24L01 Test - Simple TX/RX Test
 * 
 * Hardware:
 *   - Arduino Nano
 *   - NRF24L01+PA+LNA socket adapter (plugs into Nano headers)
 * 
 * Usage:
 *   - TX mode: Sends dummy data every 1 second
 *   - RX mode: Receives and displays data
 * 
 * Build flags:
 *   -DRF_MODE=1  for transmitter
 *   -DRF_MODE=0  for receiver
 */

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

// NRF24L01 pins
#define NRF_CE_PIN  9
#define NRF_CSN_PIN 10

// RF configuration
#define RF_CHANNEL 76
#define RF_PAYLOAD_SIZE 32

// Pipe address (same for both TX and RX)
const uint8_t ADDR[5] = {0xE8, 0xE8, 0xF0, 0xF0, 0xE1};

RF24 radio(NRF_CE_PIN, NRF_CSN_PIN);

// Test data structure
struct TestData {
  uint32_t counter;
  uint32_t timestamp;
  float dummy_float;
  uint8_t dummy_bytes[20];
};

TestData tx_data;
TestData rx_data;
uint32_t last_tx_ms = 0;
uint32_t last_rx_ms = 0;
uint32_t rx_count = 0;

void setup() {
  Serial.begin(115200);
  delay(2000); // Give time for serial to initialize
  
  // Debug: Show what RF_MODE was compiled with
  Serial.print(F("RF_MODE = "));
  Serial.println(RF_MODE);
  
#if RF_MODE == 1
  Serial.println(F("NRF24L01 Test - TRANSMITTER Mode"));
#else
  Serial.println(F("NRF24L01 Test - RECEIVER Mode"));
#endif
  
  // Debug: Check SPI communication
  Serial.println(F("Testing SPI communication..."));
  SPI.begin();
  delay(100);
  
  // Debug: Check radio status before begin()
  Serial.print(F("Radio status before begin(): "));
  Serial.println(radio.isChipConnected() ? F("Connected") : F("Not connected"));
  
  // Initialize NRF24L01
  Serial.println(F("Initializing NRF24L01..."));
  if (!radio.begin()) {
    Serial.println(F("NRF24L01 initialization failed!"));
    Serial.println(F("Troubleshooting checklist:"));
    Serial.println(F("1. Check 3.3V power to VCC pin"));
    Serial.println(F("2. Check GND connection"));
    Serial.println(F("3. Verify SPI wiring (CE=9, CSN=10, SCK=13, MOSI=11, MISO=12)"));
    Serial.println(F("4. Try different NRF24L01 module"));
    Serial.println(F("5. Check for loose connections"));
    while (1) delay(1000);
  }
  
  Serial.println(F("NRF24L01 initialized successfully!"));
  
  // Configure radio
  radio.setPALevel(RF24_PA_HIGH);  // Use HIGH instead of MAX for stability
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(RF_CHANNEL);
  radio.setRetries(5, 15);  // 5*250us delay, 15 retries
  radio.setAutoAck(true);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  
  // Print radio configuration for debugging
  Serial.print(F("PA Level: "));
  Serial.println(radio.getPALevel());
  Serial.print(F("Data Rate: "));
  Serial.println(radio.getDataRate());
  Serial.print(F("Channel: "));
  Serial.println(radio.getChannel());
  Serial.print(F("Payload Size: "));
  Serial.println(radio.getPayloadSize());
  
  Serial.println(F("\n=== Radio Details ==="));
  radio.printDetails();
  
#if RF_MODE == 1
  // Transmitter setup
  radio.openWritingPipe(ADDR);
  radio.stopListening();
  
  // Initialize test data
  tx_data.counter = 0;
  tx_data.dummy_float = 3.14159;
  for (int i = 0; i < 20; i++) {
    tx_data.dummy_bytes[i] = i;
  }
  
  Serial.println(F("Transmitter ready - Channel 76"));
  Serial.println(F("Sending test data every 1 second..."));
  
#else
  // Receiver setup
  radio.openReadingPipe(0, ADDR);  // Use pipe 0 for better reliability
  radio.startListening();
  
  Serial.println(F("Receiver ready - Channel 76"));
  Serial.println(F("Listening for test data..."));
#endif
}

void loop() {
  uint32_t now = millis();
  
#if RF_MODE == 1
  // Transmitter - send data every 1 second
  if (now - last_tx_ms >= 1000) {
    tx_data.counter++;
    tx_data.timestamp = now;
    tx_data.dummy_float += 0.1;
    
    bool success = radio.write(&tx_data, sizeof(tx_data));
    
    Serial.print(F("TX #"));
    Serial.print(tx_data.counter);
    Serial.print(success ? F(" OK: ") : F(" FAIL: "));
    Serial.print(F("float="));
    Serial.print(tx_data.dummy_float, 2);
    Serial.print(F(" time="));
    Serial.print(tx_data.timestamp);
    
    // Add detailed failure diagnostics
    if (!success) {
      Serial.print(F(" ["));
      Serial.print(F("FIFO_FULL="));
      Serial.print(radio.isFifo(true, false) ? F("1") : F("0"));
      Serial.print(F(" RPD="));
      Serial.print(radio.testRPD() ? F("1") : F("0"));
      Serial.print(F("]"));
      radio.flush_tx();  // Clear TX FIFO on failure
    }
    Serial.println();
    
    last_tx_ms = now;
  }
  
#else
  // Receiver - check for data
  if (radio.available()) {
    uint8_t pipe;
    radio.read(&rx_data, sizeof(rx_data));
    rx_count++;
    
    Serial.print(F("RX #"));
    Serial.print(rx_count);
    Serial.print(F(" from pipe "));
    Serial.print(pipe);
    Serial.print(F(": counter="));
    Serial.print(rx_data.counter);
    Serial.print(F(" float="));
    Serial.print(rx_data.dummy_float, 2);
    Serial.print(F(" time="));
    Serial.print(rx_data.timestamp);
    Serial.print(F(" bytes=["));
    for (int i = 0; i < 5; i++) {
      Serial.print(rx_data.dummy_bytes[i]);
      if (i < 4) Serial.print(F(","));
    }
    Serial.println(F("]"));
    
    last_rx_ms = now;
  }
  
  // Show status every 10 seconds
  if (now - last_rx_ms >= 10000 && last_rx_ms > 0) {
    Serial.println(F("No data received in 10 seconds"));
    last_rx_ms = now;
  }
#endif
  
  delay(100);
}
