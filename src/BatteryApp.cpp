
#include "pch.h"
#include "BatteryApp.h"

// Here you can define the start size of the window and 
// optionally the application name (%appdata% folder and the window title)
BatteryApp::BatteryApp() : Battery::Application(600, 600, "MyApplication") {	// Called before anything else, engine core not initialized yet!
	LOG_SET_LOGLEVEL(BATTERY_LOG_LEVEL_DEBUG);
}

bool BatteryApp::OnStartup() {		// Called once on startup, engine is initializedby  now

	// Here you can initialize everything

	ui = std::make_shared<UserInterface>();		// User interface layer is created and pushed onto the layer stack as an overlay
	PushOverlay(ui);							// PushLayer() or PushOverlay(), overlays are always above normal layers

	LOG_DEBUG("Application loaded");
	return true;
}

void BatteryApp::OnUpdate() {		// Called every frame before render, used for calculations

}

void BatteryApp::OnRender() {		// Called every frame after update, used to display calculated results onto the screen

}

void BatteryApp::OnShutdown() {		// Called once on shutdown, engine is still initialized

}

void BatteryApp::OnEvent(Battery::Event* e) {		// Called when an event arrives
	if (e->GetType() == Battery::EventType::WindowClose) {		// This filters the event type to a WindowClose event (X pressed)
		CloseApplication();
	}
}

// And finally, this creates the actual application from your class defined above.
Battery::Application* Battery::CreateApplication() {	// DO NOT ALTER!
	return new BatteryApp();
}

