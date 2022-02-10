
#include "pch.h"
#include "BatteryApp.h"

void BatteryApp::enumerateODrives() {

	LOG_DEBUG("Scanning devices...");
	devList = std::make_unique<libusb::DeviceList>();
	LOG_DEBUG("Done");

	for (auto& device : devList->getDevices()) {
		//LOG_DEBUG("Device: VID={:04x}, PID={:04x} -> {}", device.info.vendorID, device.info.productID, device.info.description);
		if (device.info.vendorID == 0x1209 && device.info.productID == 0x0D32) {
			LOG_ERROR("ODrive V3.6 was detected!");
			ODriveUSB.push_back(device);
		}
	}

	for (auto& odrv : ODriveUSB) {
		odrv.get().open(2);
		ODrives.push_back(ODrive(odrv.get()));
	}
}

BatteryApp::BatteryApp() : Battery::Application(600, 600, "MyApplication") {
	LOG_SET_LOGLEVEL(BATTERY_LOG_LEVEL_DEBUG);
	//libusb::init();
}

bool BatteryApp::OnStartup() {

	ui = std::make_shared<UserInterface>();
	PushOverlay(ui);
	
	enumerateODrives();

	if (ODrives.size() == 0) {
		LOG_ERROR("No ODrives have been found!");
		return false;
	}

	return true;
}

void BatteryApp::OnUpdate() {

	ODrives[0].sendRequest(0x0180, 0x0400, {}, 0x409B);
	//ODrives[0].write(&buffer[0], buffer.size());

	auto received = ODrives[0].read(32);
	std::cout << "Received [" << received.size() << "]:";
	for (size_t i = 0; i < received.size(); i++) {
		std::cout << (int)received[i] << " ";
	}
	std::cout << std::endl;

}

void BatteryApp::OnRender() {

}

void BatteryApp::OnShutdown() {

}

void BatteryApp::OnEvent(Battery::Event* e) {
	if (e->GetType() == Battery::EventType::WindowClose) {
		CloseApplication();
	}
}

Battery::Application* Battery::CreateApplication() {
	return new BatteryApp();
}

