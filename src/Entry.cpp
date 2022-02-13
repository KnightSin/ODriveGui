
#include "pch.h"
#include "Entry.h"
#include "Backend.h"
#include "config.h"
#include "ODriveDocs.h"

#define IMGUI_FLAGS_FLOAT ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsScientific
#define IMGUI_FLAGS_INT ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal

Entry::Entry(const Endpoint& ep) : endpoint(ep), value(ep.basic.type) {
	entryID = entryIDCounter;
	entryIDCounter++;
	memset(imguiBuffer, 0, sizeof(imguiBuffer));
}

void Entry::updateValue() {

	// Update the changed flags
	oldValues[endpoint->fullPath] = value;
	for (Endpoint& e : endpoint.inputs) {
		oldValues[e->fullPath] = value;
	}
	for (Endpoint& e : endpoint.outputs) {
		oldValues[e->fullPath] = value;
	}

	// And now read
	auto temp = backend->readEndpointDirect(endpoint.basic);
	if (temp.type() != EndpointValueType::INVALID) {
		value = temp;
	}

	for (Endpoint& e : endpoint.inputs) {
		auto temp = backend->readEndpointDirect(e.basic);
		if (temp.type() != EndpointValueType::INVALID) {
			ioValues[e->fullPath] = temp;
		}
	}
	for (Endpoint& e : endpoint.outputs) {
		auto temp = backend->readEndpointDirect(e.basic);
		if (temp.type() != EndpointValueType::INVALID) {
			ioValues[e->fullPath] = temp;
		}
	}
}

bool Entry::drawImGuiNumberInputField(const std::string& imguiIdentifier, bool isfloat) {
	ImGuiInputTextFlags flags = IMGUI_FLAGS_INT;

	if (isfloat)
		flags = IMGUI_FLAGS_FLOAT;

	return ImGui::InputText(imguiIdentifier.c_str(), imguiBuffer, IMGUI_BUFFER_SIZE, flags);
}

void Entry::drawImGuiDropdownField(const std::string& imguiIdentifier, const std::vector<std::string>& enumNames) {

	if (ImGui::BeginCombo(imguiIdentifier.c_str(), enumNames[selected].c_str(), 0)) {
		for (size_t i = 0; i < enumNames.size(); i++) {
			size_t enumValue = EnumIndexToValue(endpoint.basic, i);
			std::stringstream name;
			name << enumNames[i] << " (0x" << std::hex << enumValue << ")" << imguiIdentifier;
			bool sel = (selected == i);
			if (ImGui::Selectable(name.str().c_str(), sel)) {
				selected = i;
			}
		}
		ImGui::EndCombo();
	}
}

void Entry::drawImGuiNumberInput(Endpoint& ep, bool isfloat) {

	ImGui::SameLine();
	bool set = false;

	EndpointValue writeValue(this->value.type());
	std::vector<std::string> enumNames = ListEnumValues(ep.basic);
	if (enumNames.size() > 0) {
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 300);
		ImGui::PushItemWidth(200);
		drawImGuiDropdownField("##" + ep->fullPath + std::to_string(entryID), enumNames);
		writeValue.set<uint64_t>(EnumIndexToValue(endpoint.basic, selected));
	}
	else {
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
		ImGui::PushItemWidth(100);
		if (drawImGuiNumberInputField("##" + ep->fullPath + std::to_string(entryID), isfloat)) {
			set = true;
			ImGui::SetKeyboardFocusHere(-1);
		}
		writeValue.fromString(imguiBuffer);
	}

	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (ImGui::Button(("Set##" + ep->fullPath + std::to_string(entryID)).c_str(), { 60, 0 })) {
		set = true;
	}

	bool load = false;
	if (set) {
		try {
			backend->writeEndpointDirect(ep.basic, writeValue);
			backend->updateEntryCache();
			LOG_DEBUG("Setting {} to {}", ep->fullPath, writeValue.toString());
		}
		catch (...) {
			load = true;
		}
	}
	if (load || std::string(imguiBuffer) == "") {
		EndpointValue value = backend->readEndpointDirect(ep.basic);
		if (value.type() != EndpointValueType::INVALID) {
			strncpy_s(imguiBuffer, value.toString().c_str(), IMGUI_BUFFER_SIZE);
		}
	}
}

