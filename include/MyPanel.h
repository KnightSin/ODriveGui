#pragma once

#include "pch.h"
#include "Fonts.h"

class MyPanel : public Battery::ImGuiPanel<> {

	// Here you can store data which is specific to this panel
	float fpsFiltered = 0;
	std::string currentTitle;
	char titleBuffer[64] = "Battery Application";

public:
	MyPanel() : Battery::ImGuiPanel<>("MyPanel", { 0, 0 }, { 400, 0 }) {	// Specify a unique name and the position and size of the panel
		currentTitle = titleBuffer;
		Battery::GetMainWindow().SetTitle(currentTitle);
	}

	void OnUpdate() override {	// Called every frame before render, only for logic

		size.y = Battery::GetMainWindow().GetSize().y;	// This continually overwrites the vertical size with the vertical window size
														// -> When you resize the window vertically, the panel will adjust because of that

		fpsFiltered = fpsFiltered * 0.95 + Battery::GetApp().framerate * 0.05;	// Simple filter

		if (std::string(titleBuffer) != currentTitle) {		// Only update the title when it's changed
			currentTitle = titleBuffer;
			Battery::GetMainWindow().SetTitle(currentTitle);
			LOG_DEBUG("Title was updated to '{}'", currentTitle);
		}
	}

	void OnRender() override {	// Called every frame after update, this is within an ImGui window -> call ImGui widgets directly
		auto fonts = GetFontContainer<FontContainer>();		// Here you can access the fonts from Fonts.h
		
		ImGui::PushFont(fonts->openSans25);

		ImGui::Text("FPS: %.1f", fpsFiltered);
		ImGui::Text("Runtime: %fs", Battery::GetRuntime());
		
		if (ImGui::Button("My Button")) {
			LOG_INFO("My Button was pressed!");
		}

		ImGui::Text("Window title");
		ImGui::SameLine();
		ImGui::PushItemWidth(200);
		ImGui::InputText("##title", titleBuffer, sizeof(titleBuffer));
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ShowHelpMarker("The title is updated whenever the content changes", fonts->openSans18);
		
		//ImGui::ShowDemoWindow();

		ImGui::PopFont();
	}

	void ShowHelpMarker(const char* desc, ImFont* font) {
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::PushFont(font);
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
			ImGui::PopFont();
		}
	}
};
