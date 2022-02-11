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
	std::vector<Endpoint> endpoints;

	ODrive(std::shared_ptr<libusb::device> device, int odriveID) : device(device), odriveID(odriveID) {
		if (!device) {
			throw std::runtime_error("ODrive device is nullptr!");
		}
	}

	template<typename T>
	T read(uint16_t endpoint) {

		if (!loaded || !connected)
			throw std::runtime_error("ODrive not yet loaded or not connected anymore");

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
		LOG_WARN("Timeout: Failed to read endpoint {} from odrv{}", endpoint, odriveID);
		throw std::runtime_error("Timeout while reading from ODrive");
	}

	template<typename T>
	T read(const std::string& identifier) {
		auto endpoint = findEndpoint(identifier);
		if (endpoint)
			return read<T>(endpoint->get().id);
		return 0;
	}

	template<typename T>
	bool write(uint16_t endpoint, T value) {

		if (!loaded || !connected)
			return false;

		std::vector<uint8_t> payload(sizeof(T), 0);
		memcpy(&payload[0], &value, sizeof(T));

		sendWriteRequest(endpoint, sizeof(T), payload, jsonCRC);

		return true;
	}

	template<typename T>
	bool write(const std::string& identifier, T value) {
		auto endpoint = findEndpoint(identifier);
		if (endpoint) {
			return write<T>(endpoint->get().id, value);
		}
		return false;
	}

	void executeFunction(const std::string& identifier) {
		auto endpoint = findEndpoint(identifier);
		if (endpoint) {
			sendWriteRequest(endpoint->get().id, 1, { 0 }, jsonCRC);
		}
	}

	uint64_t getSerialNumber() {
		serialNumber = read<uint64_t>("serial_number");
		return serialNumber;
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

	Endpoint makeNode(njson node, const std::string& parentPath) {

		std::string type = node["type"];

		Endpoint ep;
		if (parentPath != "") {
			ep.identifier = parentPath + "." + (std::string)node["name"];
		}
		else {
			ep.identifier = node["name"];
		}
		ep.name = node["name"];

		if (type == "function") {
			ep.id = node["id"];
			ep.type = node["type"];

			for (auto& input : node["inputs"]) {
				ep.inputs.push_back(makeNode(input, ep.identifier));
			}
			for (auto& output : node["outputs"]) {
				ep.outputs.push_back(makeNode(output, ep.identifier));
			}
		}
		else if (type == "object") {
			for (auto& subnode : node["members"]) {
				ep.children.push_back(makeNode(subnode, ep.identifier));
			}
		}
		else {		// All other numeric types
			ep.id = node["id"];
			ep.readonly = (node["access"] == "r");
			ep.type = node["type"];
		}

		//LOG_DEBUG("{} -> \"{}\" id={} readonly={}", ep.path, ep.name, ep.id, ep.readonly);
		return ep;
	}

	std::vector<Endpoint> generateEndpoints(const std::string& json) {
		try {
			std::vector<Endpoint> endpoints;

			for (auto& subnode : njson::parse(json)) {
				if (subnode["type"] == "json") {
					// Ignore the json endpoint (endpoint 0)
					continue;
				}

				endpoints.push_back(makeNode(subnode, ""));
			}

			return endpoints;
		}
		catch (...) {
			LOG_ERROR("Error while parsing json definition!");
			disconnect();
		}

		return {};
	}

	void disconnect() {
		if (connected) {
			device->close();
			LOG_ERROR("Connection to odrv{} has been lost!", odriveID);
		}
		connected = false;
	}

	void load() {
		connected = true;
		std::string json = getJSON();
		endpoints = generateEndpoints(json);
		if (!connected) return;
		jsonCRC = CRC16_JSON((uint8_t*)&json[0], json.length());
		loaded = true;
		LOG_DEBUG("ODrive JSON CRC is 0x{:04X}", jsonCRC);
	}

	void setODriveID(int odriveID) {
		this->odriveID = odriveID;
	}

	operator bool() {
		return (bool)device && loaded;
	}

	std::optional<std::reference_wrapper<Endpoint>> findEndpoint(const std::string& identifier) {
		
		if (!loaded)
			return std::nullopt;

		for (Endpoint& ep : endpoints) {
			auto e = testEndpoint(ep, identifier);
			if (e) {
				return e;
			}
		}
		LOG_ERROR("Endpoint '{}' was not found in odrv{}", identifier, odriveID);
		return std::nullopt;
	}

private:
	std::optional<std::reference_wrapper<Endpoint>> testEndpoint(Endpoint& ep, const std::string& identifier) {
		if (ep.identifier == identifier) {
			return ep;
		}

		if (ep.children.size() > 0) {
			for (Endpoint& _ep : ep.children) {
				auto e = testEndpoint(_ep, identifier);
				if (e) {
					return e;
				}
			}
		}

		if (ep.inputs.size() > 0) {
			for (Endpoint& _ep : ep.inputs) {
				auto e = testEndpoint(_ep, identifier);
				if (e) {
					return e;
				}
			}
		}

		if (ep.outputs.size() > 0) {
			for (Endpoint& _ep : ep.outputs) {
				auto e = testEndpoint(_ep, identifier);
				if (e) {
					return e;
				}
			}
		}
		return std::nullopt;
	}

	int odriveID = -1;
	std::shared_ptr<libusb::device> device;
	inline static uint16_t sequenceNumber = 0;
};
