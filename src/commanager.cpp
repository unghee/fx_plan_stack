#include "flexseastack/commanager.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <cstring>
#include "flexseastack/comm_string_generation.h"

extern "C" {
    #include "flexseastack/flexsea-system/inc/flexsea_cmd_sysdata.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"
}
#include "flexseastack/flexsea-system/inc/flexsea_system.h"

namespace csg = CommStringGeneration;

CommManager::CommManager() : PeriodicTask(), FlexseaSerial()
{
    //this needs to be in order from smallest to largest
    int timerFreqsInHz[NUM_TIMER_FREQS] = {1, 5, 10, 20, 33, 50, 100, 200, 300, 500, 1000};
    for(int i = 0; i < NUM_TIMER_FREQS; i++)
    {
        timerFrequencies[i] = timerFreqsInHz[i];
        timerIntervals[i] = 1000.0f / timerFreqsInHz[i];
        autoStreamLists[i] = StreamList();
        streamLists[i] = StreamList();
    }

    streamCount = 0;
}

CommManager::~CommManager(){}

int CommManager::getIndexOfFrequency(int freq)
{
    int indexOfFreq = -1;
    for(int i = 0; i < NUM_TIMER_FREQS; i++)
    {
        if(freq == timerFrequencies[i])
        {
            indexOfFreq = i;
            i = NUM_TIMER_FREQS;
        }
    }
    return indexOfFreq;
}

std::vector<int> CommManager::getStreamingFrequencies() const
{
    std::vector<int> r;
    r.resize(NUM_TIMER_FREQS);
    int *d = r.data();
    memcpy(d, timerFrequencies, sizeof(int) * NUM_TIMER_FREQS);
    return r;
}

bool CommManager::startStreaming(int devId, int freq, bool shouldLog, int shouldAuto, uint8_t cmdCode)
{
    int indexOfFreq = getIndexOfFrequency(freq);

    if(indexOfFreq < 0 || indexOfFreq >= NUM_TIMER_FREQS)
    {
        std::cout << "Invalid frequency" << std::endl;
        return false;
    }

    const FlexseaDevice &d = this->getDevice(devId);
    if(!d.isValid())
    {
        std::cout << "Invalid device id" << std::endl;
        return false;
    }

    std::cout << "Started " << (shouldLog ? " logged " : "") << (shouldAuto ? "auto" : "") << "streaming cmd: " << (int)cmdCode << ", for slave id: " << devId << " at frequency: " << freq << std::endl;
    if(shouldAuto)
    {
        sendAutoStream(devId, cmdCode, 1000 / freq, true);
        autoStreamLists[indexOfFreq].emplace_back(devId, (int)cmdCode, nullptr);
    }
    else
        streamLists[indexOfFreq].emplace_back(devId, (int)cmdCode, nullptr);

    // increase stream count only for regular streaming
    bool doNotify = false;
    if(!shouldAuto)
    {
        std::lock_guard<std::mutex> l(conditionMutex);
        streamCount++;
        doNotify = streamCount == 1;
    }
    // if stream count is exactly one, then we need to wake our sleeping worker thread
    if(doNotify)
        wakeCV.notify_all();

    return true;
}

bool CommManager::startStreaming(int devId, int freq, bool shouldLog, const StreamFunc &streamFunc)
{
    int idx = getIndexOfFrequency(freq);
    if(idx < 0 && !connectedDevices.count(devId))
        return false;
    std::cout << "Started " << (shouldLog ? " logged " : "") << "streaming cmd: custom for slave id: " << devId << " at frequency: " << freq << std::endl;
    streamLists[idx].emplace_back(devId, -1, new StreamFunc(streamFunc));
    return true;
}

bool CommManager::stopStreaming(int devId)
{
    StreamList* listArray[2] = {autoStreamLists, streamLists};

    bool found = false;

    for(int listIndex = 0; listIndex < 2; listIndex++)
    {
        StreamList *l = listArray[listIndex];

        for(int indexOfFreq = 0; indexOfFreq < NUM_TIMER_FREQS; indexOfFreq++)
        {
            for(unsigned int i = 0; i < (l)[indexOfFreq].size(); /* no increment */)
            {
                auto record = (l)[indexOfFreq].at(i);
                if(record.devId == devId)
                {
                    if(record.func) delete record.func;
                    (l)[indexOfFreq].erase( (l)[indexOfFreq].begin() + i);

                    if(listIndex == 0)
                    {
                        sendAutoStream(devId, record.cmdCode, 1000 / timerFrequencies[indexOfFreq], false);
                    }
                    else
                    {
                        std::lock_guard<std::mutex> l(conditionMutex);
                        streamCount--;
                    }

                        std::cout << "Stopped " << (listIndex == 0 ? "autostreaming" : "streaming");
                        if(record.cmdCode > 0) std::cout << " cmd: " << record.cmdCode;
                        else std::cout << " cmd: custom";

                        std::cout << ", for slave id: " << devId
                                  << " at frequency: " << timerFrequencies[indexOfFreq] << std::endl;

                        found = true;
                }
                else    //only increment i if we aren't erasing from the vector
                    i++;
            }
        }
    }

    return found;
}

void CommManager::periodicTask()
{
    serviceStreams(taskPeriod);
    serviceOpenAttempts(taskPeriod);

    if(!serviceCount)
        serviceOpenPorts();

    serviceCount++;
    if(serviceCount >= 4)
        serviceCount = 0;
}

