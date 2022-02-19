
#include "pch.h"
#include "Backend.h"

#include "Endpoint.h"

std::unique_ptr<Backend> backend;

Backend::Backend() {
	importEntries(Battery::GetExecutableDirectory() + "endpoints.json");
	usbListener = std::thread(std::bind(&Backend::listenerThread, this));
}

Backend::~Backend() {
	exportEntries(Battery::GetExecutableDirectory() + "endpoints.json");
	stopListener = true;
	usbListener.join();
}

void Backend::listenerThread() {
	while (!stopListener) {
		auto& devices = libusbcpp::findDevice(context, ODRIVE_VENDOR_ID, ODRIVE_PRODUCT_ID);
		for (auto& device : devices) {
			try {

				LOG_DEBUG("New device connected, probing...");
				std::shared_ptr<ODrive> odrive = std::make_shared<ODrive>(device);
				odrive->getSerialNumber();

				while (deviceWaiting) {
					Battery::Sleep(USB_SCAN_INTERVAL);
				}
				std::lock_guard<std::mutex> lock(temporaryDeviceMutex);
				temporaryDevice = odrive;
				deviceWaiting = true;
			}
			catch (const std::exception& e) {
				LOG_ERROR("Failed to connect device: {}", e.what());
			}
		}
		Battery::Sleep(USB_SCAN_INTERVAL);
	}
}

void Backend::handleNewDevices() {
	if (deviceWaiting) {
		std::lock_guard<std::mutex> lock(temporaryDeviceMutex);
		connectDevice(temporaryDevice);
		temporaryDevice.reset();
		deviceWaiting = false;
	}
}

void Backend::connectDevice(std::shared_ptr<ODrive> odrv) {

	// Check if a device with the same serial number is already known
	int index = -1;
	for (int i = 0; i < MAX_NUMBER_OF_ODRIVES; i++) {
		if (odrives[i]) {
			if (odrives[i]->serialNumber == odrv->serialNumber) {
				index = i;
				break;
			}
		}
	}

	if (index == -1) {	// Device is not known yet, choose an empty slot
		for (int i = 0; i < MAX_NUMBER_OF_ODRIVES; i++) {
			if (!odrives[i]) {
				index = i;
				break;
			}
		}
	}

	if (index == -1) {
		LOG_ERROR("Device can't be connected: No slot is available!");
		return;
	}

	if (odrv->serialNumber == 0) {
		LOG_ERROR("Device can't be connected: Cannot read serial number!");
		return;
	}

	odrv->setODriveID(index);

	odrives[index] = odrv;	// Transfer ownership into the odrives array

	LOG_INFO("Device with serial number 0x{:08X} connected as odrv{}", odrv->serialNumber, index);
}

void Backend::addEntry(const Entry& entry) {
	LOG_INFO("Adding endpoint entry {}", entry.endpoint.basic.fullPath);
	entries.push_back(entry);
}

void Backend::removeEntry(const std::string& fullPath) {
	LOG_INFO("Removing endpoint entry {}", fullPath);
	for (size_t i = 0; i < entries.size(); i++) {
		if (entries[i].endpoint->fullPath == fullPath) {
			entries.erase(entries.begin() + i);
			return;
		}
	}
}

void Backend::updateEntryCache() {

	for (Entry& e : entries) {
		e.updateValue();
	}
}

void Backend::importEntries(std::string path) {

	if (path.length() == 0) {
		path = Battery::PromptFileOpenDialog({ "*.json" }, Battery::GetMainWindow());
		if (path.length() == 0)
			return;
	}

	// Read the file
	LOG_DEBUG("Loading entries from {}", path);
	auto file = Battery::ReadFile(path);
	if (file.fail()) {
		LOG_ERROR("Failed to load entries from {}: Cannot open file!", path);
		return;
	}

	// Now import it
	entries.clear();
	try {
		njson json = njson::parse(file.content());
		for (njson entry : json) {
			Entry e(entry);
			if (e.endpoint->id != -1) {
				addEntry(e);
			}
			else {
				LOG_WARN("Failed to import an entry: JSON definition was invalid!");
			}
		}
	}
	catch (...) {
		LOG_ERROR("Error while importing: Not a valid JSON file!");
		return;
	}
	LOG_DEBUG("Done");
}

