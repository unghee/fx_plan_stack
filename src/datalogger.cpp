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

bool DataLogger::sessionInitialized = false;


// Initialization of static variables
std::mutex DataLogger::_additionalValuesLock;
std::vector<std::string> DataLogger::_additionalColumnLabels;
std::vector<int> DataLogger::_additionalColumnValues;
std::atomic<int> DataLogger::_folderNumber(0);
std::mutex DataLogger::_folderLock;
std::string DataLogger::_logFolderPath;
std::string DataLogger::_sessionPath;

DataLogger::DataLogger(bool logAdditionalField, FlexseaDevice* flexseaDevice):  logAdditionalField(logAdditionalField),
                                                                                flexseaDevice(flexseaDevice)
{
    if(!DataLogger::sessionInitialized){
        std::cerr << "session has not been initialized yet, please call createSessionFolder() first" << std::endl;
    }
    logFileSplitIndex = 0;
    logFileSize = 0;
    initialized = false;
    initLogging();
}

DataLogger::~DataLogger()
{
    fileObject->flush();
    fileObject->close();
    delete fileObject;
}

bool DataLogger::createSessionFolder(std::string session_name)
{
    // current date/time based on current system
    time_t now = time(0);
    if(session_name == ""){
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

    std::string sessionPath =  _logFolderPath + "\\" + session_name;
    bool folder_was_created = createFolder(sessionPath);
    assert(folder_was_created);

    // Only update on success
    if(folder_was_created){
        // std::unique_lock<std::shared_timed_mutex> lk(rwFolderLock);
        std::unique_lock<std::mutex> lk(_folderLock);
        _folderNumber++;
        _sessionPath = sessionPath;
    }   
    //session folder was successfully initialized, can begin logging
    // sessionInitialized = folder_was_created;

    return folder_was_created;
}

bool DataLogger::setLogFolder(std::string folderPath)
{
    bool success = createFolder(folderPath);
    assert(success);

    // Only update on success
    if(success){
        // std::unique_lock<std::shared_timed_mutex> lk(rwFolderLock);
        std::unique_lock<std::mutex> lk(_folderLock);
        _logFolderPath = folderPath;
        saveLogFolderConfig();
    }
    _sessionPath = _logFolderPath;
    sessionInitialized = success;
    return success;
}

bool DataLogger::setDefaultLogFolder()
{
    setLogFolder(DEFAULT_LOG_FOLDER);
}

void DataLogger::setColumnValue(unsigned col, int val)
{
    std::unique_lock<std::mutex> lk(_additionalValuesLock);
    _additionalColumnValues.at(col) = val;
}

void DataLogger::setAdditionalColumn(std::vector<std::string> addLabel, std::vector<int> addValue)
{
    std::unique_lock<std::mutex> lk(_additionalValuesLock);
    _additionalColumnLabels = addLabel;
    _additionalColumnValues = addValue;
}

void DataLogger::initLogging()
{
    std::string filename = generateFileName();
    fileObject = new std::ofstream(filename);
    if(!fileObject->is_open()){
        delete fileObject;
        fileObject = nullptr;
        throw std::bad_alloc();
    }
    std::cout << "Log filename: " + filename << std::endl;
    writeLogHeader();

    lastTimestamp = 0;
    if(flexseaDevice->dataCount()){
        lastTimestamp = flexseaDevice->getLatestTimestamp();
    }
    numActiveFields = flexseaDevice->getNumActiveFields();

    initialized = true;
}

void DataLogger::logDevice()
{
    if(!initialized){
        std::cerr << "DataLogger for this device has not been initialized" << std::endl;
    }

    if(logFileSize > MAX_LOG_SIZE || (folderNumber != _folderNumber)){
        std::string nextFileName = generateFileName(std::to_string(++logFileSplitIndex));
        changeFileName(nextFileName);
    }

    auto fieldIds = flexseaDevice->getActiveFieldIds();

    if(fieldIds.empty()){ // No active fields, logging for this device is done
        return;
    }

    std::vector<uint32_t> timestampOutput;
    std::vector<std::vector<int32_t>> data;
    lastTimestamp = flexseaDevice->getDataAfterTime(lastTimestamp, timestampOutput, data);

    // If timestampOutput and data mismatch in size, we have some kind of problem
    assert(timestampOutput.size() == data.size());

    for(unsigned int line = 0; line < timestampOutput.size(); line++){
        (*fileObject) << timestampOutput.at(line);

        auto&& dataline = data.at(line);
        for(auto&& fid : fieldIds){
            (*fileObject) << ", " << dataline.at(fid);
        }

        if(logAdditionalField){
            std::unique_lock<std::mutex> lk(_additionalValuesLock);
            for(auto&& l : _additionalColumnValues){
                (*fileObject) << ", " << l;
            }
        }

        (*fileObject) << "\n";
    }

    logFileSize += timestampOutput.size();
    fileObject->flush();
}

//We should revisit this function
bool DataLogger::createFolder(std::string path)
{
   bool success = false;
   std::string pathOK = path;

#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
    std::replace(pathOK.begin(), pathOK.end(), '/', '\\');

    CreateDirectoryA(pathOK.c_str(), NULL);

    int status = GetLastError();
    if(status == ERROR_ALREADY_EXISTS || status == 0) {
        success = true;
    }

#elif __linux__
    std::replace(pathOK.begin(), pathOK.end(), '\\', '/');

    mkdir(pathOK.c_str(), ACCESSPERMS); // Is this too permissive?
    // #ifdef __linux__
    // #else
    //     _mkdir(pathOK.c_str());
    // #endif

    if(errno == EEXIST || errno == 0) {
        success = true;
    }

#else
#   error "Unknown compiler"
#endif
    if(success){
        std::cout << "Folder created : " << pathOK << std::endl;
    }

    return success;
}

bool isIllegalFileChar(char c)
{
    return c == '\n' || c == '\t';
}

std::string DataLogger::generateFileName(std::string suffix)
{
    std::stringstream ss;
    ss << flexseaDevice->getName() << "_id_" << flexseaDevice->getShortId() << "_" << flexseaDevice->_devId;

    if(suffix.compare("") != 0){
        ss << "_" << suffix;
    }

    ss << ".csv";

    std::string filename = ss.str();

    // current date/time based on current system
    time_t now = time(0);

    // convert now to string form and prepend date and time to file name.
    struct tm * timeinfo = localtime(&now);
    char dt[80];
    strftime (dt,80,"%Y-%m-%d_%Hh%Mm%Ss_", timeinfo);

    filename.insert(0,dt);

    // replace spaces and : with _
    std::replace(filename.begin(), filename.end(), ' ', '_');
    std::replace(filename.begin(), filename.end(), ':', '_');

    // remove invalid characters
    filename.erase( std::remove_if(filename.begin(), filename.end(), isIllegalFileChar),
                filename.end());
    filename.insert(0, "/");

    {
        // std::shared_lock<std::shared_timed_mutex> lk(rwFolderLock);
        std::unique_lock<std::mutex> lk(_folderLock);
        folderNumber = _folderNumber; //Synchronizes local folderNumber with global folder number 
        filename.insert(0, _sessionPath);
    }

    std::replace(filename.begin(), filename.end(), '\\', '/'); //May be able to remove this lines

    return filename;
}

bool DataLogger::loadLogFolderConfig()
{
    char tempPath[MAX_PATH_LENGTH];
    bool success;

    std::ifstream fin;
    fin.open(LOG_FOLDER_CONFIG_FILE, std::ifstream::in);

    assert(fin.is_open());

    fin.getline(tempPath, MAX_PATH_LENGTH);
    fin.close();

    success = createFolder(tempPath);
    assert(success);

    if(success){
        // std::unique_lock<std::shared_timed_mutex> lk(rwFolderLock);
        std::unique_lock<std::mutex> lk(_folderLock);
        _logFolderPath = tempPath;
    }

    return success;
}

void DataLogger::saveLogFolderConfig()
{
    std::ofstream fout;
    fout.open(LOG_FOLDER_CONFIG_FILE, std::ios::trunc);

    if(!fout.is_open()){
        return;
    }

    fout << _logFolderPath;
    fout.close();
}


void DataLogger::writeLogHeader()
{
    std::vector<std::string> fieldLabels = flexseaDevice->getActiveFieldLabels();
    (*fileObject) << "timestamp";
    for(auto&& l : fieldLabels){
        (*fileObject) << ", " << l;
    }

    if(logAdditionalField){
        std::unique_lock<std::mutex> lk(_additionalValuesLock);
        for(auto&& l : _additionalColumnLabels){
            (*fileObject) << ", " << l;
        }
    }

    (*fileObject) << "\n";
    fileObject->flush();

    return;
}


void DataLogger::changeFileName(std::string newFileName)
{
    // get rid of the old file object
    fileObject->close();
    delete fileObject;

    // generate the new file object
    std::replace(newFileName.begin(), newFileName.end(), '\\', '/');
    fileObject = new std::ofstream(newFileName);
    
    writeLogHeader();
    logFileSize = 0;
}

