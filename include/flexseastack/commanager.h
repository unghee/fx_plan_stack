#ifndef STREAMMANAGER_H
#define STREAMMANAGER_H

#include <ctime>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>

#include "flexseaserial.h"
#include "periodictask.h"

class CommManager : public PeriodicTask
{

public:
    CommManager();
    virtual ~CommManager();

    static const int NUM_TIMER_FREQS = 11;

    FlexseaSerial* fxSerial;

    void init(FlexseaSerial* serialDriver);

    void startStreaming(const FlexseaDevice& device, int cmd, int freq, bool shouldLog);
    void startAutoStreaming(const FlexseaDevice& device, int cmd, int freq, bool shouldLog);

    void stopStreaming(const FlexseaDevice &device, int cmd, int freq);
    void stopStreaming(const FlexseaDevice &device);
    void stopStreaming(int cmd, uint8_t slave, int freq);

    uint8_t streamCount;

    void enqueueCommand(uint8_t numb, uint8_t* dataPacket);

protected:
    virtual void periodicTask();
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

private:


	//Variables & Objects:
	class Message {
	public:
		static void do_delete(uint8_t buf[]) { delete[] buf; }

		Message(uint8_t nb, uint8_t* data) {
			numBytes = nb;
            dataPacket = std::shared_ptr<uint8_t>(new uint8_t[nb], do_delete);
            uint8_t* temp = dataPacket.get();
			for(int i = 0; i < numBytes; i++)
				temp[i] = data[i];
		}

		uint8_t numBytes;
        std::shared_ptr<uint8_t> dataPacket;
		uint8_t r_w;
	};
	std::queue<Message> outgoingBuffer;

	class CmdSlaveRecord
	{
	public:
        CmdSlaveRecord(int c, int s, bool l, const FlexseaDevice* d):
            cmdType(c), slaveIndex(s), shouldLog(l), device(d) {}
		int cmdType;
		int slaveIndex;
		bool shouldLog;
		clock_t initialTime;
        std::string date;
        const FlexseaDevice* device;
	};

	std::vector<CmdSlaveRecord> autoStreamLists[NUM_TIMER_FREQS];
	std::vector<CmdSlaveRecord> streamLists[NUM_TIMER_FREQS];

    void tryPackAndSend(int cmd, int slaveId);
    int getIndexOfFrequency(int freq);

    int timerFrequencies[NUM_TIMER_FREQS];
	float timerIntervals[NUM_TIMER_FREQS];
    float msSinceLast[NUM_TIMER_FREQS] = {0};

    std::vector<std::string> experimentLabels;
    std::vector<int> experimentCodes;

    //this should go somewhere else probly
    static const int COMM_STR_LEN = 150;
    uint8_t comm_str_usb[COMM_STR_LEN];

    void sendCommands(int index);
    void sendCommandReadAll(const FlexseaDevice* d);
    void sendSysDataRead(uint8_t slaveId);
    void sendCommandRigid(uint8_t slaveId);
};
#endif // STREAMMANAGER_H
