#include "flexseadevice.h"
#include "cstring"
#include "flexsea_device_spec.h"

#include <iostream>


FlexseaDevice::FlexseaDevice(int id, int port, FlexseaDeviceType type, int role, int dataBuffSize):
	_devId(id)
	, _portIdx(port)
	, _devType(type)
	, _numFields( deviceSpecs[type].numFields )
	, _shortId(_devId)
	, _role(role)
	, _data(dataBuffSize, deviceSpecs[type].numFields + 1 )
{
	memset(this->bitmap, 0, FX_BITMAP_WIDTH * sizeof(uint32_t));

	for(int i = 0; i < _numFields; ++i){
		const char* c_str = deviceSpecs[type].fieldLabels[i];
		if(c_str){
			fieldLabels.push_back(c_str);
		}
		else{
			throw std::invalid_argument("Device Spec for given type is invalid, causing null pointer access");
		}
	}
}

FlexseaDevice::FlexseaDevice(int id, int shortid, int port, FlexseaDeviceType type, int role, int dataBuffSize):
	_devId(id)
	, _portIdx(port)
	, _devType(type)
	, _numFields( deviceSpecs[type].numFields )
	, _shortId(shortid)
	, _role(role)
	, _data(dataBuffSize, deviceSpecs[type].numFields + 1 )
{
	memset(this->bitmap, 0, FX_BITMAP_WIDTH * sizeof(uint32_t));

	for(int i = 0; i < _numFields; ++i){
		const char* c_str = deviceSpecs[type].fieldLabels[i];
		if(c_str){
			fieldLabels.push_back(c_str);
		}
		else{
			throw std::invalid_argument("Device Spec for given type is invalid, causing null pointer access");
		}
	}

}

FlexseaDevice::FlexseaDevice(int id, int port, std::vector<std::string> fieldLabels, int role, int dataBuffSize)
	: _devId(id)
	, _portIdx(port)
	, _devType(FX_CUSTOM)
	, _numFields(fieldLabels.size())
	, _shortId(id)
	, _role(role)
	, fieldLabels(fieldLabels)
	, _data( dataBuffSize, fieldLabels.size() + 1 )
{
	memset(this->bitmap, 0, FX_BITMAP_WIDTH * sizeof(uint32_t));
}


bool FlexseaDevice::hasData() const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);
	return !_data.empty();
}

size_t FlexseaDevice::dataCount() const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);
	return _data.count(); 
}

/* Returns a vector of strings which describe the fields specified by map  */
std::vector<std::string> FlexseaDevice::getActiveFieldLabels() const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);

	bool equal = true;
	for(int i = 0; i < FX_BITMAP_WIDTH && equal; i++){
		if(lastFieldLabelMap[i] != this->bitmap[i]){
			equal = false;
		}
	}

	if(equal){
		return lastRequest;
	}

	uint16_t fieldId = 0;
	lastRequest.clear();

	while(fieldId < 32*FX_BITMAP_WIDTH && fieldId < _numFields){
		if(IS_FIELD_HIGH(fieldId, this->bitmap)){
			lastRequest.push_back( fieldLabels.at(fieldId) );
		}

		fieldId++;
	}
	for(int i = 0; i < FX_BITMAP_WIDTH; i++){
		lastFieldLabelMap[i] = this->bitmap[i];
	}

	return lastRequest;
}

std::vector<int> FlexseaDevice::getActiveFieldIds() const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);

	std::vector<int> r;
	r.reserve(_numFields);
	for(int fieldId = 0; fieldId < _numFields; ++fieldId)
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


// NOT THE BEST DESIGN TO CALL A GETTER WITHIN A MEMBER FUNCTION, CHANGE LATER
uint32_t FlexseaDevice::getData(int* fieldIds, int32_t* output, uint16_t outputSize)
{
	return getData(fieldIds, output, outputSize, dataCount() - 1);
}

