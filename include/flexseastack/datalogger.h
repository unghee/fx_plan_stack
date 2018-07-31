#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <vector>
#include <fstream>
#include <mutex>

#include "flexseastack/periodictask.h"
#include "flexseastack/flexseadeviceprovider.h"

#define MAX_LOG_SIZE 50000

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

    bool logDevice(int idx);

private:

struct LogRecord {
    int devId;
    std::ofstream* fileObject;
    unsigned int lastTimestamp;
    unsigned int logFileSize;
    unsigned int logFileSplitIndex;
    unsigned int numActiveFields;
};

    unsigned int writeLogHeader(std::ofstream* fout, const FxDevicePtr dev);
    void swapFileObject(LogRecord &record, std::string newfilename, const FxDevicePtr dev);

    std::vector<LogRecord> logRecords;
    bool removeRecord(int idx);

    FlexseaDeviceProvider *devProvider;
    int numLogDevices;

    std::mutex resMutex;

    void clearRecords();
    std::string generateFileName(FxDevicePtr dev, std::string suffix="");
};

#endif // DATALOGGER_H
