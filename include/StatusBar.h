#pragma once

#include "pch.h"
#include "Fonts.h"

#include <map>

#include "ODrive.h"
#include "ODriveDocs.h"
#include "Backend.h"
#include "config.h"

#define ENDPOINT_TREE_INDENT 30

class StatusBar : public Battery::ImGuiPanel<> {

	int odriveSelected = 0;
	bool openODriveInfo = false;
	bool openEndpointSelector = false;

	float windowWidth = 0.f;
	float windowHeight = 0.f;

public:
	FontContainer* fonts = nullptr;

	StatusBar() : Battery::ImGuiPanel<>("StatusBar", { 0, 0 }, { 400, 0 }, 
		DEFAULT_IMGUI_PANEL_FLAGS | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_HorizontalScrollbar)
	{
	}

	void OnUpdate() override {
		size = { Battery::GetMainWindow().GetSize().x, STATUS_BAR_HEIGHT };
		position = { 0, Battery::GetMainWindow().GetSize().y - STATUS_BAR_HEIGHT };

		windowWidth = Battery::GetMainWindow().GetSize().x;
		windowHeight = Battery::GetMainWindow().GetSize().y;
	}

	void makeODriveClickableFields() {

		ImGui::Columns(4);

		for (int i = 0; i < MAX_NUMBER_OF_ODRIVES; i++) {
			ImGui::SetColumnWidth(i, STATUS_BAR_ELEMENTS_WIDTH);
			auto& odrive = backend->odrives[i];
			if (odrive) {

				ImGui::SetCursorPosY(0);
				openODriveInfo = ImGui::Selectable(("##Selectable" + std::to_string(i)).c_str(), false, 0, ImVec2(size.x / 4.f - 15, 45));
				if (openODriveInfo) {
					odriveSelected = i;
				}

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50);
				ImGui::SetCursorPosY(8.5);

				if (backend->odrives[i]->error) {
					ImGui::TextColored(RED, "odrv%d", i);
				}
				else {
					ImGui::Text("odrv%d", i);
				}
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
	}

	template<typename T>
	void errorTooltip(std::shared_ptr<ODrive>& odrive, std::map<std::string, std::string>& desc, int32_t& error) {
		
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::PushFont(fonts->openSans21);

			for (size_t i = 0; i < T::_size(); i++) {
				auto enumValue = T::_from_index((int32_t)i);
				if (error & (int32_t)enumValue) {
					auto& enumName = std::string(enumValue._to_string());
					auto& enumDesc = desc[enumName.c_str()];

					ImGui::TextColored(RED, "%s", enumName.c_str());
					if (enumDesc.length() > 0) {
						ImGui::SameLine();
						ImGui::Text(" -> %s", enumDesc.c_str());
					}
				}
			}
			ImGui::PopFont();
			ImGui::EndTooltip();
		}
	}

