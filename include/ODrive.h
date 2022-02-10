#pragma once

#include "libusbcpp.h"
#include "CRC.h"

typedef std::vector<uint8_t> buffer_t;

class ODrive {
public:
	ODrive(libusb::Device& device) : device(device) {

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

		buffer.push_back((uint8_t)(endpointID >> 8));
		buffer.push_back((uint8_t)(endpointID));

		buffer.push_back((uint8_t)(expectedResponseSize >> 8));
		buffer.push_back((uint8_t)(expectedResponseSize));

		for (uint8_t b : payload) {
			buffer.push_back(b);
		}

		buffer.push_back((uint8_t)(jsonCRC >> 8));
		buffer.push_back((uint8_t)(jsonCRC));

		write(&buffer[0], buffer.size());
	}

	void write(uint8_t* data, size_t length) {
		device.bulkWrite(data, length);
	}

	std::vector<uint8_t> read(size_t expectedLength) {
		return device.bulkRead(expectedLength);
	}

private:
	libusb::Device& device;
	inline static uint16_t sequenceNumber = 0;
};
