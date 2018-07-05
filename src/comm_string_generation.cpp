#include "flexseastack/comm_string_generation.h"

bool CommStringGeneration::getBtConfigField(unsigned int field, uint8_t *str, uint16_t *strlen)
{
    //Settings - GUI:
    //---------------
    //Name = FlexSEA-ADDR
    //Baudrate = 230k
    //TX Power = max
    //Inquiry window = 0012
    //Page Window = 0012
    //CfgTimer = 15s
    //Clear any remote address
    //Slave mode
    //Optimize for throughput


    const int BT_FIELDS = 9;
    static uint8_t config[BT_FIELDS][20] = {{"S-,FlexSEA\r"},
                                            {"SU,230K\r"},
                                            {"SY,0010\r"},
                                            {"SI,0012\r"},
                                            {"SJ,0012\r"},
                                            {"ST,15\r"},
                                            {"SR,Z\r"},
                                            {"SM,0\r"},
                                            {"SQ,0\r"}};

    static uint8_t len[BT_FIELDS] = {11,8,8,8,8,6,5,5,5};


    if(field >= BT_FIELDS) return false;

    *strlen = len[field];
    memcpy(str, config[field], len[field]);

    return true;
}

bool CommStringGeneration::getBtBilateralConfigField(unsigned int field, uint8_t *str, uint16_t *strlen)
{
    //Settings - Bilateral:
    //---------------------
    //Name = FlexSEA-BWC-ADDR
    //Baudrate = 460k
    //TX Power = max
    //Inquiry window = 0012
    //Page Window = 0012
    //CfgTimer = 15s
    //Pair mode
    //Authentification
    //Buddy's address
    //Latency optimization SQ,16

    const int BT_FIELDS = 10;
    static uint8_t config[BT_FIELDS][20] = {{"S-,FlexSEA-BWC\r"}, \
                                            {"SU,460K\r"}, \
                                            {"SY,0010\r"}, \
                                            {"SI,0012\r"}, \
                                            {"SJ,0012\r"}, \
                                            {"ST,15\r"}, \
                                            {"SM,6\r"}, \
                                            {"SA,4\r"}, \
                                            {"SR,000000000000\r"},
                                            {"SQ,16\r"}};

    static uint8_t len[BT_FIELDS] = {15,8,8,8,8,6,5,5,16,6};


    if(field >= BT_FIELDS) return false;

    *strlen = len[field];
    memcpy(str, config[field], len[field]);

    return true;
}
