#ifndef COMM_STRING_GENERATION_H
#define COMM_STRING_GENERATION_H

#include <utility>
#include <cstring>
#include "flexsea_comm_multi.h"
#include "flexsea_sys_def.h"

/// \brief This namespace contains some convenience functions for generalized method of generating comm strings
namespace CommStringGeneration
{

    /// \brief Calls a tx function in order to generate a comm string inside of a multiwrapper. \n
    /// tx_func must be of the form [return type] tx_func(uint8_t* buf, uint8_t* cmdCode, uint8_t* cmdType, uint16_t* len, Args...)
    /// @param out : the multiwrapper to fill with the resulting comm string
    /// @param tx_func : the function to call
    /// @param tx_args : additional arguments that the function needs besides the following:
    ///                  uint8_t* buf, uint8_t* cmdCode, uint8_t* cmdType, uint16_t* len
    template<typename T, typename... Args>
    bool generateCommString(int devId, MultiWrapper *out, T tx_func, Args&&... tx_args)
    {
        uint8_t cmdCode, cmdType;
        out->unpackedIdx = 0;

        // NOTE: in a response, we need to reserve bytes for XID, RID, CMD, and TIMESTAMP
        tx_func(out->unpacked + MP_DATA1, &cmdCode, &cmdType, &out->unpackedIdx,
                std::forward<Args>(tx_args)...
                );

        if(out->unpackedIdx)
        {
            setMsgInfo(out->unpacked, FLEXSEA_PLAN_1, devId, cmdCode, cmdType == CMD_READ ? RX_PTYPE_READ : RX_PTYPE_WRITE, 0);
            out->unpackedIdx += MULTI_PACKET_OVERHEAD;

            out->currentMultiPacket++;
            out->currentMultiPacket%=4;

            return packMultiPacket(out);
        }
        else
            return false;

    }

    /// \brief retrieves the bluetooth configuration string for the field specified for regular device-computer configurations
    bool getBtConfigField(unsigned int field, uint8_t *str, uint16_t *strlen);
    /// \brief retrieves the bluetooth configuration string for the field specified for regular device-device configurations (deprecated)
    bool getBtBilateralConfigField(unsigned int field, uint8_t *str, uint16_t *strlen);

}

#endif // COMM_STRING_GENERATION_H
