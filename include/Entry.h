#pragma once

#include "pch.h"
#include "Endpoint.h"
#include "config.h"

class Entry {
public:
	Endpoint endpoint;
	EndpointValue value;
	std::map<std::string, EndpointValue> ioValues;
	std::map<std::string, EndpointValue> oldValues;
	bool toBeRemoved = false;
	
	size_t entryID;
	inline static size_t entryIDCounter = 0;
	char imguiBuffer[IMGUI_BUFFER_SIZE + 1];
	size_t selected = 0;

	Entry(const Endpoint& bep);

	void updateValue();

	bool drawImGuiNumberInputField(const std::string& imguiIdentifier, bool isfloat);
	void drawImGuiDropdownField(const std::string& imguiIdentifier, const std::vector<std::string>& enumNames);
	void drawImGuiNumberInput(Endpoint& ep, bool isfloat);
	void drawImGuiBoolInput(Endpoint& ep);
	void drawEndpointInput(Endpoint& ep);
	void drawNumericEndpointValue(EndpointValue& value, Endpoint& ep, bool input, bool output);
	void draw();
};
