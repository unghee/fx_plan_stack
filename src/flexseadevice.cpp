#include "flexseastack/flexseadevice.h"
#include "cstring"
#include "flexseastack/flexsea-system/inc/flexsea_device_spec.h"


FlexseaDevice::FlexseaDevice(int _id, int _port, FlexseaDeviceType _type, int dataBuffSize):
    id(_id)
    , port(_port)
    , type(_type)
    , numFields( deviceSpecs[_type].numFields )
{
    data = new circular_buffer<FX_DataPtr>(dataBuffSize);
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
const std::vector<std::string>& FlexseaDevice::getFieldLabels() const
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

    while(fieldId < 32*FX_BITMAP_WIDTH && fieldId < deviceSpecs[this->type].numFields)
    {
        if(IS_FIELD_HIGH(fieldId, this->bitmap))
        {
            lastRequest.push_back( deviceSpecs[this->type].fieldLabels[fieldId] );
        }

        fieldId++;
    }
    for(int i = 0; i < FX_BITMAP_WIDTH; i++)
        lastFieldLabelMap[i] = this->bitmap[i];

    return lastRequest;
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
        return (deviceSpecs[this->type].fieldLabels[0]);

    return  "";
}

void FlexseaDevice::getBitmap(uint32_t* out) const {
    memcpy(out, bitmap, FX_BITMAP_WIDTH*sizeof(uint32_t));
}

void FlexseaDevice::setBitmap(uint32_t* in) {
    memcpy(bitmap, in, FX_BITMAP_WIDTH*sizeof(uint32_t));
}
