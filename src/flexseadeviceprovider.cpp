#include "flexseastack/flexseadeviceprovider.h"
#include <cstring>

FlexseaDeviceProvider::FlexseaDeviceProvider() : defaultDevice(-1, -1, FX_NONE, nullptr, nullptr)
{}

FlexseaDeviceProvider::~FlexseaDeviceProvider()
{
    // de-alloc all devices
    while(deviceIds.size())
        removeDevice(deviceIds.at(0));
}

const std::vector<int>& FlexseaDeviceProvider::getDeviceIds() const
{
    return deviceIds;
}

std::vector<int> FlexseaDeviceProvider::getDeviceIds(int portIdx) const
{
    std::vector<int> ids;
    for(const auto &it : connectedDevices)
    //for(auto it = connectedDevices.begin(); it != connectedDevices.end(); it++)
    {
        if(it.second.port == portIdx)
            ids.push_back(it.first);
    }
    return ids;
}

const FlexseaDevice& FlexseaDeviceProvider::getDevice(int id) const
{
    if(connectedDevices.count(id))
        return connectedDevices.at(id);
    else
        return defaultDevice;
}

const uint32_t* FlexseaDeviceProvider::getMap(int id) const
{
    if(fieldMaps.count(id))
        return fieldMaps.at(id);
    else
        return nullptr;
}

int FlexseaDeviceProvider::addDevice(int id, int port, FlexseaDeviceType type)
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

    uint32_t* map = new uint32_t[FX_BITMAP_WIDTH];
    memset(map, 0, sizeof(uint32_t) * FX_BITMAP_WIDTH);
    circular_buffer<FX_DataPtr> *cb = new circular_buffer<FX_DataPtr>(FX_DATA_BUFFER_SIZE);
    std::recursive_mutex *cbMutex = new std::recursive_mutex();

    deviceIds.push_back(id);
    databuffers.insert({id, cb});
    connectedDevices.insert({id, FlexseaDevice(id, port, type, map, cb, cbMutex)});
    fieldMaps.insert({id, map});

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

    //deallocate the circular buffer
    std::recursive_mutex *m = connectedDevices.at(id).dataMutex;
    m->lock();

    circular_buffer<FX_DataPtr> *cb = databuffers.at(id);
    FX_DataPtr dataptr;
    while(cb->count())
    {
        dataptr = cb->get();
        if(dataptr)
            delete dataptr;
    }
    if(cb)
        delete cb;
    databuffers.erase(id);

    //remove record from connected devices
    connectedDevices.erase(id);

    //deallocate from field maps
    uint32_t *map = fieldMaps.at(id);
    delete[] map;
    fieldMaps.erase(id);

    //Notify device connected
    deviceConnectedFlags.notify();

    m->unlock();
    delete m;
    m = nullptr;

    return 0;
}
