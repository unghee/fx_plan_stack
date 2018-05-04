#include <iostream>
#include <stdio.h>
#include <algorithm>

#include "flexseastack/flexseaserial.h"
#include "serial/serial.h"
#include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"

#ifndef TEST_CODE

extern "C" {
    #include "flexseastack/flexsea-system/inc/flexsea_device_spec.h"
    #include "flexseastack/flexsea-system/inc/flexsea_cmd_sysdata.h"
    #include "flexseastack/flexsea-system/inc/flexsea_sys_def.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_payload.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"
    #include "flexseastack/flexsea-system/inc/flexsea_system.h"
}

#else
    #include "fake_flexsea.h"
#endif

FlexseaSerial::FlexseaSerial() : defaultDevice(-1, -1, FX_NONE, NULL, NULL), openPorts(0)
{
    ports = new serial::Serial[FX_NUMPORTS];
    portPeriphs = new MultiCommPeriph[FX_NUMPORTS];

    initializeDeviceSpecs();

#ifndef TEST_CODE

    circularBuffer_t * cbs[FX_NUMPORTS] = {&rx_buf_circ_1, &rx_buf_circ_2, &rx_buf_circ_3, &rx_buf_circ_4};
    for(int i = 0; i < FX_NUMPORTS; i++)
        initMultiPeriph(this->portPeriphs + i, PORT_USB, SLAVE, cbs[i]);

#endif //TEST_CODE
}

FlexseaSerial::~FlexseaSerial()
{
    delete[] ports;
    delete[] portPeriphs;


    for(std::unordered_map<int, uint32_t*>::iterator it = fieldMaps.begin(); it != fieldMaps.end(); ++it)
    {
        if(it->second)
            delete[] (it->second);

        (it->second) = nullptr;
    }
    fieldMaps.clear();
}

void FlexseaSerial::processReceivedData(int port, size_t len)
{
#ifndef TEST_CODE
    int numMessagesReceived = 0;
    int numMessagesExpected = (len / COMM_STR_BUF_LEN);
    int maxMessagesExpected = (len / COMM_STR_BUF_LEN + (len % COMM_STR_BUF_LEN != 0));

    uint16_t bytesToWrite, cbSpace, bytesWritten=0;
    int error, successfulParse;

    while(len > 0)
    {
        cbSpace = CB_BUF_LEN - circ_buff_get_size(portPeriphs[port].circularBuff);
        bytesToWrite = MIN(len, cbSpace);

        error = circ_buff_write(portPeriphs[port].circularBuff, (&largeRxBuffer[bytesWritten]), (bytesToWrite));
        if(error) std::cout << "circ_buff_write error:" << error << std::endl;

        do {
            portPeriphs[port].bytesReadyFlag = 1;

            successfulParse = tryParse(portPeriphs + port);
            if(portPeriphs[port].in.isMultiComplete)
            {
                uint8_t parseResult = parseReadyMultiString(portPeriphs + port);
                numMessagesReceived++;
                (void) parseResult;
            }

        } while(successfulParse && numMessagesReceived < maxMessagesExpected);

        len -= bytesToWrite;
        bytesWritten += bytesToWrite;

        if(CB_BUF_LEN == circ_buff_get_size(portPeriphs[port].circularBuff) && len)
        {
            std::cout << "circ buffer is full with non valid frames; clearing..." << std::endl;
            circ_buff_init(portPeriphs[port].circularBuff);
        }
    }

#else
    //throw base::NotImplementedException();
    (void) len;
    (void) port;
#endif
}

void FlexseaSerial::periodicTask()
{
#ifndef TEST_CODE
    size_t i, nb, nr;

    for(i = 0; i < FX_NUMPORTS; i++)
    {
        if(ports[i].isOpen())
        {
            try {
                nb = ports[i].available();
            } catch (serial::IOException e) {
                ports[i].close();
                nb = 0;
            }

            while(nb > 0)
            {
                nr = nb > MAX_SERIAL_RX_LEN ? MAX_SERIAL_RX_LEN : nb;
                nb -= nr;
                ports[i].read(largeRxBuffer, nr);
                processReceivedData(i, nr);
            }

            // after processing messages from a port we may have a new device
            if(fx_spec_numConnectedDevices != deviceIds.size())
                handleFlexseaDevice(i);
        }
    }
#endif
}

bool FlexseaSerial::wakeFromLongSleep() { return openPorts > 0; }
bool FlexseaSerial::goToLongSleep() { return openPorts == 0; }

