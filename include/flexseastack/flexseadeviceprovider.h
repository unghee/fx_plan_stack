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

/// \brief convenience class which manages a thread safe vector of uint8_t* flags
/// FlagList operates with pointers but doesn't own any memory/pointers
class FlagList {

public:
    /// \brief adds the flag to the list
    void add(uint8_t* f) {
        std::lock_guard<std::mutex> lk(m_);
        l_.push_back(f);
    }
    /// \brief removes the flag to the list
    void remove(uint8_t* f) {
        std::lock_guard<std::mutex> lk(m_);
        l_.erase(std::remove(l_.begin(), l_.end(), f), l_.end());
    }

    /// \brief sets all flags in the list to 1
    void notify() {
        std::lock_guard<std::mutex> lk(m_);
        for(unsigned short i = 0; i < l_.size(); i++)
            *(l_.at(i)) = 1;
    }

    /// \brief empties the list of all flags
    void clear() {
        std::lock_guard<std::mutex> lk(m_);
        l_.clear();
    }

private:
    std::mutex m_;
    std::vector<uint8_t*> l_;
};

typedef std::shared_ptr<FlexseaDevice> FxDevicePtr;

class FlexseaDeviceProvider
{
public:
    FlexseaDeviceProvider();
    virtual ~FlexseaDeviceProvider();

    /// \brief Returns a vector containing the ids of all connected devices.
    std::vector<int> getDeviceIds() const;

    /// \brief Returns a vector containing the ids of all connected devices at the specified port.
    std::vector<int> getDeviceIds(int portIdx) const;

    /// \brief Returns a FlexseaDevice with matching ID, returns default device if no match.
    /// FlexseaDevice provides an interface to incoming data from a connected device
//    const FlexseaDevice& getDevice(int id) const;

    /// \brief Returns a FxDevicePtr with matching ID, returns a nullptr if no match.
    /// FlexseaDevice provides an interface to incoming data from a connected device
    const FxDevicePtr getDevicePtr(int id) const;

    /// \brief Returns true if this provider contains a device with given id, false otherwise
    bool haveDevice(int id) const { return connectedDevices.count(id) > 0; }

    // These functions allow users to be notified of the corresponding events.
    // flag ownership does not transfer to the device provider
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

    int addDevice(int id, int port, FlexseaDeviceType type, int role=FLEXSEA_MANAGE_1);

    template<typename ... Args>
    int addDevice(int id, Args&&... args)
    {
        if(haveDevice(id)) return 1;

        deviceIds.push_back(id);
        FxDevicePtr devPtr(new FlexseaDevice( id, std::forward<Args>(args)... ));
        connectedDevices.insert({id, devPtr});

        //Notify device connected
        deviceConnectedFlags.notify();

        return 0;
    }

    int removeDevice(int id);
};

#endif // FLEXSEADEVICEPROVIDER_H
