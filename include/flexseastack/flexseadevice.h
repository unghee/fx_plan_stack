#ifndef FLEXSEADEVICE_H
#define FLEXSEADEVICE_H

#include <vector>
#include <string>
#include <mutex>
#include <cassert>
#include <limits>
#include <shared_mutex>
#include "flexseadevicetypes.h"
#include "circular_buffer.h"
#include "flexsea_multi_frame_packet_def.h"

#include "fxdata.h"

#include "flexsea_device_spec.h"
#include "flexsea_sys_def.h"

#include <exception>

struct InvalidIndex : public std::exception {
	const char * what () const throw () {
	return "Invalid index requested..";}
};

struct InaccessiblePointer : public std::exception {
	const char * what () const throw () {
	return "Error accessing data ptr";}
};

/// \brief FlexseaDevice class provides read access to connected devices
class FlexseaDevice
{
public:
	explicit FlexseaDevice(int id, int port, FlexseaDeviceType type,              int role, int dataBuffSize=FX_DATA_BUFFER_SIZE);
	explicit FlexseaDevice(int id, int shortid, int port, FlexseaDeviceType type, int role, int dataBuffSize=FX_DATA_BUFFER_SIZE);
	explicit FlexseaDevice(int id, int port, std::vector<std::string> fieldLabels, int role, int dataBuffSize);

	const int _devId;
	const int _portIdx;
	const FlexseaDeviceType _devType;
	const int _numFields;

	int getShortId() const { return _shortId; }
	int getRole() const { return _role; }
	bool hasData() const;
	size_t dataCount() const;

	// Returns a vector of strings which describe the fields specified by map
	std::vector<std::string> getActiveFieldLabels() const;
	std::vector<int> getActiveFieldIds() const;
	std::vector<std::string> getAllFieldLabels() const;

	uint32_t getLatestTimestamp() const;

	// Data retrieval functions

	/// \brief fills the output buffer with the requested field ids
	/// skips over invalid field ids, updating the fieldIds array to reflect what is actually read
	uint32_t getData(int* fieldIds, int* output, uint16_t outputSize);
	uint32_t getData(int* fieldIds, int* output, uint16_t outputSize, int index);
	uint32_t  getDataPtr(uint32_t index, FX_DataPtr ptr, uint16_t outputSize) const;

	/// \brief returns the first index of data whose timestamp is later than given timeStamp
	uint16_t getIndexAfterTime(uint32_t timeStamp) const;

	uint32_t getDataAfterTime(int field, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<int32_t> &data_output) const;
	uint32_t getDataAfterTime(const std::vector<int> &fields, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<std::vector<int32_t>> &data_output) const;
	uint32_t getDataAfterTime(const std::vector<int> &fields, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<std::vector<int32_t>> &data_output, unsigned max) const;

	/// \brief fills the vectors with all data whose timestamps are after timeStamp.
	/// timestamps and data are emptied and then filled as parallel vectors
	/// note this is considerably slower than the array alternative
	uint32_t getDataAfterTime(uint32_t timeStamp, std::vector<uint32_t> &timestamps, std::vector<std::vector<int32_t>> &data) const;

	/// \brief A convenience function which counts through the bitmap to tell you how many active fields this device has
	int getNumActiveFields() const;
	std::string getName() const;
	void getBitmap(uint32_t* out) const;
	void setBitmap(uint32_t* in);

	void updateData(uint8_t* buf);

	// FxDevData* getCircBuff() { return &_data; }

	bool isValid() const { return this->_devId != -1; }
	/// \brief Returns the rate at which this device is/was receiving data in Hz
	double getDataRate() const;


private:
	//MUST HOLD DATALOCK BEFORE CALLING
	inline size_t findIndexAfterTime(uint32_t timestamp) const;
	/// bitmap indicating which fields are active for the device with specified id
	/// bitmap is a uint32_t[FX_BITMAP_WIDTH]
	/// so if (0x01 & active()[0]) then field 0 is active
	/// if (0x01 & active()[1]) then field 32 is active
	/// or in general if (1 << x) & active()[y] then field 32*y+x is active

	uint32_t bitmap[FX_BITMAP_WIDTH];
	std::vector<std::string> fieldLabels;
	
	//mutable because they're used for caching
	mutable uint32_t lastFieldLabelMap[FX_BITMAP_WIDTH] = {0};
	mutable std::vector<std::string> lastRequest;

	int _shortId;
	int _role;
	
	mutable std::shared_timed_mutex dataLock;
	// mutable std::mutex dataLock;
	FxDevData _data;
};

#endif // FLEXSEADEVICE_H