void FlexseaSerial::handleFlexseaDevice(int port)
{
#ifndef TEST_CODE
    if(fx_spec_numConnectedDevices > deviceIds.size())
    {   // new device
        // figure which device is new
        unsigned int i;
        for(i=0;i<fx_spec_numConnectedDevices;i++)
        {
            if(connectedDevices.count( DEVICESPEC_UUID_BY_IDX(i) ) == 0)
                break;
        }

        if(i == fx_spec_numConnectedDevices)
        {
            std::cout << "Failed to find a connected device in the c interface that is not in the gui stack interface already\n";
        }
        else
        {
            addDevice(DEVICESPEC_UUID_BY_IDX(i), port, static_cast<FlexseaDeviceType>(DEVICESPEC_TYPE_BY_IDX(i)));
            int devId = deviceIds.back();
            FlexseaDeviceType type = connectedDevices.at(devId).type;
            std::cout << "Connected to a Flexsea Device at port " << ports[port].getPort() << std::endl
                      << "Device id: " << devId << ", type: " << type << std::endl;
        }
    }
    else
    {
        ;
        // device disconnected
        // figure which device and removeIt
    }
#else
    (void)port;
#endif
}

void FlexseaSerial::sendDeviceWhoAmI(int port)
{
#ifndef TEST_CODE
    uint32_t flag = 0;
    uint8_t lenFlags = 1, error;

    // NOTE: in a response, we need to reserve bytes for XID, RID, CMD, and TIMESTAMP
    const uint8_t RESERVEDBYTES = 4;
    uint8_t cmdCode, cmdType;
    MultiWrapper *out = &portPeriphs[port].out;
    tx_cmd_sysdata_r(out->unpacked + RESERVEDBYTES, &cmdCode, &cmdType, &out->unpackedIdx, &flag, lenFlags);

    out->unpacked[0] = FLEXSEA_PLAN_1;
    out->unpacked[1] = FLEXSEA_MANAGE_1;
    out->unpacked[2] = 1;   // garbage ? should reuse this for timestamp
    out->unpacked[3] = (cmdCode << 1) | 0x1;

    out->unpackedIdx += RESERVEDBYTES;
    out->currentMultiPacket = 0;
    portPeriphs[port].in.currentMultiPacket = 0;
    portPeriphs[port].in.unpackedIdx = 0;

    error = packMultiPacket(out);

    if(error)
        std::cout << "Error packing multipacket" << std::endl;
    else
    {
        unsigned int frameId = 0;
        while(out->frameMap > 0)
        {
            this->write(PACKET_WRAPPER_LEN, out->packed[frameId], port);
            out->frameMap &= (   ~(1 << frameId)   );
            frameId++;
        }
        out->isMultiComplete = 1;

//        std::cout << "Wrote who am i message" << std::endl;
    }
#else
    (void) port;
#endif
}

void FlexseaSerial::setDeviceMap(const FlexseaDevice &d, uint32_t *map)
{
    (void)d;
    (void)map;
    // not implemented
    throw new std::exception();
}

const std::vector<int>& FlexseaSerial::getDeviceIds() const
{
    return deviceIds;
}

const FlexseaDevice& FlexseaSerial::getDevice(int id) const
{
    if(connectedDevices.count(id))
        return connectedDevices.at(id);
    else
        return defaultDevice;
}

const uint32_t* FlexseaSerial::getMap(int id) const
{
    if(fieldMaps.count(id))
        return fieldMaps.at(id);
    else
        return NULL;

}

void FlexseaSerial::registerConnectionChangeFlag(uint8_t *flag) const {
    std::lock_guard<std::mutex> guard(connectionMutex);
    deviceConnectedFlags.push_back(flag);
}

void FlexseaSerial::registerMapChangeFlag(uint8_t *flag) const {
    std::lock_guard<std::mutex> guard(mapMutex);
    mapChangedFlags.push_back(flag);
}

void FlexseaSerial::unregisterConnectionChangeFlag(uint8_t *flag) const {
    std::lock_guard<std::mutex> guard(connectionMutex);
    deviceConnectedFlags.erase(std::remove(deviceConnectedFlags.begin(), deviceConnectedFlags.end(), flag), deviceConnectedFlags.end());
}

void FlexseaSerial::unregisterMapChangeFlag(uint8_t *flag) const {
    std::lock_guard<std::mutex> guard(mapMutex);
    mapChangedFlags.erase(std::remove(mapChangedFlags.begin(), mapChangedFlags.end(), flag), mapChangedFlags.end());
}

void FlexseaSerial::notifyConnectionChange()
{
    std::lock_guard<std::mutex> guard(connectionMutex);
    for(std::vector<uint8_t*>::iterator it = deviceConnectedFlags.begin(); it != deviceConnectedFlags.end(); ++it)
    {
        if(*it)
            *(*it) = 1;
    }
}
void FlexseaSerial::notifyMapChange()
{
    std::lock_guard<std::mutex> guard(mapMutex);
    for(std::vector<uint8_t*>::iterator it = mapChangedFlags.begin(); it != mapChangedFlags.end(); ++it)
    {
        if(*it)
            *(*it) = 1;
    }
}