void Backend::exportEntries(const std::string& file) {
	nlohmann::json json = nlohmann::json::array();
	for (Entry& e : entries) {
		json.push_back(e.toJson());
	}
	std::string content = json.dump(4);

	if (file.length() > 0) {
		Battery::WriteFile(file, content);
	}
	else {
		Battery::SaveFileWithDialog("json", content, Battery::GetMainWindow());
	}
}

void Backend::loadDefaultEntries() {

	std::string file = DEFAULT_ENTRIES_JSON;

	LOG_DEBUG("Loading default entries...");
	entries.clear();
	try {
		njson json = njson::parse(file);
		for (njson entry : json) {
			Entry e(entry);
			if (e.endpoint->id != -1) {
				addEntry(e);
			}
			else {
				LOG_WARN("Failed to import an entry: JSON definition was invalid!");
			}
		}
	}
	catch (...) {
		LOG_ERROR("Error while importing: Not a valid JSON file!");
		return;
	}
	LOG_DEBUG("Done");
}

void Backend::executeFunction(int odriveID, const std::string& identifier) {

	if (!odrives[odriveID])
		return;

	odrives[odriveID]->executeFunction(identifier);
}

void Backend::odriveDisconnected(int odriveID) {
	LOG_ERROR("Lost connection to odrv{}", odriveID);
}

void Backend::updateEndpointCache(int odriveID) {

	if (!odrives[odriveID])
		return;

	// Loop through every endpoint of the odrive
	cachedEndpointValues.clear();
	for (BasicEndpoint& ep : odrives[odriveID]->cachedEndpoints) {
		if (ep.type != "function") {	// It's a numeric type, objects are not in the cached list	
			EndpointValue& value = readEndpointDirect(ep);
			if (value.type() != EndpointValueType::INVALID) {
				cachedEndpointValues.emplace(ep.fullPath, value);
			}
		}
	}
}

EndpointValue Backend::getCachedEndpointValue(const std::string& fullPath) {
	
	auto it = cachedEndpointValues.find(fullPath);
	if (it != cachedEndpointValues.end()) {
		return it->second;
	}

	return EndpointValue(EndpointValueType::INVALID);
}

#define READ_ENDPOINT(_type, T)	if (ep.type == _type)	{ T temp = 0; if (readEndpointDirectRaw<T>(ep, &temp)) return EndpointValue(temp); }

EndpointValue Backend::readEndpointDirect(const BasicEndpoint& ep) {

	READ_ENDPOINT("bool", bool);
	READ_ENDPOINT("float", float);
	READ_ENDPOINT("uint8", uint8_t);
	READ_ENDPOINT("uint16", uint16_t);
	READ_ENDPOINT("uint32", uint32_t);
	READ_ENDPOINT("uint64", uint64_t);
	READ_ENDPOINT("int32", int32_t);

	return EndpointValue(EndpointValueType::INVALID);
}

void Backend::writeEndpointDirect(const BasicEndpoint& ep, const EndpointValue& value) {
	switch (value.type()) {
	case EndpointValueType::BOOL:	writeEndpointDirectRaw(ep, value.get<bool>()); break;
	case EndpointValueType::FLOAT:	writeEndpointDirectRaw(ep, value.get<float>()); break;
	case EndpointValueType::UINT8:	writeEndpointDirectRaw(ep, value.get<uint8_t>()); break;
	case EndpointValueType::UINT16:	writeEndpointDirectRaw(ep, value.get<uint16_t>()); break;
	case EndpointValueType::UINT32:	writeEndpointDirectRaw(ep, value.get<uint32_t>()); break;
	case EndpointValueType::UINT64:	writeEndpointDirectRaw(ep, value.get<uint64_t>()); break;
	case EndpointValueType::INT32:	writeEndpointDirectRaw(ep, value.get<int32_t>()); break;
	}
}

const char* DEFAULT_ENTRIES_JSON = " \
[] \
";
