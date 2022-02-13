
#include "pch.h"
#include "Backend.h"

#include "Endpoint.h"

std::unique_ptr<Backend> backend;

Backend::Backend() : hotplugListener(context) {
}

Backend::~Backend() {

}

void Backend::scanDevices() {
	using namespace std::placeholders;
	hotplugListener.scanOnce(std::bind(&Backend::deviceConnected, this, _1));
}

void Backend::deviceConnected(std::shared_ptr<libusb::device> device) {
	if (device->getInfo().vendorID == ODRIVE_VENDOR_ID && device->getInfo().productID == ODRIVE_PRODUCT_ID) {
		connectDevice(device);
	}
}

void Backend::connectDevice(std::shared_ptr<libusb::device> device) {

	LOG_DEBUG("New device connected, probing...");
	std::shared_ptr<ODrive> odrive = std::make_shared<ODrive>(device, -1);
	device->claimInterface(ODRIVE_USB_INTERFACE);
	odrive->load(999);
	uint64_t serialNumber = odrive->getSerialNumber();

	// Check if a device with the same serial number is already known
	int index = -1;
	for (int i = 0; i < MAX_NUMBER_OF_ODRIVES; i++) {
		if (odrives[i]) {
			if (odrives[i]->serialNumber == serialNumber) {
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

	if (serialNumber == 0) {
		LOG_ERROR("Device can't be connected: Cannot read serial number!");
		return;
	}

	odrive->setODriveID(index);

	odrives[index] = odrive;	// Transfer ownership into the odrives array

	LOG_INFO("Device with serial number 0x{:08X} connected as odrv{}: {}", serialNumber, index, device->getInfo().description);
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

EndpointValue Backend::readEndpointDirect(const BasicEndpoint& ep) {

	if (ep.type == "bool")		{ auto temp = readEndpointDirectRaw<bool>(ep); if (temp) return EndpointValue(temp.value()); }
	if (ep.type == "float")		{ auto temp = readEndpointDirectRaw<float>(ep); if (temp) return EndpointValue(temp.value()); }
	if (ep.type == "uint8")		{ auto temp = readEndpointDirectRaw<uint8_t>(ep); if (temp) return EndpointValue(temp.value()); }
	if (ep.type == "uint16")	{ auto temp = readEndpointDirectRaw<uint16_t>(ep); if (temp) return EndpointValue(temp.value()); }
	if (ep.type == "uint32")	{ auto temp = readEndpointDirectRaw<uint32_t>(ep); if (temp) return EndpointValue(temp.value()); }
	if (ep.type == "uint64")	{ auto temp = readEndpointDirectRaw<uint64_t>(ep); if (temp) return EndpointValue(temp.value()); }
	if (ep.type == "int32")		{ auto temp = readEndpointDirectRaw<int32_t>(ep); if (temp) return EndpointValue(temp.value()); }

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
