#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <vector>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <assert.h>
#include <atomic>
#include "flexseadevice.h"

#define MAX_LOG_SIZE 50000
#define DEFAULT_LOG_FOLDER "Plan-GUI-Logs"
#define LOG_FOLDER_CONFIG_FILE "logFolderConfigFile.txt"

/*
    Class intended to be coupled with a single Device that is CONNECTED 
    Periodically reads data from a FlexseaDevice and writes it to the log file
    The static variables are shared amongst DataLoggers and will modify the logging
    settings for all Devices 
*/ 
class DataLogger {

public:
    DataLogger(bool logAdditionalField, FlexseaDevice* flexseaDevice);
    ~DataLogger();

    static bool sessionInitialized;

    static bool createSessionFolder(std::string session_name);
    static bool setLogFolder(std::string folderPath);
    static bool setDefaultLogFolder();

    static void setColumnValue(unsigned col, int val);
    static void setAdditionalColumn(std::vector<std::string> addLabel, std::vector<int> addValue);

    void initLogging();
    void logDevice();

private:
    static const int MAX_PATH_LENGTH = 256;

    static std::mutex _additionalValuesLock;
    static std::vector<std::string> _additionalColumnLabels;
    static std::vector<int> _additionalColumnValues;

    // folderNumber is for each dataLogger object to make sure it reflect the most recent
    // folder change. IE, if dataLogger.folderNumber != _folderNumber --> changeFilename() 
    static std::atomic<int> _folderNumber;
    // If we want to increase concurrency, we can switch this to a reader/writer lock
    // I'm interested to see if it makes a difference
    // static std::shared_timed_mutex rwFolderLock;
    static std::mutex _folderLock;
    static std::string _logFolderPath;
    static std::string _sessionPath;

    // Per Device variables
    bool initialized;
    int folderNumber;
    std::ofstream* fileObject;
    unsigned int lastTimestamp;
    unsigned int logFileSize;
    unsigned int logFileSplitIndex;
    unsigned int numActiveFields;
    unsigned int logAdditionalField;

    FlexseaDevice* flexseaDevice;

    static bool createFolder(std::string path);
    static bool loadLogFolderConfig();
    static void saveLogFolderConfig();
    
    std::string generateFileName(std::string suffix="");
    void writeLogHeader();

    // Runs when we change folders or fill up a log file
    void changeFileName(std::string newfilename);
};

#endif // DATALOGGER_H
