#pragma once

#include "libusbcpp.h"
#include "CRC.h"
#include "Endpoint.h"

#include "json.hpp"

#define ODRIVE_VENDOR_ID 0x1209
#define ODRIVE_PRODUCT_ID 0x0D32
#define ODRIVE_USB_INTERFACE 2
#define ODRIVE_TIMEOUT 0.5		// Read/Write timeout in seconds

typedef std::vector<uint8_t> buffer_t;
using njson = nlohmann::json;

class ODrive {
public:

	bool connected = true;
	bool loaded = false;
	uint16_t jsonCRC = 0x00;
	uint64_t serialNumber = 0;
	std::string json;
	std::vector<Endpoint> endpoints;
	std::vector<BasicEndpoint> cachedEndpoints;

	bool error = false;
	int32_t axisError = 0x00;
	int32_t motorError = 0x00;
	int32_t encoderError = 0x00;
	int32_t controllerError = 0x00;

	ODrive(std::shared_ptr<libusb::device> device, int odriveID) : device(device) {
		if (!device) {
			throw std::runtime_error("ODrive device is nullptr!");
		}
	}

	template<typename T>
	T read(uint16_t endpoint) {

		if (!loaded || !connected)
			throw std::runtime_error("ODrive not yet loaded or not connected anymore");

		std::lock_guard<std::mutex> lock(transferMutex);
		uint16_t sequence = sendReadRequest(endpoint, sizeof(T), {}, jsonCRC);

		double start = Battery::GetRuntime();
		while (Battery::GetRuntime() < start + ODRIVE_TIMEOUT) {
			auto& response = getResponse(sizeof(T));
			if ((response.first & 0b0111111111111111) == sequence && response.second.size() == sizeof(T)) {
				T value = 0;
				memcpy(&value, &response.second[0], sizeof(value));
				return value;
			}
		}
		LOG_WARN("Timeout: Failed to read endpoint {}", endpoint);
		throw std::runtime_error("Timeout while reading from ODrive");
	}

	template<typename T>
	T read(const std::string& identifier) {
		auto endpoint = findEndpoint(identifier);
		if (endpoint)
			return read<T>(endpoint->id);
		return 0;
	}

	template<typename T>
	bool write(uint16_t endpoint, T value) {

		if (!loaded || !connected)
			return false;

		std::vector<uint8_t> payload(sizeof(T), 0);
		memcpy(&payload[0], &value, sizeof(T));

		std::lock_guard<std::mutex> lock(transferMutex);
		sendWriteRequest(endpoint, sizeof(T), payload, jsonCRC);

		return true;
	}

	template<typename T>
	bool write(const std::string& identifier, T value) {
		auto endpoint = findEndpoint(identifier);
		if (endpoint) {
			return write<T>(endpoint->id, value);
		}
		return false;
	}

	void executeFunction(const std::string& identifier) {
		auto endpoint = findEndpoint(identifier);
		if (endpoint) {
			std::lock_guard<std::mutex> lock(transferMutex);
			sendWriteRequest(endpoint->id, 1, { 0 }, jsonCRC);
		}
	}

	void updateErrors() {
		axisError = read<int32_t>("axis0.error");
		motorError = read<int32_t>("axis0.motor.error");
		encoderError = read<int32_t>("axis0.encoder.error");
		controllerError = read<int32_t>("axis0.controller.error");

		error = axisError || motorError || encoderError || controllerError;
	}

	uint64_t getSerialNumber() {
		serialNumber = read<uint64_t>("serial_number");
		return serialNumber;
	}

	void disconnect() {
		if (connected) {
			device->close();
		}
		connected = false;
	}

	void load(int odriveID) {

		std::lock_guard<std::mutex> lock(transferMutex);
		connected = true;
		json = getJSON();
		jsonCRC = CRC16_JSON((uint8_t*)&json[0], json.length());
		setODriveID(odriveID);

		if (!connected)
			return;

		loaded = true;
		LOG_DEBUG("ODrive JSON CRC is 0x{:04X}", jsonCRC);
	}

	void setODriveID(int odriveID) {
		// Reload the cache
		generateEndpoints(odriveID);
	}

	operator bool() {
		return (bool)device && loaded;
	}

private:
	void generateEndpoints(int odriveID) {

		endpoints.clear();
		cachedEndpoints.clear();

		try {

			for (auto& subnode : njson::parse(json)) {
				if (subnode["type"] == "json") {
					// Ignore the json endpoint (endpoint 0)
					continue;
				}

				endpoints.push_back(makeNode(subnode, "", odriveID));
			}
		}
		catch (...) {
			LOG_ERROR("Error while parsing json definition!");
			disconnect();
			endpoints.clear();
			cachedEndpoints.clear();
		}
	}

