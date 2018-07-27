#include "flexseastack/flexseadevice.h"
#include "cstring"
#include "flexseastack/flexsea-system/inc/flexsea_device_spec.h"

#include <iostream>

FlexseaDevice::FlexseaDevice(int _id, int _port, FlexseaDeviceType _type, int role, int dataBuffSize):
    id(_id)
    , port(_port)
    , type(_type)
    , numFields( deviceSpecs[_type].numFields )
    , _role(role)
{
    data = new FxDevData(dataBuffSize);
    dataMutex = new std::recursive_mutex();
    memset(this->bitmap, 0, FX_BITMAP_WIDTH * sizeof(uint32_t));

    for(int i = 0; i < numFields; ++i)
    {
        const char* c_str = deviceSpecs[_type].fieldLabels[i];
        if(c_str)
            fieldLabels.push_back(c_str);
        else
            throw std::invalid_argument("Device Spec for given type is invalid, causing null pointer access");
     }
}

FlexseaDevice::FlexseaDevice(int _id, int _port, std::vector<std::string> fieldLabels, int role, int dataBuffSize)
    : id(_id), port(_port), type(FX_CUSTOM)
    , numFields(fieldLabels.size())
    , data( new FxDevData(dataBuffSize) )
    , _role(role)
    , fieldLabels(fieldLabels)

{
    dataMutex = new std::recursive_mutex();
    memset(this->bitmap, 0, FX_BITMAP_WIDTH * sizeof(uint32_t));
}

FlexseaDevice::~FlexseaDevice()
{
    //deallocate the circular buffer
    dataMutex->lock();

    FX_DataPtr dataptr;
    while(data->count())
    {
        dataptr = data->get();
        if(dataptr)
            delete dataptr;
    }

    delete data;
    data=nullptr;

    delete dataMutex;
    dataMutex=nullptr;
}

/* Returns a vector of strings which describe the fields specified by map  */
std::vector<std::string> FlexseaDevice::getActiveFieldLabels() const
{
    /* make these private not static */
    bool equal = true;
    for(int i = 0; i < FX_BITMAP_WIDTH && equal; i++)
    {
        if(lastFieldLabelMap[i] != this->bitmap[i])
            equal = false;
    }

    if(equal)
        return lastRequest;

    uint16_t fieldId = 0;
    lastRequest.clear();

    while(fieldId < 32*FX_BITMAP_WIDTH && fieldId < numFields)
    {
        if(IS_FIELD_HIGH(fieldId, this->bitmap))
        {
            lastRequest.push_back( fieldLabels.at(fieldId) );
        }

        fieldId++;
    }
    for(int i = 0; i < FX_BITMAP_WIDTH; i++)
        lastFieldLabelMap[i] = this->bitmap[i];

    return lastRequest;
}

std::vector<int> FlexseaDevice::getActiveFieldIds() const
{
    std::vector<int> r;
    r.reserve(numFields);
    for(int fieldId = 0; fieldId < numFields; ++fieldId)
    {
        if(IS_FIELD_HIGH(fieldId, this->bitmap))
            r.push_back( fieldId );
    }

    return r;
}

std::vector<std::string> FlexseaDevice::getAllFieldLabels() const
{
    return fieldLabels;
}

uint32_t FlexseaDevice::getLastData(int32_t *output, uint16_t outputSize)
{
    return getData(data->count() - 1, output, outputSize);
}

uint32_t FlexseaDevice::getData(uint32_t index, int32_t *output, uint16_t outputSize) const
{
    /*  Not a real implementation just for dev/testing purposes
    */
    if(index >= dataCount()) return 0;

    std::lock_guard<std::recursive_mutex> lk(*dataMutex);

    uint16_t fieldId = 0;
    uint16_t i = 0;
    int32_t *ptr = ((int32_t*)data->peek(index));

    if(!ptr) return 0;

    while(fieldId < 32*FX_BITMAP_WIDTH && i < outputSize && (fieldId) < numFields)
    {
        if(IS_FIELD_HIGH(fieldId, bitmap))
        {
            output[i++] = ptr[1+fieldId];
        }

        fieldId++;
    }

    return ptr[0];
}

uint32_t FlexseaDevice::getDataPtr(uint32_t index, FX_DataPtr ptr, uint16_t outputSize) const
{
    if(index >= dataCount())
    {
        std::cout << "Invalid index requested.." << std::endl;
        return 0;
    }
    int32_t *srcPtr = ((int32_t*)data->peek(index));
    if(!srcPtr)
    {
        std::cout << "Error accessing data ptr" << std::endl;
        return 0;
    }

    int s = outputSize >  (1 + numFields) ? (1 + numFields) : outputSize;
    size_t sizeData =  s  * sizeof(int32_t);
    memcpy(ptr, srcPtr, sizeData);

    return srcPtr[0];
}

uint32_t FlexseaDevice::getLatestTimestamp() const
{
    if(data->count())
        return data->peekBack()[0];

    return 0;
}

