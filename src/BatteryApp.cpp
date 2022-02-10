
#include "pch.h"
#include "BatteryApp.h"
#include "Battery/AllegroDeps.h"
#include "Backend.h"

BatteryApp::BatteryApp() : Battery::Application(1200, 800, "MyApplication") {
	LOG_SET_LOGLEVEL(BATTERY_LOG_LEVEL_DEBUG);
	//libusb::init();
}

bool BatteryApp::OnStartup() {

	ui = std::make_shared<UserInterface>();
	PushOverlay(ui);
	
	Backend::getInstance().init();

	return true;
}

void BatteryApp::OnUpdate() {

	/*ODrives[0].sendReadRequest(0x01, 0x0004, {}, 0x409B);
	//ODrives[0].write(&buffer[0], buffer.size());

	auto received = ODrives[0].read(32);
	std::cout << "Received [" << received.size() << "]:";
	for (size_t i = 0; i < received.size(); i++) {
		std::cout << (int)received[i] << " ";
	}
	std::cout << std::endl;*/

}

void BatteryApp::OnRender() {

}

void BatteryApp::OnShutdown() {
	Backend::getInstance().cleanup();
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

