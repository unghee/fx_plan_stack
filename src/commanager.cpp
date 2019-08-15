#include "commanager.h"

extern "C" {
	#include "flexsea_cmd_sysdata.h"
	#include "flexsea_comm_multi.h"
}

using namespace std::chrono_literals;


CommManager::CommManager()
{
	DataLogger::setDefaultLogFolder();
	for(int i = 0; i < FX_NUMPORTS; ++i){
		devicePortMap[i] = new Device(i);
	}
}

CommManager::~CommManager()
{
	for(auto deviceEntry : devicePortMap){
		delete deviceEntry.second;
	}
	deviceMap.clear();
	devicePortMap.clear();
	deviceIds.clear();
}

int CommManager::isOpen(int portIdx) const
{
	return devicePortMap.at(portIdx)->getConnectionState() >= OPEN;
}

int CommManager::openDevice(const char* portName, uint16_t portIdx)
{
	int attempts = 0;
	std::string pName = portName;
	if(devicePortMap[portIdx]->tryOpen(pName)){
		while(devicePortMap[portIdx]->getDevId() == -1 && attempts++ < 5){
			std::this_thread::sleep_for(1s);
		}

		if(attempts > 5){
			std::cout << "Exceeded number of attempts" << std::endl;
			return -1;
		}
		else{
			int devId = devicePortMap[portIdx]->getDevId();
			deviceMap[devId] = devicePortMap[portIdx];
			deviceIds.push_back(devId);
			return devId;
		}
	}
	std::cerr << "Could not open device" << std::endl;
	return -1;
}

void CommManager::closeDevice(uint16_t portIdx)
{
	Device* device = devicePortMap.at(portIdx);
	int devId = device->getDevId();
	deviceMap.erase(devId);
	deviceIds.erase(std::remove(deviceIds.begin(), deviceIds.end(), devId), deviceIds.end());
	device->close();
}

std::vector<int> CommManager::getDeviceIds() const
{
	return deviceIds;
}

// We may want to bake this in to accessing a device
bool CommManager::isValidDevId(int devId) const
{
	if(deviceMap.find(devId) == deviceMap.end()){
		std::cerr << "Cannot find device with devId: " << devId << std::endl;
		return false;
	}
	return true;
}

std::vector<int> CommManager::getStreamingFrequencies() const
{
	std::vector<int> frequencies;
	frequencies.resize(NUM_TIMER_FREQS);
	memcpy(frequencies.data(), TIMER_FREQS_IN_HZ, sizeof(int) * NUM_TIMER_FREQS);
	return frequencies;
}

bool isValidFreq(int freq)
{
	if(TIMER_FREQS_SET.find(freq) == TIMER_FREQS_SET.end()){
		std::cerr << "Timer freq: " << freq << " not valid." << std::endl;
		return false;
	}
	return true;
}

bool CommManager::startStreaming(int devId, int freq, bool shouldLog, int shouldAuto, uint8_t cmdCode)
{
	if(!isValidFreq(freq) || !isValidDevId(devId)){
		return false;
	}

	Device* device = deviceMap.at(devId);
	if(shouldAuto){
		device->sendAutoStream(cmdCode, freq, true);
	}
	else{
		device->addStream(freq, cmdCode);
	}

	device->setShouldLog(shouldLog);
}

int CommManager::startStreaming(int devId, int freq, bool shouldLog, const StreamFunc &streamFunc)
{
	if(!isValidFreq(freq) || !isValidDevId(devId)){
		return false;
	}

	Device* device = deviceMap.at(devId);
	device->addStream(freq, streamFunc);

	device->setShouldLog(shouldLog);
}

bool CommManager::stopStreaming(int devId, int cmdCode)
{
	Device* device = deviceMap.at(devId);
	device->stopStreaming(cmdCode);
}

int CommManager::writeDeviceMap(int devId, uint32_t *map)
{
	if(!isValidDevId(devId)){
		return false;
	}
	Device* device = deviceMap.at(devId);

	device->writeDeviceMap(map);
	return 0;
}

int CommManager::writeDeviceMap(int devId, const std::vector<int> &fields)
{
	int nf = deviceMap[devId]->getFlexseaDevice()->_numFields;
	uint32_t map[FX_BITMAP_WIDTH];
	memset(map, 0, sizeof(uint32_t)*FX_BITMAP_WIDTH);

	for(auto&& f : fields){
		if(f < nf){
			SET_FIELD_HIGH(f, map);
		}
	}

	return writeDeviceMap(devId, map);
}

bool CommManager::readDevice(int devId, int* dataBuffer, int numFields) const
{
	if(!isValidDevId(devId)){
		std::cerr << "Invalid devId" << std::endl;
		return false;
	}

	Device* device = deviceMap.at(devId);
	return device->getDeviceData(dataBuffer, numFields);
}

const FlexseaDevice* CommManager::getDevicePtr(int devId) const
{
	if(!isValidDevId(devId)){
		std::cerr << "Invalid devId" << std::endl;
		return nullptr;
	}

	return deviceMap.at(devId)->getFlexseaDevice();
}

bool CommManager::createSessionFolder(std::string sessionName)
{
	return DataLogger::createSessionFolder(sessionName);
}

bool CommManager::setLogFolder(std::string logFolderPath)
{
	return DataLogger::setLogFolder(logFolderPath);
}

bool CommManager::setDefaultLogFolder()
{
	return DataLogger::setDefaultLogFolder();
}

void CommManager::setAdditionalColumn(std::vector<std::string> addLabel, std::vector<int> addValue)
{
	DataLogger::setAdditionalColumn(addLabel, addValue);
}

void CommManager::setColumnValue(unsigned col, int val)
{
	DataLogger::setColumnValue(col, val);
}