#include "flexseastack/serialdriver.h"

#include <iostream>

#define CHECK_PORTIDX(idx) do { if(idx >= _NUMPORTS) throw std::out_of_range("Port Index outside of range"); } while(0)
#define LOCK_MTX(idx) std::lock_guard<std::mutex> lk(serialMutexes[idx])

SerialDriver::SerialDriver(int n) :
    _NUMPORTS(n)
    , ports(new serial::Serial[n])
    , serialMutexes(new std::mutex[n])
    , isPortOpen(new bool[n])
    , openPorts(0)
{
    memset(isPortOpen, 0, sizeof(bool)*n);
}

SerialDriver::~SerialDriver()
{
    openPorts = 0;

    for(int i = 0; i <_NUMPORTS; i++)
        if(ports[i].isOpen()) this->tryClose(i);

    delete[] ports;
    ports = nullptr;
    delete[] serialMutexes;
    serialMutexes = nullptr;
}

/* Serial functions */
std::vector<std::string> SerialDriver::getAvailablePorts() const
{
    std::vector<std::string> result;

    std::vector<serial::PortInfo> pi = serial::list_ports();
    for(unsigned int i = 0; i < pi.size(); i++)
        result.push_back(pi.at(i).port);

    return result;
}

int SerialDriver::isOpen(uint16_t portIdx) const {
    CHECK_PORTIDX(portIdx);
    LOCK_MTX(portIdx);

    if(!isPortOpen[portIdx] && ports[portIdx].isOpen())
    {
        std::lock_guard<std::mutex> lk(_portCountMutex);
        openPorts++;
    }

    return ports[portIdx].isOpen();
}

bool SerialDriver::tryOpen(const std::string &portName, uint16_t portIdx) {
    CHECK_PORTIDX(portIdx);
    LOCK_MTX(portIdx);

    if(!ports[portIdx].isOpen())
    {
        serial::Serial *s = ports+portIdx;
        s->setPort(portName);
        s->setBaudrate(400000);
        s->setBytesize(serial::eightbits);
        s->setParity(serial::parity_none);
        s->setStopbits(serial::stopbits_one);
        s->setFlowcontrol(serial::flowcontrol_hardware);

//#ifdef __WIN32
#if defined(__WIN32) || defined(__WIN64)
        try { s->openAsync(); } catch (...) {}
#else
        try { s->open(); } catch (...) {}
#endif

    }

    bool isOpen = ports[portIdx].isOpen();
    if(!isPortOpen[portIdx] && isOpen)
    {
        std::lock_guard<std::mutex> lk(_portCountMutex);
        openPorts++;
        std::cout << "Port " << portIdx << " opened" << std::endl;
    }

    return isOpen;
}

serial::state_t SerialDriver::getPortState(int portIdx) const
{
    CHECK_PORTIDX(portIdx);
    LOCK_MTX(portIdx);

    if(!isPortOpen[portIdx] && ports[portIdx].isOpen())
    {
        std::lock_guard<std::mutex> lk(_portCountMutex);
        openPorts++;
    }

    return ports[portIdx].getState();
}

size_t SerialDriver::bytesAvailable(int portIdx) const
{
    CHECK_PORTIDX(portIdx);

    bool serialError = false;

    {
        LOCK_MTX(portIdx);
        try
        {
            if(ports[portIdx].isOpen())
                return ports[portIdx].available();
        }
        catch (...)
        {
            // we expect to hit this block if we turn off a physical device
            // before disconnecting the port
            serialError = true;
        }
    }

    if(serialError)
    {
        // calling a non const function from a const context - spooooky
        // semantically this function is still const in that if the serial port errors on calling available
        // it is effectively already "closed"
        // here we just do the book keeping...
        ((SerialDriver*)this)->tryClose(portIdx);   // should update state so that isOpen(...) returns false
        ((SerialDriver*)this)->close(portIdx);      // allow derived classes to handle port closing stuff
    }

    return 0;
}

size_t SerialDriver::readPort(int portIdx, uint8_t *buf, uint16_t nb)
{
    CHECK_PORTIDX(portIdx);
    LOCK_MTX(portIdx);

    if(ports[portIdx].isOpen())
        return ports[portIdx].read(buf, nb);

    return 0;
}

void SerialDriver::tryClose(uint16_t portIdx) {
    CHECK_PORTIDX(portIdx);
    LOCK_MTX(portIdx);

    if(ports[portIdx].isOpen())
        ports[portIdx].close();

    if(!ports[portIdx].isOpen() && isPortOpen[portIdx])
    {
        std::cout << "Closed port " << portIdx << "." << std::endl;
        std::lock_guard<std::mutex> lk(_portCountMutex);
        openPorts--;
    }
}

void SerialDriver::write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx)
{
    CHECK_PORTIDX(portIdx);
    LOCK_MTX(portIdx);

    bool success = false;

    if(ports[portIdx].isOpen())
    {
        try {
            ports[portIdx].write(serial_tx_data, bytes_to_send);
            success = true;
        } catch (serial::IOException e) {
            std::cout << "IO Exception:  " << e.what() << std::endl;
        } catch (serial::SerialException e) {
            std::cout << "Serial Exception:  " << e.what() << std::endl;
        }
    }

    if(!success && ports[portIdx].isOpen())
        ports[portIdx].close();
}

void SerialDriver::flush(uint16_t portIdx)
{
    CHECK_PORTIDX(portIdx);
    ports[portIdx].flush();
}
void SerialDriver::clear(uint16_t portIdx)
{
    CHECK_PORTIDX(portIdx);
    ports[portIdx].flushInput();
    ports[portIdx].flushOutput();
}

std::string SerialDriver::getPortName(uint16_t portIdx) const
{
    CHECK_PORTIDX(portIdx);
    LOCK_MTX(portIdx);
    if(ports[portIdx].isOpen())
        return ports[portIdx].getPort();

    return "";
}
