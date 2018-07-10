#ifndef SERIALDRIVER_H
#define SERIALDRIVER_H

#include <vector>
#include <string>
#include <mutex>

namespace serial {
    class Serial;
}

class SerialDriver
{
public:
    explicit SerialDriver(int n=4);
    virtual ~SerialDriver();

    virtual std::vector<std::string> getAvailablePorts() const;
    virtual int isOpen(uint16_t portIdx=0) const;
    virtual void tryClose(uint16_t portIdx=0);
    virtual void close(uint16_t portIdx=0) {tryClose(portIdx);}
    virtual void write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx);
    virtual void flush(uint16_t portIdx=0);
    virtual void clear(uint16_t portIdx=0);

    std::string getPortName(uint16_t portIdx);

protected:
    int numPortsOpen() const;
    virtual void cleanupPort(int portIdx) {(void)portIdx;}
    virtual bool tryOpen(const std::string &portName, uint16_t portIdx=0);

    const int _NUMPORTS;
    serial::Serial *ports;
    std::mutex _portsMutex;
    uint16_t openPorts;
};

#endif // SERIALDRIVER_H
