#pragma once

#include "ODrive.h"
#include "libusbcpp.h"

#include "Entry.h"

#define MAX_NUMBER_OF_ODRIVES 4
#define REF std::reference_wrapper

class Backend;
extern std::unique_ptr<Backend> backend;    // Accessible globally, allocated and deleted by BatteryApp

class Backend {
public:

    libusb::context context;
    libusb::HotplugListener hotplugListener;
    std::array<std::shared_ptr<ODrive>, MAX_NUMBER_OF_ODRIVES> odrives;

    std::vector<Entry> entries;   // Every entry is one line in the control panel
    std::map<std::string, EndpointValue> cachedEndpointValues;   // For endpoint selector, always only one odrive

    Backend();
    ~Backend();

    void scanDevices();
    void deviceConnected(std::shared_ptr<libusb::device> device);
    void connectDevice(std::shared_ptr<libusb::device> device);

    void addEntry(const Entry& entry);
    void removeEntry(const std::string& fullPath);
    void updateEntryCache();

    void executeFunction(int odriveID, const std::string& identifier);
    void odriveDisconnected(int odriveID);

    void updateEndpointCache(int odriveID);
    EndpointValue getCachedEndpointValue(const std::string& fullPath);


    EndpointValue readEndpointDirect(const BasicEndpoint& ep);
    void writeEndpointDirect(const BasicEndpoint& ep, const EndpointValue& value);

    template<typename T>
    std::optional<T> readEndpointDirectRaw(const BasicEndpoint& ep) {
        if (odrives[ep.odriveID]) {
            try {
                return odrives[ep.odriveID]->read<T>(ep.identifier);
            }
            catch (...) {
                if (odrives[ep.odriveID]->connected) {
                    odrives[ep.odriveID]->disconnect();
                    odriveDisconnected(ep.odriveID);
                }
            }
        }
        return std::nullopt;
    }

    template<typename T>
    void writeEndpointDirectRaw(const BasicEndpoint& ep, T value) {
        if (!odrives[ep.odriveID])
            return;

        try {
            odrives[ep.odriveID]->write<T>(ep.identifier, value);
            LOG_DEBUG("Writing {} to endpoint {}", value, ep.fullPath);
        }
        catch (...) {
            if (odrives[ep.odriveID]->connected) {
                odrives[ep.odriveID]->disconnect();
                odriveDisconnected(ep.odriveID);
            }
        }
    }

};