int FlexseaSerial::addDevice(int id, int port, FlexseaDeviceType type)
{
    int unique = 1;
    for(std::vector<int>::iterator it = deviceIds.begin(); it != deviceIds.end() && unique; ++it)
    {
        if((*it) == id)
            unique = 0;
    }
    if(!unique) return 1;

    uint32_t* map = new uint32_t[FX_BITMAP_WIDTH]{0};
    circular_buffer<FX_DataPtr> *cb = new circular_buffer<FX_DataPtr>(FX_DATA_BUFFER_SIZE);
    std::recursive_mutex *cbMutex = new std::recursive_mutex();

    deviceIds.push_back(id);
    databuffers.insert({id, cb});
    connectedDevices.insert({id, FlexseaDevice(id, port, type, map, cb, cbMutex)});
    fieldMaps.insert({id, map});

    //Notify device connected
    notifyConnectionChange();

    return 0;
}

int FlexseaSerial::removeDevice(int id)
{
    int found = 0;
    std::vector<int>::iterator it;
    for(it = deviceIds.begin(); it != deviceIds.end() && !found; ++it)
    {
        if((*it) == id)
            found = 1;
    }
    if(!found) return 1;

    //remove record in device ids
    deviceIds.erase(--it);

    //deallocate the circular buffer
    std::recursive_mutex *m = connectedDevices.at(id).dataMutex;
    m->lock();

    circular_buffer<FX_DataPtr> *cb = databuffers.at(id);
    FX_DataPtr dataptr;
    while(cb->count())
    {
        dataptr = cb->get();
        if(dataptr)
            delete dataptr;
    }
    if(cb)
        delete cb;
    databuffers.erase(id);

    //remove record from connected devices
    connectedDevices.erase(id);

    //deallocate from field maps
    uint32_t *map = fieldMaps.at(id);
    delete[] map;
    fieldMaps.erase(id);

    //Notify device connected
    notifyConnectionChange();

    m->unlock();
    delete m;
    m = NULL;

    return 0;
}

/* Serial functions */
std::vector<std::string> FlexseaSerial::getPortList() const {
#ifndef TEST_CODE

    std::vector<serial::PortInfo> pi = serial::list_ports();
    for(unsigned int i = 0; i < pi.size(); i++)
    {
        serial::PortInfo p = pi.at(i);
        if(p.description.find("STM") != std::string::npos)
            std::cout << p.port << " " << p.hardware_id << " " << p.description << std::endl;
    }
#endif
    return std::vector<std::string>();
}

int FlexseaSerial::isOpen(uint16_t portIdx) const { return ports[portIdx].isOpen(); }

void FlexseaSerial::open(const std::string &portName, uint16_t portIdx) {

    if(portIdx >= FX_NUMPORTS)
    {
        std::cout << "Can't to open port outside range (" << portIdx << ")" << std::endl;
        return;
    }

    if(ports[portIdx].isOpen())
    {
        std::cout << "Port " << portIdx << " already open" << std::endl;
        return;
    }
    serial::Serial *s = ports+portIdx;
    s->setPort(portName);
    s->setBaudrate(400000);
    s->setBytesize(serial::eightbits);
    s->setParity(serial::parity_none);
    s->setStopbits(serial::stopbits_one);
    s->setFlowcontrol(serial::flowcontrol_hardware);

    unsigned int cnt=0, tries=5;
    while(cnt++ < tries && !s->isOpen())
        try { s->open(); } catch (...) {}

    if(s->isOpen())
    {
        bool doNotify;
        std::cout << "Opened port " << portName << " at index " << portIdx << " in " << cnt << " tries" << std::endl;
        {
            std::lock_guard<std::mutex> lk(conditionMutex);
            openPorts++;
            doNotify = openPorts == 1;

            this->sendDeviceWhoAmI(portIdx);
        }

        if(doNotify)
            wakeCV.notify_all();

    }
    else
        std::cout << "Failed to open port " << portName << " at index " << portIdx << " after " << tries << " tries" << std::endl;

}

void FlexseaSerial::close(uint16_t portIdx) {
    if(portIdx >= FX_NUMPORTS)
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
        std::lock_guard<std::mutex> lk(conditionMutex);
        openPorts--;
    }
}

void FlexseaSerial::tryReadWrite(uint8_t bytes_to_send, uint8_t *serial_tx_data, int timeout, uint16_t portIdx) {
    (void)bytes_to_send;
    (void)serial_tx_data;
    (void)timeout;
    (void)portIdx;
}

void FlexseaSerial::write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx)
{
    if(portIdx >= FX_NUMPORTS)
    {
        std::cout << "Can't write to port outside range (" << portIdx << ")" << std::endl;
        return;
    }
    if(!ports[portIdx].isOpen())
    {
        std::cout << "Port " << portIdx << " not open" << std::endl;
        for(unsigned int i = 0; i < deviceIds.size(); i++)
        {
            if(connectedDevices.at(deviceIds.at(i)).port == portIdx)
                removeDevice(deviceIds.at(i));
        }
        return;
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
