
#include "pch.h"
#include "libusbcpp.h"
#include <exception>
#include <chrono>

namespace libusb {






    // ====================================================
    // ===                Device class                  ===
    // ====================================================

    device::device(libusb_device_handle* handle) {
        this->handle = handle;
        if (!handle) {
            LOG_ERROR("[libusb]: Cannot construct a device with a null handle!");
            throw std::runtime_error("[libusb]: Cannot construct a device with a null handle!");
        }
    }

    device::~device() {
        close();
    }

    void device::claimInterface(int interface) {

        if (!open)
            return;

        if (libusb_kernel_driver_active(handle, interface) == 1) {
            if (libusb_detach_kernel_driver(handle, interface) == LIBUSB_SUCCESS) {
                throw std::runtime_error("[libusb]: Failed to open USB device: Can't detach Kernel driver!");
            }
        }

        if (libusb_claim_interface(handle, interface) < 0) {
            throw std::runtime_error("[libusb]: Failed to open USB device: Cannot claim interface!");
        }

        interfaces.push_back(interface);
    }

    void device::close() {
        if (open) {
            for (int interface : interfaces) {
                libusb_release_interface(handle, interface);
            }
            libusb_close(handle); 
            open = false;
        }
    }

    deviceInfo device::getInfo() {

        if (!open)
            return deviceInfo();

        libusb_device* dev = libusb_get_device(handle);
        if (dev == nullptr) {
            return deviceInfo();
        }

        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) != LIBUSB_SUCCESS) {
            return deviceInfo();
        }

        unsigned char buffer[1024];
        size_t length = libusb_get_string_descriptor_ascii(handle, desc.iProduct, buffer, sizeof(buffer));

        libusb::deviceInfo device;
        device.vendorID = desc.idVendor;
        device.productID = desc.idProduct;
        device.description = std::string((const char*)buffer, length);
        return device;
    }

    std::vector<uint8_t> device::bulkRead(size_t expectedLength) {

        if (!open) {
            return {};
        }

        std::vector<uint8_t> buffer;
        buffer.reserve(expectedLength);
        for (size_t i = 0; i < expectedLength; i++) {
            buffer.push_back(0);
        }

        int transferred;
        try {
            if (libusb_bulk_transfer(handle, (0x83 | LIBUSB_ENDPOINT_IN), &buffer[0], (int)expectedLength, &transferred, 1000) != LIBUSB_SUCCESS) {
                return std::vector<uint8_t>();
            }
        }
        catch (...) {
            return {};
        }

        buffer.resize(transferred);
        return buffer;
    }

    size_t device::bulkWrite(std::vector<uint8_t> data) {
        return bulkWrite((uint8_t*)&data[0], data.size());
    }

    size_t device::bulkWrite(const std::string& data) {
        return bulkWrite((uint8_t*)data.c_str(), data.length());
    }

    size_t device::bulkWrite(uint8_t* data, size_t length) {

        if (!open) {
            return -1;
        }

        int transferred = 0;
        try {
            int errorCode = libusb_bulk_transfer(handle, (0x03 | LIBUSB_ENDPOINT_OUT), data, (int)length, &transferred, 1000);
            if (errorCode != LIBUSB_SUCCESS) {
                return -1;
            }
        }
        catch (...) {
            return -1;
        }

        return transferred;
    }





    // ========================================================
    // ===              HotplugListener class               ===
    // ========================================================

    HotplugListener::~HotplugListener() {
        stop();
    }

    void HotplugListener::start(std::function<void(std::shared_ptr<device> device)> onConnect, float interval) {
        using namespace std::placeholders;

        running = true;
        listener = std::thread(std::bind(&HotplugListener::listen, this, _1, _2), onConnect, interval);
    }

    void HotplugListener::scanOnce(std::function<void(std::shared_ptr<device> device)> onConnect) {

        try {
            LOG_TRACE("[libusbcpp]: HotplugListener: Scanning for devices...");
            auto newDevices = scanDevices(context);
            for (auto device : newDevices) {
                if (!isDeviceKnown(device)) {
                    onConnect(openDevice(context, device.vendorID, device.productID));
                }
            }
            LOG_TRACE("[libusbcpp]: HotplugListener: Scan finished");

            knownDevices.clear();
            knownDevices = std::move(newDevices);
        }
        catch (...) {}
    }

    void HotplugListener::stop() {
        if (running) {
            running = false;
            listener.join();
        }
    }

    bool HotplugListener::isDeviceKnown(deviceInfo& info) {
        for (auto device : knownDevices) {
            if (device.vendorID == info.vendorID && device.productID == info.productID) {
                return true;
            }
        }

        return false;
    }

    void HotplugListener::listen(std::function<void(std::shared_ptr<device> device)> onConnect, float interval) {

        LOG_TRACE("[libusbcpp]: Hotpluglistener: Thread started");
        while (running) {

            scanOnce(onConnect);

            Battery::Sleep(interval);

        }
        LOG_TRACE("[libusbcpp]: Hotpluglistener: Thread stopped");
    }






    // ====================================================
    // ===               General functions              ===
    // ====================================================

    std::vector<libusb::deviceInfo> scanDevices(const context& ctx) {
        std::vector<libusb::deviceInfo> devices;
        LOG_TRACE("[libusbcpp]: Scanning for devices...");

        libusb_device** rawDeviceList;
        size_t count = libusb_get_device_list(ctx, &rawDeviceList);

        if (count < 0)
            return devices;

        for (size_t i = 0; i < count; i++) {
            libusb_device* rawDevice = rawDeviceList[i];

            // Get device information
            struct libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(rawDevice, &desc) != LIBUSB_SUCCESS) {
                continue;   // Jump back to top
            }

            // Open device for reading
            libusb_device_handle* handle = NULL;
            if (libusb_open(rawDevice, &handle) != LIBUSB_SUCCESS) {
                continue;   // Jump back to top
            }

            // Parse the description text
            unsigned char buffer[1024];
            size_t length = libusb_get_string_descriptor_ascii(handle, desc.iProduct, buffer, sizeof(buffer));

            libusb_close(handle);

            if (length < 0) {
                continue;
            }

            libusb::deviceInfo device;
            device.vendorID = desc.idVendor;
            device.productID = desc.idProduct;
            device.description = std::string((const char*)buffer, length);

            devices.push_back(std::move(device));
        }

        LOG_TRACE("[libusbcpp]: Scanning done");
        libusb_free_device_list(rawDeviceList, 1);
        return devices;
    }

    std::shared_ptr<device> openDevice(const context& ctx, uint16_t vendorID, uint16_t productID) {
        LOG_TRACE("[libusbcpp]: Opening device vid={:04X} pid={:04X}", vendorID, productID);

        libusb_device** rawDeviceList;
        size_t count = libusb_get_device_list(ctx, &rawDeviceList);

        if (count < 0)
            return nullptr;

        libusb_device* found = nullptr;
        for (size_t i = 0; i < count; i++) {
            libusb_device* rawDevice = rawDeviceList[i];

            // Get device information
            struct libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(rawDevice, &desc) != LIBUSB_SUCCESS) {
                continue;   // Jump back to top
            }

            // Check if it's the device we are looking for
            if (desc.idVendor == vendorID && desc.idProduct == productID) {
                found = rawDevice;
            }
        }

        libusb_device_handle* handle = nullptr;
        if (found) {
            if (libusb_open(found, &handle) < 0) {
                LOG_TRACE("[libusbcpp]: Device could not be opened!");
                handle = nullptr;
            }
        }

        libusb_free_device_list(rawDeviceList, 1);

        if (handle == nullptr) {
            LOG_TRACE("[libusbcpp]: Device handle was invalid!");
            return nullptr;
        }
        LOG_TRACE("[libusbcpp]: Device opened");

        return std::make_shared<device>(handle);
    }

}