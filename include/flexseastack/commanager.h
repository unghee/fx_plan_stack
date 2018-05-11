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
    void cleanup();

    /// \brief Returns a vector containing the frequencies that can be streamed at, in Hz
    std::vector<int> getStreamingFrequencies() const;

    /// \brief Tries to start streaming from the selected device with the given parameters.
    /// Streams all fields by default
    /// Returns true if the the attempt was successful. May be unsuccessful if:
    ///     no device exists with device id == devId
    ///     freq not in getStreamingFrequences()
    bool startStreaming(int devId, int freq, bool shouldLog, bool shouldAuto);

    /// \brief Tries to start streaming from the selected device with the given parameters.
    /// Streams all fields if fieldIds is empty. Otherwise only streams ids in fieldIds
    /// Returns true if the attempt was successful. May be unsuccessful if:
    ///     no device exists with device id == devId
    ///     freq not in getStreamingFrequences()
    ///     fieldIds contains an invalid id
    bool startStreaming(int devId, int freq, bool shouldLog, bool shouldAuto, const std::vector<int> &fieldIds);

    /// \brief Tries to stop streaming from the selected device with the given parameters.
    /// Returns true if the attempt was successful. May be unsuccessful if no stream for given id exists
    bool stopStreaming(int devId);

    /// \brief populates a list of device ids that are at the specified portIdx;
    std::vector<int> getDeviceIds(int portIdx) const;

    bool enqueueCommand(uint8_t numb, uint8_t* dataPacket, int portIdx=0);


protected:
    virtual void periodicTask();
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

private:


	//Variables & Objects:
	class Message {
	public:
		static void do_delete(uint8_t buf[]) { delete[] buf; }

        Message(uint8_t nb, uint8_t* data, int portIdx_=0):
        numBytes(nb)
        , dataPacket(std::shared_ptr<uint8_t>(new uint8_t[nb], do_delete))
        , portIdx(portIdx_)
        {
            uint8_t* temp = dataPacket.get();
			for(int i = 0; i < numBytes; i++)
				temp[i] = data[i];
		}

		uint8_t numBytes;
        std::shared_ptr<uint8_t> dataPacket;
        int portIdx;
	};
	std::queue<Message> outgoingBuffer;

	class CmdSlaveRecord
	{
	public:
        CmdSlaveRecord(int c, int s, bool l):
            cmdType(c), slaveIndex(s), shouldLog(l){}
		int cmdType;
		int slaveIndex;
		bool shouldLog;
		clock_t initialTime;
        std::string date;
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

    uint8_t streamCount;
};
#endif // STREAMMANAGER_H
