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

#define MAKE_IMGUI_TEXT_INPUT(type, valueMap, textStatement, STD_TO_NUM) \
	if (!endpoint.readonly) { \
		valueMap[(int)i];	\
		ImGui::SameLine(); \
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200); \
		ImGui::PushItemWidth(100); \
		bool set = false; \
		\
		std::vector<std::string> enumNames = EndpointToEnum(endpoint); \
		if (enumNames.size() > 0) { \
			/*if (ImGui::BeginCombo(("##" + path + std::to_string(i)).c_str(), enumNames[0], 0)) { \
				for (int n = 0; n < enumNames.size(); n++) { \
					const bool is_selected = (item_current_idx == n); \
					if (ImGui::Selectable(items[n], is_selected)) \
						item_current_idx = n; \
\			
					if (is_selected) \
						ImGui::SetItemDefaultFocus(); \
				} \
				ImGui::EndCombo(); \
			} \*/ \
		} \
		else { \
			if (textStatement) { \
				set = true; \
			} \
		} \
		\
		ImGui::PopItemWidth(); \
		ImGui::SameLine(); \
		if (ImGui::Button(("Set##" + path + std::to_string(i)).c_str(), { 60, 0 })) { \
			set = true; \
		} \
		bool load = false;\
		if (set) { \
			try { \
				backend->setValue(path, STD_TO_NUM(std::string(buffers[(int)i], IMGUI_BUFFER_SIZE))); \
			} \
			catch (...) { \
				load = true; \
			} \
		} \
		if (load || std::string(buffers[(int)i]) == "") { \
			std::string value = std::to_string(odrive->read<type>(endpoint.identifier)); \
			strncpy_s(buffers[(int)i], value.c_str(), 16); \
		} \
	}



class ControlPanel : public Battery::ImGuiPanel<> {

	std::map<int, float> floatValues;
	std::map<int, uint8_t> uint8Values;
	std::map<int, uint16_t> uint16Values;
	std::map<int, uint32_t> uint32Values;
	std::map<int, int32_t> int32Values;
	std::map<int, uint64_t> uint64Values;

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
	void drawNumericEndpoint(ImVec4 color, const std::string& path, const char* fmt, Endpoint& ep) {
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

	void drawNumericEndpoint(const std::string& path, size_t i, Endpoint& endpoint, std::shared_ptr<ODrive>& odrive) {
		if (endpoint.type == "float") {
			drawNumericEndpoint<float>(COLOR_FLOAT, path, "%.06ff", endpoint);
			MAKE_IMGUI_TEXT_INPUT(
				float,
				floatValues,
				ImGui::InputText(("##" + path + std::to_string(i)).c_str(), buffers[(int)i], IMGUI_BUFFER_SIZE, IMGUI_FLAGS_FLOAT),
				std::stof
			);
		}
		else if (endpoint.type == "uint8") {
			drawNumericEndpoint<uint8_t>(COLOR_UINT, path, "%d", endpoint);
			MAKE_IMGUI_TEXT_INPUT(
				uint8_t,
				uint8Values,
				ImGui::InputText(("##" + path + std::to_string(i)).c_str(), buffers[(int)i], IMGUI_BUFFER_SIZE, IMGUI_FLAGS_INT),
				std::stoi
			);
		}
		else if (endpoint.type == "uint16") {
			drawNumericEndpoint<uint8_t>(COLOR_UINT, path, "%d", endpoint);
			MAKE_IMGUI_TEXT_INPUT(
				uint16_t,
				uint16Values,
				ImGui::InputText(("##" + path + std::to_string(i)).c_str(), buffers[(int)i], IMGUI_BUFFER_SIZE, IMGUI_FLAGS_INT),
				std::stoi
			);
		}
		else if (endpoint.type == "uint32") {
			drawNumericEndpoint<uint32_t>(COLOR_UINT, path, "%d", endpoint);
			MAKE_IMGUI_TEXT_INPUT(
				uint32_t,
				uint32Values,
				ImGui::InputText(("##" + path + std::to_string(i)).c_str(), buffers[(int)i], IMGUI_BUFFER_SIZE, IMGUI_FLAGS_INT),
				std::stol
			);
		}
		else if (endpoint.type == "int32") {
			drawNumericEndpoint<int32_t>(COLOR_UINT, path, "%d", endpoint);
			MAKE_IMGUI_TEXT_INPUT(
				int32_t,
				int32Values,
				ImGui::InputText(("##" + path + std::to_string(i)).c_str(), buffers[(int)i], IMGUI_BUFFER_SIZE, IMGUI_FLAGS_INT),
				std::stol
			);
		}
		else if (endpoint.type == "uint64") {
			drawNumericEndpoint<uint64_t>(COLOR_UINT, path, "%d", endpoint);
			MAKE_IMGUI_TEXT_INPUT(
				uint64_t,
				uint64Values,
				ImGui::InputText(("##" + path + std::to_string(i)).c_str(), buffers[(int)i], IMGUI_BUFFER_SIZE, IMGUI_FLAGS_INT),
				std::stol
			);
		}
		else if (endpoint.type == "bool") {
			bool v = getEndpointValue<bool>(backend->values[path]);
			drawNumericEndpoint<bool>(COLOR_BOOL, path, v ? "true" : "false", endpoint);
			if (!endpoint.readonly) {
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

					drawNumericEndpoint(path, i, endpoint, odrive);
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
						drawNumericEndpoint(_path, i, ep, odrive);
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
						drawNumericEndpoint(_path, i, ep, odrive);
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
