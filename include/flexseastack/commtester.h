#ifndef INCCOMMTESTER_H
#define INCCOMMTESTER_H

#include "flexseastack/commanager.h"
#include <chrono>
#include <tuple>

class CommTestRecord;

class CommTestParams
{
public:
    CommTestParams(bool s, int f) : shouldAuto(s), freq(f){}

    bool shouldAuto;
    int freq;
};

class CommTester
{
public:
    CommTester(CommManager &cm);
    ~CommTester();

    void startTest(int devId, const CommTestParams& params);
    void stopTest();

    void resetStats();

    int getPacketsSent() const;
    std::tuple<int, int, int> getPacketsReceived() const;
    float getThroughput() const;
    float getLossRate() const;
    float getQualityRate() const;

    float getSendRate() const;
    float getReceiveRate() const;
    bool getTesting() const {return isTesting;}

private:
    CommManager &_commManager;

    mutable bool isTesting = false;
    int packetIndex = 0;
    int testId=-1;

    std::chrono::system_clock::time_point testStartTime;

    static const int AVG_SIZE = 8;
    int sendIntervals[AVG_SIZE];

    int prevSent=0, prevReceived=0;
//    std::vector<CommTestRecord> _records;
};

#endif // INCCOMMTESTER_H
