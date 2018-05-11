#include <iostream>
#include <stdio.h>
#include <algorithm>

#include "flexseastack/flexseaserial.h"
#include "serial/serial.h"
#include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"

extern "C" {
    #include "flexseastack/flexsea-system/inc/flexsea_device_spec.h"
    #include "flexseastack/flexsea-system/inc/flexsea_cmd_sysdata.h"
    #include "flexseastack/flexsea-system/inc/flexsea_sys_def.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_payload.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"
    #include "flexseastack/flexsea-system/inc/flexsea_system.h"
    #include "flexseastack/flexsea-system/inc/flexsea_dataformats.h"
}

FlexseaSerial::FlexseaSerial() : defaultDevice(-1, -1, FX_NONE, NULL, NULL), openPorts(0)
{
    ports = new serial::Serial[FX_NUMPORTS];
    portPeriphs = new MultiCommPeriph[FX_NUMPORTS];

    initializeDeviceSpecs();

#ifndef TEST_CODE

    for(int i = 0; i < FX_NUMPORTS; i++)
        initMultiPeriph(this->portPeriphs + i, PORT_USB, SLAVE);

#endif //TEST_CODE

    // set which commands to highjack
    memset(highjackedCmds, 0, NUM_COMMANDS);
    for(int i = 0; i < NUM_COMMANDS; i++)
        stringParsers[i] = &defaultStringParser;

    highjackedCmds[CMD_SYSDATA] = 1;
    stringParsers[CMD_SYSDATA] = &sysDataParser;
}

int FlexseaSerial::sysDataParser(int port)
{
    if(port < 0 || port >= FX_NUMPORTS) return 0;
    MultiCommPeriph *cp = portPeriphs+port;

    uint8_t *msgBuf = cp->in.unpacked;
    uint16_t index=P_DATA1;
    uint8_t lenFlags;
    uint32_t flags[FX_BITMAP_WIDTH];

    lenFlags = msgBuf[index++];

    //read in our fields
    int i, j, k, fieldOffset;

    for(i=0;i<lenFlags;i++)
        flags[i]=REBUILD_UINT32(msgBuf, &index);

    uint8_t devType = msgBuf[index++];
    uint16_t devId = msgBuf[index] + (msgBuf[index+1] << 8);
    index+=2;

    // if we haven't seen this device before we add it to our list
    if(connectedDevices.count(devId) == 0)
        addDevice(devId, port, static_cast<FlexseaDeviceType>(devType));
    else if(connectedDevices.at(devId).type != devType)
    {
        std::cout << "Device record's type does not match incoming message, something went wrong (two devices connected with same id?)" << std::endl;
        removeDevice(devId);
        addDevice(devId, port, static_cast<FlexseaDeviceType>(devType));
    }

    // read into the appropriate device
    FlexseaDevice &d = connectedDevices.at(devId);
    FlexseaDeviceSpec ds = deviceSpecs[devType];

    std::lock_guard<std::recursive_mutex> lk(*d.dataMutex);

    circular_buffer<FX_DataPtr> *cb = this->databuffers.at(devId);
    FX_DataPtr fxDataPtr;
    if(cb->full())
        fxDataPtr = cb->get();
    else
        fxDataPtr = new uint32_t[d.numFields+1]{0};

    //TODO: do timestamp properly
    static uint32_t fakeTimestamp = 0;
    fxDataPtr[0] = fakeTimestamp++;

    // read into the rest of the data like a buffer
    uint8_t *dataPtr = (uint8_t*)(fxDataPtr+1);
    if(dataPtr)
    {
        // first two fields are devType and devId, we could probably skip for reading in
        fieldOffset = 0;
        for(j = 0; j < ds.numFields; j++)
        {
            if(IS_FIELD_HIGH(j, flags))
            {
                uint8_t fw = FORMAT_SIZE_MAP[ds.fieldTypes[j]];
                for(k = 0; k < fw; k++)
                    dataPtr[fieldOffset + k] = msgBuf[index++];
            }

            fieldOffset += 4; // storing each value as a separate int32
        }
    }
    else
    {
        std::cout << "Couldn't allocate memory for reading data into FlexseaDevice " << d.id << std::endl; // log error?
    }

    cb->put(fxDataPtr);

    return 1;
}

