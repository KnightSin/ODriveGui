
#include "pch.h"
#include "BatteryApp.h"
#include "Battery/AllegroDeps.h"
#include "Backend.h"

#define UPDATE_CACHE_FREQUENCY 5.f

BatteryApp::BatteryApp() : Battery::Application(1280, 720, "ODriveGui") {
	LOG_SET_LOGLEVEL(BATTERY_LOG_LEVEL_DEBUG);
	libusbcpp::setLogLevel(libusbcpp::LOG_LEVEL_WARN);
}

bool BatteryApp::OnStartup() {

	for (size_t i = 1; i < args.size(); i++) {
		if (args[i] == "--verbose") {
			LOG_SET_LOGLEVEL(BATTERY_LOG_LEVEL_DEBUG);
			libusbcpp::setLogLevel(libusbcpp::LOG_LEVEL_DEBUG);
			LOG_INFO("Verbose logging enabled, set log level to LOG_LEVEL_DEBUG");
		}
		else if (args[i] == "--trace") {
			LOG_SET_LOGLEVEL(BATTERY_LOG_LEVEL_TRACE);
			libusbcpp::setLogLevel(libusbcpp::LOG_LEVEL_TRACE);
			LOG_INFO("Trace logging enabled, set log level to LOG_LEVEL_TRACE");
		}
		else {
			LOG_ERROR("[{}]: Unknown parameter! Available:", args[i]);
			LOG_ERROR("                                       --verbose  -> Debug logging");
			LOG_ERROR("                                       --trace    -> All the logging");
			CloseApplication();
		}
	}

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
	backend->handleNewDevices();
	// Request all errors as a health check of the connection
	if (framecount % 30 == 1) {
		for (int i = 0; i < MAX_NUMBER_OF_ODRIVES; i++) {
			if (backend->odrives[i]) {
				backend->odrives[i]->updateErrors();
			}
		}
	}
}

void BatteryApp::OnRender() {

}

void BatteryApp::OnShutdown() {
	shouldClose = true;
	window.Hide();
	backendUpdateThread.join();
	backend.reset();
}

void BatteryApp::OnEvent(Battery::Event* e) {
	if (e->GetType() == Battery::EventType::WindowClose) {
		CloseApplication();
	}
	else if (e->GetType() == Battery::EventType::KeyPressed) {
		if (static_cast<Battery::KeyPressedEvent*>(e)->keycode == ALLEGRO_KEY_SPACE) {
			// Space pressed
		}
	}
}

Battery::Application* Battery::CreateApplication() {
	return new BatteryApp();
}

