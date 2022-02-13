
#include "pch.h"
#include "BatteryApp.h"
#include "Battery/AllegroDeps.h"
#include "Backend.h"

#define UPDATE_CACHE_FREQUENCY 5.f

BatteryApp::BatteryApp() : Battery::Application(1280, 720, "ODriveGui") {
	LOG_SET_LOGLEVEL(BATTERY_LOG_LEVEL_DEBUG);
}

bool BatteryApp::OnStartup() {

	window.SetTitle("ODriveGui");
	backend = std::make_unique<Backend>();

	ui = std::make_shared<UserInterface>();
	PushOverlay(ui);

	backendUpdateThread = std::thread([&] { 
		while (!shouldClose) { 
			backend->updateEntryCache();
			Battery::Sleep(1.f / UPDATE_CACHE_FREQUENCY);
		} 
	});

	return true;
}

void BatteryApp::OnUpdate() {
	static float s = 0;
	if (framecount % 30 == 1) {
		s = Battery::GetRuntime();
		backend->scanDevices();

		// Request all errors as a health check of the connection
		for (int i = 0; i < MAX_NUMBER_OF_ODRIVES; i++) {
			if (backend->odrives[i]) {
				try {
					backend->odrives[i]->updateErrors();
				}
				catch (...) {}
			}
		}
	}
}

void BatteryApp::OnRender() {

}

void BatteryApp::OnShutdown() {
	shouldClose = true;
	backendUpdateThread.join();
	backend.reset();
}

void BatteryApp::OnEvent(Battery::Event* e) {
	if (e->GetType() == Battery::EventType::WindowClose) {
		CloseApplication();
	}
	else if (e->GetType() == Battery::EventType::KeyPressed) {
		if (static_cast<Battery::KeyPressedEvent*>(e)->keycode == ALLEGRO_KEY_SPACE) {
			//std::string json = Backend::getInstance().ODrives[0].getJSON();
			//ui->controlPanel->endpoints = ODrive::generateEndpoints(json, 0);
		}
	}
}

Battery::Application* Battery::CreateApplication() {
	return new BatteryApp();
}