void Entry::drawImGuiBoolInput(Endpoint& ep) {
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
	if (ImGui::Button(("false##" + ep->fullPath + std::to_string(entryID)).c_str(), { 90, 0 })) {
		backend->writeEndpointDirect(ep.basic, false);
		backend->updateEntryCache();
	}
	ImGui::SameLine();
	if (ImGui::Button(("true##" + ep->fullPath + std::to_string(entryID)).c_str(), { 90, 0 })) {
		backend->writeEndpointDirect(ep.basic, true);
		backend->updateEntryCache();
	}
}

void Entry::drawEndpointInput(Endpoint& ep) {
	if (ep->type == "float") {
		drawImGuiNumberInput(ep, true);	// Float
	}
	else if (ep->type == "bool") {
		drawImGuiBoolInput(ep);			// bool
	}
	else {
		drawImGuiNumberInput(ep, false);	// All other ints
	}
}

void Entry::drawNumericEndpointValue(EndpointValue& value, Endpoint& ep, bool input, bool output) {

	bool changed = (value != oldValues[ep->fullPath]);
	std::string text = value.toString();

	ImVec4 color = COLOR_UINT;
	if (ep->type == "float") {
		color = COLOR_FLOAT;
	}
	if (ep->type == "bool") {
		color = COLOR_BOOL;
	}
	if (changed) {
		color = RED;
	}

	// Test to see if an enum name is available for this endpoint
	const std::string& enumName = EndpointValueToEnumName(ep.basic, value.get<int64_t>());
	if (enumName.length() > 0) {
		text = enumName;
	}

	ImGui::TextColored(color, "%s", text.c_str());

	// And the tooltip
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::TextColored(color, "%s", ep->type.c_str());

		if (enumName.length() > 0) {
			ImGui::SameLine();
			ImGui::Text("->");
			ImGui::SameLine();
			std::stringstream name;
			name << enumName << " (0x" << std::hex << value.get<int64_t>() << ")";
			ImGui::TextColored(color, "%s", name.str().c_str(), value);
		}

		ImGui::EndTooltip();
	}
}

void Entry::draw() {

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

	if (endpoint->type != "function") {	// Numeric values

		if (ImGui::Button(("x##" + endpoint->fullPath).c_str(), { 40, 0 })) {
			toBeRemoved = true;
		}
		ImGui::SameLine();
		ImGui::Text("%s   =", endpoint->fullPath.c_str());
		ImGui::SameLine();

		drawNumericEndpointValue(value, endpoint, false, false);
		if (!endpoint->readonly) {
			drawEndpointInput(endpoint);
		}

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

	}
	else {					// Function

		if (ImGui::Button(("x##" + endpoint->fullPath).c_str(), { 40, 0 })) {
			toBeRemoved = true;
		}
		ImGui::SameLine();
		ImGui::Text("%s()", endpoint->fullPath.c_str());
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100);
		if (ImGui::Button(("Execute##" + endpoint->fullPath).c_str(), { 90, 0 })) {
			backend->executeFunction(endpoint->odriveID, endpoint->identifier);
		}

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

		if (endpoint.inputs.size() > 0) {
			ImGui::SetCursorPosX(40);
			ImGui::Text("Inputs:");
			ImGui::SameLine();
		}
		for (size_t j = 0; j < endpoint.inputs.size(); j++) {
			Endpoint& ep = endpoint.inputs[j];
			ImGui::SetCursorPosX(120);
			ImGui::Text("%s   =", ep->identifier.c_str());
			ImGui::SameLine();
			drawNumericEndpointValue(ioValues[ep->fullPath], ep, true, false);
			if (!ep->readonly) {
				drawEndpointInput(ep);
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
		}

		if (endpoint.outputs.size() > 0) {
			ImGui::SetCursorPosX(40);
			ImGui::Text("Outputs:");
			ImGui::SameLine();
		}
		for (size_t j = 0; j < endpoint.outputs.size(); j++) {
			Endpoint& ep = endpoint.outputs[j];
			ImGui::SetCursorPosX(120);
			ImGui::Text("%s   =", ep->identifier.c_str());
			ImGui::SameLine();
			drawNumericEndpointValue(ioValues[ep->fullPath], ep, false, true);
			if (!ep->readonly) {
				drawEndpointInput(ep);
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
		}
	}
	ImGui::Separator();
}
