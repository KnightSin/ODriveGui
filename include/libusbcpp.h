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






	// ========================================================
	// ===                Hotplug utilities                 ===
	// ========================================================

    class HotplugHandler {
    public:
        HotplugHandler() {
            if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
                throw std::runtime_error("Hotplug capabilities are not supported on this platform!");
            }
        }

        ~HotplugHandler() {
            stopListener();
        }

        void startListener() {

            if (running)
                return;

            int ret = libusb_hotplug_register_callback(context::get(),
                (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                LIBUSB_HOTPLUG_ENUMERATE,
                0x1209, // vid
                0x0D32, // pid
                LIBUSB_HOTPLUG_MATCH_ANY, // class
                (libusb_hotplug_callback_fn)OnUsbHotplugCallback,
                (void*)this,
                &handle);

            LOG_ERROR("Error code: {}", ret);

            if (ret != LIBUSB_SUCCESS) {
                throw std::runtime_error("Failed to register hotplug callback!");
            }

            running = true;
            listener = std::thread(std::bind(&HotplugHandler::listen, this));
        }

        void stopListener() {

            if (!running)
                return;

            running = false;
            libusb_hotplug_deregister_callback(context::get(), handle);

            listener.join();
        }

        static void LogEvent(int16_t vid, int16_t pid, libusb_hotplug_event event) {
            std::string event_string;

            switch (event)
            {
            case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
                event_string = "arrived";
                break;
            case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
                event_string = "left";
                break;
            default:
                event_string = "unknown";
                break;
            }
            std::cout << "Got a device: " << std::hex << vid << "," << pid << "," << event_string << std::dec << std::endl;
        }

    private:
        int OnUsbHotplug(struct libusb_context* ctx, struct libusb_device* device, libusb_hotplug_event event) {
            struct libusb_device_descriptor descriptor;

            int ret = libusb_get_device_descriptor(device, &descriptor);
            if (LIBUSB_SUCCESS == ret) {
                LogEvent(descriptor.idVendor, descriptor.idProduct, event);
            }

            return 0;
        }

        static int LIBUSB_CALL OnUsbHotplugCallback(libusb_context* ctx, libusb_device* device, libusb_hotplug_event event, void* instance) {
            return ((HotplugHandler*)instance)->OnUsbHotplug(ctx, device, event);
        }

        void listen() {
            while (running) {
                libusb_handle_events_completed(context::get(), nullptr);
            }
        }

        libusb_hotplug_callback_handle handle = 0;

        std::atomic<bool> running = false;
        std::thread listener;
    };






	// ====================================================
	// ===               General functions              ===
	// ====================================================

	void init();

	std::vector<DeviceInfo> scanDevices(libusb_device** rawDeviceList, size_t count);

}
