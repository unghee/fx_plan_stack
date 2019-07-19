#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <queue>
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

#include "flexsea_cmd_sysdata.h"
#include "flexsea_sys_def.h"

#include "datalogger.h"
#include "flexseaserial.h"
#include "flexseadevice.h"

typedef std::chrono::high_resolution_clock Clock;
typedef std::function<void(uint8_t*, uint8_t*, uint8_t*, uint16_t*)> StreamFunc;

enum ConnectionState {NODEVICE, OPEN, CONNECTED};

static const int MAX_TRY_OPEN_ATTEMPTS = 10;

static const int NUM_TIMER_FREQS = 11;
static const int TIMER_FREQS_IN_HZ[NUM_TIMER_FREQS] = {1, 5, 10, 20, 33, 50, 100, 200, 300, 500, 1000};
static const int TIMER_PERIODS_IN_MS[NUM_TIMER_FREQS] = {1000, 200, 100, 50, 30, 20, 10, 5, 3, 2, 1};
static const std::unordered_set<int> TIMER_FREQS_SET (TIMER_FREQS_IN_HZ, TIMER_FREQS_IN_HZ + NUM_TIMER_FREQS);

class Device {
public:
	
	const std::string portName;
	const int portIdx;

	Device(int portIdx);
	~Device();

	//return -1 if not set
	int getDevId();

	ConnectionState getDeviceStatus();

	//Getters and setters
	// bool getAutoStream();
	// void setAutoStream(bool shouldAutoStream);

	bool getShouldLog();
	void setShouldLog(bool shouldLog);

	FlexseaDevice* getFlexseaDevice();

	std::vector<int> getStreamingFreqs();

	void writeDeviceMap(uint32_t* map);
	void addStream(int freq, const StreamFunc& func);
	void addStream(int freq, uint8_t cmdCode);
	//Removes all streaming commands with cmdCode at all frequencies 
	void stopStreaming(uint8_t cmdCode);

	void sendAutoStream(uint8_t cmd, int freq, bool startFlag);
	void sendSysDataRead();


	template<typename T, typename... Args>
	void enqueueCommand(T tx_func, Args&&... tx_args);

	// void enqueueCommand(Message message);

private:
	struct StreamCommand{
		bool shouldStream;
		uint16_t cmdCode;
		StreamFunc* func;
	};
	// static std::mutex flexseaSerial
	static FlexseaSerial flexseaSerial;
	// std::string portName;

	std::mutex stateLock;
	int devId;
	bool shouldLog;
	bool isAutoStreaming;
	bool serialDeviceIsSetUp;
	std::atomic<ConnectionState> connectionState {NODEVICE};

	std::mutex incomingCommandsLock;
	std::queue<Message> incomingCommands;

	//Maps streaming frequency to vector of commands
	std::mutex streamLock;
	int streamingFreq;
	StreamCommand streamCmd;

	std::mutex autoStreamLock;
	std::vector<std::pair<int, uint8_t>> autoStreamList;

	bool shouldRun;
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
	void stopThreads();

	// uses flexseaserial
	bool tryOpen();
	void close();

	
    static const int CMD_CODE_BASE = 256;

};

#endif 