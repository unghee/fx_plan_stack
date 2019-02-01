#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <vector>
#include <fstream>
#include <mutex>

#include "flexseastack/periodictask.h"
#include "flexseastack/flexseadeviceprovider.h"

#define MAX_LOG_SIZE 50000
#define DEFAULT_LOG_FOLDER "Plan-GUI-Logs"

/// \brief class which manages creating log files
/// reads data from FlexseaDevices provided by a FlexseaDeviceProvider
/// employs a polling method therefore the owner MUST either
///  - trigger polls by calling serviceLogs, or
///  - run DataLogger on a thread using the PeriodicTask pattern
class DataLogger : public PeriodicTask
{
public:
    DataLogger(FlexseaDeviceProvider* fdp, std::string logFolderPath = DEFAULT_LOG_FOLDER);

    /// \brief starts logging all data received by device with id=devId
    bool startLogging(int devId, bool logAdditionalColumnsInit = false);
    /// \brief stops logging all data received by device with id=devId
    bool stopLogging(int devId);

    /// \brief stops all logs managed by this DataLogger
    bool stopAllLogs();

    /// \brief service all logs managed by this DataLogger (must be called periodically)
    void serviceLogs();
    void setColumnValue(unsigned col, int val);
    void setAdditionalColumn(std::vector<std::string> addLabel, std::vector<int> addValue);
    void setLogFolder(std::string folderPath);
    void setDefaultLogFolder();

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
    unsigned int logAdditionalField;
};

    void initializeSessionFolder();
    std::vector<std::string> additionalColumnLabels;
    std::vector<int> additionalColumnValues;
    unsigned int writeLogHeader(std::ofstream* fout, const FxDevicePtr dev, bool logAdditionalColumnsInit);
    void swapFileObject(LogRecord &record, std::string newfilename, const FxDevicePtr dev);

    std::vector<LogRecord> logRecords;
    bool removeRecord(int idx);

    FlexseaDeviceProvider *devProvider;
    int numLogDevices;
    bool isFirstLogFile;

    std::mutex resMutex;

    std::string _logFolderPath;

    std::string _sessionPath;

    void clearRecords();
    std::string generateFileName(FxDevicePtr dev, std::string suffix="");
    void createFolder(std::string path);
};

#endif // DATALOGGER_H
