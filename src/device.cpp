
#include "device.h"

using namespace std::chrono_literals;

FlexseaSerial Device::flexseaSerial;

Device::Device(int portIdx):
							portIdx(portIdx)
{
	streamCmd = {false, CMD_CODE_BASE, nullptr};
	serialDeviceIsSetUp = false;
	devId = -1;
	shouldLog = false;

	commandSender = nullptr;
	commandStreamer = nullptr;
	deviceReader = nullptr;
	deviceLogger = nullptr;

	serialDevice = nullptr;
	dataLogger = nullptr;
	connectionState = NODEVICE;

}

Device::~Device(){
	stopThreads();

	close();
	if(serialDeviceIsSetUp){
		if(streamCmd.func != nullptr){
			delete streamCmd.func;
		}
		delete dataLogger;
		delete serialDevice;
	}

}

int Device::getDevId(){
	std::unique_lock<std::mutex> lk(stateLock);
	if(serialDeviceIsSetUp){
		return devId;
	}
	else{
		return -1;
	}
}

ConnectionState Device::getConnectionState(){
	std::unique_lock<std::mutex> lk(stateLock);
	return connectionState;
}

bool Device::getShouldLog(){
	return shouldLog;
}

void Device::setShouldLog(bool shouldLog){
	this->shouldLog = shouldLog;
}

FlexseaDevice* Device::getFlexseaDevice(){
	assert(connectionState >= OPEN);
	return serialDevice;
}

std::vector<int> Device::getStreamingFreqs(){
	std::unordered_set<int> communicatingFreqs;
	{
		std::unique_lock<std::mutex> lk(streamLock);
		if(streamCmd.shouldStream){
			communicatingFreqs.insert(streamingFreq);
		}
	}

	{
		std::unique_lock<std::mutex> lk(autoStreamLock);
		for(auto & autoStream : autoStreamList){
			communicatingFreqs.insert(autoStream.first);
		}
	}
	return std::vector<int>(communicatingFreqs.begin(), communicatingFreqs.end());
}


int getMapLen(const uint32_t* map){
	uint16_t mapLen = 0;
	for(short i = FX_BITMAP_WIDTH-1; i >= 0; i--){
		if(map[i] > 0){
			mapLen = i + 1;
			break;
		}
	}

	mapLen = mapLen > 0 ? mapLen : 1;
}

void Device::writeDeviceMap(uint32_t* map){
	assert(connectionState >= OPEN);
	enqueueCommand(tx_cmd_sysdata_w, map, getMapLen(map));
}

void Device::addStream(int freq, uint8_t cmdCode){
	std::unique_lock<std::mutex> lk(streamLock);
	assert(connectionState >= OPEN);
	if(streamCmd.func){ //replace func
		delete streamCmd.func;
		streamCmd.func = nullptr;
	}
	connectionState = CONNECTED;
	streamingFreq = freq;
	streamCmd = {true, cmdCode, nullptr};
}

void Device::addStream(int freq, const StreamFunc& func){
	std::unique_lock<std::mutex> lk(streamLock);
	assert(connectionState >= OPEN);

	if(streamCmd.func){ //replace func
		delete streamCmd.func;
		streamCmd.func = nullptr;
	}

	streamingFreq = freq;
	streamCmd = {true, CMD_CODE_BASE, new StreamFunc(func)};
}

void Device::stopStreaming(uint8_t cmdCode){
	shouldLog = false;
	//streamElement is a key-value pair <int, vector<uint8_t>>
	{
		std::unique_lock<std::mutex> lk(streamLock);
		if(cmdCode == -1){
			streamCmd = {false, CMD_CODE_BASE, nullptr};
		}
		else if(cmdCode == streamCmd.cmdCode){
			streamCmd = {false, CMD_CODE_BASE, nullptr};
		}
		assert(streamCmd.func == nullptr); //must call with cmdCode = -1 if stopping custom StreamFunc
	}	

	{
		std::unique_lock<std::mutex> lk(autoStreamLock);
		for(auto const & autoStream : autoStreamList){
			sendAutoStream(autoStream.second, autoStream.first, false);
		}
		autoStreamList.clear();
	}

	isAutoStreaming = false;
	connectionState = OPEN;
}