uint32_t FlexseaDevice::getData(int* fieldIds, int32_t* output, uint16_t outputSize, int index)
{
	if(index < 0 || (unsigned int)index >= dataCount()) return 0;

	std::shared_lock<std::shared_timed_mutex> lk(dataLock);

	int32_t *dataPtr = ((int32_t*)_data.peek(index));

	int outIdx = 0;
	for(int i = 0; i < outputSize; ++i)
	{
		int field = fieldIds[i];

		assert(field >= 0 && field < _numFields); //just a sanity check
		// if( (field >= 0) && (field < _numFields) && IS_FIELD_HIGH(field, bitmap)  )
		if(IS_FIELD_HIGH(field, bitmap)  )
		{
			output[outIdx] = dataPtr[ 1 + field ];
			fieldIds[outIdx] = field;
			++outIdx;
		}
	}

	return dataPtr[0]; //why do we return the data ptr?
}

uint32_t FlexseaDevice::getDataPtr(uint32_t index, FX_DataPtr outPtr, uint16_t outputSize) const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);

	if(index == -1){
		index = _data.count() - 1;
	}

	int32_t *dataPtr = 0;
	try{
		if(index >= _data.count()){
			throw InvalidIndex();
		}
		dataPtr = ((int32_t*)_data.peek(index));
		if(!dataPtr){
			throw InaccessiblePointer();
		}

		int s = outputSize >  (1 + _numFields) ? (1 + _numFields) : outputSize;
		size_t sizeData =  s  * sizeof(int32_t);
		memcpy(outPtr, dataPtr, sizeData);
	}
	catch(InvalidIndex& e){
		std::cout << e.what() << std::endl;
		return 0;	
	}
	catch(InaccessiblePointer& e){
		std::cout << e.what() << std::endl;
		return 0;
	}

	return dataPtr[0]; //why do we return the data ptr?
}

uint32_t FlexseaDevice::getLatestTimestamp() const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);
	if(_data.count()){
		return _data.peekBack()[0];
	}

	return 0;
}

uint16_t FlexseaDevice::getIndexAfterTime(uint32_t timestamp) const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);
	return findIndexAfterTime(timestamp);
}

inline size_t FlexseaDevice::findIndexAfterTime(uint32_t timestamp) const
{
// ---- Binary search O(logn)
	size_t lb = 0, ub = _data.count();

	if(ub == 0){
		return 0;
	}

	size_t i = ub / 2;
	uint32_t t = _data.peek(i)[0];

	while(i != lb && lb != ub){
		if(timestamp >= t){
			lb = i;
		}
		else{
			ub = i;
		}

		i = (lb + ub) / 2;
		t = _data.peek(i)[0];
	}

	return i + (timestamp >= t);

// ---- std lib implementation ? would have to implement begin and end iterators.. one day!

}

uint32_t FlexseaDevice::getDataAfterTime(int field, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<int32_t> &data_output) const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);

	if(!IS_FIELD_HIGH(field, this->bitmap)) return timestamp;

	size_t i = findIndexAfterTime(timestamp);

	ts_output.clear();
	ts_output.reserve(_data.count() - i);
	data_output.clear();
	data_output.reserve(_data.count() - i);

	FX_DataPtr p = nullptr;
	while(i < _data.count()){
		p = _data.peek(i++);
		ts_output.push_back(p[0]);
		data_output.push_back(p[field+1]);
	}

	if(p){
		return p[0];
	}
	else{
		return timestamp;
	}
}

uint32_t FlexseaDevice::getDataAfterTime(const std::vector<int> &fieldIds, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<std::vector<int32_t>> &data_output, unsigned max) const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);

	for(auto && field : fieldIds ){
		if(!IS_FIELD_HIGH(field, this->bitmap)) return timestamp;
	}

	size_t i = findIndexAfterTime(timestamp);
	size_t j;
	size_t n = std::min((unsigned)(_data.count() - i), (unsigned)max);
	size_t nf = fieldIds.size();

	ts_output.clear();
	ts_output.reserve(n);
	data_output.clear();
	data_output.resize( nf );

	for(j = 0; j < nf; ++j){
		data_output.at(j).reserve(n);
	}

	FX_DataPtr p = nullptr;
	while(i < _data.count() && --n){
		p = _data.peek((int)i++);
		ts_output.push_back(p[0]);

		for(j = 0; j < nf; ++j){
			data_output.at(j).push_back(p[fieldIds.at(j)+1]);
		}
	}

	if(p){
		return p[0];
	}
	else{
		return timestamp;
	}
}


