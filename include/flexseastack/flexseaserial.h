#ifndef FLEXSEASERIAL_H
#define FLEXSEASERIAL_H

#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <algorithm>
#include <functional>
#include <atomic>

#include "rxhandler.h"
#include "flexsea_comm_multi.h"
#include "flexseadevicetypes.h"
#include "flexseadevice.h"
#include "circular_buffer.h"
#include "serialdriver.h"
#include "flexseadeviceprovider.h"

struct MultiCommPeriph_struct;
typedef MultiCommPeriph_struct MultiCommPeriph;

namespace serial {
    class Serial;
}

class OpenAttempt;
typedef std::vector<OpenAttempt> OpenAttemptList;

#define FX_NUMPORTS 4

//USB driver:
#define CHUNK_SIZE              48
#define MAX_SERIAL_RX_LEN       (CHUNK_SIZE*15 + 10)

/// \brief FlexseaSerial class manages serial ports and connected devices
class FlexseaSerial : public SerialDriver, public RxHandlerManager
{
public:
    class Message;

    FlexseaSerial();
    virtual ~FlexseaSerial();

    /// \brief opens portName at portIdx
    /// Starts an open attempt at the corresponding port. Later polls for the state of the port
    /// If the port opens successfully, FlexseaSerial periodically sends whoami messages until metadata is received
    void open(std::string portName, uint16_t portIdx);

    /// \brief close the corresponding port
    virtual void close(uint16_t portIdx);
    void readDevice(std::string portName, uint16_t portIdx);

    // / \brief processes nb bytes at the port, analyses for packets, parses, etc
    void processReceivedData(uint8_t* largeRxBuffer, size_t len, int port, int portIdx, FlexseaDevice* serialDevice);

    template<typename T, typename... Args>
    std::vector<Message> generateMessages(int devId, FlexseaDevice serialDevice, T tx_func, Args&&... tx_args);

    /// \brief DEPRECATED: sends a who am i (who are you really?) message at the given port
    /// You should never need to call this function explicitly, under the hood FlexseaSerial handles it for you
    /// --
    /// When a FlexSEA device receives a who am i message, it responds with metadata describing itself
    /// metadata includes device id, device type, device role, and currently active fields
    virtual void sendDeviceWhoAmI(int port);

    /// \brief [Blocking] write to the device specified by the device handle d
    /// GUI should prefer non blocking writes
    /// for a non blocking write, use CommManager::enqueueCommand
    virtual void write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx);

    class Message {
    public:
        static void do_delete(uint8_t buf[]) { delete[] buf; }

        Message(uint8_t numberOfBytes, uint8_t* data): numBytes(numberOfBytes), 
                                            dataPacket(std::shared_ptr<uint8_t>(new uint8_t[numberOfBytes], 
                                            do_delete)){
            uint8_t* temp = dataPacket.get();
            for(int i = 0; i < numBytes; i++)
                temp[i] = data[i];
        }

        uint8_t numBytes;
        std::shared_ptr<uint8_t> dataPacket;
    };

private:
    MultiCommPeriph multiCommPeriph[FX_NUMPORTS];
};


#endif // FLEXSEASERIAL_H
