
#include "pch.h"
#include "UserInterface.h"
#include "libusbcpp.h"
#include "ODrive.h"

class BatteryApp : public Battery::Application {

	// Here you can store data for the entire application, but it is encouraged 
	// to only use layers and then store the actual data in the corresponding layer
	std::shared_ptr<UserInterface> ui;		// This is the user interface layer based on ImGui

	std::unique_ptr<libusb::DeviceList> devList;
	std::vector<std::reference_wrapper<libusb::Device>> ODriveUSB;
	std::vector<ODrive> ODrives;

public:
	BatteryApp();

	void enumerateODrives();

	bool OnStartup() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnShutdown() override;
	void OnEvent(Battery::Event* e) override;

};
