#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <vector>
#include <fstream>
#include <mutex>

#include "periodictask.h"
#include "flexseadeviceprovider.h"

#define MAX_LOG_SIZE 50000
#define DEFAULT_LOG_FOLDER "Plan-GUI-Logs"
#define LOG_FOLDER_CONFIG_FILE "logFolderConfigFile.txt"

/// \brief class which manages creating log files
/// reads data from FlexseaDevices provided by a FlexseaDeviceProvider
/// employs a polling method therefore the owner MUST either
///  - trigger polls by calling serviceLogs, or
///  - run DataLogger on a thread using the PeriodicTask pattern
class DataLogger : public PeriodicTask
{
public:
    DataLogger(FlexseaDeviceProvider* fdp);

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
    bool setLogFolder(std::string folderPath);
    bool setDefaultLogFolder();
	bool createSessionFolder(std::string session_name);

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
    bool createFolder(std::string path);
    bool loadLogFolderConfig();
    void saveLogFolderConfig();
};

#endif // DATALOGGER_H
