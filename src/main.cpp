#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>
#include "hydroponic_controller.h"

// Global controller instance for core1 access
HydroponicController* g_controller = nullptr;

// Core 1 entry point wrapper
void core1_entry() {
	if (g_controller) {
		g_controller->core1Entry();
	}
}

int main() {
	HydroponicController controller;
	g_controller = &controller;
	
	// Initialize on Core 0
	controller.begin();
	
	// Launch Core 1 for network and servers
	multicore_launch_core1(core1_entry);
	
	// Wait for Core 1 to initialize
	printf("Waiting for Core 1 initialization...\n");
	sleep_ms(100);
	printf("Both cores running\n\n");
	
	// Core 0 main loop: sensors and control
	while (true) {
		controller.loop();
	}
	
	return 0;
}
