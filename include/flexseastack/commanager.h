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
#include "flexseastack/flexsea-system/inc/flexsea_sys_def.h"

struct MultiWrapper_struct;
typedef MultiWrapper_struct MultiWrapper;

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
    virtual bool startStreaming(int devId, int freq, bool shouldLog, bool shouldAuto, uint8_t cmdCode=CMD_SYSDATA);

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

    void registerMessageReceivedCounter(int devId, uint16_t *counter) const { messageReceivedCounters.insert({devId, counter}); }
    void deregisterMessageReceivedCounter(int devId) const { messageReceivedCounters.erase(devId); }

    void registerMessageSentCounter(int devId, uint16_t *counter) const { messageSentCounters.insert({devId, counter}); }
    void deregisterMessageSentCounter(int devId) const { messageSentCounters.erase(devId); }

    template<typename T, typename... Args>
    bool enqueueCommand(int devId, T tx_func, Args&&... tx_args) { return enqueueCommand(connectedDevices.at(devId), tx_func, std::forward<Args>(tx_args)...); }

protected:
    virtual void periodicTask();
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

    virtual int writeDeviceMap(const FlexseaDevice &d, uint32_t* map);
    virtual int enqueueMultiPacket(int devId, MultiWrapper *out);

    virtual void serviceStreams(uint8_t milliseconds);
    uint8_t serviceCount = 0;

    template<typename T, typename... Args>
    bool enqueueCommand(const FlexseaDevice &d, T tx_func, Args&&... tx_args);

private:
	//Variables & Objects:
    class Message;
    class CmdSlaveRecord;
	std::queue<Message> outgoingBuffer;
    const unsigned int MAX_Q_SIZE = 200;

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

    mutable std::unordered_map<int, uint16_t*> messageReceivedCounters;
    mutable std::unordered_map<int, uint16_t*> messageSentCounters;
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
