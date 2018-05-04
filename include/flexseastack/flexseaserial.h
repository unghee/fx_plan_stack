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

struct MultiCommPeriph_struct;
typedef MultiCommPeriph_struct MultiCommPeriph;

namespace serial {
    class Serial;
}

#ifndef TEST_CODE
#endif

#define FX_NUMPORTS 4

//USB driver:
#define CHUNK_SIZE				48
#define MAX_SERIAL_RX_LEN		(CHUNK_SIZE*15 + 10)

typedef circular_buffer<FX_DataPtr> FX_DataList;

/// \brief FlexseaSerial class manages serial ports and connected devices
class FlexseaSerial : public PeriodicTask
{
public:
    FlexseaSerial();
    virtual ~FlexseaSerial();

    /* Returns a vector containing the ids of all connected devices
    */
    virtual const std::vector<int>& getDeviceIds() const;

    /* Returns a FlexseaDevice object which provides an interface to incoming data
     * from a connected device
    */
    virtual const FlexseaDevice& getDevice(int id) const;

    /* Returns a bitmap indicating which fields are active for the device with specified id
     *
     * FX_Bitmap is a uint32_t[3],
     * so if (0x01 & active()[0]) then field 0 is active
     *    if (0x01 & active()[1]) then field 32 is active
     *
     * or in general if (1 << x) & active()[y] then field 32*y+x is active
    */
    virtual const uint32_t* getMap(int id) const;

    /* These functions allow users to be notified of the corresponding events
    */
    void registerConnectionChangeFlag(uint8_t *flag) const;
    void registerMapChangeFlag(uint8_t *flag) const;

    void unregisterConnectionChangeFlag(uint8_t *flag) const;
    void unregisterMapChangeFlag(uint8_t *flag) const;

    /* Serial functions */
    virtual std::vector<std::string> getPortList() const;
    virtual void open(const std::string &portName, uint16_t portIdx=0);
    virtual int isOpen(uint16_t portIdx=0) const;
    virtual void close(uint16_t portIdx=0);
    virtual void write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx=0);
    virtual void write(uint8_t bytes_to_send, uint8_t *serial_tx_data, const FlexseaDevice &d) {this->write(bytes_to_send, serial_tx_data, d.port);}
    virtual void tryReadWrite(uint8_t bytes_to_send, uint8_t *serial_tx_data, int timeout, uint16_t portIdx=0);
    virtual void flush(uint16_t portIdx=0){(void)portIdx;}
    virtual void clear(uint16_t portIdx=0){(void)portIdx;}

    void sendDeviceWhoAmI(int port);
    virtual void setDeviceMap(const FlexseaDevice &d, uint32_t* map);

#ifndef TEST_CODE
#endif
MultiCommPeriph *portPeriphs;

protected:
    std::vector<int> deviceIds;
    std::unordered_map<int, FlexseaDevice> connectedDevices;
    std::unordered_map<int, uint32_t*> fieldMaps;
    std::unordered_map<int, circular_buffer<FX_DataPtr>*> databuffers;

    const FlexseaDevice defaultDevice;

    void notifyConnectionChange();
    void notifyMapChange();

    int addDevice(int id, int port, FlexseaDeviceType type);
    int removeDevice(int id);

    virtual void periodicTask();
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

    uint8_t largeRxBuffer[MAX_SERIAL_RX_LEN];
    void processReceivedData(int port, size_t nb);

private:
    mutable std::vector<uint8_t*> deviceConnectedFlags;
    mutable std::vector<uint8_t*> mapChangedFlags;

    mutable std::mutex connectionMutex;
    mutable std::mutex mapMutex;

    serial::Serial *ports;

    uint16_t openPorts;

    void handleFlexseaDevice(int port);
};

#endif // FLEXSEASERIAL_H
