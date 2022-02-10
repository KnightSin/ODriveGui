#pragma once

#include "pch.h"
#include "Fonts.h"

#include "Endpoint.h"
#include "StatusBar.h"

class ControlPanel : public Battery::ImGuiPanel<> {
public:

	std::vector<Endpoint> endpoints;

	ControlPanel() : Battery::ImGuiPanel<>("ControlPanel", { 0, 0 }, { 400, 0 }) {

	}

	void OnUpdate() override {
		size = { Battery::GetMainWindow().GetSize().x, Battery::GetMainWindow().GetSize().y - STATUS_BAR_HEIGHT };
	}

	void drawChildren(const Endpoint& endpoint) {
		if (ImGui::TreeNode((endpoint.name + "##" + std::to_string(endpoint.id)).c_str())) {

			for (const Endpoint& ep : endpoint.children) {

				if (ep.isNumeric) {		// Numeric types
					ImGui::Text(ep.name.c_str());
				}
				else if (ep.isFunction) {	// Function
					ImGui::Text(ep.name.c_str());
				}
				else {	// It's a node with children
					drawChildren(ep);
				}
			}

			ImGui::TreePop();
		}
	}

	void OnRender() override {
		auto* fonts = GetFontContainer<FontContainer>();
		ImGui::PushFont(fonts->openSans25);
		
		for (const Endpoint& ep : endpoints) {

			if (ep.isNumeric) {		// Numeric types
				ImGui::Text(ep.name.c_str());
			}
			else if (ep.isFunction) {	// Function
				ImGui::Text(ep.name.c_str());
			}
			else {	// It's a node with children
				drawChildren(ep);
			}
		}
		ImGui::ShowDemoWindow();

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
