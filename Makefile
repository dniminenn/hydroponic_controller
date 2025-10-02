.PHONY: all build flash upload-web clean rebuild help

# Configuration
BUILD_DIR := build
WEB_DIR := web
SERIAL_PORT ?= /dev/ttyACM0
PICO_SDK_PATH ?= /usr/share/pico-sdk

# Default target
all: build

help:
	@echo "Hydroponic Controller Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make build       - Build the firmware"
	@echo "  make flash       - Flash firmware to Pico"
	@echo "  make upload-web  - Upload web files via serial"
	@echo "  make all         - Build firmware (default)"
	@echo "  make rebuild     - Clean and rebuild"
	@echo "  make clean       - Clean build directory"
	@echo ""
	@echo "Options:"
	@echo "  SERIAL_PORT=/dev/ttyACM0  - Serial port for upload-web"
	@echo ""
	@echo "Examples:"
	@echo "  make build && make flash && make upload-web"
	@echo "  make upload-web SERIAL_PORT=/dev/ttyUSB0"

build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DPICO_SDK_PATH=$(PICO_SDK_PATH) .. >/dev/null && make -j$$(nproc)

flash: build
	@picotool load $(BUILD_DIR)/hydroponic_controller.uf2 -F 2>/dev/null || \
		(echo "Error: picotool not found or device not in BOOTSEL mode" && exit 1)

upload-web:
	@python3 tools/upload_web_files.py $(SERIAL_PORT) $(WEB_DIR)

clean:
	@rm -rf $(BUILD_DIR)

rebuild: clean build

monitor:
	@minicom -D $(SERIAL_PORT) -b 115200 2>/dev/null || screen $(SERIAL_PORT) 115200

deploy: build flash
	@sleep 3 && $(MAKE) upload-web

