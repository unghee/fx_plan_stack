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

#ifndef INC_FLEXSEA_CMD_GAIT_STATS_H
#define INC_FLEXSEA_CMD_GAIT_STATS_H
	
#ifdef INCLUDE_UPROJ_DPEB42

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include <stdint.h>
#include "flexsea_user_structs.h"

//****************************************************************************
// Prototype(s):
//****************************************************************************

void init_gait_stats(void);

//****************************************************************************
// RX/TX Prototype(s):
//****************************************************************************

void rx_cmd_gait_stats_rw(uint8_t *buf, uint8_t *info);
void rx_cmd_gait_stats_rr(uint8_t *buf, uint8_t *info);
void rx_cmd_gait_stats_w(uint8_t *buf, uint8_t *info);

void tx_cmd_gait_stats_r(uint8_t *shBuf, uint8_t *cmd, uint8_t *cmdType, \
					uint16_t *len, uint8_t offset);
void tx_cmd_gait_stats_w(uint8_t *shBuf, uint8_t *cmd, uint8_t *cmdType, \
					uint16_t *len, uint8_t offset, uint8_t resetRow);

//****************************************************************************
// Definition(s):
//****************************************************************************

#define GS_ROWS				3
#define GS_COLUMNS			10
#define RESET_ROW_MASK		0x80

//****************************************************************************
// Structure(s):
//****************************************************************************

struct gaitStats_s
{
	uint8_t n[GS_ROWS][GS_COLUMNS];
	uint8_t rows;
	uint8_t columns;
	uint8_t i;
	uint8_t j;
	uint8_t clear_stats_flag[GS_ROWS];
};

//****************************************************************************
// Shared variable(s)
//****************************************************************************

extern struct gaitStats_s gaitStats;

#ifdef __cplusplus
}
#endif

#endif	//INCLUDE_UPROJ_DPEB42
#endif	//INC_FLEXSEA_CMD_GAIT_STATS_H
