#include "flexseadeviceprovider.h"
#include <cstring>

FlexseaDeviceProvider::FlexseaDeviceProvider() : defaultDevice(-1, -1, FX_NONE, FLEXSEA_MANAGE_1, 0)
{}

FlexseaDeviceProvider::~FlexseaDeviceProvider()
{
	deviceConnectedFlags.clear();
	mapChangedFlags.clear();

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

const FxDevicePtr FlexseaDeviceProvider::getDevicePtr(int id) const
{
	if(connectedDevices.count(id))
		return connectedDevices.at(id);

	return nullptr;
}

int FlexseaDeviceProvider::addDevice(int id, int port, FlexseaDeviceType type, int role)
{
	if(haveDevice(id)) return 1;

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
