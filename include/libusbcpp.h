#pragma once

#include <string>
#include <vector>

#pragma warning( disable : 4200 )
#include <libusb.h>

// TODO: 
// libusb_open_device_with_vid_pid(ctx, VID, PID);
// libusb_claim_interface(handle, 1);

namespace libusb {

	struct no_deleter {
		template <typename T> 
		void operator() (T const&) const noexcept { }
	};

	struct DeviceInfo {
		uint16_t vendorID = 0x00;
		uint16_t productID = 0x00;
		std::string description;
		libusb_device* rawDevice = nullptr;
	};

	class Device {
	public:

		DeviceInfo info;

		Device();
		Device(const DeviceInfo& info);
		~Device();

		void open(int interface);
		void close();

		std::vector<uint8_t> bulkRead(size_t expectedLength);
		size_t bulkWrite(std::vector<uint8_t> data);
		size_t bulkWrite(const std::string& data);
		size_t bulkWrite(uint8_t* data, size_t length);

	private:
		libusb_device_handle* handle = nullptr;
		int interface = 0;
	};

	class DeviceList {
	public:
		DeviceList();
		~DeviceList();

		std::vector<Device>& getDevices();

	private:
		std::vector<Device> devices;
		libusb_device** rawDeviceList = nullptr;
	};

	void init();

	std::vector<DeviceInfo> scanDevices(libusb_device** rawDeviceList, size_t count);

}
