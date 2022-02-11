#pragma once

#include "pch.h"
#include "Fonts.h"

#include "ControlPanel.h"
#include "GraphPanel.h"
#include "StatusBar.h"

class UserInterface : public Battery::ImGuiLayer<FontContainer> {
public:

	std::shared_ptr<ControlPanel> controlPanel;
	std::shared_ptr<GraphPanel> graphPanel;
	std::shared_ptr<StatusBar> statusBar;

	UserInterface() {}

	void OnImGuiAttach() override {

		controlPanel = std::make_shared<ControlPanel>();
		PushPanel(controlPanel);

		graphPanel = std::make_shared<GraphPanel>();
		PushPanel(graphPanel);

		statusBar = std::make_shared<StatusBar>();
		PushPanel(statusBar);
	}

	void OnImGuiDetach() override {

	}

	void OnImGuiUpdate() override {

	}

	void OnImGuiRender() override {
		auto fonts = GetFontContainer<FontContainer>();

	}

	void OnImGuiEvent(Battery::Event* event) override {

	}
};