FlexseaSerial::~FlexseaSerial()
{
    delete[] ports;
    ports = NULL;
    delete[] portPeriphs;
    portPeriphs = NULL;

    for(std::unordered_map<int, uint32_t*>::iterator it = fieldMaps.begin(); it != fieldMaps.end(); ++it)
    {
        if(it->second)
            delete[] (it->second);

        (it->second) = nullptr;
    }
    fieldMaps.clear();
}

#define CALL_MEMBER_FN(object,ptrToMember)  ((object)->*(ptrToMember))

void FlexseaSerial::processReceivedData(int port, size_t len)
{
#ifndef TEST_CODE
    int totalBuffered = len + circ_buff_get_size(&(portPeriphs[port].circularBuff));
    int numMessagesReceived = 0;
    int numMessagesExpected = (totalBuffered / COMM_STR_BUF_LEN);
    int maxMessagesExpected = (totalBuffered / COMM_STR_BUF_LEN + (totalBuffered % COMM_STR_BUF_LEN != 0));

    uint16_t bytesToWrite, cbSpace, bytesWritten=0;
    int error, successfulParse;

    while(len > 0)
    {
        cbSpace = CB_BUF_LEN - circ_buff_get_size(&(portPeriphs[port].circularBuff));
        bytesToWrite = MIN(len, cbSpace);

        error = circ_buff_write(&(portPeriphs[port].circularBuff), (&largeRxBuffer[bytesWritten]), (bytesToWrite));
        if(error) std::cout << "circ_buff_write error:" << error << std::endl;

        do {
            portPeriphs[port].bytesReadyFlag = 1;
            successfulParse = tryParse(portPeriphs + port);
            if(portPeriphs[port].in.isMultiComplete)
            {
                uint8_t cmd = portPeriphs[port].in.unpacked[P_CMD1] >> 1;
                int parseResult;
                if(highjackedCmds[cmd])
                    parseResult = CALL_MEMBER_FN(this, stringParsers[cmd])(port);
                else
                    parseResult = parseReadyMultiString(portPeriphs + port);

                numMessagesReceived++;
                (void) parseResult;
            }

        } while(successfulParse && numMessagesReceived < maxMessagesExpected);

        len -= bytesToWrite;
        bytesWritten += bytesToWrite;

        if(CB_BUF_LEN == circ_buff_get_size(&(portPeriphs[port].circularBuff)) && len)
        {
            std::cout << "circ buffer is full with non valid frames; clearing..." << std::endl;
            circ_buff_init(&(portPeriphs[port].circularBuff));
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
    size_t i, nr;
    long int nb;

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

void FlexseaSerial::setDeviceMap(const FlexseaDevice &d, const std::vector<int> &fields)
{
    if(!d.isValid() || !fields.size()) return;

    uint32_t map[FX_BITMAP_WIDTH];
    memset(map, 0, sizeof(uint32_t) * FX_BITMAP_WIDTH);

    uint32_t i, fieldId;
    for(i=0;i<fields.size();i++)
    {
        fieldId = fields.at(i);
        if((int)fieldId < d.numFields)
        {
            SET_FIELD_HIGH(fieldId, map);
        }
    }

    setDeviceMap(d, map);

}

const std::vector<int>& FlexseaSerial::getDeviceIds() const
{
    return deviceIds;
}

std::vector<int> FlexseaSerial::getDeviceIds(int portIdx) const
{
    std::vector<int> ids;
    for(auto it = connectedDevices.begin(); it != connectedDevices.end(); it++)
    {
        if(it->second.port == portIdx)
            ids.push_back(it->first);
    }
    return ids;
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
    deviceConnectedFlags.notify();

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
    deviceConnectedFlags.notify();

    m->unlock();
    delete m;
    m = NULL;

    return 0;
}

/* Serial functions */
std::vector<std::string> FlexseaSerial::getAvailablePorts() const {
    std::vector<std::string> result;

#ifndef TEST_CODE
    std::vector<serial::PortInfo> pi = serial::list_ports();
    for(unsigned int i = 0; i < pi.size(); i++)
        result.push_back(pi.at(i).port);
#endif

    return result;
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
