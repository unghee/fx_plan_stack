/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-dephy' Exo Controllers
	Copyright (C) 2018 Dephy, Inc. <http://dephy.com/>
*****************************************************************************
	[Lead developer] Jean-Francois (JF) Duval, jfduval at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] cmd-UTT: DpEb3.1 User Testing Tweaks
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2018-02-06 | jfduval | Copied from DpEb31
****************************************************************************/

#ifndef INC_FLEXSEA_CMD_UTT_H
#define INC_FLEXSEA_CMD_UTT_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include <stdint.h>

//****************************************************************************
// Definition(s):
//****************************************************************************

#define UTT_RIGHT	0
#define UTT_LEFT	1
#define UTT_DUAL	2

//****************************************************************************
// Structure(s)
//****************************************************************************

struct utt_s
{
	uint8_t ctrl;
	uint8_t ctrlOption;
	uint8_t amplitude;
	int8_t timing;
	uint8_t powerOn;

	int8_t torquePoints[6][2];
};

struct dual_utt_s
{
	uint8_t target;
	struct utt_s leg[2];
	int16_t val[2][10];
};

struct _MultiPacketInfo_s;
typedef struct _MultiPacketInfo_s MultiPacketInfo;

//****************************************************************************
// Prototype(s):
//****************************************************************************

void init_utt(void);
uint32_t compTorqueX(struct dual_utt_s *wutt, uint8_t leg);
void decompTorqueX(struct dual_utt_s *wutt, uint8_t leg, uint32_t tp);

//****************************************************************************
// RX/TX Prototype(s):
//****************************************************************************

void rx_cmd_utt_rw(uint8_t *buf, uint8_t *info);
void rx_cmd_utt_rr(uint8_t *buf, uint8_t *info);
void rx_cmd_utt_w(uint8_t *buf, uint8_t *info);

void rx_multi_cmd_utt_rw(uint8_t *msgBuf, MultiPacketInfo *mInfo, uint8_t *responseBuf, uint16_t* responseLen);
void rx_multi_cmd_utt_w(uint8_t *msgBuf, MultiPacketInfo *mInfo, uint8_t *responseBuf, uint16_t* responseLen);
void rx_multi_cmd_utt_rr(uint8_t *msgBuf, MultiPacketInfo *mInfo, uint8_t *responseBuf, uint16_t* responseLen);

void tx_cmd_utt_r(uint8_t *shBuf, uint8_t *cmd, uint8_t *cmdType, \
					uint16_t *len, uint8_t offset);
void tx_cmd_utt_w(uint8_t *shBuf, uint8_t *cmd, uint8_t *cmdType, \
					uint16_t *len, uint8_t offset, struct dual_utt_s *wutt);

//****************************************************************************
// Shared variable(s)
//****************************************************************************

extern uint8_t sendTweaksToSlave;
extern struct dual_utt_s utt;

#ifdef __cplusplus
}
#endif

#endif	//INC_FLEXSEA_CMD_UTT_H
