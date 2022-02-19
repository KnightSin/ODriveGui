#pragma once

#include "ODrive.h"
#include "libusbcpp.h"
#include "Entry.h"

#define USB_SCAN_INTERVAL 1.0f

#define MAX_NUMBER_OF_ODRIVES 4

#define REF std::reference_wrapper
extern const char* DEFAULT_ENTRIES_JSON;

class Backend;
extern std::unique_ptr<Backend> backend;    // Accessible globally, allocated and deleted by BatteryApp

class Backend {
public:

    libusbcpp::context context;
    std::array<std::shared_ptr<ODrive>, MAX_NUMBER_OF_ODRIVES> odrives;
    std::shared_ptr<ODrive> temporaryDevice;
    std::mutex temporaryDeviceMutex;
    std::atomic<bool> deviceWaiting = false;

    std::vector<Entry> entries;   // Every entry is one line in the control panel
    std::map<std::string, EndpointValue> cachedEndpointValues;   // For endpoint selector, always only one odrive

    Backend();
    ~Backend();

    void listenerThread();
    void handleNewDevices();
    void connectDevice(std::shared_ptr<ODrive> odrv);

    void addEntry(const Entry& entry);
    void removeEntry(const std::string& fullPath);
    void updateEntryCache();
    void importEntries(std::string path = "");
    void exportEntries(const std::string& file = "");
    void loadDefaultEntries();

    void executeFunction(int odriveID, const std::string& identifier);
    void odriveDisconnected(int odriveID);

    void updateEndpointCache(int odriveID);
    EndpointValue getCachedEndpointValue(const std::string& fullPath);


    EndpointValue readEndpointDirect(const BasicEndpoint& ep);
    void writeEndpointDirect(const BasicEndpoint& ep, const EndpointValue& value);

    template<typename T>
    bool readEndpointDirectRaw(const BasicEndpoint& ep, T* value_ptr) {
        if (!odrives[ep.odriveID])
            return false;

        return odrives[ep.odriveID]->read<T>(ep.identifier, value_ptr);
    }

    template<typename T>
    void writeEndpointDirectRaw(const BasicEndpoint& ep, T value) {
        if (!odrives[ep.odriveID])
            return;

        odrives[ep.odriveID]->write<T>(ep.identifier, value);
        LOG_DEBUG("Writing {} to endpoint {}", value, ep.fullPath);
    }

private:
    std::thread usbListener;
    std::atomic<bool> stopListener = false;
};
