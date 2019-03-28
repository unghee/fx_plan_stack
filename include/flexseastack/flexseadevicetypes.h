#ifndef FLEXSEADEVICETYPES_H
#define FLEXSEADEVICETYPES_H

#include <vector>
#include <string>

// some convenience declarations used throughout the flexsea device code base

typedef uint32_t* FX_DataPtr;

#define FX_BITMAP_WIDTH 3
#define IS_FIELD_HIGH(i, map) ( (map)[(i)/32] & (1U << ((i)%32)) )
#define SET_FIELD_HIGH(i, map)( (map)[(i)/32] |= (1U << (i%32))  )
#define FX_DATA_BUFFER_SIZE 64

#endif // FLEXSEADEVICETYPES_H
