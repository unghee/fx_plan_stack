#include "flexseastack/commtester.h"

class CommTestParams
{
public:
    bool shouldAuto;
    int freq;

};

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
{}

void CommTester::startTest(int devId, CommTestParams& params)
{
    const FlexseaDevice &d = _commManager.getDevice(devId);
    if(!d.isValid())    return;

    _records.emplace_back(devId, params.freq, params.shouldAuto);

    auto && r = _records.at(devId);

    _commManager.registerMessageReceivedCounter(devId, &r.numPacketsReceived);
    _commManager.registerMessageSentCounter(devId, &r.numPacketsExpected);

    _commManager.startStreaming(devId, params.freq, false, params.shouldAuto);
}



void CommTester::stopTest(int devId)
{
    std::vector<CommTestRecord>::iterator it = std::find_if(_records.begin(), _records.end(), [&devId](const CommTestRecord &r) {return r.devId == devId; });
    if(it == _records.end()) return;

    _commManager.deregisterMessageReceivedCounter(devId);
    _commManager.deregisterMessageSentCounter(devId);

    _commManager.stopStreaming(devId);

    _records.erase(it);
}
