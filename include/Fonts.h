#pragma once

#include "pch.h"
#include "OpenSansFontData.h"	// Here an OpenSans font is loaded, this header was auto-generated 
								// from a .ttf file using the Battery::ImGuiUtils::... functions

struct FontContainer : public Battery::FontContainer {

	// Here you can load any number of fonts to be used throughout the application
	ImFont* openSans25 = Battery::ImGuiUtils::AddEmbeddedFont(OpenSansFontData_compressed_data, OpenSansFontData_compressed_size, 25);
	ImFont* openSans18 = Battery::ImGuiUtils::AddEmbeddedFont(OpenSansFontData_compressed_data, OpenSansFontData_compressed_size, 18);

};
