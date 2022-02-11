
#include "pch.h"
#include "Backend.h"

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
	odrive->load();
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

void Backend::addEndpoint(std::reference_wrapper<Endpoint> info, int odriveID) {
	endpoints.push_back(std::make_pair(odriveID, info));
}

void Backend::removeEndpoint(size_t index) {
	LOG_INFO("Removing endpoint entry #{}", index);
	endpoints.erase(endpoints.begin() + index);
}

void Backend::readEndpoint(const std::string& type, std::shared_ptr<ODrive>& odrive, const std::string& path, const std::string& identifier) {
	if (type == "float") {
		backendReadEndpoint<float>(odrive, path, identifier);
	}
	else if (type == "bool") {
		backendReadEndpoint<bool>(odrive, path, identifier);
	}
	else if (type == "uint8") {
		backendReadEndpoint<uint8_t>(odrive, path, identifier);
	}
	else if (type == "uint16") {
		backendReadEndpoint<uint16_t>(odrive, path, identifier);
	}
	else if (type == "uint32") {
		backendReadEndpoint<uint32_t>(odrive, path, identifier);
	}
	else if (type == "int32") {
		backendReadEndpoint<int32_t>(odrive, path, identifier);
	}
	else if (type == "uint64") {
		backendReadEndpoint<uint64_t>(odrive, path, identifier);
	}
}

void Backend::updateEndpoints() {

	oldValues = std::move(values);
	values.clear();

	try {
		for (size_t i = 0; i < endpoints.size(); i++) {
			std::string identifier = backend->endpoints[i].second.identifier;
			std::string path = "odrv" + std::to_string(backend->endpoints[i].first) + "." + identifier;
			std::string type = endpoints[i].second.type;

			auto& odrive = odrives[endpoints[i].first];

			// Read the endpoint and write into 'values[]'
			readEndpoint(type, odrive, path, identifier);

			if (type == "function") {
				Endpoint& endpoint = backend->endpoints[i].second;
				for (Endpoint& ep : endpoint.inputs) {
					std::string _path = "odrv" + std::to_string(backend->endpoints[i].first) + "." + ep.identifier;
					readEndpoint(ep.type, odrive, _path, ep.identifier);
				}
				for (Endpoint& ep : endpoint.outputs) {
					std::string _path = "odrv" + std::to_string(backend->endpoints[i].first) + "." + ep.identifier;
					readEndpoint(ep.type, odrive, _path, ep.identifier);
				}
			}
		}
	}
	catch (...) {
		values = oldValues;		// Restore
	}
}

void Backend::executeFunction(const std::string& path) {
	std::string identifier = path.substr(6);

	uint8_t index = path[4] - '0';		// Get ODrive index
	if (index >= MAX_NUMBER_OF_ODRIVES)
		return;

	LOG_DEBUG("Executing function {}", path);
	odrives[index]->executeFunction(identifier);
}
