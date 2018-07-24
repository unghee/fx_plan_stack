#ifndef FLEXSEADEVICE_H
#define FLEXSEADEVICE_H

#include <vector>
#include <string>
#include <mutex>
#include "flexseadevicetypes.h"
#include "circular_buffer.h"
#include "flexseastack/flexsea-system/inc/flexsea_device_spec.h"
#include "flexseastack/flexsea-system/inc/flexsea_sys_def.h"

/// \brief FlexseaDevice class provides read access to connected devices
typedef circular_buffer<FX_DataPtr> FxDevData;
class FlexseaDevice
{
public:
    explicit FlexseaDevice(int _id=-1, int _port=-1, FlexseaDeviceType _type=FX_NONE, int role=FLEXSEA_MANAGE_1, int dataBuffSize=FX_DATA_BUFFER_SIZE);
    ~FlexseaDevice();

    const int id;
    const int port;
    const FlexseaDeviceType type;
    const int numFields;

    int getRole() const { return _role; }
    bool hasData() const { return !data->empty(); }
    size_t dataCount() const { return data->count(); }

    /* dataMutex should be locked while accessing data to ensure thread safety
    */
    std::recursive_mutex *dataMutex;

    /* Returns a vector of strings which describe the fields specified by map  */
    std::vector<std::string> getActiveFieldLabels() const;
    std::vector<int> getActiveFieldIds() const;

    std::vector<std::string> getAllFieldLabels() const;

    /* Interprets the data in this devices circular buffer at index
     *      and copies the fields specified in by this devices bitmap into output
     *      user should specify the length of output buffer to avoid buffer overflow
     *      outputSize is specified as length of int32_t array
     *      returns the timestamp
    */
    virtual uint32_t getLastData(int32_t *output, uint16_t outputSize);
    uint32_t getData(uint32_t index, int32_t *output, uint16_t outputSize) const;
    uint32_t  getDataPtr(uint32_t index, FX_DataPtr ptr, uint16_t outputSize) const;
    uint32_t getLatestTimestamp() const;

    /// \brief returns the first index of data whose timestamp is later than given timeStamp
    uint16_t getIndexAfterTime(uint32_t timeStamp) const;

    /// \brief fills output with all the data whose timestamps are after timeStamp
    /// output is filled much like a 2D array, sequential data are sequentially found in the array
    /// outputSize specifies the length of the array output. The maximimum number of data points that can fit
    ///     in an array of outputSize is given by outputSize / (numFields+1)
    /// returns the timestamp of the last data read into output.
    /// the width of one data is (numFields+1) and the data are stored sequentially, thus,
    ///     to access the 3rd field in the 2nd datum, you would use:  ( output + 2 * (numFields+1) )[3];
    /// note this method is faster than reading into nested std::vectors
//    uint32_t getDataAfterTime(uint32_t timeStamp, uint32_t *output, uint16_t outputSize) const;

    uint32_t getDataAfterTime(int field, uint32_t timestamp, std::vector<uint32_t> &ts_output, std::vector<int32_t> &data_output) const;

    /// \brief fills the vectors with all data whose timestamps are after timeStamp.
    /// timestamps and data are emptied and then filled as parallel vectors
    /// note this is considerably slower than the array alternative
    uint32_t getDataAfterTime(uint32_t timeStamp, std::vector<uint32_t> &timestamps, std::vector<std::vector<int32_t>> &data) const;

    /// \brief A convenience function which counts through the bitmap to tell you how many active fields this device has
    int getNumActiveFields() const;
    std::string getName() const;
    void getBitmap(uint32_t* out) const;
    void setBitmap(uint32_t* in);

    bool isValid() const { return this->id != -1; }

    /// \brief Returns the rate at which this device is/was receiving data in Hz
    double getDataRate() const;

    FxDevData* getCircBuff() { return data; }

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
    FxDevData* data;
    int _role;

private:
    inline size_t findIndexAfterTime(uint32_t timestamp) const;
};

#endif // FLEXSEADEVICE_H