uint16_t FlexseaDevice::getIndexAfterTime(uint32_t timestamp) const
{
    std::lock_guard<std::recursive_mutex> lk(*this->dataMutex);

    size_t lb = 0, ub = data->count();
    size_t i = ub/2;
    uint32_t t = data->peek(i)[0];

    while(i != lb && lb != ub)
    {
        if(timestamp > t)
            lb = i;             //go right
        else
            ub = i;             //go left

        i = (lb + ub) / 2;
        t = data->peek(i)[0];
    }

    return i + (t >= timestamp);
}

//uint32_t FlexseaDevice::getDataAfterTime(uint32_t timestamp, uint32_t *output, uint16_t outputSize) const
//{
//    size_t i = 0, j = 0, sizeData = (this->numFields+1)*sizeof(int32_t);

//    std::lock_guard<std::recursive_mutex> lk(*this->dataMutex);

//    while(i < data->count() && data->peek(i)[0] <= timestamp)
//        i++;

//    while(i < data->count() && j < outputSize)
//    {
//        memcpy(output+j, data->peek(i++), sizeData);
//        j+=numFields+1;
//    }

//    uint32_t last = data->peek(i-1)[0];

//    return last;
//}

inline size_t FlexseaDevice::findIndexAfterTime(uint32_t timestamp) const
{
// ---- Linear search O(n)
//    size_t i=0;
//    while(i < data->count() && data->peek(i)[0] <= timestamp)
//        i++;

// ---- Binary search O(logn)

    size_t lb = 0, ub = data->count();

    if(ub == 0) return 0;

    size_t i = ub / 2;
    uint32_t t = data->peek(i)[0];

    while(i != lb && lb != ub)
    {
        if(timestamp >= t)
            lb = i;
        else
            ub = i;

        i = (lb + ub) / 2;
        t = data->peek(i)[0];
    }

    return i + (timestamp >= t);

// ---- std lib implementation ? would have to implement begin and end iterators.. one day!

}

uint32_t FlexseaDevice::getDataAfterTime(int field, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<int32_t> &data_output) const
{
    std::lock_guard<std::recursive_mutex> lk(*this->dataMutex);

    if(!IS_FIELD_HIGH(field, this->bitmap)) return timestamp;

    size_t i = findIndexAfterTime(timestamp);

    ts_output.clear();
    ts_output.reserve(data->count() - i);
    data_output.clear();
    data_output.reserve(data->count() - i);

    FX_DataPtr p = nullptr;
    while(i < data->count())
    {
        p = data->peek(i++);
        ts_output.push_back(p[0]);
        data_output.push_back(p[field+1]);
    }

    if(p)
        return p[0];
    else
        return timestamp;
}

uint32_t FlexseaDevice::getDataAfterTime(uint32_t timestamp, std::vector<uint32_t> &timestamps, std::vector<std::vector<int32_t>> &outputData) const
{
    size_t i = 0, sizeData = numFields * sizeof(int32_t);
    std::lock_guard<std::recursive_mutex> lk(*this->dataMutex);

    while(i < data->count() && data->peek(i)[0] <= timestamp)
        i++;

    timestamps.clear();
    timestamps.reserve(data->count() - i);
    outputData.clear();
    outputData.reserve(data->count() - i);

    FX_DataPtr p = nullptr;

    while(i < data->count())
    {
        p = data->peek(i++);
        timestamps.push_back(p[0]);
        outputData.emplace_back(numFields);
        memcpy(outputData.back().data(), p+1, sizeData);
    }

    if(p)
        return p[0];
    else
        return timestamp;
}

// looks awful but works
// https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
uint32_t numberOfSetBits(uint32_t i)
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

int FlexseaDevice::getNumActiveFields() const
{
    int count = 0;
    for(int i = 0; i < FX_BITMAP_WIDTH; i++)
        count += numberOfSetBits(this->bitmap[i]);

    return count;
}

double FlexseaDevice::getDataRate() const
{
    std::lock_guard<std::recursive_mutex> lk(*dataMutex);
    const int AVG_OVER = 10;
    if(data->count() < AVG_OVER)
        return -1;

    size_t i = data->count() - AVG_OVER;
    double avg_period = ((double)(data->peekBack()[0] - data->peek(i)[0])) / AVG_OVER;
    return 1000.0 / avg_period;
}

std::string FlexseaDevice::getName() const
{
    if(this->type < NUM_DEVICE_TYPES && this->type != FX_NONE)
        return ( fieldLabels.at(0) );

    return  "";
}

void FlexseaDevice::getBitmap(uint32_t* out) const {
    memcpy(out, bitmap, FX_BITMAP_WIDTH*sizeof(uint32_t));
}

void FlexseaDevice::setBitmap(uint32_t* in) {
    memcpy(bitmap, in, FX_BITMAP_WIDTH*sizeof(uint32_t));
}
