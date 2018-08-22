#ifndef __FX_DATA_
#define __FX_DATA_

#include <cstdint>
#include <cstring>

struct FxDevData {

	FxDevData(int rows, int cols) 
	: _rows(rows) , _cols(cols)
	, data( new uint32_t[rows*cols] ) 
	, wIdx(0), rIdx(0), size(0)
    {
        memset(data, 0, sizeof(uint32_t) * rows * cols);
    }

	~FxDevData() { delete[] data; }

	// get the next pointer to write to
	uint32_t* getWrite() 
	{ 
		uint32_t *p = data + wIdx * _cols;

		// advance write index
		if((++wIdx) >= _rows) 
			wIdx = 0;

		// increase size conditionally
		// advance read index conditionally
        if((++size) > _rows)
		{
			--size;
			if((++rIdx) >= _rows) 
				rIdx = 0;
		}

		return p;
	}

        inline uint32_t* peek(int i) const { return getRead(i); }
        inline uint32_t* peekBack() const { return getRead(size-1); }

        // get the next pointer to read from
        uint32_t* getRead(unsigned int i) const
	{
		if(!size || i >= size) return nullptr;

                unsigned int t = (rIdx + i);

		if(t >= _rows) 
			t -= _rows;

		return data + t * _cols;
	}

        size_t count() const { return size; }
        bool empty()  const { return size == 0; }

private:

	uint32_t _rows, _cols;
	uint32_t *data;
	uint32_t wIdx, rIdx;
	size_t size;

};

#endif // __FX_DATA_
