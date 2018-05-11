#ifndef FLEXSEADEVICETYPES_H
#define FLEXSEADEVICETYPES_H

#include <vector>
#include <string>

typedef uint32_t* FX_DataPtr;

#define FX_BITMAP_WIDTH 3
#define IS_FIELD_HIGH(i, map) ( (map)[(i)/32] & (1 << ((i)%32)) )
#define SET_FIELD_HIGH(i, map) ( (map)[(i)/32] |= ((i)%32) )
#define FX_DATA_BUFFER_SIZE 50

#endif // FLEXSEADEVICETYPES_H
