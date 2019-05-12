#include "datalogger.h"
#include <chrono>
#include <sstream>
#include <string>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include<sys/stat.h>
#endif


DataLogger::DataLogger(FlexseaDeviceProvider* fdp)
    : devProvider(fdp)
    , numLogDevices(0)
    , isFirstLogFile(true)
    , _logFolderPath(DEFAULT_LOG_FOLDER)
{
    loadLogFolderConfig();
    createFolder(_logFolderPath);
}

bool DataLogger::startLogging(int devId, bool logAdditionalFieldInit)
{
    if(isFirstLogFile)
    {
        createSessionFolder("");
    }

    FxDevicePtr dev = devProvider->getDevicePtr(devId);
    if(!dev ||
       dev->type == FX_NONE) return false;

    std::string fileName = generateFileName(dev);
    std::ofstream* fout = nullptr;

    unsigned int numActiveFields = dev->getNumActiveFields();
    unsigned int ts = 0;

    if(numActiveFields)
    {
        std::replace(fileName.begin(), fileName.end(), '\\', '/');
        fout = new std::ofstream(fileName);

        if(!fout || !fout->is_open())
        {
            if(fout) delete fout;
            fout = nullptr;
            std::cout << "Can't open file" << std::endl;
            throw std::bad_alloc();
        }

        writeLogHeader(fout, dev, logAdditionalFieldInit);
    }

    if(dev->dataCount())
    {
        ts = dev->getLatestTimestamp();
    }

    {
        std::lock_guard<std::mutex> lk(resMutex);
        logRecords.push_back( {devId, fout, ts, 0, 0, numActiveFields, logAdditionalFieldInit} );
        numLogDevices++;
    }

    return true;
}

bool DataLogger::stopLogging(int devId)
{
    std::lock_guard<std::mutex> lk(resMutex);

    unsigned int i=0;
    while( i < logRecords.size() && logRecords.at(i).devId != devId )
        ++i;

    return removeRecord(i);
}


void DataLogger::setAdditionalColumn(std::vector<std::string> addLabel, std::vector<int> addValue)
{
    additionalColumnLabels = addLabel;
    additionalColumnValues = addValue;
}

bool DataLogger::setLogFolder(std::string folderPath)
{
    bool success = createFolder(folderPath);

    if(success)
    {
        _logFolderPath = folderPath;
        saveLogFolderConfig();
        isFirstLogFile = true;
    }
    return success;
}

bool DataLogger::setDefaultLogFolder()
{
   return setLogFolder(DEFAULT_LOG_FOLDER);
}

// public, allows user to set values
void DataLogger::setColumnValue(unsigned col, int val)
{
    additionalColumnValues.at(col) = val;
}

bool DataLogger::removeRecord(int idx)
{
    if((unsigned int)idx >= logRecords.size() ) return false;

    std::ofstream* fout = logRecords.at(idx).fileObject;

    logRecords.erase(logRecords.begin() + idx);

    if(fout)
    {
        fout->close();
        delete fout;
    }

    numLogDevices--;
    return true;
}

bool DataLogger::stopAllLogs()
{
    std::lock_guard<std::mutex> lk(resMutex);
    clearRecords();

    return true;
}

bool DataLogger::logDevice(int idx)
{
    FxDevicePtr dev = devProvider->getDevicePtr(logRecords.at(idx).devId);
    if(!dev) return false;

    auto fids = dev->getActiveFieldIds();
    if(fids.size() < 1) return true;

    LogRecord& record = logRecords.at(idx);
    int32_t ts = logRecords.at(idx).lastTimestamp;

    std::vector<uint32_t> stamps;
    std::vector<std::vector<int32_t>> data;
    ts = dev->getDataAfterTime(ts, stamps, data);
    logRecords.at(idx).lastTimestamp = ts;

    // if stamps and data mismatch in size, we have some kind of problem
    if(stamps.size() != data.size())
        return false;

    // if the record's active field num is different than current active field num
    // this indicates that we started a log file immediately after sending a configuration command
    // and we didn't receive configuration response until after starting the log file
    // in the spirit of being tolerant of async work flows, we just swap to a new file
    if(record.numActiveFields != fids.size())
    {
        std::string nextFileName;
        if(record.fileObject)
            nextFileName = generateFileName(dev, std::to_string(++logRecords.at(idx).logFileSplitIndex));
        else
            nextFileName = generateFileName(dev);

        std::cout << "Swapping files to new name: " << nextFileName << std::endl;
        swapFileObject(logRecords.at(idx), nextFileName, dev);
    }

    std::ofstream *fout = logRecords.at(idx).fileObject;

    if(fout && fids.size())
    {
        for(unsigned int line = 0; line < stamps.size(); line++)
        {
            (*fout) << stamps.at(line);

            auto&& dataline = data.at(line);
            for(auto&& fid : fids)
            {
                (*fout) << "," << dataline.at(fid);
            }
            if(record.logAdditionalField)
            {
                for(auto&& l : additionalColumnValues)
                    (*fout) << "," << l;
            }

            (*fout) << "\n";
        }

        logRecords.at(idx).logFileSize += stamps.size();

        fout->flush();
    }

    return true;
}