void Device::sendAutoStream(uint8_t cmdCode, int freq, bool startFlag){
	assert(connectionState >= OPEN);
	if(startFlag){
		std::unique_lock<std::mutex> lk(autoStreamLock);
		connectionState = CONNECTED;
		autoStreamList.push_back(std::pair<int, uint8_t>(freq, cmdCode));
		isAutoStreaming = true;
	}

	int period = 1000 / freq;
	enqueueCommand(tx_cmd_stream_w,
				   cmdCode, period, startFlag, 0, 0);
}

void Device::sendSysDataRead(){
	enqueueCommand(tx_cmd_sysdata_r, nullptr, 0);
}



void Device::startInitialThreads(){
	shouldRun = true;
	deviceReader = new std::thread(&Device::readFromDevice, this);
	commandSender = new std::thread(&Device::sendCommands, this);
}

void Device::startStreamingThreads(){
	assert(connectionState >= OPEN);
	commandStreamer = new std::thread(&Device::streamCommands, this);
	// for(int i = 0; i < 5; ++i){
	// 	commandSenders.push_back(new std::thread(&Device::sendCommands, this));
	// }
}

void Device::startLoggingThread(){
	assert(connectionState == CONNECTED);
	if(!deviceLogger){
		deviceLogger = new std::thread(&Device::logDevice, this);
	}
}

void Device::stopThreads(){
	shouldRun = false;

	// if(commandSenders.size()){
	// 	for(std::thread* th : commandSenders){
	// 		th->join();
	// 		delete th;
	// 	}
	// }
	if(commandSender){
		commandSender->join();
		delete commandSender;
	}
	if(commandStreamer){
		commandStreamer->join();
		delete commandStreamer;
	}
	if(deviceReader){
		deviceReader->join();
		delete deviceReader;
	}
	if(deviceLogger){
		deviceLogger->join();
		delete deviceLogger;
	}
	// commandSenders.clear();
	commandSender = nullptr;
	commandStreamer = nullptr;
	deviceReader = nullptr;
	deviceLogger = nullptr;
}

void Device::streamCommands(){
	auto t1 = Clock::now();
	while(shouldRun){
		std::unique_lock<std::mutex> lk(streamLock);
		if(streamCmd.shouldStream){
			assert(connectionState == CONNECTED);
			assert(TIMER_FREQS_SET.find(streamingFreq) != TIMER_FREQS_SET.end());
			int period = 1000 / streamingFreq;	

			auto t2 = Clock::now(); //this won't be exactly the period, may have to change to be more accurate
			if(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() > period){
				t1 = t2;
				if(streamCmd.cmdCode == CMD_SYSDATA){
					sendSysDataRead();
				}
				// else{
				// 	enqueueCommand(streamCmd.func);
				// }
			}
		}
		else{
			lk.unlock();
			std::this_thread::sleep_for(50ms);	
		}
	}
}

void Device::sendCommands(){
	while(shouldRun){
		// assert(connectionState >= OPEN);
		std::unique_lock<std::mutex> lk(incomingCommandsLock);
		//we can try writing more than 1 command per iteration if it's too slow
		if(!incomingCommands.empty()){
			Message& m = incomingCommands.front();
			incomingCommands.pop();
			lk.unlock();

			flexseaSerial.write(m.numBytes, m.dataPacket.get(), portIdx);
		}
	}
}

void Device::readFromDevice(){
	while(shouldRun){
		flexseaSerial.readAndProcessData(portIdx, serialDevice);
		
		if(!serialDeviceIsSetUp && serialDevice != nullptr){
			connectionState = OPEN; //now it is actually open
			serialDeviceIsSetUp = true;
			devId = serialDevice->_devId;
			// setUpLogging();
			startStreamingThreads();
		}
	}
}

void Device::logDevice(){
	while(shouldRun){
		if(shouldLog){
			startLoggingThread();
			assert(connectionState == CONNECTED);
			if(!dataLogger){
				std::cerr << "Device has not been configured to log yet" << std::endl;
				assert(dataLogger);
				continue;
			}
			dataLogger->logDevice();
		}
		else{
			std::this_thread::sleep_for(50ms); //max 50ms delay
		}
	}
}

void Device::setUpLogging(){
	dataLogger = new DataLogger(true, this->serialDevice);
}

bool Device::tryOpen(std::string portName){
	this->portName = portName;
	startInitialThreads();
	bool opened = flexseaSerial.open(portName, portIdx);
	if(opened){
		sendSysDataRead();
		flexseaSerial.sendDeviceWhoAmI(portIdx);
		// startInitialThreads();
	}
	// flexseaSerial.open(portName, portIdx);
	return opened;
}

void Device::close(){
	flexseaSerial.close(portIdx);
}
