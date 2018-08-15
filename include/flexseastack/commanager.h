#ifndef STREAMMANAGER_H
#define STREAMMANAGER_H

#include <ctime>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <functional>

#include "flexseaserial.h"
#include "periodictask.h"
#include "flexseastack/flexsea-system/inc/flexsea_sys_def.h"
#include "comm_string_generation.h"
#include "datalogger.h"

struct MultiWrapper_struct;
typedef MultiWrapper_struct MultiWrapper;
typedef std::function<void(uint8_t*, uint8_t*, uint8_t*, uint16_t*)> StreamFunc;

class CommManager : public FlexseaSerial
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
    virtual bool startStreaming(int devId, int freq, bool shouldLog, int shouldAuto, uint8_t cmdCode=CMD_SYSDATA);

    /// \brief Tries to start streaming from the selected device using a custom function to build comm msgs.
    /// Returns true if the the attempt was successful. May be unsuccessful if:
    ///     no device exists with device id == devId
    ///     freq not in getStreamingFrequences()
    int startStreaming(int devId, int freq, bool shouldLog, const StreamFunc &streamFunc);

    /// \brief Tries to start streaming from the selected device with the given parameters.
    /// Streams all fields if fieldIds is empty. Otherwise only streams ids in fieldIds
    /// Returns true if the attempt was successful. May be unsuccessful if:
    ///     no device exists with device id == devId
    ///     freq not in getStreamingFrequences()
    ///     fieldIds contains an invalid id

    /// \brief Tries to stop streaming from the selected device with the given parameters.
    /// Returns true if the attempt was successful. May be unsuccessful if no stream for given id exists
    bool stopStreaming(int devId, int cmdCode=-1);

    /// \brief writes a message to the device to set its active fields
    int writeDeviceMap(int devId, const std::vector<int> &fields);
    int writeDeviceMap(int devId, uint32_t* map);

    /// \brief adds a message to a queue of messages to be written to the port periodically
    bool enqueueCommand(uint8_t numb, uint8_t* dataPacket, int portIdx=0);

    /// \brief overloaded to manage streams and connected devices
    virtual void close(uint16_t portIdx);

    template<typename T, typename... Args>
    bool enqueueCommand(int devId, T tx_func, Args&&... tx_args) { return enqueueCommand(connectedDevices.at(devId), tx_func, std::forward<Args>(tx_args)...); }

protected:
    virtual void periodicTask();
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

    virtual int writeDeviceMap(const FxDevicePtr d, uint32_t* map);
    int enqueueMultiPacket(int devId, MultiWrapper *out);
    int enqueueMultiPacket(int devId, int port, MultiWrapper *out);

    virtual void serviceStreams(uint8_t milliseconds);
    uint8_t serviceCount = 0;

    template<typename T, typename... Args>
    bool enqueueCommand(const FxDevicePtr d, T tx_func, Args&&... tx_args)
    {
        if(!d->isValid()) return false;
        MultiWrapper *out = &(portPeriphs[d->port].out);

        bool error = CommStringGeneration::generateCommString(d->getShortId(), out,
                                                       tx_func,
                                                       std::forward<Args>(tx_args)...);
        if(error)
        {
//            std::cout << "Error packing multipacket" << std::endl;
            return false;
        }

        return !enqueueMultiPacket(d->id, d->port, out);
    }


private:
	//Variables & Objects:
    class Message;
    struct StreamRcd;
    typedef std::vector<StreamRcd> StreamList;

    std::queue<Message> outgoingBuffer[FX_NUMPORTS];
    const unsigned int MAX_Q_SIZE = 200;

    StreamList autoStreamLists[NUM_TIMER_FREQS];
    StreamList streamLists[NUM_TIMER_FREQS];

    int getIndexOfFrequency(int freq);

    int timerFrequencies[NUM_TIMER_FREQS];
	float timerIntervals[NUM_TIMER_FREQS];
    float msSinceLast[NUM_TIMER_FREQS] = {0};

    void sendCommands(int index);
    void sendAutoStream(int devId, int cmd, int period, bool start);
    void sendSysDataRead(int slaveId);

    int streamCount;
    static const int CMD_CODE_BASE = 256;

    DataLogger *dataLogger;
};

class CommManager::Message {
public:
    static void do_delete(uint8_t buf[]) { delete[] buf; }

    Message(uint8_t nb, uint8_t* data):
    numBytes(nb)
    , dataPacket(std::shared_ptr<uint8_t>(new uint8_t[nb], do_delete))
    {
        uint8_t* temp = dataPacket.get();
        for(int i = 0; i < numBytes; i++)
            temp[i] = data[i];
    }

    uint8_t numBytes;
    std::shared_ptr<uint8_t> dataPacket;
};

struct CommManager::StreamRcd {

    StreamRcd(int id=-1, int cc=-1, bool sl=false, StreamFunc* fc=nullptr) : devId(id), cmdCode(cc), shouldLog(sl), func(fc) {}

    int devId;
    int cmdCode;
    bool shouldLog;
    StreamFunc* func;

};


#endif // STREAMMANAGER_H
