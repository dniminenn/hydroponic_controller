# Arduino Nano NRF24L01 Transmitter Makefile

# Configuration
PORT ?= /dev/ttyUSB0
BAUD = 57600

# AVRDUDE settings
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_MCU = m328p
AVRDUDE_FLAGS = -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P $(PORT) -b $(BAUD)

.PHONY: help build upload monitor clean ph tds

# Default target
help:
	@echo "Arduino Nano NRF24L01 Transmitter"
	@echo ""
	@echo "Targets:"
	@echo "  build     - Build firmware (PlatformIO)"
	@echo "  upload    - Upload to Nano (avrdude)"
	@echo "  monitor   - Serial monitor"
	@echo "  clean     - Clean build files"
	@echo "  ph        - Build and upload pH sensor firmware"
	@echo "  tds       - Build and upload TDS sensor firmware"
	@echo ""
	@echo "Usage:"
	@echo "  make ph PORT=/dev/ttyUSB0   # Build and upload pH sensor"
	@echo "  make tds PORT=/dev/ttyACM0  # Build and upload TDS sensor"

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

# pH sensor - set build flag and upload
ph:
	@echo "Building pH sensor firmware..."
	@sed -i 's/-DSENSOR_TYPE_PH=[01]/-DSENSOR_TYPE_PH=1/' platformio.ini
	pio run
	@echo "Uploading with avrdude..."
	$(MAKE) upload
	@echo "pH sensor firmware uploaded!"

# TDS sensor - set build flag and upload  
tds:
	@echo "Building TDS sensor firmware..."
	@sed -i 's/-DSENSOR_TYPE_PH=[01]/-DSENSOR_TYPE_PH=0/' platformio.ini
	pio run
	@echo "Uploading with avrdude..."
	$(MAKE) upload
	@echo "TDS sensor firmware uploaded!"
