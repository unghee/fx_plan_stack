#ifndef SERIALDRIVER_H
#define SERIALDRIVER_H

#include "serial/serial.h"

#include <vector>
#include <string>
#include <mutex>

class SerialDriver
{
public:
    explicit SerialDriver(int n=4);
    virtual ~SerialDriver();

    virtual std::vector<std::string> getAvailablePorts() const;
    virtual int isOpen(uint16_t portIdx) const;
    virtual void tryClose(uint16_t portIdx);
    virtual void close(uint16_t portIdx) {tryClose(portIdx);}
    virtual void write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx);
    virtual void flush(uint16_t portIdx);
    virtual void clear(uint16_t portIdx);

    std::string getPortName(uint16_t portIdx);
    serial::state_t getPortState(uint16_t portIdx);

protected:
    int numPortsOpen() const {return openPorts;}
    virtual void cleanupPort(int portIdx) {(void)portIdx;}
    virtual bool tryOpen(const std::string &portName, uint16_t portIdx);
    virtual serial::state_t getState(int portIdx) const;
    size_t bytesAvailable(int portIdx) const;
    size_t readPort(int portIdx, uint8_t *buf, uint16_t nb);


private:
    const int _NUMPORTS;
    serial::Serial *ports;
    std::mutex *serialMutexes;
    bool *isPortOpen;

    mutable std::mutex _portCountMutex;
    mutable uint16_t openPorts;

    friend class TestSerial;
};

#endif // SERIALDRIVER_H
