#include <iostream>
#include <stdio.h>
#include <algorithm>

#include "flexseastack/flexseaserial.h"
#include "serial/serial.h"
#include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"
#include "flexseastack/flexsea-comm/inc/flexsea_multi_frame_packet_def.h"
#include "flexseastack/comm_string_generation.h"

extern "C" {
    #include "flexseastack/flexsea-system/inc/flexsea_device_spec.h"
    #include "flexseastack/flexsea-system/inc/flexsea_cmd_sysdata.h"
    #include "flexseastack/flexsea-system/inc/flexsea_sys_def.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_payload.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"
    #include "flexseastack/flexsea-system/inc/flexsea_system.h"
    #include "flexseastack/flexsea-system/inc/flexsea_dataformats.h"
}

FlexseaSerial::FlexseaSerial() : SerialDriver(FX_NUMPORTS)
{
    portPeriphs = new MultiCommPeriph[FX_NUMPORTS];
    initializeDeviceSpecs();

    for(int i = 0; i < FX_NUMPORTS; i++)
        initMultiPeriph(this->portPeriphs + i, PORT_USB, SLAVE);

    // set which commands to highjack
    memset(highjackedCmds, 0, NUM_COMMANDS);
    for(int i = 0; i < NUM_COMMANDS; i++)
        stringParsers[i] = &FlexseaSerial::defaultStringParser;

    highjackedCmds[CMD_SYSDATA] = 1;
    stringParsers[CMD_SYSDATA] = &FlexseaSerial::sysDataParser;
    memset(devicesAtPort, 0, FX_NUMPORTS * sizeof(int));
}

FlexseaSerial::~FlexseaSerial()
{
    delete[] portPeriphs;
    portPeriphs = nullptr;
}

inline int FlexseaSerial::updateDeviceMetadata(int port, uint8_t *buf)
{
    uint16_t i=MP_DATA1+1;
    uint8_t devType, devId, j, mapLen, devRole;
    devType = buf[i++];
    devId = buf[i++];
    mapLen = buf[i++];
    devRole = buf[i + mapLen * sizeof(int32_t)];

    bool addedDevice = false;
    if(!connectedDevices.count(devId))
    {
        addedDevice = !addDevice(devId, port, static_cast<FlexseaDeviceType>(devType), devRole);
        devicesAtPort[port]++;
    }
    else if(connectedDevices.at(devId)->type != devType)
    {
        std::cout << "Device record's type does not match incoming message, something went wrong (two devices connected with same id?)" << std::endl;
        removeDevice(devId);
        addedDevice = !addDevice(devId, port, static_cast<FlexseaDeviceType>(devType), devRole);
    }

    uint32_t bitmap[FX_BITMAP_WIDTH];
    auto dev = connectedDevices.at(devId);
    dev->getBitmap(bitmap);
    uint32_t temp;
    // if bitmap is null something is very wrong
    bool bitmapChanged = false;

    for(j=0;j<FX_BITMAP_WIDTH && j<mapLen; j++)
    {
        temp = REBUILD_UINT32(buf, &i);
        if(temp != bitmap[j])
            bitmapChanged = true;
        bitmap[j] = temp;
    }

    if(bitmapChanged)
    {
        // need to clear old data ptrs as they are wrong size
        std::lock_guard<std::recursive_mutex> lk(*(dev->dataMutex));
        dev->setBitmap(bitmap);
        mapChangedFlags.notify();
    }

    if(addedDevice)
    {
        std::cout << "Added device\n";
        fflush(stdout);
        deviceConnectedFlags.notify();
    }


    return 0;
}
inline int FlexseaSerial::updateDeviceData(uint8_t *buf)
{
    uint8_t devId;
    devId = buf[MP_XID];
    if(!connectedDevices.count(devId))
        return -1;

    FxDevicePtr d = connectedDevices.at(devId);
    FlexseaDeviceSpec ds = deviceSpecs[d->type];
    std::lock_guard<std::recursive_mutex> lk(*(d->dataMutex));

    FxDevData *cb = d->getCircBuff();
    FX_DataPtr fxDataPtr = cb->getWrite();

    uint32_t deviceBitmap[FX_BITMAP_WIDTH];
    d->getBitmap(deviceBitmap);

    // read into the rest of the data like a buffer
    if(fxDataPtr)
    {
        memcpy(fxDataPtr, buf+MP_TSTP, sizeof(uint32_t));
        uint8_t *dataPtr = (uint8_t*)(fxDataPtr+1);
        uint16_t j, fieldOffset=0, index=MP_DATA1+1;
        for(j = 0; j < ds.numFields; j++)
        {
            if(IS_FIELD_HIGH(j, deviceBitmap))
            {
                uint8_t ft = ds.fieldTypes[j];
                uint8_t fw = FORMAT_SIZE_MAP[ft];
                memcpy(dataPtr + fieldOffset, buf + index, fw);

                if( ft == FORMAT_16S || ft == FORMAT_8S )
                {
                    uint8_t val = ( *(dataPtr + fieldOffset + fw - 1) >> 7 ) ? 0xFF : 0;
                    memset( dataPtr + fieldOffset + fw, val, sizeof(int32_t) - fw);
                }

                index+=fw;
            }

            fieldOffset += 4; // storing each value as a separate int32
        }

    }
    else
    {
        std::cout << "Couldn't allocate memory for reading data into FlexseaDevice " << devId << std::endl;
        return -1;
    }

    return 0;
}

