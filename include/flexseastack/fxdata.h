#ifndef FX_DATA_
#define FX_DATA_

#include <cstdint>
#include <cstring>

/// \brief a data structure used for storing data received from flexsea devices
/// FxDevData returns int32_t*, but owns all the memory pointed to by these return values
/// the point of this structure is to provide a convenience 2D buffer (circular in first dimension) and avoid continuous memory allocations
struct FxDevData {

    FxDevData(uint32_t rows, uint32_t cols)
	: _rows(rows) , _cols(cols)
	, data( new uint32_t[rows*cols] ) 
	, wIdx(0), rIdx(0), size(0)
    {
        memset(data, 0, sizeof(uint32_t) * rows * cols);
    }

	~FxDevData() { delete[] data; }

    /// \brief Get the next pointer to write to
	uint32_t* getWrite() 
	{ 
		uint32_t *p = data + wIdx * _cols;

		// advance write index
		if((++wIdx) >= _rows) 
			wIdx = 0;

		// increase size conditionally
		// advance read index conditionally
		if((++size) >= _rows)
		{
			--size;
			if((++rIdx) >= _rows) 
				rIdx = 0;
		}

		return p;
	}

    /// \brief Get the pointer at the corresponding index
    inline uint32_t* peek(int i) const { return getRead(i); }

    /// \brief Get the last pointer accessed by getWrite()
    inline uint32_t* peekBack() const { return getRead(size-1); }

    /// \brief Get the pointer at the corresponding index
    uint32_t* getRead(unsigned int i) const
	{
		if(!size || i >= size) return nullptr;

                unsigned int t = (rIdx + i);

		if(t >= _rows) 
			t -= _rows;

		return data + t * _cols;
	}

    /// \brief Get number of valid data pointers
    size_t count() const { return size; }

    /// \brief Check if the container contains any data
    bool empty()  const { return size == 0; }

private:

	uint32_t _rows, _cols;
    uint32_t *data;
	uint32_t wIdx, rIdx;
	size_t size;

};

#endif // FX_DATA_