	void drawODriveInfoWindow() {
		
		if (openODriveInfo) {
			openODriveInfo = false;
			ImGui::OpenPopup("ODriveInfo");
		}
		
		// Handle the odrive info popup
		ImGui::SetNextWindowPos({ (float)STATUS_BAR_ELEMENTS_WIDTH * odriveSelected, windowHeight - STATUS_BAR_HEIGHT - ODRIVE_POPUP_HEIGHT });
		ImGui::SetNextWindowSize({ STATUS_BAR_ELEMENTS_WIDTH, ODRIVE_POPUP_HEIGHT });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12, 12 });
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

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 20 });

			ImGui::Text("JSON CRC: ");
			ImGui::SameLine();
			ImGui::TextColored(LIGHT_BLUE, "0x%02X", odrive->jsonCRC);
			ImGui::PopStyleVar();

			if (odrive->connected) {
				openEndpointSelector = ImGui::Button("Show endpoints", { -1, 40 });

				ImGui::Text("Axis error: ");
				ImGui::SameLine();
				if (odrive->axisError) {
					ImGui::TextColored(RED, "0x%04X", odrive->axisError);
					errorTooltip<AxisError>(odrive, AxisErrorDesc, odrive->axisError);
				}
				else {
					ImGui::TextColored(GREEN, "None");
				}

				ImGui::Text("Motor error: ");
				ImGui::SameLine();
				if (odrive->motorError) {
					ImGui::TextColored(RED, "0x%04X", odrive->motorError);
					errorTooltip<MotorError>(odrive, MotorErrorDesc, odrive->motorError);
				}
				else {
					ImGui::TextColored(GREEN, "None");
				}

				ImGui::Text("Encoder error: ");
				ImGui::SameLine();
				if (odrive->encoderError) {
					ImGui::TextColored(RED, "0x%04X", odrive->encoderError);
					errorTooltip<EncoderError>(odrive, EncoderErrorDesc, odrive->encoderError);
				}
				else {
					ImGui::TextColored(GREEN, "None");
				}

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 20 });

				ImGui::Text("Controller error: ");
				ImGui::SameLine();
				if (odrive->controllerError) {
					ImGui::TextColored(RED, "0x%04X", odrive->controllerError);
					errorTooltip<ControllerError>(odrive, ControllerErrorDesc, odrive->controllerError);
				}
				else {
					ImGui::TextColored(GREEN, "None");
				}


				if (ImGui::Button("Clear errors", { -1, 40 })) {
					odrive->executeFunction("axis0.clear_errors");
				}

				ImGui::PopStyleVar();
			}

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
	}

	template<typename T>
	void drawEndpointValue(ImVec4 color, Endpoint& ep, const char* fmt) {

		EndpointValue& v = backend->getCachedEndpointValue(ep->fullPath);
		T value = 0;
		if (v.type() != EndpointValueType::INVALID) {
			value = v.get<T>();
		}

		const std::string& enumName = EndpointValueToEnumName(ep.basic, (int32_t)value);
		if (enumName.length() > 0) {
			ImGui::TextColored(color, "%s", enumName.c_str());
		}
		else {
			ImGui::TextColored(color, fmt, value);
		}

		if (ImGui::IsItemHovered()) {
			ImGui::PushFont(fonts->openSans21);
			ImGui::BeginTooltip();
			ImGui::TextColored(color, "%s", ep->type.c_str());

			if (enumName.length() > 0) {
				ImGui::SameLine();
				ImGui::Text("->");
				ImGui::SameLine();
				ImGui::TextColored(color, "%s (0x%X)", enumName.c_str(), value);
			}

			ImGui::EndTooltip();
			ImGui::PopFont();
		}
	}

	void drawEndpointValueBool(ImVec4 color, Endpoint& ep) {

		EndpointValue& v = backend->getCachedEndpointValue(ep->fullPath);

		ImGui::TextColored(color, "%s", v.get<bool>() ? "true" : "false");

		if (ImGui::IsItemHovered()) {
			ImGui::PushFont(fonts->openSans21);
			ImGui::BeginTooltip();
			ImGui::TextColored(color, "%s", ep->type.c_str());
			ImGui::EndTooltip();
			ImGui::PopFont();
		}
	}
	
	void drawEndpoint(Endpoint& ep, int indent) {

		ImGui::SetCursorPosX(indent);

		if (ep.children.size() > 0) {	// It's a node with children
			if (ImGui::TreeNode((ep->identifier + "##" + ep->fullPath).c_str())) {
				for (Endpoint& e : ep.children) {
					drawEndpoint(e, indent + ENDPOINT_TREE_INDENT);
				}
				ImGui::TreePop();
			}
		}
		else if (ep->type == "function") {		// It's a function
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 40);

			ImGui::Text("%s()   ->", ep->identifier.c_str());
			ImGui::SameLine();
			ImGui::TextColored(COLOR_FUNCTION, "function");
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 70);
			if (ImGui::Button(("+##" + ep->fullPath).c_str(), { 40, 0 })) {
				Endpoint e = ep;
				e.children.clear();
				backend->addEntry(Entry(e));
			}
		}
		else {			// It's a numeric endpoint with a value
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 40);

			ImGui::Text("%s   = ", ep->identifier.c_str());
			ImGui::SameLine();

			if (ep->type == "float") {
				drawEndpointValue<float>(COLOR_FLOAT, ep, "%.03ff");
			}
			else if (ep->type == "uint8") {
				drawEndpointValue<uint8_t>(COLOR_UINT, ep, "%d");
			}
			else if (ep->type == "uint16") {
				drawEndpointValue<uint16_t>(COLOR_UINT, ep, "%d");
			}
			else if (ep->type == "uint32") {
				drawEndpointValue<uint32_t>(COLOR_UINT, ep, "%d");
			}
			else if (ep->type == "uint64") {
				drawEndpointValue<uint64_t>(COLOR_UINT, ep, "%d");
			}
			else if (ep->type == "int32") {
				drawEndpointValue<uint32_t>(COLOR_UINT, ep, "%d");
			}
			else if (ep->type == "bool") {
				drawEndpointValueBool(COLOR_BOOL, ep);
			}

			ImGui::SameLine();
			ImGui::Text("                ");
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 70);
			if (ImGui::Button(("+##" + ep->fullPath).c_str(), { 40, 0 })) {
				Endpoint e = ep;
				e.children.clear();
				backend->addEntry(Entry(e));
			}
		}
	}

	void drawEndpointList() {

		if (!backend->odrives[odriveSelected])
			return;

		for (Endpoint& ep : backend->odrives[odriveSelected]->endpoints) {
			drawEndpoint(ep, ImGui::GetCursorPosX());
		}
	}

	void drawEndpointSelectorWindow() {

		if (openEndpointSelector) {
			openEndpointSelector = false;
			backend->updateEndpointCache(odriveSelected);
			ImGui::OpenPopup("EndpointSelector");
		}

		// Endpoint selector
		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSizeConstraints({ ENDPOINT_SELECTOR_WIDTH, -1 }, { windowWidth, -1 });
		if (ImGui::BeginPopupContextWindow("EndpointSelector")) {
			auto odrive = backend->odrives[std::clamp(odriveSelected, 0, 3)];

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 15 });
			ImGui::Text("Endpoints of odrv%d:", odriveSelected);
			ImGui::Separator();
			ImGui::PopStyleVar();

			drawEndpointList();

			ImGui::EndPopup();
		}
	}

	void OnRender() override {
		fonts = GetFontContainer<FontContainer>();
		ImGui::PushFont(fonts->openSans25);

		makeODriveClickableFields();

		drawODriveInfoWindow();

		drawEndpointSelectorWindow();

		ImGui::PopFont();
	}
};

