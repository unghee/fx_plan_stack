#include "flexseastack/flexseadeviceprovider.h"
#include <cstring>

FlexseaDeviceProvider::FlexseaDeviceProvider() : defaultDevice(-1, -1, FX_NONE)
{}

FlexseaDeviceProvider::~FlexseaDeviceProvider()
{
    // de-alloc all devices
    while(deviceIds.size())
        removeDevice(deviceIds.at(0));
}

std::vector<int> FlexseaDeviceProvider::getDeviceIds() const
{
    return deviceIds;
}

std::vector<int> FlexseaDeviceProvider::getDeviceIds(int portIdx) const
{
    std::vector<int> ids;
    for(const auto &it : connectedDevices)
    //for(auto it = connectedDevices.begin(); it != connectedDevices.end(); it++)
    {
        if(it.second->port == portIdx)
            ids.push_back(it.first);
    }
    return ids;
}

const FlexseaDevice& FlexseaDeviceProvider::getDevice(int id) const
{
    if(connectedDevices.count(id))
        return *(connectedDevices.at(id));
    else
        return defaultDevice;
}

const FxDevicePtr FlexseaDeviceProvider::getDevicePtr(int id) const
{
    if(connectedDevices.count(id))
        return connectedDevices.at(id);

    return nullptr;
}

int FlexseaDeviceProvider::addDevice(int id, int port, FlexseaDeviceType type, int role)
{
    int unique = 1;
    for(const auto& it : deviceIds)
    {
        if(it == id)
        {
            unique = 0;
            break;
        }
    }
    if(!unique) return 1;

    deviceIds.push_back(id);
    FxDevicePtr devPtr(new FlexseaDevice(id, port, type, role));
    connectedDevices.insert({id, devPtr});

    //Notify device connected
    deviceConnectedFlags.notify();

    return 0;
}

int FlexseaDeviceProvider::removeDevice(int id)
{
    int found = 0;
    std::vector<int>::iterator it;
    for(it = deviceIds.begin(); it != deviceIds.end() && !found; ++it)
    {
        if((*it) == id)
            found = 1;
    }
    if(!found) return 1;

    //remove record in device ids
    deviceIds.erase(--it);

    //remove record from connected devices
    connectedDevices.erase(id);

    //Notify device connected
    deviceConnectedFlags.notify();

    return 0;
}
