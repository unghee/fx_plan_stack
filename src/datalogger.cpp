#include "flexseastack/datalogger.h"
#include <chrono>
#include <sstream>
#include <string>
#include <algorithm>

DataLogger::DataLogger(FlexseaDeviceProvider* fdp) : devProvider(fdp), numLogDevices(0)
{}

bool DataLogger::startLogging(int devId)
{
    FxDevicePtr dev = devProvider->getDevicePtr(devId);
    if(!dev || dev->type == FX_NONE) return false;

    std::string fileName = generateFileName(dev);
    std::ofstream* fout = new std::ofstream(fileName);

    if(!fout || !fout->is_open())
    {
        if(fout) delete fout;
        fout = nullptr;
        return false;
    }

    unsigned int ts = 0;
    if(dev->dataCount())
    {
        ts = dev->getLatestTimestamp();
    }

    std::vector<std::string> fieldLabels = dev->getActiveFieldLabels();
    (*fout) << "timestamp";
    for(auto&& l : fieldLabels)
        (*fout) << ", " << l;

    (*fout) << "\n";
    fout->flush();

    {
        std::lock_guard<std::mutex> lk(resMutex);
        logRecords.push_back( {devId, fout, ts, 0, 0} );
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

    auto fids = dev->getActiveFieldIds();
    if(fids.size() < 1) return false;

    int32_t ts = logRecords.at(idx).lastTimestamp;

    std::vector<uint32_t> stamps;
    std::vector<std::vector<int32_t>> data;
    ts = dev->getDataAfterTime(ts, stamps, data);
    logRecords.at(idx).lastTimestamp = ts;

    if(stamps.size() != data.size())
        return false;

    std::ofstream *fout = logRecords.at(idx).fileObject;

    for(unsigned int line = 0; line < stamps.size(); line++)
    {
        (*fout) << stamps.at(line);

        auto&& dataline = data.at(line);
        for(auto&& fid : fids)
        {
            (*fout) << ", " << dataline.at(fid);
        }
        (*fout) << "\n";
    }

    logRecords.at(idx).logFileSize += stamps.size();

    fout->flush();

    return true;
}

void DataLogger::clearRecords()
{
    for(auto&& r : logRecords)
    {
        if(r.fileObject->is_open())
            r.fileObject->close();
        delete r.fileObject;
    }

    logRecords.clear();
    numLogDevices = 0;
}

bool isIllegalFileChar(char c)
{
    return c == '\\' || c == '\n' || c == '\t' || c == ' ';
}

std::string DataLogger::generateFileName(FxDevicePtr dev, std::string suffix)
{
    std::stringstream ss;
    std::time_t logStart = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << dev->getName() << "_id" << dev->id << "_" << std::ctime(&logStart);

    if(suffix.compare("") != 0)
        ss << "_" << suffix;

    ss << ".csv";

    std::string result = ss.str();

    // replace spaces and : with _
    std::replace(result.begin(), result.end(), ' ', '_');
    std::replace(result.begin(), result.end(), ':', '_');

    // remove invalid characters
    result.erase( std::remove_if(result.begin(), result.end(), isIllegalFileChar),
                result.end());

    return result;
}

void DataLogger::serviceLogs()
{
    std::lock_guard<std::mutex> lk(resMutex);

    if(logRecords.size() != (unsigned int)numLogDevices)
    {    // something very wrong
        clearRecords();
        return;
    }

    for(int i = 0; i < numLogDevices; i++)
    {
        if(!logDevice(i))
            removeRecord(i);
    }

    for(auto &r : logRecords)
    {
        if(r.logFileSize > MAX_LOG_SIZE)
        {
            FxDevicePtr dev = devProvider->getDevicePtr(r.devId);

            std::string nextFileName = generateFileName(dev, std::to_string(++r.logFileSplitIndex));
            r.fileObject->close();
            delete r.fileObject;
            r.fileObject = new std::ofstream(nextFileName);
            r.logFileSize = 0;
        }
    }

}

bool DataLogger::wakeFromLongSleep()
{
    return numLogDevices > 0;
}

bool DataLogger::goToLongSleep()
{
    return numLogDevices < 1;
}
