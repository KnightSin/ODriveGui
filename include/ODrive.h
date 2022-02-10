#pragma once

#include "libusbcpp.h"
#include "CRC.h"
#include "Endpoint.h"

#include "json.hpp"

#define ODRIVE_VENDOR_ID 0x1209
#define ODRIVE_PRODUCT_ID 0x0D32

typedef std::vector<uint8_t> buffer_t;
using njson = nlohmann::json;

class ODrive {
public:

	bool connected = false;

	ODrive(libusb::Device& device) : device(device) {

	}

	void sendReadRequest(uint16_t endpointID, uint16_t expectedResponseSize, const buffer_t& payload, uint16_t jsonCRC) {
		sendRequest((1 << 15) | endpointID, expectedResponseSize, payload, jsonCRC);
	}

	void sendWriteRequest(uint16_t endpointID, uint16_t expectedResponseSize, const buffer_t& payload, uint16_t jsonCRC) {
		sendRequest(endpointID, expectedResponseSize, payload, jsonCRC);
	}

	void sendRequest(uint16_t endpointID, uint16_t expectedResponseSize, const buffer_t& payload, uint16_t jsonCRC) {
		sendRequest(sequenceNumber, endpointID, expectedResponseSize, payload, jsonCRC);
		sequenceNumber = (sequenceNumber + 1) % 4096;
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
		device.bulkWrite(data, length);
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

	std::vector<uint8_t> read(size_t expectedLength) {
		return device.bulkRead(expectedLength);
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

	static Endpoint makeNode(njson node, const std::string& parentPath) {

		std::string type = node["type"];

		Endpoint ep;
		ep.path = parentPath + "." + (std::string)node["name"];
		ep.name = node["name"];

		if (type == "function") {
			ep.id = node["id"];
			ep.isFunction = true;

			for (auto& input : node["inputs"]) {
				ep.inputs.push_back(makeNode(input, ep.path));
			}
			for (auto& output : node["outputs"]) {
				ep.outputs.push_back(makeNode(output, ep.path));
			}
		}
		else if (type == "object") {
			LOG_DEBUG("object");
			for (auto& subnode : node["members"]) {
				ep.children.push_back(makeNode(subnode, ep.path));
			}
		}
		else {		// All other numeric types
			ep.id = node["id"];
			ep.readonly = node["access"] == "r";
			ep.isNumeric = true;
		}

		//LOG_DEBUG("{} -> \"{}\" id={} readonly={}", ep.path, ep.name, ep.id, ep.readonly);
		return ep;
	}

	static std::vector<Endpoint> generateEndpoints(const std::string& json, int odriveID) {
		try {
			std::vector<Endpoint> endpoints;

			for (auto& subnode : njson::parse(json)) {
				if (subnode["type"] == "json") {
					// Ignore the json endpoint (endpoint 0)
					continue;
				}

				endpoints.push_back(makeNode(subnode, "odrv" + std::to_string(odriveID)));
			}

			return endpoints;
		}
		catch (...) {
			LOG_ERROR("Error while parsing json definition!");
		}

		return {};
	}

private:
	libusb::Device& device;
	inline static uint16_t sequenceNumber = 0;
};
