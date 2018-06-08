#ifndef FLEXSEADEVICEPROVIDER_H
#define FLEXSEADEVICEPROVIDER_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include "flexseadevicetypes.h"
#include "circular_buffer.h"
#include "flexseastack/flexseadevice.h"
#include <memory>

class FlagList {
    std::mutex m_;
    std::vector<uint8_t*> l_;

public:
    void add(uint8_t* f) {
        std::lock_guard<std::mutex> lk(m_);
        l_.push_back(f);
    }
    void remove(uint8_t* f) {
        std::lock_guard<std::mutex> lk(m_);
        l_.erase(std::remove(l_.begin(), l_.end(), f), l_.end());
    }
    void notify() {
        std::lock_guard<std::mutex> lk(m_);
        for(unsigned short i = 0; i < l_.size(); i++)
            *(l_.at(i)) = 1;
    }
};

typedef std::shared_ptr<FlexseaDevice> FxDevicePtr;

class FlexseaDeviceProvider
{
public:
    FlexseaDeviceProvider();
    virtual ~FlexseaDeviceProvider();

    /// \brief Returns a vector containing the ids of all connected devices.
    virtual const std::vector<int>& getDeviceIds() const;

    /// \brief Returns a vector containing the ids of all connected devices at the specified port.
    virtual std::vector<int> getDeviceIds(int portIdx) const;

    /// \brief Returns a FlexseaDevice with matching ID, returns default device if no match.
    /// FlexseaDevice provides an interface to incoming data from a connected device
    virtual const FlexseaDevice& getDevice(int id) const;
    virtual const FxDevicePtr getDevicePtr(int id) const;
    bool haveDevice(int id) const { return connectedDevices.count(id) > 0; }

    /// These functions allow users to be notified of the corresponding events
    void registerConnectionChangeFlag(uint8_t *flag) const {deviceConnectedFlags.add(flag);}
    void registerMapChangeFlag(uint8_t *flag) const {mapChangedFlags.add(flag);}
    void unregisterConnectionChangeFlag(uint8_t *flag) const {deviceConnectedFlags.remove(flag);}
    void unregisterMapChangeFlag(uint8_t *flag) const {mapChangedFlags.remove(flag);}

protected:
    mutable FlagList deviceConnectedFlags;
    mutable FlagList mapChangedFlags;

    std::vector<int> deviceIds;
    std::unordered_map<int, FxDevicePtr> connectedDevices;
    const FlexseaDevice defaultDevice;

    int addDevice(int id, int port, FlexseaDeviceType type);
    int removeDevice(int id);
};

#endif // FLEXSEADEVICEPROVIDER_H
