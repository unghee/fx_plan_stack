#ifndef FLEXSEASERIAL_H
#define FLEXSEASERIAL_H

#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include "flexseadevicetypes.h"
#include "flexseadevice.h"
#include "circular_buffer.h"
#include "periodictask.h"
#include <algorithm>
#include "flexseastack/serialdriver.h"
#include "flexseastack/flexseadeviceprovider.h"

struct MultiCommPeriph_struct;
typedef MultiCommPeriph_struct MultiCommPeriph;

namespace serial {
    class Serial;
}

#define FX_NUMPORTS 4

//USB driver:
#define CHUNK_SIZE				48
#define MAX_SERIAL_RX_LEN		(CHUNK_SIZE*15 + 10)

typedef circular_buffer<FX_DataPtr> FX_DataList;

class OpenAttempt;
typedef std::vector<OpenAttempt> OpenAttemptList;

/// \brief FlexseaSerial class manages serial ports and connected devices
class FlexseaSerial : virtual public PeriodicTask, public SerialDriver, public FlexseaDeviceProvider
{
public:
    FlexseaSerial();
    virtual ~FlexseaSerial();

    virtual void sendDeviceWhoAmI(int port);
    virtual void setDeviceMap(const FlexseaDevice &d, uint32_t* map);
    virtual void setDeviceMap(const FlexseaDevice &d, const std::vector<int> &fields);

    virtual void open(std::string, int portIdx);
    virtual void openCancelRequest(int portIdx);
    virtual void write(uint8_t bytes_to_send, uint8_t *serial_tx_data, const FlexseaDevice &d) {this->write(bytes_to_send, serial_tx_data, d.port);}
    virtual void close(uint16_t portIdx);

    MultiCommPeriph *portPeriphs;

protected:
    virtual void periodicTask();
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

    OpenAttemptList openAttempts;
    bool haveOpenAttempts = false;
    std::mutex openAttemptMut_;
    void serviceOpenAttempts(uint8_t delayed);

    uint8_t largeRxBuffer[MAX_SERIAL_RX_LEN];
    void processReceivedData(int port, size_t nb);
    virtual void serviceOpenPorts();

private:
    // multi comm periph string parsing stuff
    static const int NUM_COMMANDS = 256;
    uint8_t highjackedCmds[NUM_COMMANDS];
    typedef int (FlexseaSerial::*_cpParser)(int port);
    _cpParser stringParsers[NUM_COMMANDS];

    int defaultStringParser(int port) {(void) port; return 1; }
    int sysDataParser(int port);
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