uint32_t FlexseaDevice::getDataAfterTime(const std::vector<int> &fieldIds, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<std::vector<int32_t>> &data_output) const
{
	// -1 ensures that std::min always returns (_data.count() - 1)
	// unsigned(-1) = std::numeric_limits(size_t)
	return getDataAfterTime(fieldIds, timestamp, ts_output, data_output, std::numeric_limits<unsigned>::max());
}

uint32_t FlexseaDevice::getDataAfterTime(uint32_t timestamp, std::vector<uint32_t> &timestamps, std::vector<std::vector<int32_t>> &outputData) const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);
	size_t i = 0, sizeData = _numFields * sizeof(int32_t);

	while(i < _data.count() && _data.peek(i)[0] <= timestamp){
		i++;
	}

	timestamps.clear();
	timestamps.reserve(_data.count() - i);
	outputData.clear();
	outputData.reserve(_data.count() - i);

	FX_DataPtr dataPtr = nullptr;

	while(i < _data.count()){
		dataPtr = _data.peek(i++);
		timestamps.push_back(dataPtr[0]);
		outputData.emplace_back(_numFields);
		memcpy(outputData.back().data(), dataPtr+1, sizeData);
	}

	if(dataPtr){
		return dataPtr[0];
	}
	else{
		return timestamp;
	}
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
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);
	int count = 0;
	for(int i = 0; i < FX_BITMAP_WIDTH; i++){
		count += numberOfSetBits(this->bitmap[i]);
	}

	return count;
}

double FlexseaDevice::getDataRate() const
{
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);
	const int AVG_OVER = 10;
	if(_data.count() < AVG_OVER){
		return -1;
	}

	size_t i = _data.count() - AVG_OVER;
	double avg_period = ((double)(_data.peekBack()[0] - _data.peek(i)[0])) / AVG_OVER;
	return 1000.0 / avg_period;
}

std::string FlexseaDevice::getName() const
{
	if(_devType < NUM_DEVICE_TYPES && _devType != FX_NONE){
		return ( fieldLabels.at(0) );
	}
	else if(_devType == FX_CUSTOM){
		return ( "Custom Device" );
	}
	else{
		return  "";
	}
}

void FlexseaDevice::getBitmap(uint32_t* out) const {
	std::shared_lock<std::shared_timed_mutex> lk(dataLock);
	memcpy(out, bitmap, FX_BITMAP_WIDTH*sizeof(uint32_t));
}

void FlexseaDevice::setBitmap(uint32_t* in) {
	std::unique_lock<std::shared_timed_mutex> lk(dataLock);
	memcpy(bitmap, in, FX_BITMAP_WIDTH*sizeof(uint32_t));
}

void FlexseaDevice::updateData(uint8_t *buf){
	std::unique_lock<std::shared_timed_mutex> lk(dataLock);

	FlexseaDeviceSpec ds = deviceSpecs[_devType];
	FX_DataPtr fxDataPtr = _data.getWrite();

	// read into the rest of the data like a buffer
	assert(fxDataPtr);
	if(fxDataPtr){
		memcpy(fxDataPtr, buf+MP_TSTP, sizeof(uint32_t));
		uint8_t *dataPtr = (uint8_t*)(fxDataPtr+1);
		uint16_t j, fieldOffset=0, index=MP_DATA1+1;
		for(j = 0; j < ds.numFields; j++)
		{
			if(IS_FIELD_HIGH(j, bitmap))
			{
				uint8_t ft = ds.fieldTypes[j];
				uint8_t fw = FORMAT_SIZE_MAP[ft];
				memcpy(dataPtr + fieldOffset, buf + index, fw);

				if( ft == FORMAT_16S || ft == FORMAT_8S )
				{
					uint8_t val = ( *(dataPtr + fieldOffset + fw - 1) >> 7 ) ? 0xFF : 0;
					memset( dataPtr + fieldOffset + fw, val, sizeof(int32_t) - fw);
				}

				index+=fw;
			}
			fieldOffset += 4; // storing each value as a separate int32
		}
	}
}