int FlexseaSerial::sysDataParser(int port)
{
    if(port < 0 || port >= FX_NUMPORTS)
    {
        std::cout << "invalid port" << std::endl;
        return 0;
    }
    MultiCommPeriph *cp = portPeriphs+port;

    uint8_t *msgBuf = cp->in.unpacked;
    bool isMeantForPlan = msgBuf[MP_RID] / 10 == 1;
    if(!isMeantForPlan)
    {
        std::cout << "Received message with invalid RID, probably some kind of device-side error\n";
        return -1;
    }

    uint8_t isMetaData = msgBuf[MP_DATA1];
    if(isMetaData)
        return updateDeviceMetadata(port, msgBuf);
    else
        return updateDeviceData(msgBuf);
}

#define CALL_MEMBER_FN(object,ptrToMember)  ((object)->*(ptrToMember))

void FlexseaSerial::processReceivedData(int port, size_t len)
{
    int totalBuffered = len + circ_buff_get_size(&(portPeriphs[port].circularBuff));
    int numMessagesReceived = 0;
//    int numMessagesExpected = (totalBuffered / COMM_STR_BUF_LEN);
    int maxMessagesExpected = (totalBuffered / COMM_STR_BUF_LEN + (totalBuffered % COMM_STR_BUF_LEN != 0));

    uint16_t bytesToWrite, cbSpace, bytesWritten=0;
    int error, successfulParse;

    while(len > 0)
    {
        cbSpace = CB_BUF_LEN - circ_buff_get_size(&(portPeriphs[port].circularBuff));
        bytesToWrite = MIN(len, cbSpace);

        error = circ_buff_write(&(portPeriphs[port].circularBuff), (largeRxBuffer+bytesWritten), bytesToWrite);
        if(error) std::cout << "circ_buff_write error:" << error << std::endl;

        do {
            portPeriphs[port].bytesReadyFlag = 1;
            portPeriphs[port].in.isMultiComplete = 0;

            MultiCommPeriph *cp = portPeriphs+port;
            int convertedBytes = unpack_multi_payload_cb(&cp->circularBuff, &cp->in);
            error = circ_buff_move_head(&cp->circularBuff, convertedBytes);
            if(portPeriphs[port].in.isMultiComplete)
        {
                uint8_t cmd = MULTI_GET_CMD7(portPeriphs[port].in.unpacked);
            int parseResult;
            if(highjackedCmds[cmd])
                parseResult = CALL_MEMBER_FN(this, stringParsers[cmd])(port);
            else
            {
                // c stack functions use device roles as ids...
                    auto dev = getDevicePtr( cp->in.unpacked[MP_XID] );
                if(dev)
                        cp->in.unpacked[MP_XID] = dev->getRole();

                    parseResult = parseReadyMultiString(cp);
            }

                numMessagesReceived++;
                (void) parseResult;
        }

            successfulParse = convertedBytes > 0 && !error;
        } while(successfulParse && numMessagesReceived < maxMessagesExpected);

        len -= bytesToWrite;
        bytesWritten += bytesToWrite;

        if(CB_BUF_LEN == circ_buff_get_size(&(portPeriphs[port].circularBuff)) && len)
        {
            std::cout << "circ buffer is full with non valid frames; clearing..." << std::endl;
            // erase all the bytes except the ones we just wrote
            circ_buff_move_head(&(portPeriphs[port].circularBuff), CB_BUF_LEN - bytesToWrite);
        }
    }

}

