#ifndef TESTSERIAL_H
#define TESTSERIAL_H

#include "flexseastack/commanager.h"

class TestSerial : public CommManager
{
public:
    TestSerial();

    /// \brief Test sim 1 provides tests for read only windows like device display and 2d plot
    ///
    /// Test sim 2 tests:
    ///  - Devices being connected and disconnected without data throughput
    ///  - Devices' active maps changing without data throughput
    ///  - Simulating device data throughput
    void runTestSim1();

    /// \brief Test sim 2 provides tests for windows that manipulate connections and streams
    ///
    /// Test sim 2 tests:
    ///  - fakes functionality for opening / closing com ports
    ///  - fakes functionality for starting / stopping streams (handled in TestCommManager)
    ///  - fakes functionality for setting field maps
    ///  - simulates receiving data
    void runTestSim2();

    // set low if you want TestSerial to quit
    bool notQuit;
    //set high if you want more info coming through stdout
    bool runVerbose;

    //  ***************************************
    //  overriding serial functions
    //  ***************************************
    virtual std::vector<std::string> getAvailablePorts() const;
    virtual bool tryOpen(const std::string &portName, uint16_t portIdx=0);
    virtual int isOpen(uint16_t portIdx=0) const;
    virtual void tryClose(uint16_t portIdx=0);
    virtual void write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx=0);
    virtual void writeDevice(uint8_t bytes_to_send, uint8_t *serial_tx_data, const FlexseaDevice &d);

    // overriding flexseaserial functions
    virtual int writeDeviceMap(const FlexseaDevice &d, uint32_t* map);
    virtual void sendDeviceWhoAmI(int port);
    virtual void serviceOpenPorts() {} // do nothing as data is fake received when written to

    // overriding commmanager functions
    virtual bool startStreaming(int devId, int freq, bool shouldLog, int shouldAuto, uint8_t cmdCode=CMD_SYSDATA);

private:
    /* Three "Test Cases" */

    /* Tests connection / disconnection behaviour.
     * Every 2 seconds, TestSerial randomly decides to connect or disconnect a fake device
     *
     * GUI windows should reflect changes in connected devices
     * (drop down lists should be populated only with currently connected devices)
    */
    void randomConnections();

    /* Tests GUI's response to devices changing their available fields
     * TestSerial connects three devices,
     * then every 2 seconds randomly selects a device and randomly sets its field map
     *
     * GUI 2D Plot should populate field selection dropdown menus only with currently active fields
     * (here field means things like x-accel, y-gyro)
     *
     * Device windows should show all available fields and their most recent value (which in this case will be '-' as their won't be incoming data)
    */
    void randomDeviceMapChanges();

    /* Tests GUI's response to incoming data over multiple devices
     * TestSerial connects three devices,
     * then fakes receiving data at 200Hz
     *
     * 2D plot should be able to plot while data is coming through
     * Device windows should update to most recent value
    */
    void dataThroughput();

    ///  ***************************************
    ///  test utility functions, not important
    ///  ***************************************

    void testConnectDevice(int port=-1);
    void testDisconnectDevice(int id=-1);
    void testChangeDeviceMap();
    void testReceiveData();
    void testReceiveDataFromDevice(int id, uint32_t timestamp);

    void printData(int id,  int numFields, FX_DataPtr data);
    void printBitMap(const uint32_t* map, int numFields);
    void printDeviceMaps(const std::vector<int>& deviceIds, const std::unordered_map<int, FlexseaDevice> &connectedDevices);
    const char TAB = '\t';

    std::vector<std::string> fakePortList = {"COM3", "COM2", "ttyACM0", "ttyACM1", "ttyACM2" };
    int portMapping[FX_NUMPORTS] = {-1, -1, -1, -1};
};

#endif // TESTSERIAL_H
