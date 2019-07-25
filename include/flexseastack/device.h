#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <iostream>
#include <chrono>
#include <functional>

#include "flexseaserial.h"
#include "flexseadevice.h"
#include "datalogger.h"

#include "flexsea_cmd_sysdata.h"
#include "flexsea_cmd_stream.h"
#include "flexsea_sys_def.h"

typedef std::chrono::high_resolution_clock Clock;
typedef std::function<void(uint8_t*, uint8_t*, uint8_t*, uint16_t*)> StreamFunc;

/* 	
	NODEVICE = Device default state, serial port has not been opened
	OPEN = Serial port has been opened, commands can be sent and data can be read
	CONNECTED = Is currently streaming commands, including through an AutoStream command
*/  
enum ConnectionState {NODEVICE, OPEN, CONNECTED};

static const int NUM_TIMER_FREQS = 11;
static const int TIMER_FREQS_IN_HZ[NUM_TIMER_FREQS] = {1, 5, 10, 20, 33, 50, 100, 200, 300, 500, 1000};
static const std::unordered_set<int> TIMER_FREQS_SET (TIMER_FREQS_IN_HZ, TIMER_FREQS_IN_HZ + NUM_TIMER_FREQS);

class Device {
public:
	
	const int portIdx;
	std::string portName;

	Device(int portIdx);
	~Device();

	// uses flexseaserial
	bool tryOpen(std::string portName);
	void close();

	//return -1 if not set
	int getDevId();

	ConnectionState getConnectionState();

	// Getters and setters
	bool getAutoStreamStatus();
	void setAutoStreamStatus(bool shouldAutoStream);

	bool getShouldLog();
	void setShouldLog(bool shouldLog);

	FlexseaDevice* getFlexseaDevice();

	std::vector<int> getStreamingFreqs();

	void writeDeviceMap(uint32_t* map);
	void addStream(int freq, const StreamFunc& func);
	void addStream(int freq, uint8_t cmdCode);
	//Removes all streaming commands with cmdCode at all frequencies 
	void stopStreaming(uint8_t cmdCode = -1);

	void sendAutoStream(int cmd, int freq, bool startFlag);
	void sendSysDataRead();


	template<typename T, typename... Args>
	bool enqueueCommand(T tx_func, Args&&... tx_args){
		// assert(connectionState >= OPEN);
		int shortId = serialDevice == nullptr ? 0 : serialDevice->getShortId();

		std::vector<Message> packedMessages = flexseaSerial.generateMessages(shortId,
														portIdx,
														tx_func,
														std::forward<Args>(tx_args)...);
		if(packedMessages.empty()){
			return false;
		}
		std::unique_lock<std::mutex> incomingQueueLock(incomingCommandsLock);
		for(auto & message : packedMessages){
			incomingCommands.push_back(message);
		}
		return true;
	}

	// void enqueueCommand(Message message);

private:
	struct StreamCommand{
		bool shouldStream;
		uint16_t cmdCode;
		StreamFunc* func;
	};
	// static std::mutex flexseaSerial
	static FlexseaSerial flexseaSerial;

	std::mutex stateLock;
	int devId;
	bool shouldLog;
	bool isAutoStreaming;
	bool serialDeviceIsSetUp;
	std::atomic<ConnectionState> connectionState {NODEVICE};

	std::mutex incomingCommandsLock;
	std::deque<Message> incomingCommands;

	//Maps streaming frequency to vector of commands
	std::mutex streamLock;
	int streamingFreq;
	StreamCommand streamCmd;

	std::mutex autoStreamLock;
	std::vector<std::pair<int, uint8_t>> autoStreamList;

	bool shouldRun;
	// std::vector<std::thread*> commandSenders;
	std::thread* commandSender;
	std::thread* commandStreamer;
	std::thread* deviceReader;
	std::thread* deviceLogger;

	FlexseaDevice* serialDevice;
	DataLogger* dataLogger;

	// functions to call after device serial port is opened successfully
	void setUpFlexseaSerialDevice();
	void setUpLogging();

	bool passedPeriod(int freqIndex);
	// runnable functions (separate threads)
	void streamCommands();
	void sendCommands();
	void readFromDevice();
	void logDevice();

	void startInitialThreads();
	void startStreamingThreads();
	void startLoggingThread();
	void stopThreads();

    static const int CMD_CODE_BASE = 256;

};

#endif 