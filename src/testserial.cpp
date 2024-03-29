#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <iostream>
#include <assert.h>
#include <cstring>
#include "flexsea_device_spec.h"
#include "flexsea_sys_def.h"
#include "flexseadevicetypes.h"
#include "flexsea_multi_frame_packet_def.h"
#include "testserial.h"


TestSerial::TestSerial() : notQuit(1), runVerbose(0)
{ srand(time(0)); }

void TestSerial::runTestSim1()
{
    // must call at beginning of tests (and in constructor?)
    // since the seed is thread local
    srand( time(0) );

    if(notQuit)
    {
       std::cout << "Test random connections and disconnections..." <<std::endl;
       randomConnections();
    }
    if(notQuit)
    {
       std::cout << "Testing random device map changes..." <<std::endl;
       randomDeviceMapChanges();
    }
    if(notQuit)
    {
        std::cout << "Testing data throughput..." <<std::endl;
        dataThroughput();
    }

}

// actually empty since all functionality in testsim2 is implemented by calling into class functions
void TestSerial::runTestSim2() {}

std::vector<std::string> TestSerial::getAvailablePorts() const { return fakePortList; }

bool TestSerial::tryOpen(const std::string &portName, uint16_t portIdx)
{
    if(isOpen(portIdx))
    {
        std::cout << "Port index " << portIdx << " already open." << std::endl;
        return true;
    }

    // get the idx of this name within the fakePortList
    size_t i, j;
    for(i=0;i<fakePortList.size();i++)
    {
        if(portName.compare(fakePortList.at(i)) == 0)
            break;
    }

    // if another serial object owns this physical port, it should fail
    for(j=0; j < FX_NUMPORTS; j++)
    {
        if(portMapping[j] == (int)i)
        {
            std::cout << "Port \"" << portName << "\" already open at index " << j << std::endl;
            return false;
        }
    }
                                // 1 in a 10 times we fail to connect
    if(i < fakePortList.size() && rand() % 10 != 0 )
    {
        std::cout << "Connecting to port \"" << portName << "\" at index " << portIdx << std::endl;
        // connect a couple devices (random number between 1 and 3) at this port
        portMapping[portIdx] = i;
        std::lock_guard<std::mutex> lk(_portCountMutex);
        openPorts++;
        return true;
    }
    return false;
}

int TestSerial::isOpen(uint16_t portIdx) const {    return (portMapping[portIdx] != -1);   }

void TestSerial::tryClose(uint16_t portIdx)
{

    if(portMapping[portIdx] != -1)
    {
        std::string portName = fakePortList.at(portMapping[portIdx]);
        std::cout << "[Fake] Disconnecting port \"" << portName << "\" at index " << portIdx << std::endl;
        portMapping[portIdx] = -1;

        std::lock_guard<std::mutex> lk(_portCountMutex);
        openPorts--;
    }
}

int doesRidMatchType(int rid, int type)
{
    if((rid & FLEXSEA_MANAGE_BASE) && (type == FX_MANAGE || type == FX_RIGID))
        return true;
    if((rid & FLEXSEA_EXECUTE_BASE) && type == FX_EXECUTE)
        return true;

    return false;
}

void TestSerial::write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx)
{
    (void)bytes_to_send;
    (void)portIdx;
    static uint32_t timestamp = 0;
    timestamp++;

    for(auto &x : connectedDevices)
    {
        if(x.second->id == serial_tx_data[MULTI_DATA_OFFSET + MP_RID]) //portIdx && doesRidMatchType(rid, x.second.type))
        {
            //we found our device
            testReceiveDataFromDevice(x.second->id, timestamp);
            return;
        }
    }

    std::cout << "TestSerial::write failed to match the message to a connected device" << std::endl;
}

void TestSerial::writeDevice(uint8_t bytes_to_send, uint8_t *serial_tx_data, const FlexseaDevice &d)
{
    static uint32_t timestamp = 0;
    timestamp++;

    if(0 && runVerbose) // make this not 0 if you want to see what's printed
    {
        printf("Writing to \"serial\": ");
        fwrite(serial_tx_data, bytes_to_send, 1, stdout);
        fflush(stdout);
    }

    testReceiveDataFromDevice(d.id, timestamp);
}

int TestSerial::writeDeviceMap(const FxDevicePtr d, uint32_t *map)
{
    d->setBitmap(map);
    mapChangedFlags.notify();
    return 0;
}

serial::state_t TestSerial::getPortState(int port) const
{
    return isOpen(port) ? serial::state_open : serial::state_none;
}

void TestSerial::sendDeviceWhoAmI(int port)
{
    // fake receiving replies from random # of devices

    unsigned int j, n = rand() % 3 + 1;
    for(j=0;j<n;j++)
        testConnectDevice(port);

    devicesAtPort[port] += n;
}

