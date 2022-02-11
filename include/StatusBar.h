#pragma once

#include "pch.h"
#include "Fonts.h"

#include <map>

#include "ODrive.h"
#include "Backend.h"
#include "config.h"

class StatusBar : public Battery::ImGuiPanel<> {

	int odriveSelected = 0;
	std::map<std::string, uint64_t> endpointValues;		// uint64_t must be interpreted as the correct data type

public:

	StatusBar() : Battery::ImGuiPanel<>("StatusBar", { 0, 0 }, { 400, 0 }, 
		DEFAULT_IMGUI_PANEL_FLAGS | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar) 
	{
	}

	void OnUpdate() override {
		size = { Battery::GetMainWindow().GetSize().x, STATUS_BAR_HEIGHT };
		position = { 0, Battery::GetMainWindow().GetSize().y - STATUS_BAR_HEIGHT };
	}

	/*void drawChildren(const Endpoint& endpoint) {
		if (ImGui::TreeNode((endpoint.name + "##" + std::to_string(endpoint.id)).c_str())) {

			for (const Endpoint& ep : endpoint.children) {

				if (ep.type == "function") {	// Function
					ImGui::Text(ep.name.c_str());
				}
				else if (ep.type == "object") {		// It's a node with children
					drawChildren(ep);
				}
				else {			// Numeric types
					ImGui::Text(ep.name.c_str());
				}
			}

			ImGui::TreePop();
		}
	}*/

	void OnRender() override {
		auto* fonts = GetFontContainer<FontContainer>();
		ImGui::PushFont(fonts->openSans25);

		float windowWidth = Battery::GetMainWindow().GetSize().x;
		float windowHeight = Battery::GetMainWindow().GetSize().y;

		ImGui::Columns(4);

		for (int i = 0; i < MAX_NUMBER_OF_ODRIVES; i++) {
			auto& odrive = backend->odrives[i];
			if (odrive) {

				ImGui::SetCursorPosY(0);
				if (ImGui::Selectable(("##Selectable" + std::to_string(i)).c_str(), false, 0, ImVec2(size.x / 4.f - 15, 45))) {
					ImGui::OpenPopup("ODriveInfo");
					odriveSelected = i;
				}

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50);
				ImGui::SetCursorPosY(8.5);

				ImGui::Text("odrv%d", i);
				ImGui::SameLine();
				if (backend->odrives[i]->connected) {
					ImGui::TextColored(GREEN, "[Connected]");
				}
				else {
					ImGui::TextColored(RED, "[Disconnected]");
				}
			}

			ImGui::NextColumn();
		}

		// Handle the odrive info popup
		bool openEndpointSelector = false;
		float popupHeight = 400;
		ImGui::SetNextWindowPos({ windowWidth / 4.f * odriveSelected, windowHeight - STATUS_BAR_HEIGHT - popupHeight });
		ImGui::SetNextWindowSize({ Battery::GetMainWindow().GetSize().x / 4.f, popupHeight });
		if (ImGui::BeginPopupContextWindow("ODriveInfo")) {
			auto odrive = backend->odrives[std::clamp(odriveSelected, 0, 3)];

			static float vbus_voltage = 0.f;

			if (Battery::GetApp().framecount % 10 == 0) {
				try {
					vbus_voltage = odrive->read<float>("vbus_voltage");
				}
				catch (...) {}
			}

			ImGui::Text("Serial number: 0x%08X", odrive->serialNumber);

			ImGui::Text("Voltage: ");
			ImGui::SameLine();
			ImGui::TextColored(GREEN, "%.03f V", vbus_voltage);

			ImGui::Text("JSON CRC: ");
			ImGui::SameLine();
			ImGui::TextColored(LIGHT_BLUE, "0x%02X", odrive->jsonCRC);

			//ImGui::Text("JSON CRC: ");
			//ImGui::SameLine();
			//ImGui::TextColored(LIGHT_BLUE, "0x%02X", odrive->jsonCRC);
			if (odrive->connected) {
				if (ImGui::Button("Endpoint list")) {
					openEndpointSelector = true;
				}
			}

			ImGui::EndPopup();
		}

		if (openEndpointSelector) {
			ImGui::OpenPopup("EndpointSelector");
			updateEndpointValues();
		}

		// Endpoint selector
		float endpointSelectorWidth = 400;
		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSizeConstraints({ endpointSelectorWidth, -1 }, { windowWidth, -1 });
		if (ImGui::BeginPopupContextWindow("EndpointSelector")) {
			auto odrive = backend->odrives[std::clamp(odriveSelected, 0, 3)];

			ImGui::Text("Endpoints of odrv%d:", odriveSelected);
			ImGui::Separator();

			showEndpoints(odriveSelected);

			ImGui::EndPopup();
		}

