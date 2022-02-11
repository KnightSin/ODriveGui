#pragma once

#include "pch.h"
#include "Fonts.h"

#include "config.h"

class GraphPanel : public Battery::ImGuiPanel<> {
public:

	GraphPanel() : Battery::ImGuiPanel<>("GraphPanel", { 0, 0 }, { 400, 0 }) {

	}

	void OnUpdate() override {
		size = { Battery::GetMainWindow().GetSize().x - CONTROL_PANEL_WIDTH , Battery::GetMainWindow().GetSize().y - STATUS_BAR_HEIGHT };
		position = { CONTROL_PANEL_WIDTH, 0 };
	}

	void OnRender() override {
		auto* fonts = GetFontContainer<FontContainer>();
		ImGui::PushFont(fonts->openSans25);

		ImGui::ShowDemoWindow();

		ImGui::PopFont();
	}
};
