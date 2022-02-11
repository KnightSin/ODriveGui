#pragma once

#include <string>
#include <vector>

#pragma warning( disable : 4200 )
#include <libusb.h>

// TODO: libusb bug: windows_winusb.c -> line 1146

// TODO: 
// libusb_open_device_with_vid_pid(ctx, VID, PID);
// libusb_claim_interface(handle, 1);

namespace libusb {

    class context {
    public:
        context() {
            if (libusb_init(&_context) < 0) {
                throw std::runtime_error("[libusb]: libusb could not be initialized!");
            }
        }

        ~context() {
            libusb_exit(_context);
        }

        operator libusb_context*() const {
            return _context;
        }

        context(context const&) = delete;
        void operator=(context const&) = delete;

    private:
        libusb_context* _context = nullptr;
    };

	struct deviceInfo {
		uint16_t vendorID = 0x00;
		uint16_t productID = 0x00;
		std::string description;
	};

	class device {
	public:
		device(libusb_device_handle* handle);
		~device();

		void claimInterface(int interface);
		void close();

        deviceInfo getInfo();

		std::vector<uint8_t> bulkRead(size_t expectedLength);
		size_t bulkWrite(std::vector<uint8_t> data);
		size_t bulkWrite(const std::string& data);
		size_t bulkWrite(uint8_t* data, size_t length);

        device(device const&) = delete;
        void operator=(device const&) = delete;

	private:
		libusb_device_handle* handle = nullptr;
        std::vector<int> interfaces;
        bool open = true;
	};






	// ========================================================
	// ===              HotplugListener class               ===
	// ========================================================

    class HotplugListener {
    public:
        HotplugListener(context& ctx) : context(ctx) {}
        ~HotplugListener();

        void start(std::function<void(std::shared_ptr<device> device)> onConnect, float interval = 0.2f);
        void scanOnce(std::function<void(std::shared_ptr<device> device)> onConnect);
        void stop();

    private:
        bool isDeviceKnown(deviceInfo& info);
        void listen(std::function<void(std::shared_ptr<device> device)> onConnect, float interval);

        std::thread listener;
        std::atomic<bool> running = false;
        std::vector<deviceInfo> knownDevices;
        libusb::context& context;
    };

    /*class HotplugHandler {
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
    };*/






    // ====================================================
    // ===               General functions              ===
    // ====================================================

    std::vector<deviceInfo> scanDevices(const context& ctx);

    std::shared_ptr<device> openDevice(const context& ctx, uint16_t vendorID, uint16_t productID);

}