bool TestSerial::startStreaming(int devId, int freq, bool shouldLog, int shouldAuto, uint8_t cmdCode)
{
    (void) shouldAuto;
    std::cout << "TestSerial only supports regular streaming..." <<std::endl;
    return CommManager::startStreaming(devId, freq, shouldLog, false, cmdCode);
}

void TestSerial::randomConnections()
{
    std::cout << TAB << "Connecting Device..." <<std::endl;
    testConnectDevice();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << TAB << "Disconnecting Device..." <<std::endl;
    testDisconnectDevice();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    for(int i=0;i<6 && notQuit;i++)
    {
        int x = rand() % 10 + 1; // random number from 1 to 10

        if(x <= 7)               // 7 of 10 chance for this to be true
        {
            std::cout << TAB << "Connecting Device..." <<std::endl;
            testConnectDevice();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        else
        {
            std::cout << TAB << "Disconnecting Device..." <<std::endl;
            testDisconnectDevice();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    while(deviceIds.size())
    {
        std::cout << TAB << "Disconnecting Device..." <<std::endl;
        testDisconnectDevice();
        if(notQuit)
            std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    assert(deviceIds.size() == 0);
    assert(connectedDevices.empty());
}

void TestSerial::randomDeviceMapChanges()
{
    const int NUM_DEVICES= 4;
    const int NUM_RAND_CHANGES = 6;
    std::cout << TAB << "Connecting " << NUM_DEVICES << " Devices..." <<std::endl;
    for(int i=0;i<NUM_DEVICES;i++)
        testConnectDevice();

    bool existsNonTrivialDevice = false;
    size_t idx;
    int id;
    for(idx=0;idx<deviceIds.size();idx++)
    {
        id = deviceIds.at(idx);
        if(connectedDevices.at(id)->numFields > 0)
        {
            existsNonTrivialDevice = true;
            break;
        }
    }
    if(!existsNonTrivialDevice)
    {
        std::cout << TAB << "All devices have 0 fields :(" << std::endl;
        return;
    }

    if(runVerbose)
        printDeviceMaps(this->deviceIds, this->connectedDevices);

    for(int i=0; i<NUM_RAND_CHANGES && notQuit; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        int numToChange = rand() % NUM_DEVICES + 1;
        std::cout << TAB << "Changing device maps " << numToChange << " time(s) (may change some device twice)..." <<std::endl;

        for(int j=0;j<numToChange;j++)
            testChangeDeviceMap();

        if(runVerbose)
            printDeviceMaps(this->deviceIds, this->connectedDevices);
    }

    std::cout << TAB << "Disconnecting " << NUM_DEVICES << " Devices..." <<std::endl;
    for(int i=0;i<NUM_DEVICES;i++)
        testDisconnectDevice();
}

void TestSerial::dataThroughput()
{
    const int NUM_DEVICES= 3;
    const int freq = 200;
    const int period = 1000/freq;
    //const int lengthInSeconds = 20;
    //const int NUM_LOOPS = freq * lengthInSeconds;

    std::cout << TAB << "Connecting " << NUM_DEVICES << " Devices..." <<std::endl;
    for(int i=0;i<NUM_DEVICES;i++)
        testConnectDevice();

    std::cout << TAB << "\"Receiving\" Data at " << freq << " Hz"
              // << " for " << lengthInSeconds << " seconds..."
              << std::endl;

    //for(int i=0; i<NUM_LOOPS && notQuit; i++)
    while(notQuit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(period));
        //add data
        testReceiveData();
    }

    std::cout << TAB << "Disconnecting " << NUM_DEVICES << " Devices..." <<std::endl;
    for(int i=0;i<NUM_DEVICES;i++)
        testDisconnectDevice();
}

void TestSerial::testConnectDevice(int port)
{
    // Get a unique ID
    int id;
    int unique = 0;
    while(!unique)
    {
        id = rand() % 255 + 1;
        unique = 1;
        for(std::vector<int>::iterator it = deviceIds.begin(); it != deviceIds.end(); ++it)
        {
            if((*it) == id)
                unique = 0;
        }
    }

    // select a port
    if(port < 0)
        port = rand() % FX_NUMPORTS;

    // select a type
    FlexseaDeviceType type = static_cast<FlexseaDeviceType>(rand() % (NUM_DEVICE_TYPES-1) + 1);

    if(!unique)
    {
        std::cout << "Failed to connect device, could not pick a unique type" << std::endl;
        return;
    }

    // add the device
    if(this->addDevice(id, port, type))
        std::cout << TAB << "Adding device failed..?\n";
    else
    {
        if(runVerbose)
            std::cout << TAB << "Added device: { id=" << id << ", port=" << port << ", type=" << std::string(deviceSpecs[(int)type].fieldLabels[0]) << " }" << std::endl;

        // set the map randomly
        FxDevicePtr dev = getDevicePtr(id);
        if(dev->numFields)
        {
            uint32_t map[FX_BITMAP_WIDTH];
            memset(map, 0, FX_BITMAP_WIDTH*sizeof(uint32_t));
            map[0] = (rand() % (int)(pow(2, dev->numFields) - 1));
            dev->setBitmap(map);
            mapChangedFlags.notify();
        }
    }
}

void TestSerial::testDisconnectDevice(int id)
{
    if(!deviceIds.size()) return;

    if(id < 0)
    {
        //select a random device to disconnect
        int idx = rand() % deviceIds.size();
        id = deviceIds.at(idx);
    }

    this->removeDevice(id);

    if(runVerbose)
        std::cout << TAB << "Disconnected device with id=" << id << std::endl;
}

void TestSerial::testChangeDeviceMap()
{
    if(!deviceIds.size()) return;

    //select a random device with a non trivial map
    int idx, id;
    do {
        idx = rand() % deviceIds.size();
        id = deviceIds.at(idx);
    } while(connectedDevices.at(id)->numFields == 0);

    // get their map
    FxDevicePtr dev = connectedDevices.at(id);

    // figure out how many fields the device actually has
    int numFields = dev->numFields;
    if(numFields)
    {
        uint32_t map[FX_BITMAP_WIDTH];
        memset(map, 0, FX_BITMAP_WIDTH*sizeof(uint32_t));
        // randomly flip some bits  (only actually hitting the first 32 fields... my guess is that's fine)
        map[0] ^= (rand() % (int)(pow(2, numFields) - 1));

        dev->setBitmap(map);
        //Notify device connected
        mapChangedFlags.notify();
    }
}

void TestSerial::printData(int id,  int numFields, FX_DataPtr data)
{
    std::cout << TAB << TAB << "id=" << id << ", # fields=" << numFields << ", received {";
    for(int i = 0; i <= numFields; i++)
        std::cout << data[i] << (i==numFields ? "" : ", ");
    std::cout << "}" << std::endl;
}

void TestSerial::testReceiveData()
{
    int id;
    static uint32_t timestamp = 0;
    timestamp++;

    for(unsigned int i=0;i<deviceIds.size();i++)
    {
        // get the device
        id = deviceIds.at(i);
        testReceiveDataFromDevice(id, timestamp);

    }
}

void TestSerial::testReceiveDataFromDevice(int id, uint32_t timestamp)
{
    FxDevicePtr d = connectedDevices.at(id);
    double x = (double)timestamp * (double)(2 * M_PI / 1000); // 1 cycle per 10 second about

    //if the device has no fields we can skip it
    if(d->numFields < 1)
        return;

    //lock the mutex before accessing data buffer
    std::lock_guard<std::recursive_mutex> lk(*(d->dataMutex));

    //get the data buffer for this device
    FxDevData *cb = d->getCircBuff();
    if(!cb) return;

    FX_DataPtr dataptr = cb->getWrite();

    dataptr[0] = timestamp;
    dataptr[1] = d->type;
    dataptr[2] = d->id;
    if(d->numFields > 2) dataptr[3] = (sin(x) + 1)*1000;
    if(d->numFields > 3) dataptr[4] = (cos(x) + 1)*1000;
    if(d->numFields > 4) dataptr[5] = sin(x)*sin(x)*1000;
    if(d->numFields > 5) dataptr[6] = timestamp % 500;
    for(int k = 7; k <= d->numFields; k++)
        dataptr[k] = dataptr[k%3 + 2];

    if(runVerbose && timestamp % 100 == 0)
        printData(d->id, d->numFields, dataptr);
}

void TestSerial::printBitMap(const uint32_t* map, int numFields)
{
    if(numFields < 0 || !map) return;
    std::cout << numFields << ", ";
    for(int i = numFields-1; i >= 0; i--)
    {
        std::cout << (IS_FIELD_HIGH(i, map) ? '1' : '0');
    }
}

void TestSerial::printDeviceMaps(
        const std::vector<int>& deviceIds,
        const std::unordered_map<int, FxDevicePtr> &connectedDevices)
{
    int id, nf;
    unsigned int i;
    std::cout << TAB << "Devices {id, numfields, map}: [ ";
    uint32_t dMap[FX_BITMAP_WIDTH];
    for(i = 0; i < deviceIds.size(); i++)
    {
        id = deviceIds.at(i);
        nf = connectedDevices.at(id)->numFields;
        std::cout << "{" << id << ", ";

        connectedDevices.at(id)->getBitmap(dMap);
        printBitMap(dMap, nf);
        std::cout << "}";
        if(i+1!=deviceIds.size())
            std::cout << " , ";
    }
    std::cout << " ] " << std::endl;
}
