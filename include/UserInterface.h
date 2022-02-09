#pragma once

#include "pch.h"
#include "Fonts.h"

// Include all your panels
#include "MyPanel.h"

// This is the main user interface layer based on ImGui: Only one instance of it can exist.
// It can draw basic ImGui windows and it can contain any number of Battery::ImGuiPanels.
class UserInterface : public Battery::ImGuiLayer<FontContainer> {
public:

	// Here you can allocate any number of panels for later use
	std::shared_ptr<MyPanel> myPanel;

	UserInterface() {}	// Do not use this contructor

	void OnImGuiAttach() override {		// Called once on startup

		// Here the panels are initialized and then pushed onto the panel stack
		myPanel = std::make_shared<MyPanel>();
		PushPanel(myPanel);
	}

	void OnImGuiDetach() override {		// Called once on shutdown

	}

	void OnImGuiUpdate() override {		// Called every frame before render, only for logic

	}

	void OnImGuiRender() override {		// Called every frame after update, here ImGui can be rendered
		auto fonts = GetFontContainer<FontContainer>();		// Here you can access the fonts from Fonts.h


	}

	void OnImGuiEvent(Battery::Event* event) override {		// Called when an event arrives which wasn't handled yet by the layer above

	}
};
