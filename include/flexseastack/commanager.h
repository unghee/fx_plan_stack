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



class CommManager : virtual public PeriodicTask, public FlexseaSerial
{

public:
    CommManager();
    virtual ~CommManager();

    static const int NUM_TIMER_FREQS = 11;

    /// \brief Returns a vector containing the frequencies that can be streamed at, in Hz
    std::vector<int> getStreamingFrequencies() const;

    /// \brief Tries to start streaming from the selected device with the given parameters.
    /// Streams all fields by default
    /// Returns true if the the attempt was successful. May be unsuccessful if:
    ///     no device exists with device id == devId
    ///     freq not in getStreamingFrequences()
    virtual bool startStreaming(int devId, int freq, bool shouldLog, bool shouldAuto);

    /// \brief Tries to start streaming from the selected device with the given parameters.
    /// Streams all fields if fieldIds is empty. Otherwise only streams ids in fieldIds
    /// Returns true if the attempt was successful. May be unsuccessful if:
    ///     no device exists with device id == devId
    ///     freq not in getStreamingFrequences()
    ///     fieldIds contains an invalid id

    /// \brief Tries to stop streaming from the selected device with the given parameters.
    /// Returns true if the attempt was successful. May be unsuccessful if no stream for given id exists
    bool stopStreaming(int devId);

    /// \brief writes a message to the device to set its active fields
    int writeDeviceMap(int devId, const std::vector<int> &fields);
    int writeDeviceMap(int devId, uint32_t* map);

    /// \brief adds a message to a queue of messages to be written to the port periodically
    bool enqueueCommand(uint8_t numb, uint8_t* dataPacket, int portIdx=0);

    /// \brief overloaded to manage streams and connected devices
    virtual void close(uint16_t portIdx);

protected:
    virtual void periodicTask();
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

    virtual int writeDeviceMap(const FlexseaDevice &d, uint32_t* map);

    virtual void serviceStreams(uint8_t milliseconds);
    uint8_t serviceCount = 0;
private:
	//Variables & Objects:
    class Message;
    class CmdSlaveRecord;
	std::queue<Message> outgoingBuffer;

	std::vector<CmdSlaveRecord> autoStreamLists[NUM_TIMER_FREQS];
	std::vector<CmdSlaveRecord> streamLists[NUM_TIMER_FREQS];

    int getIndexOfFrequency(int freq);

    int timerFrequencies[NUM_TIMER_FREQS];
	float timerIntervals[NUM_TIMER_FREQS];
    float msSinceLast[NUM_TIMER_FREQS] = {0};

    void sendCommands(int index);
    void sendAutoStream(int devId, int cmd, int period, bool start);
    void sendSysDataRead(uint8_t slaveId);

    uint8_t streamCount;
};

class CommManager::Message {
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

class CommManager::CmdSlaveRecord
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

#endif // STREAMMANAGER_H
