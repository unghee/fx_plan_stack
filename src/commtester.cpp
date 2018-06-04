#include "flexseastack/commtester.h"
#include <cstring>

extern "C" {
    #include "flexseastack/flexsea-system/inc/flexsea_cmd_tools.h"
}

class CommTestRecord
{
public:
    CommTestRecord(int id, int f, bool a) : devId(id), freq(f), usedAuto(a) {}

    int devId, freq;
    bool usedAuto;

    float throughputRate, lossRate, qualityRate;
    uint16_t  numPacketsReceived, numPacketsExpected;
};

CommTester::CommTester(CommManager &cm):
    _commManager(cm)
{
    resetStats();
}
CommTester::~CommTester()
{
    if(isTesting)
        stopTest();
}

void CommTester::startTest(int devId, const CommTestParams& params)
{
    const FlexseaDevice &d = _commManager.getDevice(devId);
    if(!d.isValid())    return;

    testStartTime = std::chrono::system_clock::now();

    auto tx_func = [this] (uint8_t* buf, uint8_t* cmdCode, uint8_t* cmdType, uint16_t* len) {
        tx_cmd_tools_comm_test_r(buf, cmdCode, cmdType, len, 1, 20, this->packetIndex);
        this->packetIndex++;
        sendIntervals[this->packetIndex % this->AVG_SIZE] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->testStartTime).count();
    };

    isTesting = _commManager.startStreaming(devId, params.freq, false, tx_func);
    if(isTesting)
        testId = devId;
}

void CommTester::stopTest()
{
    _commManager.stopStreaming(testId);
    testId = -1;
    isTesting = false;

    prevSent=sentPackets;
    prevReceived=goodPackets+badPackets;
}

void CommTester::resetStats()
{
    sentPackets = 0;
    goodPackets = 0;
    badPackets = 0;
    prevSent = 0;
    prevReceived = 0;
    this->packetIndex = 0;
    memset(sendIntervals, 0, sizeof(sendIntervals));

    if(isTesting) testStartTime = std::chrono::system_clock::now();
}

int CommTester::getPacketsSent() const { return this->packetIndex; }

std::tuple<int, int, int> CommTester::getPacketsReceived() const {
    std::tuple<int, int, int> r { goodPackets + badPackets, goodPackets, badPackets};
    return r;
}

float CommTester::getThroughput() const { return sentPackets == 0 ? 0 : ((float)(goodPackets+badPackets)) / sentPackets; }
float CommTester::getLossRate() const { return sentPackets == 0 ? 0 : ((float)(sentPackets-goodPackets-badPackets)) / sentPackets; }
float CommTester::getQualityRate() const {
    int received = goodPackets + badPackets;
    return (received) == 0 ? 0 : ((float)goodPackets) / (received);
}

float CommTester::getSendRate() const {
    if(isTesting)
        return (1000.0f * (sentPackets - prevSent)) / std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->testStartTime).count();

    return 0;
}
float CommTester::getReceiveRate() const {
    if(isTesting)
        return (1000.0f * (goodPackets+badPackets - prevReceived)) / std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->testStartTime).count();

    return 0;
}