void CommManager::serviceStreams(uint8_t milliseconds)
{
    const float TOLERANCE = 0.0001;
    int i;

    for(i = 0; i < NUM_TIMER_FREQS; i++)
    {
        if(!streamLists[i].size()) continue;

        //received clocks comes in at 5ms/clock
        msSinceLast[i] += milliseconds;

        float timerInterval = timerIntervals[i];
        if((msSinceLast[i] + TOLERANCE) > timerInterval)
        {
            sendCommands(i);

            while((msSinceLast[i] + TOLERANCE) > timerInterval)
                msSinceLast[i] -= timerInterval;
        }
    }

    if(outgoingBuffer.size())
    {
        Message m = outgoingBuffer.front();
        outgoingBuffer.pop();
        this->write(m.numBytes, m.dataPacket.get(), 0);
    }
}

bool CommManager::wakeFromLongSleep()
{
    return FlexseaSerial::wakeFromLongSleep() || (this->outgoingBuffer.size() || this->streamCount > 0);
}
bool CommManager::goToLongSleep()
{
    return streamCount == 0 && FlexseaSerial::goToLongSleep();
}

void CommManager::close(uint16_t portIdx)
{
    for(auto &kvp : connectedDevices)
    {
        if(kvp.second.port == portIdx)
            stopStreaming(kvp.second.id);
    }

    FlexseaSerial::close(portIdx);
}

int CommManager::writeDeviceMap(const FlexseaDevice &d, uint32_t *map)
{
    uint16_t mapLen = 0;
    for(short i=FX_BITMAP_WIDTH-1; i >= 0; i--)
    {
        if(map[i] > 0)
        {
            mapLen = i+1;
            break;
        }
    }

    mapLen = mapLen > 0 ? mapLen : 1;

    enqueueCommand(d,
                   tx_cmd_sysdata_w,
                   map, mapLen
                   );

    return 0;
}

int CommManager::writeDeviceMap(int devId, uint32_t *map)
{
    if(!connectedDevices.count(devId)) return -1;
    FlexseaDevice &d = connectedDevices.at(devId);
    return writeDeviceMap(d, map);
}

int CommManager::writeDeviceMap(int devId, const std::vector<int> &fields)
{
    if(!connectedDevices.count(devId)) return -1;
    FlexseaDevice &d = connectedDevices.at(devId);

    int nf = d.numFields;
    uint32_t map[FX_BITMAP_WIDTH];
    memset(map, 0, sizeof(uint32_t)*FX_BITMAP_WIDTH);

    for(auto&& f : fields)
    {
        if(f < nf)
        {
            SET_FIELD_HIGH(f, map);
        }
    }

    return writeDeviceMap(d, map);
}

int CommManager::enqueueMultiPacket(int devId, MultiWrapper *out)
{
    if(!connectedDevices.count(devId)) return -1;
    FlexseaDevice &d = connectedDevices.at(devId);

    uint8_t frameId = 0;
    while(out->frameMap > 0)
    {
        outgoingBuffer.push(Message(
                                SIZE_OF_MULTIFRAME(out->packed[frameId])
                                ,out->packed[frameId]  , d.port  ));

        out->frameMap &= (   ~(1 << frameId)   );
        frameId++;
    }

    out->isMultiComplete = 1;

    while(outgoingBuffer.size() > MAX_Q_SIZE)
        outgoingBuffer.pop();

    return 0;
}

bool CommManager::enqueueCommand(uint8_t numb, uint8_t* dataPacket, int portIdx)
{
    //If we are over a max size, clear the queue
    if(outgoingBuffer.size() > MAX_Q_SIZE)
    {
        std::cout << "ComManager::enqueueCommand, queue is above max size (" << MAX_Q_SIZE  << "), clearing queue..." << std::endl;
        while(outgoingBuffer.size())
            outgoingBuffer.pop();
    }

    outgoingBuffer.push(Message(numb, dataPacket, portIdx));

    bool doNotify;
    {
        std::lock_guard<std::mutex> lk(conditionMutex);
        doNotify = streamCount < 1;
    }
    if(doNotify)
        wakeCV.notify_all();

    return true;
}

template<typename T, typename... Args>
bool CommManager::enqueueCommand(const FlexseaDevice &d, T tx_func, Args&&... tx_args)
{
    if(!d.isValid()) return false;
    MultiWrapper *out = &(portPeriphs[d.port].out);

    bool error = csg::generateCommString(d.id, out,
                                                   tx_func,
                                                   std::forward<Args>(tx_args)...);
    if(error)
    {
        std::cout << "Error packing multipacket" << std::endl;
        return false;
    }

    return !enqueueMultiPacket(d.id, out);
}

void CommManager::sendCommands(int index)
{
    if(index < 0 || index >= NUM_TIMER_FREQS) return;
    StreamFunc *f;
    for(unsigned int i = 0; i < streamLists[index].size(); i++)
    {
        auto& record = streamLists[index].at(i);
        switch(record.cmdCode)
        {
        case -1:
            f = record.func;
            if(f) enqueueCommand(record.devId, *f);
            break;

        case CMD_SYSDATA:
            sendSysDataRead(record.devId);
            break;

            default:
                std::cout<< "Unsupported command was given: " << record.cmdCode << std::endl;
                //stopStreaming(record.cmdType, record.slaveIndex, timerFrequencies[index]);
                break;
        }
    }
}

void CommManager::sendAutoStream(int devId, int cmd, int period, bool start)
{
    enqueueCommand(devId,
                   tx_cmd_stream_w,
                   cmd, period, start, 0, 0);
}

void CommManager::sendSysDataRead(uint8_t slaveId)
{
    enqueueCommand(slaveId,
                   tx_cmd_sysdata_r,
                   nullptr, 0);
}

