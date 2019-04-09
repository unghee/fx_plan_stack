#include "flexseadevice.h"
#include "cstring"
#include "flexsea_device_spec.h"

#include <iostream>

FlexseaDevice::FlexseaDevice(int _id, int _port, FlexseaDeviceType _type, int role, int dataBuffSize):
	id(_id)
	, port(_port)
	, type(_type)
	, numFields( deviceSpecs[_type].numFields )
	, dataMutex(&_dataMutex)
	, shortId(id)
	, _role(role)
	, _data(dataBuffSize, deviceSpecs[_type].numFields + 1 )
{
	memset(this->bitmap, 0, FX_BITMAP_WIDTH * sizeof(uint32_t));

	//Debug only:
	printf("numFields = %i\n", numFields);
	
	for(int i = 0; i < numFields; ++i)
	{
		const char* c_str = deviceSpecs[_type].fieldLabels[i];
		if(c_str)
			fieldLabels.push_back(c_str);
		else
			throw std::invalid_argument("Device Spec for given type is invalid, causing null pointer access");
	 }
}

FlexseaDevice::FlexseaDevice(int _id, int _shortid, int _port, FlexseaDeviceType _type, int role, int dataBuffSize):
	id(_id)
	, port(_port)
	, type(_type)
	, numFields( deviceSpecs[_type].numFields )
	, dataMutex(&_dataMutex)
	, shortId(_shortid)
	, _role(role)
	, _data(dataBuffSize, deviceSpecs[_type].numFields + 1 )
{
	memset(this->bitmap, 0, FX_BITMAP_WIDTH * sizeof(uint32_t));
	
	//Debug only:
	printf("numFields B = %i\n", numFields);

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
	, dataMutex(&_dataMutex)
	, shortId(id)
	, _role(role)
	, fieldLabels(fieldLabels)
	, _data( dataBuffSize, fieldLabels.size() + 1 )
{
	memset(this->bitmap, 0, FX_BITMAP_WIDTH * sizeof(uint32_t));
}

/* Returns a vector of strings which describe the fields specified by map  */
std::vector<std::string> FlexseaDevice::getActiveFieldLabels() const
{
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

uint32_t FlexseaDevice::getData(int* fieldIds, int32_t* output, uint16_t outputSize)
{
	return getData(fieldIds, output, outputSize, dataCount() - 1);
}

uint32_t FlexseaDevice::getData(int* fieldIds, int32_t* output, uint16_t outputSize, int index)
{
	if(index < 0 || (unsigned int)index >= dataCount()) return 0;

	std::lock_guard<std::recursive_mutex> lk(*dataMutex);

	int32_t *ptr = ((int32_t*)_data.peek(index));

	int outIdx = 0;
	for(int i = 0; i < outputSize; ++i)
	{
		int field = fieldIds[i];

		if( (field >= 0) && (field < numFields) && IS_FIELD_HIGH(field, bitmap)  )
		{
			output[outIdx] = ptr[ 1 + field ];
			fieldIds[outIdx] = field;
			outIdx++;
		}

	}

	return ptr[0];
}

uint32_t FlexseaDevice::getDataPtr(uint32_t index, FX_DataPtr ptr, uint16_t outputSize) const
{
	int32_t *srcPtr = 0;
	try
	{
		if(index >= dataCount())
		{
			throw InvalidIndex();
		}
		srcPtr = ((int32_t*)_data.peek(index));
		if(!srcPtr)
		{
			throw InaccessiblePointer();
		}

		int s = outputSize >  (1 + numFields) ? (1 + numFields) : outputSize;
		size_t sizeData =  s  * sizeof(int32_t);
		memcpy(ptr, srcPtr, sizeData);
	}
	catch(InvalidIndex& e)
	{
		std::cout << e.what() << std::endl;
		return 0;	
	}
	catch(InaccessiblePointer& e)
	{
		std::cout << e.what() << std::endl;
		return 0;
	}

	return srcPtr[0];
}

uint32_t FlexseaDevice::getLatestTimestamp() const
{
	if(_data.count())
		return _data.peekBack()[0];

	return 0;
}

uint16_t FlexseaDevice::getIndexAfterTime(uint32_t timestamp) const
{
	std::lock_guard<std::recursive_mutex> lk(*this->dataMutex);

	size_t lb = 0, ub = _data.count();
	size_t i = ub/2;
	uint32_t t = _data.peek(i)[0];

	while(i != lb && lb != ub)
	{
		if(timestamp > t)
			lb = i;             //go right
		else
			ub = i;             //go left

		i = (lb + ub) / 2;
		t = _data.peek(i)[0];
	}

	return i + (t >= timestamp);
}

//uint32_t FlexseaDevice::getDataAfterTime(uint32_t timestamp, uint32_t *output, uint16_t outputSize) const
//{
//    size_t i = 0, j = 0, sizeData = (this->numFields+1)*sizeof(int32_t);

//    std::lock_guard<std::recursive_mutex> lk(*this->dataMutex);

//    while(i < _data.count() && _data.peek(i)[0] <= timestamp)
//        i++;

//    while(i < _data.count() && j < outputSize)
//    {
//        memcpy(output+j, _data.peek(i++), sizeData);
//        j+=numFields+1;
//    }

//    uint32_t last = _data.peek(i-1)[0];

//    return last;
//}

inline size_t FlexseaDevice::findIndexAfterTime(uint32_t timestamp) const
{
// ---- Linear search O(n)
//    size_t i=0;
//    while(i < _data.count() && _data.peek(i)[0] <= timestamp)
//        i++;

// ---- Binary search O(logn)

	size_t lb = 0, ub = _data.count();

	if(ub == 0) return 0;

	size_t i = ub / 2;
	uint32_t t = _data.peek(i)[0];

	while(i != lb && lb != ub)
	{
		if(timestamp >= t)
			lb = i;
		else
			ub = i;

		i = (lb + ub) / 2;
		t = _data.peek(i)[0];
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
	ts_output.reserve(_data.count() - i);
	data_output.clear();
	data_output.reserve(_data.count() - i);

	FX_DataPtr p = nullptr;
	while(i < _data.count())
	{
		p = _data.peek(i++);
		ts_output.push_back(p[0]);
		data_output.push_back(p[field+1]);
	}

	if(p)
		return p[0];
	else
		return timestamp;
}

uint32_t FlexseaDevice::getDataAfterTime(const std::vector<int> &fieldIds, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<std::vector<int32_t>> &data_output, unsigned max) const
{
	std::lock_guard<std::recursive_mutex> lk(*this->dataMutex);

	for(auto && field : fieldIds )
		if(!IS_FIELD_HIGH(field, this->bitmap)) return timestamp;

	size_t i = findIndexAfterTime(timestamp), j;
	size_t n = std::min((unsigned)(_data.count() - i), (unsigned)max);
	size_t nf = fieldIds.size();

	ts_output.clear();
	ts_output.reserve(n);
	data_output.clear();
	data_output.resize( nf );

	for(j = 0; j < nf; ++j)
		data_output.at(j).reserve(n);

	FX_DataPtr p = nullptr;
	while(i < _data.count() && --n)
	{
		p = _data.peek((int)i++);
		ts_output.push_back(p[0]);

		for(j = 0; j < nf; ++j)
			data_output.at(j).push_back(p[fieldIds.at(j)+1]);
	}

	if(p)
		return p[0];
	else
		return timestamp;
}


uint32_t FlexseaDevice::getDataAfterTime(const std::vector<int> &fieldIds, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<std::vector<int32_t>> &data_output) const
{
	return getDataAfterTime(fieldIds, timestamp, ts_output, data_output, -1);
}

uint32_t FlexseaDevice::getDataAfterTime(uint32_t timestamp, std::vector<uint32_t> &timestamps, std::vector<std::vector<int32_t>> &outputData) const
{
	size_t i = 0, sizeData = numFields * sizeof(int32_t);
	std::lock_guard<std::recursive_mutex> lk(*this->dataMutex);

	while(i < _data.count() && _data.peek(i)[0] <= timestamp)
		i++;

	timestamps.clear();
	timestamps.reserve(_data.count() - i);
	outputData.clear();
	outputData.reserve(_data.count() - i);

	FX_DataPtr p = nullptr;

	while(i < _data.count())
	{
		p = _data.peek(i++);
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
	if(_data.count() < AVG_OVER)
		return -1;

	size_t i = _data.count() - AVG_OVER;
	double avg_period = ((double)(_data.peekBack()[0] - _data.peek(i)[0])) / AVG_OVER;
	return 1000.0 / avg_period;
}

std::string FlexseaDevice::getName() const
{
	if(this->type < NUM_DEVICE_TYPES && this->type != FX_NONE)
		return ( fieldLabels.at(0) );
	else if(this->type == FX_CUSTOM)
		return ( "Custom Device" );

	return  "";
}

void FlexseaDevice::getBitmap(uint32_t* out) const {
	memcpy(out, bitmap, FX_BITMAP_WIDTH*sizeof(uint32_t));
}

void FlexseaDevice::setBitmap(uint32_t* in) {
	memcpy(bitmap, in, FX_BITMAP_WIDTH*sizeof(uint32_t));
}