bool DataLogger::createSessionFolder(std::string session_name)
{
    // current date/time based on current system
    time_t now = time(0);
	if(session_name == "")
	{
		// convert now to string form and format properly
		struct tm * timeinfo = localtime(&now);
		char dt[80];
		strftime (dt,80,"%Y-%m-%d_%Hh%Mm%Sss", timeinfo);
		std::string str(dt);
		str.erase(str.end() - 1);
		replace(str.begin(), str.end(), ' ', '_');
		replace(str.begin(), str.end(), ':', '.');
		session_name = str;
	}
	_sessionPath = _logFolderPath + "\\" + session_name;

	bool folder_was_created = createFolder(_sessionPath);
	// Change global variable to avoid having the file path reset
	if(folder_was_created)
	{
		isFirstLogFile = false;
	}
	return folder_was_created;
}

void DataLogger::clearRecords()
{
    for(auto&& r : logRecords)
    {
        if(r.fileObject)
        {
            if(r.fileObject->is_open())
                r.fileObject->close();

            delete r.fileObject;
        }
    }

    logRecords.clear();
    numLogDevices = 0;
}

bool isIllegalFileChar(char c)
{
    return c == '\n' || c == '\t';
}

std::string DataLogger::generateFileName(FxDevicePtr dev, std::string suffix)
{
    std::stringstream ss;
    ss << dev->getName() << "_id_" << dev->getShortId() << "_" << dev->id;

    if(suffix.compare("") != 0)
        ss << "_" << suffix;

    ss << ".csv";

    std::string result = ss.str();

    // current date/time based on current system
    time_t now = time(0);
    // convert now to string form and prepend date and time to file name.
    struct tm * timeinfo = localtime(&now);
    char dt[80];
    strftime (dt,80,"%Y-%m-%d_%Hh%Mm%Ss_", timeinfo);

    result.insert(0,dt);

    // replace spaces and : with _
    std::replace(result.begin(), result.end(), ' ', '_');
    std::replace(result.begin(), result.end(), ':', '_');

    // remove invalid characters
    result.erase( std::remove_if(result.begin(), result.end(), isIllegalFileChar),
                result.end());
    result.insert(0, "/");
    result.insert(0, _sessionPath);

    return result;
}

bool DataLogger::createFolder(std::string path)
{
   bool success = false;
   std::string pathOK = path;
#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
    std::replace(pathOK.begin(), pathOK.end(), '/', '\\');

    CreateDirectoryA(pathOK.c_str(), NULL);

    int status = GetLastError();
    if(status == ERROR_ALREADY_EXISTS || status == 0) success = true;

#elif __linux__
    std::replace(pathOK.begin(), pathOK.end(), '\\', '/');

#ifdef __linux__
	mkdir(pathOK.c_str(), 777); // Is this too permissive?
#else
	_mkdir(pathOK.c_str());
#endif

    if(errno == EEXIST || errno == 0) success = true;

#else
#   error "Unknown compiler"
#endif
   if(success) std::cout << "Folder created : " << pathOK << std::endl;

   return success;
}



bool DataLogger::DataLogger::loadLogFolderConfig()
{
    const int MAX = 256;
    char temp[MAX];
    bool success = false;

    std::ifstream fin;
    fin.open(LOG_FOLDER_CONFIG_FILE, std::ifstream::in);

    if(!fin.is_open()) return success;

    fin.getline(temp, MAX);
    fin.close();

    success = createFolder(temp);

    if(success)
    {
        _logFolderPath = temp;
    }
    return success;
}

void DataLogger::saveLogFolderConfig()
{
    std::ofstream fout;
    fout.open(LOG_FOLDER_CONFIG_FILE, std::ios::trunc);

    if(!fout.is_open()) return;

    fout << _logFolderPath;
    fout.close();
}

/*
** This routine is called via the periodic task mechanism.
*/
void DataLogger::serviceLogs()
{
    std::lock_guard<std::mutex> lk(resMutex);

    if(logRecords.size() != (unsigned int)numLogDevices)
    {    // something very wrong
        clearRecords();
        return;
    }

    // log each device we have a record for
    // if the logging fails, remove the record
    for(int i = 0; i < numLogDevices; i++)
    {
        if(!logDevice(i))
            removeRecord(i);
    }

    // for each record, we can check the new file size
    // switch to a new log file if we have gone over the max number of lines
    for(auto &r : logRecords)
    {
        if(r.logFileSize > MAX_LOG_SIZE)
        {
            FxDevicePtr dev = devProvider->getDevicePtr(r.devId);
            std::string nextFileName = generateFileName(dev, std::to_string(++r.logFileSplitIndex));
            swapFileObject(r, nextFileName, dev);
        }
    }
}

unsigned int DataLogger::writeLogHeader(std::ofstream* fout, const FxDevicePtr dev, bool logAdditionalColumnsInit)
{
    std::vector<std::string> fieldLabels = dev->getActiveFieldLabels();
    (*fout) << "timestamp";
    for(auto&& l : fieldLabels)
        (*fout) << "," << l;
    if(logAdditionalColumnsInit)
    {
        for(auto&& l : additionalColumnLabels)
            (*fout) << "," << l;
    }

    (*fout) << "\n";
    fout->flush();

    return fieldLabels.size();
}


void DataLogger::swapFileObject(LogRecord &record, std::string newFileName, const FxDevicePtr dev)
{
    // get rid of the old file object
    if(record.fileObject)
    {
        record.fileObject->close();
        delete record.fileObject;
    }

    // generate the new file object
    std::replace(newFileName.begin(), newFileName.end(), '\\', '/');
    record.fileObject = new std::ofstream(newFileName);
    record.numActiveFields = writeLogHeader(record.fileObject, dev, record.logAdditionalField);
    record.logFileSize = 0;
}

bool DataLogger::wakeFromLongSleep()
{
    return numLogDevices > 0;
}

bool DataLogger::goToLongSleep()
{
    return numLogDevices < 1;
}
