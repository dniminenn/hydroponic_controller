/*
 * Arduino Nano ADC + NRF24L01 Transmitter
 * 
 * Replaces I2C communication with wireless NRF24L01
 * 
 * Hardware:
 *   - Arduino Nano
 *   - NRF24L01+PA+LNA socket adapter (plugs into Nano headers)
 *   - pH or TDS sensor on A0
 * 
 * The socket adapter:
 *   - Has NRF24L01+PA+LNA pre-mounted
 *   - Has built-in 3.3V regulator
 *   - Plugs directly into Arduino headers
 *   - Auto-maps pins: CE=D9, CSN=D10, SPI on D11/D12/D13
 */

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <avr/wdt.h>

// ===== CONFIGURATION =====
#define SENSOR_TYPE_PH  1   // Set to 1 for pH sensor, 0 for TDS sensor
#define SAMPLES 10
#define USE_EMA true
#define EMA_ALPHA 0.333
#define TX_INTERVAL_MS 1000  // Send every 1 second
#define TX_TIMEOUT_MS 60000  // Restart if no successful TX for 60 seconds

// NRF24L01 pins
#define NRF_CE_PIN  9
#define NRF_CSN_PIN 10

// RF configuration (must match receiver)
#define RF_CHANNEL 76

// Pipe addresses (must match receiver config.h)
const uint8_t ADDR_PH[5]  = {'p', 'H', 's', 'n', 's'};
const uint8_t ADDR_TDS[5] = {'T', 'D', 'S', 's', 'n'};

// Transform functions
float raw(int adc) { return (float)adc; }
float voltage(int adc) { return adc * 5.0 / 1023.0; }
float ph(int adc) { return 7.0 + ((587.0 - adc) / 39.0); }
float tds(int adc) { return adc * 2.31; }

// Sensor config
struct SensorConfig {
  uint8_t pin;
  float (*transform)(int);
};

SensorConfig sensors[] = {
#if SENSOR_TYPE_PH
  {A0, ph},    // pH sensor
#else
  {A0, tds},   // TDS sensor
#endif
  {A1, raw},   // Spare
  {A2, raw},   // Spare
  {A3, raw}    // Spare
};
const int NUM_SENSORS = sizeof(sensors) / sizeof(sensors[0]);

// ===== END CONFIGURATION =====

RF24 radio(NRF_CE_PIN, NRF_CSN_PIN);
float ema_values[4];
float current_values[4];
bool first_run = true;
uint32_t last_tx_ms = 0;
uint8_t tx_fail_count = 0;

void setup() {
  wdt_disable();  // Disable watchdog during setup
  
  Serial.begin(115200);
  Serial.println(F("NRF24L01 Transmitter Starting..."));
  
  // Initialize NRF24L01
  if (!radio.begin()) {
    Serial.println(F("NRF24L01 initialization failed!"));
    while (1) {
      delay(1000);
    }
  }
  
  // Configure radio
  radio.setPALevel(RF24_PA_HIGH);    // HIGH for stability
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(RF_CHANNEL);
  radio.setRetries(5, 15);           // 5*250us delay, 15 retries
  radio.setAutoAck(true);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  
  // Open writing pipe
#if SENSOR_TYPE_PH
  radio.openWritingPipe(ADDR_PH);
  Serial.println(F("Configured as pH sensor"));
#else
  radio.openWritingPipe(ADDR_TDS);
  Serial.println(F("Configured as TDS sensor"));
#endif
  
  radio.stopListening();  // TX mode
  
  // Initialize sensor pins
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(sensors[i].pin, INPUT);
  }
  
  Serial.println(F("Ready to transmit"));
  Serial.print(F("Channel: "));
  Serial.println(RF_CHANNEL);
  Serial.print(F("Watchdog enabled: "));
  Serial.print(TX_TIMEOUT_MS / 1000);
  Serial.println(F("s timeout"));
  
  wdt_enable(WDTO_8S);  // Enable 8s watchdog
}

void loop() {
  uint32_t now = millis();
  
  // Read sensors
  for (int i = 0; i < NUM_SENSORS; i++) {
    int adc_sum = 0;
    for (int j = 0; j < SAMPLES; j++) {
      adc_sum += analogRead(sensors[i].pin);
      delay(5);
    }
    int adc_avg = adc_sum / SAMPLES;
    float value = sensors[i].transform(adc_avg);
    
    if (USE_EMA) {
      if (first_run) {
        ema_values[i] = value;
      } else {
        ema_values[i] = EMA_ALPHA * value + (1.0 - EMA_ALPHA) * ema_values[i];
      }
      current_values[i] = ema_values[i];
    } else {
      current_values[i] = value;
    }
  }
  first_run = false;
  
  // Pet watchdog regularly (loop runs ~10 times per second)
  wdt_reset();
  
  // Transmit at interval
  if (now - last_tx_ms >= TX_INTERVAL_MS) {
    bool success = radio.write(current_values, sizeof(current_values));
    
    if (success) {
      Serial.print(F("TX OK: "));
      tx_fail_count = 0;
    } else {
      Serial.print(F("TX FAIL: "));
      radio.flush_tx();
      tx_fail_count++;
      
      // If too many consecutive failures, stop petting watchdog
      if (tx_fail_count >= (TX_TIMEOUT_MS / TX_INTERVAL_MS)) {
        Serial.println(F("\n!!! 60 consecutive TX failures - waiting for watchdog reset..."));
        while(1);  // Stop petting watchdog, will reset in 8s
      }
    }
    
    // Print values
    for (int i = 0; i < NUM_SENSORS; i++) {
      Serial.print(current_values[i], 2);
      if (i < NUM_SENSORS - 1) Serial.print(F(", "));
    }
    Serial.println();
    
    last_tx_ms = now;
  }
  
  delay(100);
}
