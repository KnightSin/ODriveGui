#pragma once

#include "ODrive.h"
#include "libusbcpp.h"

#define MAX_NUMBER_OF_ODRIVES 4

class Backend {

    Backend() {}

public:

    //std::unique_ptr<libusb::DeviceList> rawDeviceList;
    libusb::HotplugHandler hotplutHandler;
    //std::array<ODrive, MAX_NUMBER_OF_ODRIVES> odrives;

    void init();
    void cleanup();

    void connectDevice();

public:
    Backend(Backend const&) = delete;
    void operator=(Backend const&) = delete;

    static Backend& getInstance() {
        static Backend instance;
        return instance;
    }
};