void FlexseaSerial::periodicTask()
{
    serviceOpenPorts();
    serviceOpenAttempts(taskPeriod);
}

void FlexseaSerial::serviceOpenPorts()
{
    size_t i, nr;
    long int nb;

    {
        std::lock_guard<std::mutex> lk(_portsMutex);
        if(!openPorts)
            return;
    }

    for(i = 0; i < FX_NUMPORTS; i++)
    {
        if(ports[i].isOpen())
        {
            try {
                nb = ports[i].available();
            } catch (...) {
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
}

bool FlexseaSerial::wakeFromLongSleep() { return openPorts > 0 || haveOpenAttempts; }
bool FlexseaSerial::goToLongSleep() { return !openPorts && !haveOpenAttempts; }

void FlexseaSerial::sendDeviceWhoAmI(int port)
{

    uint32_t flag = 0;
    uint8_t lenFlags = 1, error;

    MultiWrapper *out = &portPeriphs[port].out;
    error = CommStringGeneration::generateCommString(0, out, tx_cmd_sysdata_r, &flag, lenFlags);

    if(error)
        std::cout << "Error packing multipacket" << std::endl;
    else
    {
        unsigned int frameId = 0;
        while(out->frameMap > 0)
        {
            write(PACKET_WRAPPER_LEN, out->packed[frameId], port);
            out->frameMap &= (   ~(1 << frameId)   );
            frameId++;
        }
        out->isMultiComplete = 1;
        std::cout << "Wrote who am i message" << std::endl;
    }
}


void FlexseaSerial::open(std::string portName, int portIdx)
{
    if(portIdx >= _NUMPORTS) return;

    tryOpen(portName, portIdx);

    std::lock_guard<std::mutex> lk(openAttemptMut_);
    const int OPEN_DELAY = 250;
    openAttempts.emplace_back(portIdx, portName, 0, 10, OPEN_DELAY, 0);

    if(!haveOpenAttempts)
    {
        std::lock_guard<std::mutex> pTaskLock(conditionMutex);
        haveOpenAttempts = openAttempts.size();
    }

    wakeCV.notify_all();
}

void FlexseaSerial::openCancelRequest(int portIdx)
{
    std::lock_guard<std::mutex> lk(openAttemptMut_);
    for(auto& oa : openAttempts)
    {
        if(oa.portIdx == portIdx)
            oa.markedToRemove = true;
    }
}

void FlexseaSerial::writeDevice(uint8_t bytes_to_send, uint8_t *serial_tx_data, const FlexseaDevice &d) {
    write(bytes_to_send, serial_tx_data, (uint16_t)(d.port));
}


void FlexseaSerial::close(uint16_t portIdx)
{
    std::vector<int> idsToRemove;
    idsToRemove.reserve(connectedDevices.size());

    for(const auto &d : connectedDevices)
    {
        if(d.second->port == portIdx)
            idsToRemove.push_back(d.first);
    }

    for(const int &id : idsToRemove)
    {
        this->removeDevice(id);
        std::cout << "Removed device : " << id << std::endl;
    }

    devicesAtPort[portIdx] = 0;
    tryClose(portIdx);
}

void FlexseaSerial::serviceOpenAttempts(uint8_t delayed)
{
    std::lock_guard<std::mutex> lk(openAttemptMut_);
    if(!haveOpenAttempts) return;

    // iterate through and service each attempt
    for(auto& attempt : openAttempts)
    {
        auto state = getState(attempt.portIdx);

        if(state == serial::state_opening)
        {
            continue;
        }
        else if(state == serial::state_open && devicesAtPort[attempt.portIdx] < 1)
        {
            attempt.delayed += delayed;
            if(attempt.delayed >= attempt.delay)
            {
                attempt.delayed -= attempt.delay;
                if(!openPorts) openPorts = 1;
                sendDeviceWhoAmI(attempt.portIdx);
            }
        }
        else
        {
            // tryOpen is called once more on success to properly update state
            if(state == serial::state_open)
                tryOpen(attempt.portName, attempt.portIdx);

            attempt.markedToRemove = true;
        }
    }

    // remove attempts that are complete
    openAttempts.erase(
                std::remove_if(openAttempts.begin(), openAttempts.end(),
                               [=](OpenAttempt oa){return oa.markedToRemove;} ),
                openAttempts.end());

    std::lock_guard<std::mutex> pTaskLock(conditionMutex);
    haveOpenAttempts = openAttempts.size();
}
