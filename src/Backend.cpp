
#include "pch.h"
#include "Backend.h"

int hotplug_callback(struct libusb_context* ctx, struct libusb_device* dev, libusb_hotplug_event event, void* user_data) {

	struct libusb_device_descriptor desc;
	libusb_get_device_descriptor(dev, &desc);

	if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
		//libusb_device_handle* dev_handle = NULL;
		//if (libusb_open(dev, &dev_handle) != LIBUSB_SUCCESS) {
		//	LOG_ERROR("Failed to open USB device!");
		//}
		LOG_WARN("Connected");
	}
	else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
		//if (dev_handle) {
		//	libusb_close(libusb_get_device);
		//	dev_handle = NULL;
		//}
		LOG_WARN("Disconnected");
	}

	return 0;
}

void Backend::init() {
	//libusb::registerHotplugDevice(hotplug_callback, &hotplugCallbackHandle, ODRIVE_VENDOR_ID, ODRIVE_PRODUCT_ID);
	hotplutHandler.startListener();
}

void Backend::cleanup() {
	//odrives.clear();
	//usbDevices.clear();
	//rawDeviceList.reset();
	hotplutHandler.stopListener();
}

void connectDevice(libusb::Device& device) {

}
