#ifndef STREAMMANAGER_H
#define STREAMMANAGER_H

#include <ctime>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstring>
#include <thread>
#include <chrono>

// #include "flexsea_system.h"
#include "flexseadevicetypes.h"
#include "device.h"

class CommManager
{

public:
    CommManager();
    ~CommManager();

    //returns -1 if cannot connect to device
    int loadAndGetDeviceId(const char* portName, uint16_t portIdx);
    FlexseaDevice* getDevicePtr(int devId);
    /// \brief overloaded to manage streams and connected devices
    int isOpen(int portIdx);
    void closeDevice(uint16_t portIdx);//
    std::vector<int> getDeviceIds();

    bool isValidDevId(int devId);

    std::vector<int> getStreamingFrequencies() const;//
    bool startStreaming(int devId, int freq, bool shouldLog, int shouldAuto, uint8_t cmdCode=CMD_SYSDATA);//
    int startStreaming(int devId, int freq, bool shouldLog, const StreamFunc &streamFunc);//
    bool stopStreaming(int devId, int cmdCode=-1);//
    int writeDeviceMap(int devId, const std::vector<int> &fields);//
    int writeDeviceMap(int devId, uint32_t* map);//


    bool createSessionFolder(std::string sessionName);//
    void setAdditionalColumn(std::vector<std::string> addLabel, std::vector<int> addValue);//
    void setColumnValue(unsigned col, int val);//
    bool setLogFolder(std::string logFolderPath);//
    bool setDefaultLogFolder();//

    /// \brief adds a message to a queue of messages to be written to the port periodically
    template<typename T, typename... Args>
    bool enqueueCommand(int devId, T tx_func, Args&&... tx_args)
    {
        if(!isValidDevId(devId))
            return -1;
        Device* device = deviceMap.at(devId);
        device->enqueueCommand(tx_func, std::forward<Args>(tx_args)...);

        return true;
    }

private:
    std::unordered_map<int, Device*> deviceMap;
    std::unordered_map<uint16_t, Device*> devicePortMap;
    std::vector<int> deviceIds;

};

#endif // STREAMMANAGER_H
