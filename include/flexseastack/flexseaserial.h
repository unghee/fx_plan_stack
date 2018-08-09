#ifndef FLEXSEASERIAL_H
#define FLEXSEASERIAL_H

#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <algorithm>
#include <functional>
#include <atomic>

#include "flexseastack/rxhandler.h"
#include "flexseadevicetypes.h"
#include "flexseadevice.h"
#include "circular_buffer.h"
#include "periodictask.h"
#include "flexseastack/serialdriver.h"
#include "flexseastack/flexseadeviceprovider.h"

struct MultiCommPeriph_struct;
typedef MultiCommPeriph_struct MultiCommPeriph;

namespace serial {
    class Serial;
}

class OpenAttempt;
typedef std::vector<OpenAttempt> OpenAttemptList;

#define FX_NUMPORTS 4

//USB driver:
#define CHUNK_SIZE				48
#define MAX_SERIAL_RX_LEN		(CHUNK_SIZE*15 + 10)


/// \brief FlexseaSerial class manages serial ports and connected devices
class FlexseaSerial : public PeriodicTask, public SerialDriver, public FlexseaDeviceProvider, public RxHandlerManager
{
public:
    FlexseaSerial();
    virtual ~FlexseaSerial();

    /// \brief opens portName at portIdx
    /// Starts an open attempt at the corresponding port. Later polls for the state of the port
    /// If the port opens successfully, FlexseaSerial periodically sends whoami messages until metadata is received
    void open(std::string portName, int portIdx);

    /// \brief DEPRECATED: sends a who am i (who are you really?) message at the given port
    /// You should never need to call this function explicitly, under the hood FlexseaSerial handles it for you
    /// --
    /// When a FlexSEA device receives a who am i message, it responds with metadata describing itself
    /// metadata includes device id, device type, device role, and currently active fields
    virtual void sendDeviceWhoAmI(int port);

    /// \brief [Blocking] write to the device specified by the device handle d
    /// GUI should prefer non blocking writes
    /// for a non blocking write, use CommManager::enqueueCommand
    virtual void writeDevice(uint8_t bytes_to_send, uint8_t *serial_tx_data, const FlexseaDevice &d);

    /// \brief close the corresponding port
    virtual void close(uint16_t portIdx);

protected:
    /// \brief see class PeriodicTask for more info
    virtual void periodicTask();
    /// \brief see class PeriodicTask for more info
    virtual bool wakeFromLongSleep();
    /// \brief see class PeriodicTask for more info
    virtual bool goToLongSleep();

    /// \brief checks any ports that currently have open attempts
    void serviceOpenAttempts(uint8_t delayed);

    /// \brief checks any ports that currently open, receives data if any bytes are available
    virtual void serviceOpenPorts();

    /// \brief processes nb bytes at the port, analyses for packets, parses, etc
    void processReceivedData(int port, size_t nb);

    MultiCommPeriph *portPeriphs;
    std::atomic<int> devicesAtPort[FX_NUMPORTS];

private:
    int sysDataParser(int port);
    inline int updateDeviceMetadata(int port, uint8_t *buf);
    inline int updateDeviceData(uint8_t *buf);

    // open attempts needs serialization.
    // It is written to from the control thread, read from the worker thread
    OpenAttemptList openAttempts;
    std::mutex openAttemptMut_;
    std::atomic<int> haveOpenAttempts;
    uint8_t largeRxBuffer[MAX_SERIAL_RX_LEN];
};

class OpenAttempt {
public:
    explicit OpenAttempt(int portIdx_, std::string portName_, int tries_, int tryMax_, int delay_, int delayed_) :
        portIdx(portIdx_), portName(portName_), tries(tries_), tryMax(tryMax_), delay(delay_), delayed(delayed_), markedToRemove(false) {}

    int portIdx;
    std::string portName;
    int tries, tryMax, delay, delayed;
    bool markedToRemove;
};

#endif // FLEXSEASERIAL_H
