#ifndef INCPERIODICTASK_H
#define INCPERIODICTASK_H

#include <condition_variable>

class PeriodicTask
{
public:
    PeriodicTask() :runPeriodicThread(0), taskPeriod(0) {}
    virtual ~PeriodicTask(){}

    void runPeriodicTask();
    void quitPeriodicTask();

    bool runPeriodicThread;
    unsigned int taskPeriod;
    std::mutex conditionMutex;
    std::condition_variable wakeCV;

protected:
    virtual void periodicTask() = 0;
    virtual bool wakeFromLongSleep()=0;
    virtual bool goToLongSleep()=0;

};

#endif // INCPERIODICTASK_H
