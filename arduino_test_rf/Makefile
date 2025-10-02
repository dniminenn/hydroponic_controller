# NRF24L01 Test Makefile

# Configuration
PORT ?= /dev/ttyUSB0
BAUD = 57600

# AVRDUDE settings
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_MCU = m328p
AVRDUDE_FLAGS = -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P $(PORT) -b $(BAUD)

.PHONY: help build upload monitor clean tx rx

# Default target
help:
	@echo "NRF24L01 Test - Simple TX/RX Test"
	@echo ""
	@echo "Targets:"
	@echo "  build     - Build firmware (PlatformIO)"
	@echo "  upload    - Upload to Nano (avrdude)"
	@echo "  monitor   - Serial monitor"
	@echo "  clean     - Clean build files"
	@echo "  tx        - Build and upload transmitter firmware"
	@echo "  rx        - Build and upload receiver firmware"
	@echo ""
	@echo "Usage:"
	@echo "  make tx PORT=/dev/ttyUSB0   # Build and upload transmitter"
	@echo "  make rx PORT=/dev/ttyACM0   # Build and upload receiver"

# Build firmware using PlatformIO
build:
	pio run

# Upload using avrdude (faster than PlatformIO)
upload:
	@if [ ! -f .pio/build/nano/firmware.hex ]; then \
		echo "Firmware not found. Run 'make build' first."; \
		exit 1; \
	fi
	avrdude $(AVRDUDE_FLAGS) -U flash:w:.pio/build/nano/firmware.hex:i

# Serial monitor
monitor:
	minicom -D $(PORT) -b 115200

# Clean build files
clean:
	pio run --target clean

# Transmitter - set build flag and upload
tx:
	@echo "Building transmitter firmware..."
	@sed -i 's/-DRF_MODE=0/-DRF_MODE=1/' platformio.ini
	pio run
	@echo "Uploading with avrdude..."
	$(MAKE) upload
	@echo "Transmitter firmware uploaded!"

# Receiver - set build flag and upload  
rx:
	@echo "Building receiver firmware..."
	@sed -i 's/-DRF_MODE=1/-DRF_MODE=0/' platformio.ini
	pio run
	@echo "Uploading with avrdude..."
	$(MAKE) upload
	@echo "Receiver firmware uploaded!"
