// #ifndef INCCOMMTESTER_H
// #define INCCOMMTESTER_H

// #include "commanager.h"
// #include <chrono>
// #include <tuple>

// class CommTestRecord;

// class CommTestParams
// {
// public:
//     CommTestParams(bool s, int f) : shouldAuto(s), freq(f){}

//     bool shouldAuto;
//     int freq;
// };

// class CommTester
// {
// public:
//     CommTester(CommManager &cm);
//     ~CommTester();

//     /// \brief starts a stream of comm test commands to the given device
//     void startTest(int devId, const CommTestParams& params);
//     /// \brief stops whichever stream is ongoing
//     void stopTest();

//     /// \brief resets caches which store data used to compute stats
//     void resetStats();

//     /// \brief get the number of packets sent since the last start of test or since the last time resetStats was called
//     int getPacketsSent() const;

//     /// \brief get the number of packets received since the last start of test or since the last time resetStats was called
//     std::tuple<int, int, int> getPacketsReceived() const;

//     /// \brief get the computed throughput rate for the current or last test
//     float getThroughput() const;

//     /// \brief get the computed loss rate for the current or last test
//     float getLossRate() const;

//     /// \brief get the computed quality rate for the current or last test
//     float getQualityRate() const;

//     /// \brief get the computed rate of packet sending achieved by the stack for the current or last test
//     float getSendRate() const;

//     /// \brief get the computed rate of packet receiving for the current or last test
//     float getReceiveRate() const;

//     /// \brief get whether a test is currently running
//     bool getTesting() const {return isTesting;}

// private:
//     CommManager &_commManager;

//     mutable bool isTesting = false;
//     int packetIndex = 0;
//     int testId=-1;

//     std::chrono::system_clock::time_point testStartTime;

//     int prevSent=0, prevReceived=0;
// };

// #endif // INCCOMMTESTER_H
