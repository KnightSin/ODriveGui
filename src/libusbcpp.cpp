
#include "pch.h"
#include "libusbcpp.h"
#include <exception>

namespace libusb {

    class context {     // libusb context as a singleton, gets deleted automatically

        context() {
            if (libusb_init(&_context) < 0) {
                throw std::runtime_error("[libusb]: libusb could not be initialized!");
            }
        }

    public:
        ~context() {
            libusb_exit(_context);
        }

        static libusb_context* get() {
            return getInstance()._context;
        }

        libusb_context* operator->() {
            return _context;
        }

        context(context const&) = delete;
        void operator=(context const&) = delete;
        static context& getInstance() {
            static context instance;
            return instance;
        }

    private:
        libusb_context* _context = nullptr;
    };






    // ====================================================
    // ===                Device class                  ===
    // ====================================================

    Device::Device() {
        
    }

    Device::Device(const DeviceInfo& info) {
        this->info = info;
    }

    Device::~Device() {
        close();
    }

    void Device::open(int interface) {

        if (info.rawDevice == nullptr) {
            throw std::runtime_error("[libusb]: Can't open USB device: No device was chosen!");
        }

        libusb_device_handle* handle = nullptr;
        if (libusb_open(info.rawDevice, &handle) != LIBUSB_SUCCESS) {
            throw std::runtime_error("[libusb]: Failed to open USB device '" + info.description + "'");
        }

        this->handle = handle;
        this->interface = interface;

        // Now claim the interface
        if (libusb_kernel_driver_active(handle, 0) == 1) {
            if (libusb_detach_kernel_driver(handle, 0) == LIBUSB_SUCCESS) {
                throw std::runtime_error("[libusb]: Failed to open USB device '" + info.description + "': Can't detach Kernel driver!");
            }
        }

        if (libusb_claim_interface(handle, interface) < 0) {
            throw std::runtime_error("[libusb]: Failed to open USB device '" + info.description + "': Cannot claim interface!");
        }
    }

    void Device::close() {
        if (handle) {
            libusb_release_interface(handle, interface);
            libusb_close(handle);
            handle = nullptr;
        }
    }

    std::vector<uint8_t> Device::bulkRead(size_t expectedLength) {

        if (!handle) {
            throw std::runtime_error("[libusb]: Device is invalid!");
        }

        std::vector<uint8_t> buffer;
        buffer.reserve(expectedLength);
        for (size_t i = 0; i < expectedLength; i++) {
            buffer.push_back(0);
        }

        int transferred;
        if (libusb_bulk_transfer(handle, (0x83 | LIBUSB_ENDPOINT_IN), &buffer[0], (int)expectedLength, &transferred, 1000) != LIBUSB_SUCCESS) {
            return std::vector<uint8_t>();
        }

        buffer.resize(transferred);
        return buffer;
    }

    size_t Device::bulkWrite(std::vector<uint8_t> data) {
        return bulkWrite((uint8_t*)&data[0], data.size());
    }

    size_t Device::bulkWrite(const std::string& data) {
        return bulkWrite((uint8_t*)data.c_str(), data.length());
    }

    size_t Device::bulkWrite(uint8_t* data, size_t length) {

        if (!handle) {
            throw std::runtime_error("[libusb]: Device is invalid!");
        }
        
        int transferred;
        if (libusb_bulk_transfer(handle, (0x03 | LIBUSB_ENDPOINT_OUT), data, (int)length, &transferred, 1000) != LIBUSB_SUCCESS) {
            return 0;
        }

        return transferred;
    }




    // ========================================================
    // ===                DeviceList class                  ===
    // ========================================================

    DeviceList::DeviceList() {
        size_t cnt = libusb_get_device_list(context::get(), &rawDeviceList);
        if (cnt < 0) {
            throw std::runtime_error("[libusb]: Unable to retrieve device list!");
        }
        
        for (DeviceInfo& devInfo : scanDevices(rawDeviceList, cnt)) {
            devices.emplace_back(devInfo);
        }
    }

    DeviceList::~DeviceList() {
        devices.clear();
        libusb_free_device_list(rawDeviceList, 1);
    }

    std::vector<Device>& DeviceList::getDevices() {
        return devices;
    }




    // ====================================================
    // ===               General functions              ===
    // ====================================================

    void init() {
        context::getInstance();     // This creates the singleton, gets deleted automatically
    }

    std::vector<libusb::DeviceInfo> scanDevices(libusb_device** rawDeviceList, size_t count) {
        std::vector<libusb::DeviceInfo> devices;

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
            if (length < 0) {
                continue;
            }

            libusb_close(handle);

            libusb::DeviceInfo device;
            device.vendorID = desc.idVendor;
            device.productID = desc.idProduct;
            device.description = std::string((const char*)buffer, length);
            device.rawDevice = rawDevice;

            devices.push_back(std::move(device));
        }

        return devices;
    }

}



/*


    e = libusb_bulk_transfer(handle,BULK_EP_IN,my_string,length,&transferred,0);
    if(e == 0 && transferred == length)
    {
        printf("\nWrite successful!");
        printf("\nSent %d bytes with string: %s\n", transferred, my_string);
    }
    else
        printf("\nError in write! e = %d and transferred = %d\n",e,transferred);

    sleep(3);
    i = 0;

    for(i = 0; i < length; i++)
    {
        e = libusb_bulk_transfer(handle,BULK_EP_OUT,my_string1,64,&received,0);  //64 : Max Packet Lenght
        if(e == 0)
        {
            printf("\nReceived: ");
            printf("%c",my_string1[i]);    //will read a string from lcp2148
            sleep(1);
        }
        else
        {
            printf("\nError in read! e = %d and received = %d\n",e,received);
            return -1;
        }
    }


    e = libusb_release_interface(handle, 0);

*/