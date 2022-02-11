#pragma once

#include "ODrive.h"
#include "libusbcpp.h"

#define MAX_NUMBER_OF_ODRIVES 4
#define REF std::reference_wrapper

class Backend;
extern std::unique_ptr<Backend> backend;    // Accessible globally, allocated and deleted by BatteryApp

template<typename T>
void backendReadEndpoint(std::shared_ptr<ODrive>& odrive, const std::string& path, const std::string& identifier) {
    T value = odrive->read<T>(identifier);
    uint64_t rawValue;
    memcpy(&rawValue, &value, sizeof(value));
    backend->values[path] = rawValue;
}

class Backend {
public:
    Backend();
    ~Backend();

    void scanDevices();
    void deviceConnected(std::shared_ptr<libusb::device> device);
    void connectDevice(std::shared_ptr<libusb::device> device);

    void addEndpoint(std::reference_wrapper<Endpoint> info, int odriveID);
    void removeEndpoint(size_t index);
    void readEndpoint(const std::string& type, std::shared_ptr<ODrive>& odrive, const std::string& path, const std::string& identifier);
    void updateEndpoints();

    template<typename T>
    void setValue(const std::string& path, T value) {
        std::string identifier = path.substr(6);

        uint8_t index = path[4] - '0';		// Get ODrive index
        if (index >= MAX_NUMBER_OF_ODRIVES)
            return;

        LOG_DEBUG("Setting {} to {}", path, value);

        odrives[index]->write<T>(identifier, value);
    }

    void executeFunction(const std::string& path);

    libusb::context context;
    libusb::HotplugListener hotplugListener;
    std::array<std::shared_ptr<ODrive>, MAX_NUMBER_OF_ODRIVES> odrives;

    std::vector<std::pair<int, Endpoint>> endpoints;
    std::map<std::string, uint64_t> values;
    std::map<std::string, uint64_t> oldValues;
};
