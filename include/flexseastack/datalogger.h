#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <vector>
#include <fstream>
#include <mutex>

#include "flexseastack/periodictask.h"
#include "flexseastack/flexseadeviceprovider.h"

class DataLogger : public PeriodicTask
{
public:
    DataLogger(FlexseaDeviceProvider* fdp);

    bool startLogging(int devId);
    bool stopLogging(int devId);
    bool stopAllLogs();

    void serviceLogs();


protected:

    virtual void periodicTask() {serviceLogs();}
    virtual bool wakeFromLongSleep();
    virtual bool goToLongSleep();

    void logDevice(int idx);


private:
    std::vector<int> loggedDevices;
    std::vector<std::ofstream*> fileObjects;
    std::vector<unsigned int> timestamps;

    FlexseaDeviceProvider *devProvider;
    int numLogDevices;

    std::mutex resMutex;

    void clearRecords();
    std::string generateFileName(FxDevicePtr dev);
};

#endif // DATALOGGER_H
