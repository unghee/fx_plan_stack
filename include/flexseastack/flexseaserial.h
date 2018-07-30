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

    void open(std::string, int portIdx);
    void openCancelRequest(int portIdx);

    virtual void sendDeviceWhoAmI(int port);
    virtual void writeDevice(uint8_t bytes_to_send, uint8_t *serial_tx_data, const FlexseaDevice &d);
    virtual void close(uint16_t portIdx);

    MultiCommPeriph *portPeriphs;

protected:
    virtual void periodicTask();
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

    void serviceOpenAttempts(uint8_t delayed);

    void processReceivedData(int port, size_t nb);
    virtual void serviceOpenPorts();

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
