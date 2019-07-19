#include "commanager.h"

extern "C" {
	#include "flexsea_cmd_sysdata.h"
	#include "flexsea_comm_multi.h"
}

using namespace std::chrono_literals;


CommManager::CommManager(std::vector<std::string> portNames, std::vector<uint16_t> ports)
{
	if(portNames.size() != ports.size()){
		return NULL;
	}
	for(int i = 0; i < portNames.size(); ++i){
		devicePortMap[ports[i]] = new Device(portNames[i], ports[i]);
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


int CommManager::loadAndGetDevice(uint16_t portIdx){
	int attempts = 0;
	if(devicePortMap[portIdx]->tryOpen()){
		while(devicePortMap[portIdx]->getDevId() == -1 && attempts++ <= 5){
			this_thread::sleep_for(100ms);
		}

		if(attempts > 5){
			return -1;
		}
		else{
			int devId = devicePortMap[portIdx]->getDevId;
			deviceMap[devId] = devicePortMap[portIdx];
			devIds.push_back(devId);
			return devicePortMap[portIdx]->getDevId;
		}
	}
}

int isOpen(int portIdx){
	return (devicePortMap[portIdx]->getConnectionState >= OPEN);
}

void CommManager::closeDevice(uint16_t portIdx)
{
	Device* device = devicePortMap.at(portIdx);
	device->close();
}

std::vector<int> CommManager::getDeviceIds(){
	return deviceIds;
}

int CommManager::getStreamingFrequencies(int freq)
{
	std::vector<int> frequencies;
	frequencies.resize(NUM_TIMER_FREQS);
	memcpy(frequencies.data, timerFrequencies, sizeof(int) * NUM_TIMER_FREQS);
	return frequencies;
}

bool isValidFreq(int freq){
	if(TIMER_FREQS_SET.find(freq) == TIMER_FREQS_SET.end()){
		std::cerr << "Timer freq: " << freq << " not valid." << std::endl;
		return false;
	}
	return true;
}

// We may want to bake this in to accessing a device
bool isValidDevId(int devId){
	if(deviceMap.find(devId) == deviceMap.end()){
		std::cerr << "Cannot find device with devId: " << devId << std::endl;
		return false;
	}
	return true;
}

bool CommManager::startStreaming(int devId, int freq, bool shouldLog, int shouldAuto, uint8_t cmdCode)
{
	if(!isValidFreq || !isValidDevId){
		return false;
	}

	Device* device = deviceMap.at(devId);
	if(shouldAuto){
		device->sendAutoStream(cmdCode, freq, true);
	}
	else{
		device->addStream(freq, cmdCode);
	}

	device->setAutoStream(shouldAuto)
	device->setShouldLog(shouldLog);
}

int CommManager::startStreaming(int devId, int freq, bool shouldLog, const StreamFunc &streamFunc)
{
	if(!isValidFreq || !isValidDevId)
		return false;

	Device* device = deviceMap.at(devId);
	device->addStream(freq, streamFunc);

	device->setShouldLog(shouldLog);
}

bool CommManager::stopStreaming(int devId, int cmdCode)
{
	Device* device = deviceMap.at(devId);
	device->stopStreaming(cmdCode);
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

bool CommManager::writeDeviceMap(int devId, uint32_t *map)
{
	if(!isValidDevId(devId))
		return false;
	Device* device = deviceMap.at(devId);

	return device->writeDeviceMap(map);

}

int CommManager::writeDeviceMap(int devId, const std::vector<int> &fields)
{
	int nf = d->numFields;
	uint32_t map[FX_BITMAP_WIDTH];
	memset(map, 0, sizeof(uint32_t)*FX_BITMAP_WIDTH);

	for(auto&& f : fields)
	{
		if(f < nf)
		{
			SET_FIELD_HIGH(f, map);
		}
	}

	return writeDeviceMap(devId, map);
}

bool CommManager::enqueueCommand(int devId, T tx_func, Args&&... tx_args)
{
	if(!isValidDevId(devId))
		return -1;
	Device* device = deviceMap.at(devId);
	device->enqueueCommand(tx_func, std::forward<Args>(tx_args)...);

	return true;
}