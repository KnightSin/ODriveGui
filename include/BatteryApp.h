
#include "pch.h"
#include "UserInterface.h"

class BatteryApp : public Battery::Application {

	// Here you can store data for the entire application, but it is encouraged 
	// to only use layers and then store the actual data in the corresponding layer
	std::shared_ptr<UserInterface> ui;		// This is the user interface layer based on ImGui

public:
	BatteryApp();

	bool OnStartup() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnShutdown() override;
	void OnEvent(Battery::Event* e) override;

};
