#ifndef COMM_STRING_GENERATION_H
#define COMM_STRING_GENERATION_H

#include <utility>
#include <cstring>
#include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"
#include "flexseastack/flexsea-system/inc/flexsea_sys_def.h"

namespace CommStringGeneration
{
    template<typename T, typename... Args>
    bool generateCommString(int devId, MultiWrapper *out, T tx_func, Args&&... tx_args)
    {
        uint8_t cmdCode, cmdType;
        out->unpackedIdx = 0;

        // NOTE: in a response, we need to reserve bytes for XID, RID, CMD, and TIMESTAMP
        tx_func(out->unpacked + MP_DATA1, &cmdCode, &cmdType, &out->unpackedIdx,
                std::forward<Args>(tx_args)...
                );

        setMsgInfo(out->unpacked, FLEXSEA_PLAN_1, devId, cmdCode, cmdType == CMD_READ ? RX_PTYPE_READ : RX_PTYPE_WRITE, 0);
        out->unpackedIdx += MULTI_PACKET_OVERHEAD;

        out->currentMultiPacket++;
        out->currentMultiPacket%=4;

        return packMultiPacket(out);
    }

    bool getBtConfigField(unsigned int field, uint8_t *str, uint16_t *strlen);
    bool getBtBilateralConfigField(unsigned int field, uint8_t *str, uint16_t *strlen);

}

#endif // COMM_STRING_GENERATION_H
