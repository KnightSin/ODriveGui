#pragma once

#include "pch.h"
#include "Fonts.h"

#include "config.h"
#include "Backend.h"
#include "ODriveDocs.h"

#undef max
#undef min

#define IMGUI_BUFFER_SIZE 16
#define MAX_IDENTIFIER_LENGTH 43

#define IMGUI_FLAGS_FLOAT ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsScientific
#define IMGUI_FLAGS_INT ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal

class ControlPanel : public Battery::ImGuiPanel<> {

	std::map<int, float> floatValues;
	std::map<int, uint8_t> uint8Values;
	std::map<int, uint16_t> uint16Values;
	std::map<int, uint32_t> uint32Values;
	std::map<int, int32_t> int32Values;
	std::map<int, uint64_t> uint64Values;

	std::map<std::string, size_t> dropdownIndices;

	std::map<int, char[IMGUI_BUFFER_SIZE + 1]> buffers;

public:
	ControlPanel() : Battery::ImGuiPanel<>("ControlPanel", { 0, 0 }, { 400, 0 }) {

	}

	void OnUpdate() override {
		size = { CONTROL_PANEL_WIDTH, Battery::GetMainWindow().GetSize().y - STATUS_BAR_HEIGHT };
	}
	
	template<typename T>
	T getEndpointValue(uint64_t rawValue) {
		T value;
		memcpy(&value, &rawValue, sizeof(value));
		return value;
	}

