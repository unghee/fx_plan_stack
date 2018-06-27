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
        ts = dev->getData(dev->dataCount()-1, nullptr, 0);
    }

    std::vector<std::string> fieldLabels = dev->getActiveFieldLabels();
    (*fout) << "timestamp";
    for(auto&& l : fieldLabels)
        (*fout) << ", " << l;

    (*fout) << "\n";
    fout->flush();

    {
        std::lock_guard<std::mutex> lk(resMutex);
        fileObjects.push_back(fout);
        loggedDevices.push_back(devId);
        timestamps.push_back(ts);
        numLogDevices++;
    }

    return true;
}

bool DataLogger::stopLogging(int devId)
{
    std::lock_guard<std::mutex> lk(resMutex);

    unsigned int i=0;
    while( i < loggedDevices.size() && loggedDevices.at(i) != devId ) i++;

    if(i >= loggedDevices.size() ) return false;

    loggedDevices.erase(loggedDevices.begin() + i);
    timestamps.erase(timestamps.begin() + i);

    std::ofstream* fout = fileObjects.at(i);
    if(fout)
    {
        fout->close();
        delete fout;
    }
    fileObjects.erase(fileObjects.begin() + i);
    numLogDevices--;

    return true;
}

bool DataLogger::stopAllLogs()
{
    std::lock_guard<std::mutex> lk(resMutex);
    clearRecords();

    return true;
}

void DataLogger::logDevice(int idx)
{
    FxDevicePtr dev = devProvider->getDevicePtr(loggedDevices.at(idx));

    int nf = dev->getNumActiveFields();
    if(nf < 1) return;

    int32_t ts = timestamps.at(idx);

    std::vector<uint32_t> stamps;
    std::vector<std::vector<int32_t>> data;
    ts = dev->getDataAfterTime(ts, stamps, data);
    timestamps.at(idx) = ts;

    if(stamps.size() != data.size())
        return; // TODO: exception

    std::ofstream *fout = fileObjects.at(idx);

    for(unsigned int line = 0; line < stamps.size(); line++)
    {
        (*fout) << stamps.at(line);

        auto&& dataline = data.at(line);
        for(auto&& d : dataline)
        {
            (*fout) << ", " << d;
        }
        (*fout) << "\n";
    }

    fout->flush();
}

void DataLogger::clearRecords()
{
    loggedDevices.clear();

    for(auto f : fileObjects)
    {
        if(f->is_open())
            f->close();
        delete f;
    }

    fileObjects.clear();
    timestamps.clear();
    numLogDevices = 0;
}

bool isIllegalFileChar(char c)
{
    return c == '\\' || c == '\n' || c == '\t' || c == ' ';
}

std::string DataLogger::generateFileName(FxDevicePtr dev)
{
    std::stringstream ss;
    std::time_t logStart = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << dev->getName() << "_id" << dev->id << "_" << std::ctime(&logStart) << ".csv";
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

    if(loggedDevices.size() != fileObjects.size() || loggedDevices.size() != timestamps.size() || loggedDevices.size() != (unsigned int)numLogDevices)
    {    // something very wrong
        clearRecords();
        return;
    }

    for(int i = 0; i < numLogDevices; i++)
        logDevice(i);
}

bool DataLogger::wakeFromLongSleep()
{
    return numLogDevices > 0;
}

bool DataLogger::goToLongSleep()
{
    return numLogDevices < 1;
}
