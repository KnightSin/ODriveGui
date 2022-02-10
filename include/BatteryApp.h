
#include "pch.h"
#include "UserInterface.h"
#include "libusbcpp.h"
#include "ODrive.h"

class BatteryApp : public Battery::Application {

	std::shared_ptr<UserInterface> ui;

public:
	BatteryApp();

	void enumerateODrives();

	bool OnStartup() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnShutdown() override;
	void OnEvent(Battery::Event* e) override;

};
