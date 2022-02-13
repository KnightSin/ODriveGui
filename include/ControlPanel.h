#pragma once

#include "pch.h"
#include "Fonts.h"

#include "config.h"
#include "Backend.h"
#include "ODriveDocs.h"

#undef max
#undef min

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

	void OnRender() override {
		auto* fonts = GetFontContainer<FontContainer>();
		ImGui::PushFont(fonts->openSans21);

		std::string toRemove;
		for (Entry& e : backend->entries) {
			e.draw();
			if (e.toBeRemoved) {
				toRemove = e.endpoint->fullPath;
			}
		}

		if (toRemove.length() > 0) {
			backend->removeEntry(toRemove);
		}

		ImGui::PopFont();
	}
};