	Endpoint makeNode(njson node, const std::string& parentPath, int odriveID) {

		Endpoint ep;
		ep->name = node["name"];
		ep->identifier = ((parentPath.size() > 0) ? (parentPath + ".") : ("")) + ep->name;
		ep->fullPath = "odrv" + std::to_string(odriveID) + "." + ep->identifier;
		ep->type = node["type"];
		ep->odriveID = odriveID;

		if (ep->type == "object") {			// Object with children
			for (auto& subnode : node["members"]) {
				ep.children.push_back(makeNode(subnode, ep->identifier, odriveID));
			}
		}
		else if (ep->type == "function") {	// Function
			ep->id = node["id"];

			for (auto& input : node["inputs"]) {
				ep.inputs.push_back(makeNode(input, ep->identifier, odriveID));
			}
			for (auto& output : node["outputs"]) {
				ep.outputs.push_back(makeNode(output, ep->identifier, odriveID));
			}
			cachedEndpoints.push_back(ep.basic);
		}
		else {						// All other numeric types
			ep->id = node["id"];
			ep->readonly = (node["access"] == "r");
			cachedEndpoints.push_back(ep.basic);
		}

		return ep;
	}

	BasicEndpoint* findEndpoint(const std::string& identifier) {

		if (!loaded)
			return nullptr;

		for (auto& ep : cachedEndpoints) {
			if (ep.identifier == identifier) {
				return &ep;
			}
		}

		LOG_ERROR("Endpoint '{}' was not found in the cache", identifier);
		return nullptr;
	}

	uint16_t sendReadRequest(uint16_t endpointID, uint16_t expectedResponseSize, const buffer_t& payload, uint16_t jsonCRC) {
		return sendRequest((1 << 15) | endpointID, expectedResponseSize, payload, jsonCRC);
	}

	uint16_t sendWriteRequest(uint16_t endpointID, uint16_t expectedResponseSize, const buffer_t& payload, uint16_t jsonCRC) {
		return sendRequest(endpointID, expectedResponseSize, payload, jsonCRC);
	}

	uint16_t sendRequest(uint16_t endpointID, uint16_t expectedResponseSize, const buffer_t& payload, uint16_t jsonCRC) {
		sequenceNumber = (sequenceNumber + 1) % 4096;
		sendRequest(sequenceNumber, endpointID, expectedResponseSize, payload, jsonCRC);
		return sequenceNumber;
	}

	void sendRequest(uint16_t sequenceNumber, uint16_t endpointID, uint16_t expectedResponseSize, const buffer_t& payload, uint16_t jsonCRC) {
		buffer_t buffer;
		buffer.reserve(payload.size() + 8);

		buffer.push_back((uint8_t)(sequenceNumber));
		buffer.push_back((uint8_t)(sequenceNumber >> 8));

		buffer.push_back((uint8_t)(endpointID));
		buffer.push_back((uint8_t)(endpointID >> 8));

		buffer.push_back((uint8_t)(expectedResponseSize));
		buffer.push_back((uint8_t)(expectedResponseSize >> 8));

		for (uint8_t b : payload) {
			buffer.push_back(b);
		}

		buffer.push_back((uint8_t)(jsonCRC));
		buffer.push_back((uint8_t)(jsonCRC >> 8));

		write(&buffer[0], buffer.size());
	}

	void write(uint8_t* data, size_t length) {
		for (int i = 0; i < 5; i++) {
			//LOG_DEBUG("Call to libusb::bulkWrite");
			if (device->bulkWrite(data, length) != -1) {
				return;
			}
		}
		disconnect();
	}

	std::vector<uint8_t> read(size_t expectedLength) {
		auto data = device->bulkRead(expectedLength + 2);
		if (data.size() == 0) {
			disconnect();
		}
		return std::move(data);
	}

	std::pair<uint16_t, buffer_t> getResponse(size_t expectedLength) {
		buffer_t response = read(expectedLength);

		if (response.size() < 2) {
			return std::make_pair(0, buffer_t());
		}

		uint16_t sequence = response[0] | response[1] << 8;

		buffer_t buffer;
		for (size_t i = 2; i < response.size(); i++) {
			buffer.push_back(response[i]);
		}
		return std::make_pair(sequence, buffer);
	}

	std::string getJSON() {
		std::string json;

		uint32_t offset = 0;
		while (true) {
			buffer_t buffer(sizeof(offset), 0);
			memcpy(&buffer[0], &offset, sizeof(offset));

			sendReadRequest(0, 32, buffer, 1);
			auto response = getResponse(32);
			offset += (uint32_t)response.second.size();

			if (response.second.size() == 0) {
				break;
			}

			json += std::string((char*)&response.second[0], response.second.size());
		}

		return json;
	}

	std::shared_ptr<libusb::device> device;
	inline static uint16_t sequenceNumber = 0;
	std::mutex transferMutex;
};