	template<typename T>
	void renderNumericEndpointValue(ImVec4 color, const std::string& path, const char* fmt, Endpoint& ep) {
		T value = getEndpointValue<T>(backend->values[path]);
		T oldValue = getEndpointValue<T>(backend->oldValues[path]);
		ImVec4 col = (value != oldValue) ? (ImVec4(RED)) : (color);

		const std::string& enumName = EndpointToEnum(ep, value);
		if (enumName.length() > 0) {
			ImGui::TextColored(col, "%s", enumName.c_str());
		}
		else {
			ImGui::TextColored(col, fmt, value);
		}

		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::TextColored(color, "%s", ep.type.c_str());

			if (enumName.length() > 0) {
				ImGui::SameLine();
				ImGui::Text("->");
				ImGui::SameLine();
				ImGui::TextColored(color, "%s (%d)", enumName.c_str(), value);
			}

			ImGui::EndTooltip();
		}
	}

	bool makeImGuiNumberInputField(const std::string& imguiIdentifier, size_t i, int imguiFlags) {
		return ImGui::InputText(imguiIdentifier.c_str(), buffers[(int)i], IMGUI_BUFFER_SIZE, imguiFlags);
	}

	size_t makeImGuiDropdownField(const std::string& imguiIdentifier, std::vector<std::string>& enumNames, size_t i, int imguiFlags) {
		std::string currentItem = enumNames[0];

		if (ImGui::BeginCombo(imguiIdentifier.c_str(), currentItem.c_str(), 0)) {
			for (size_t i = 0; i < enumNames.size(); i++) {
				std::string name = enumNames[i] + " (" + std::to_string(i) + ")" + imguiIdentifier;
				bool selected = (dropdownIndices[imguiIdentifier] == i);
				if (ImGui::Selectable(name.c_str(), selected)) {
					dropdownIndices[imguiIdentifier] = i;
					return i;
				}
			}
			ImGui::EndCombo();
		}
		return -1;
	}

	template<typename T, typename mapT, typename callbackT>
	void makeImGuiNumberInput(std::shared_ptr<ODrive>& odrive, Endpoint& ep, const std::string& path, mapT& valueMap, size_t i, int imguiFlags, callbackT (*sto_num)(const char*)) {
		if (!ep.readonly) {
			valueMap[(int)i];
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
			ImGui::PushItemWidth(100);
			bool set = false;
			
			std::vector<std::string> enumNames = EndpointToEnum(ep);
			if (enumNames.size() > 0) {
				size_t index = makeImGuiDropdownField("##" + path + std::to_string(i), enumNames, i, imguiFlags);
				if (index != -1) {
					backend->setValue<T>(path, (T)EndpointEnumToValue(ep, index));
				}
			}
			else {
				if (makeImGuiNumberInputField("##" + path + std::to_string(i), i, imguiFlags)) {
					set = true;
				}
			}
			
			ImGui::PopItemWidth();
			ImGui::SameLine();
			if (ImGui::Button(("Set##" + path + std::to_string(i)).c_str(), { 60, 0 })) {
				set = true;
			}

			bool load = false;
			if (set) {
				try {
					backend->setValue<T>(path, (T)sto_num(std::string(buffers[(int)i], IMGUI_BUFFER_SIZE).c_str()));
				}
				catch (...) {
					load = true;
				}
			}
			if (load || std::string(buffers[(int)i]) == "") {
				std::string value = std::to_string(odrive->read<T>(ep.identifier));
				strncpy_s(buffers[(int)i], value.c_str(), 16);
			}
		}
	}

	void makeImGuiBoolInput(const std::string& path, size_t i) {
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
		if (ImGui::Button(("false##" + path + std::to_string(i)).c_str(), { 90, 0 })) {
			backend->setValue(path, false);
		}
		ImGui::SameLine();
		if (ImGui::Button(("true##" + path + std::to_string(i)).c_str(), { 90, 0 })) {
			backend->setValue(path, true);
		}
	}

	void renderNumericEndpointValue(const std::string& path, size_t i, Endpoint& endpoint, std::shared_ptr<ODrive>& odrive) {
		if (endpoint.type == "float") {
			renderNumericEndpointValue<float>(COLOR_FLOAT, path, "%.06ff", endpoint);
			makeImGuiNumberInput<float>(odrive, endpoint, path, floatValues, i, IMGUI_FLAGS_FLOAT, atof);
		}
		else if (endpoint.type == "uint8") {
			renderNumericEndpointValue<uint8_t>(COLOR_UINT, path, "%d", endpoint);
			makeImGuiNumberInput<uint8_t>(odrive, endpoint, path, uint8Values, i, IMGUI_FLAGS_INT, atoi);
		}
		else if (endpoint.type == "uint16") {
			renderNumericEndpointValue<uint16_t>(COLOR_UINT, path, "%d", endpoint);
			makeImGuiNumberInput<uint16_t>(odrive, endpoint, path, uint16Values, i, IMGUI_FLAGS_INT, atoi);
		}
		else if (endpoint.type == "uint32") {
			renderNumericEndpointValue<uint32_t>(COLOR_UINT, path, "%d", endpoint);
			makeImGuiNumberInput<uint32_t>(odrive, endpoint, path, uint32Values, i, IMGUI_FLAGS_INT, atol);
		}
		else if (endpoint.type == "int32") {
			renderNumericEndpointValue<int32_t>(COLOR_UINT, path, "%d", endpoint);
			makeImGuiNumberInput<int32_t>(odrive, endpoint, path, int32Values, i, IMGUI_FLAGS_INT, atol);
		}
		else if (endpoint.type == "uint64") {
			renderNumericEndpointValue<uint64_t>(COLOR_UINT, path, "%d", endpoint);
			makeImGuiNumberInput<uint64_t>(odrive, endpoint, path, uint64Values, i, IMGUI_FLAGS_INT, atoll);
		}
		else if (endpoint.type == "bool") {
			bool v = getEndpointValue<bool>(backend->values[path]);
			renderNumericEndpointValue<bool>(COLOR_BOOL, path, v ? "true" : "false", endpoint);
			if (!endpoint.readonly) {
				makeImGuiBoolInput(path, i);
			}
		}
	}

	void OnRender() override {
		auto* fonts = GetFontContainer<FontContainer>();
		ImGui::PushFont(fonts->openSans25);

		ImGui::PushFont(fonts->openSans21);
		size_t remove = -1;
		try {
			for (size_t i = 0; i < backend->endpoints.size(); i++) {
				int odriveID = backend->endpoints[i].first;
				Endpoint& endpoint = backend->endpoints[i].second;
				auto& odrive = backend->odrives[odriveID];
				std::string path = "odrv" + std::to_string(odriveID) + "." + endpoint.identifier;

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

				if (endpoint.type != "function") {

					if (ImGui::Button(("x##" + path + std::to_string(i)).c_str(), { 40, 0 })) {
						remove = i;
					}
					ImGui::SameLine();
					std::string name = path.substr(std::max((int64_t)path.length() - MAX_IDENTIFIER_LENGTH, 0ll));
					ImGui::Text("%s   =", name.c_str());
					ImGui::SameLine();

					renderNumericEndpointValue(path, i, endpoint, odrive);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
					
				}
				else {					// Function
					
					if (ImGui::Button(("x##" + path + std::to_string(i)).c_str(), { 40, 0 })) {
						remove = i;
					}
					ImGui::SameLine();
					std::string name = path.substr(std::max((int64_t)path.length() - MAX_IDENTIFIER_LENGTH, 0ll));
					ImGui::Text("%s()", name.c_str());
					ImGui::SameLine();
					ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100);
					if (ImGui::Button(("Execute##" + path + std::to_string(i)).c_str(), { 90, 0 })) {
						backend->executeFunction(path);
					}

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
					
					if (endpoint.inputs.size() > 0) {
						ImGui::SetCursorPosX(40);
						ImGui::Text("Inputs:");
						ImGui::SameLine();
					}
					for (size_t j = 0; j < endpoint.inputs.size(); j++) {
						Endpoint& ep = endpoint.inputs[j];
						std::string _path = "odrv" + std::to_string(odriveID) + "." + ep.identifier;

						ImGui::SetCursorPosX(120);
						ImGui::Text("%s   =", ep.identifier.c_str());
						ImGui::SameLine();
						renderNumericEndpointValue(_path, i, ep, odrive);
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
					}

					if (endpoint.outputs.size() > 0) {
						ImGui::SetCursorPosX(40);
						ImGui::Text("Outputs:");
						ImGui::SameLine();
					}
					for (size_t j = 0; j < endpoint.outputs.size(); j++) {
						Endpoint& ep = endpoint.outputs[j];
						std::string _path = "odrv" + std::to_string(odriveID) + "." + ep.identifier;

						ImGui::SetCursorPosX(120);
						ImGui::Text("%s   =", ep.identifier.c_str());
						ImGui::SameLine();
						renderNumericEndpointValue(_path, i, ep, odrive);
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
					}
				}
				ImGui::Separator();
			}
			ImGui::PopFont();

			if (remove != -1) {
				backend->removeEndpoint(remove);
			}
		}
		catch (...) {}

		ImGui::PopFont();
	}
};
