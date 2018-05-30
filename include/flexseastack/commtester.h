#ifndef INCCOMMTESTER_H
#define INCCOMMTESTER_H

#include "flexseastack/commanager.h"

class CommTestRecord;
class CommTestParams;

class CommTester
{
public:
    CommTester(CommManager &cm);

    void startTest(int devId, CommTestParams& params);
    void stopTest(int devId);

private:
    CommManager &_commManager;
    std::vector<CommTestRecord> _records;
};

#endif // INCCOMMTESTER_H