		ImGui::PopFont();
	}

	void addEndpoint(std::reference_wrapper<Endpoint> endpoint, int odriveID) {
		backend->addEndpoint(endpoint, odriveID);
	}

	template<typename T>
	T getEndpointValue(const std::string& identifier) {
		uint64_t rawValue = endpointValues[identifier];
		T value;
		memcpy(&value, &rawValue, sizeof(value));
		return value;
	}

	void showEndpoint(Endpoint& ep, int indent) {

		std::string path = "odrv" + std::to_string(odriveSelected) + "." + ep.identifier;
		ImGui::SetCursorPosX(indent);

		if (ep.children.size() > 0) {	// It's a node with children
			if (ImGui::TreeNode(path.c_str())) {
				for (Endpoint& e : ep.children) {
					showEndpoint(e, indent + 30);
				}
				ImGui::TreePop();
			}
		}
		else if (ep.type == "function") {		// It's a function
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 40);

			ImGui::Text("%s()   ->", path.c_str());
			ImGui::SameLine();
			ImGui::TextColored(COLOR_FUNCTION, "function");
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 70);
			if (ImGui::Button(("+##" + ep.identifier).c_str(), { 40, 0 })) {
				addEndpoint(ep, odriveSelected);
			}
		}
		else {			// It's a numeric endpoint with a value
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 40);

			ImGui::Text("%s   = ", path.c_str());
			ImGui::SameLine();

			if (ep.type == "float") {
				ImGui::TextColored(COLOR_FLOAT, "%.03ff", getEndpointValue<float>(ep.identifier));
			}
			else if (ep.type == "uint8") {
				ImGui::TextColored(COLOR_UINT, "%d", getEndpointValue<uint8_t>(ep.identifier));
			}
			else if (ep.type == "uint16") {
				ImGui::TextColored(COLOR_UINT, "%d", getEndpointValue<uint16_t>(ep.identifier));
			}
			else if (ep.type == "uint32") {
				ImGui::TextColored(COLOR_UINT, "%d", getEndpointValue<uint32_t>(ep.identifier));
			}
			else if (ep.type == "int32") {
				ImGui::TextColored(COLOR_UINT, "%d", getEndpointValue<int32_t>(ep.identifier));
			}
			else if (ep.type == "uint64") {
				ImGui::TextColored(COLOR_UINT, "%llu", getEndpointValue<uint64_t>(ep.identifier));
			}
			else if (ep.type == "bool") {
				ImGui::TextColored(COLOR_BOOL, getEndpointValue<bool>(ep.identifier) ? "true" : "false");
			}
			ImGui::SameLine();
			ImGui::Text("                ");
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 70);
			if (ImGui::Button(("+##" + ep.identifier).c_str(), { 40, 0 })) {
				addEndpoint(ep, odriveSelected);
			}
		}
	}

	void showEndpoints(int odriveID) {
		auto& odrive = backend->odrives[odriveID];
		if (!odrive)
			return;

		for (Endpoint& ep : odrive->endpoints) {
			showEndpoint(ep, ImGui::GetCursorPosX());
		}
	}

	template<typename T>
	void readEndpointValue(Endpoint& endpoint) {
		T value = backend->odrives[odriveSelected]->read<T>(endpoint.identifier);
		uint64_t rawValue;
		memcpy(&rawValue, &value, sizeof(value));
		endpointValues[endpoint.identifier] = rawValue;
	}

	void updateEndpointValue(Endpoint& endpoint) {

		try {
			if (endpoint.children.size() > 0) {	// It's a node with children
				for (Endpoint& ep : endpoint.children) {
					updateEndpointValue(ep);
				}
			}
			else if (endpoint.type != "function") {		// It's a numeric endpoint with a value

				if (endpoint.type == "float") {
					readEndpointValue<float>(endpoint);
				}
				else if (endpoint.type == "bool") {
					readEndpointValue<bool>(endpoint);
				}
				else if (endpoint.type == "uint8") {
					readEndpointValue<uint8_t>(endpoint);
				}
				else if (endpoint.type == "uint16") {
					readEndpointValue<uint8_t>(endpoint);
				}
				else if (endpoint.type == "uint32") {
					readEndpointValue<uint32_t>(endpoint);
				}
				else if (endpoint.type == "int32") {
					readEndpointValue<int32_t>(endpoint);
				}
				else if (endpoint.type == "uint64") {
					readEndpointValue<uint64_t>(endpoint);
				}
			}
		}
		catch (...) {}
	}

	void updateEndpointValues() {
		for (Endpoint& ep : backend->odrives[odriveSelected]->endpoints) {
			updateEndpointValue(ep);
		}
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

