#ifndef FLEXSEADEVICE_H
#define FLEXSEADEVICE_H

#include <vector>
#include <string>
#include <mutex>
#include "flexseadevicetypes.h"
#include "circular_buffer.h"

#include "flexseastack/fxdata.h"

#include "flexseastack/flexsea-system/inc/flexsea_device_spec.h"
#include "flexseastack/flexsea-system/inc/flexsea_sys_def.h"

/// \brief FlexseaDevice class provides read access to connected devices
class FlexseaDevice
{
public:
    explicit FlexseaDevice(int _id=-1, int _port=-1, FlexseaDeviceType _type=FX_NONE, int role=FLEXSEA_MANAGE_1, int dataBuffSize=FX_DATA_BUFFER_SIZE);
    explicit FlexseaDevice(int _id, int _port, std::vector<std::string> fieldLabels, int role, int dataBuffSize);

    const int id;
    const int port;
    const FlexseaDeviceType type;
    const int numFields;

    int getRole() const { return _role; }
    bool hasData() const { return !_data.empty(); }
    size_t dataCount() const { return _data.count(); }

    // dataMutex should be locked while accessing data to ensure thread safety
    // a const pointer to a (non-const) recursive_mutex
    std::recursive_mutex *const dataMutex;

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

    FxDevData* getCircBuff() { return &_data; }

    bool isValid() const { return this->id != -1; }
    /// \brief Returns the rate at which this device is/was receiving data in Hz
    double getDataRate() const;

protected:

    /// bitmap indicating which fields are active for the device with specified id
    /// bitmap is a uint32_t[FX_BITMAP_WIDTH]
    /// so if (0x01 & active()[0]) then field 0 is active
    /// if (0x01 & active()[1]) then field 32 is active
    /// or in general if (1 << x) & active()[y] then field 32*y+x is active
    uint32_t bitmap[FX_BITMAP_WIDTH];

    //mutable because they're used for caching
    mutable uint32_t lastFieldLabelMap[FX_BITMAP_WIDTH] = {0};
    mutable std::vector<std::string> lastRequest;

    /* data gives access to data thats come into this device.
     * The actual buffer is managed by whichever object created this FlexseaDevice object
    */
    int _role;
    std::vector<std::string> fieldLabels;
    std::recursive_mutex _dataMutex;
    FxDevData _data;

private:
    inline size_t findIndexAfterTime(uint32_t timestamp) const;
};

#endif // FLEXSEADEVICE_H
