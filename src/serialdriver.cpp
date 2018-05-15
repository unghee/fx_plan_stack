#include "flexseastack/serialdriver.h"
#include "serial/serial.h"
#include <iostream>

SerialDriver::SerialDriver(int n) :
    _NUMPORTS(n)
    , ports(new serial::Serial[_NUMPORTS])
    , openPorts(0)
{}


SerialDriver::~SerialDriver()
{
    std::lock_guard<std::mutex> lk(_portsMutex);
    openPorts = 0;

    for(int i = 0; i <_NUMPORTS; i++)
        if(ports[i].isOpen()) this->tryClose(i);

    delete[] ports;
    ports = nullptr;
}

/* Serial functions */
std::vector<std::string> SerialDriver::getAvailablePorts() const {
    std::vector<std::string> result;

#ifndef TEST_CODE
    std::vector<serial::PortInfo> pi = serial::list_ports();
    for(unsigned int i = 0; i < pi.size(); i++)
        result.push_back(pi.at(i).port);
#endif

    return result;
}

int SerialDriver::isOpen(uint16_t portIdx) const { return ports[portIdx].isOpen(); }

bool SerialDriver::tryOpen(const std::string &portName, uint16_t portIdx) {

    if(portIdx >= _NUMPORTS)
    {
        std::cout << "Can't to open port outside range (" << portIdx << ")" << std::endl;
        return false;
    }

    if(ports[portIdx].isOpen())
    {
        std::cout << "Port " << portIdx << " already open" << std::endl;
        return false;
    }
    serial::Serial *s = ports+portIdx;
    s->setPort(portName);
    s->setBaudrate(400000);
    s->setBytesize(serial::eightbits);
    s->setParity(serial::parity_none);
    s->setStopbits(serial::stopbits_one);
    s->setFlowcontrol(serial::flowcontrol_hardware);

    try { s->open(); } catch (...) {}

    if(s->isOpen())
    {
        std::cout << "Opened port " << portName << " at index " << portIdx << std::endl;
        std::lock_guard<std::mutex> lk(_portsMutex);
        openPorts++;
        return true;
    }

    std::cout << "Failed to open port " << portName << " at index " << portIdx << std::endl;
    return false;
}

void SerialDriver::tryClose(uint16_t portIdx) {
    if(portIdx >= _NUMPORTS)
    {
        std::cout << "Can't close port outside range (" << portIdx << ")" << std::endl;
        return;
    }

    if(ports[portIdx].isOpen())
    {
        std::cout << "Port " << portIdx << " already closed" << std::endl;
        return;
    }

    ports[portIdx].close();
    if(ports[portIdx].isOpen())
    {
        std::cout << "Closed port " << portIdx << "." << std::endl;
        std::lock_guard<std::mutex> lk(_portsMutex);
        openPorts--;
    }
}

void SerialDriver::write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx)
{
    if(portIdx >= _NUMPORTS)
    {
        std::cout << "Can't write to port outside range (" << portIdx << ")" << std::endl;
        return;
    }
    if(!ports[portIdx].isOpen())
    {
        cleanupPort(portIdx);
//        std::cout << "Port " << portIdx << " not open" << std::endl;
//        for(unsigned int i = 0; i < deviceIds.size(); i++)
//        {
//            if(connectedDevices.at(deviceIds.at(i)).port == portIdx)
//                removeDevice(deviceIds.at(i));
//        }
//        return;
    }

    try {
        ports[portIdx].write(serial_tx_data, bytes_to_send);
    } catch (serial::IOException e) {
        std::cout << "IO Exception:  " << e.what() << std::endl;
    } catch (serial::SerialException e) {
        std::cout << "Serial Exception:  " << e.what() << std::endl;
    } catch (serial::PortNotOpenedException e) {
        std::cout << "Port wasn't open" << std::endl;
    }
}

void SerialDriver::flush(uint16_t portIdx)
{
    if(portIdx >= _NUMPORTS) return;
    ports[portIdx].flush();
}
void SerialDriver::clear(uint16_t portIdx)
{
    if(portIdx >= _NUMPORTS) return;
    ports[portIdx].flushInput();
    ports[portIdx].flushOutput();
}
