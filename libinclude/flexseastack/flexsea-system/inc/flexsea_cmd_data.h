/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-system' System commands & functions
	Copyright (C) 2016 Dephy, Inc. <http://dephy.com/>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************
	[Lead developper] Jean-Francois (JF) Duval, jfduval at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] flexsea_cmd_data: Data Commands
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2016-09-09 | jfduval | Initial GPL-3.0 release
	*
****************************************************************************/

#ifndef INC_FLEXSEA_CMD_DATA_H
#define INC_FLEXSEA_CMD_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include <stdint.h>
#include "flexsea_global_structs.h"

//****************************************************************************
// Prototype(s):
//****************************************************************************
struct _MultiPacketInfo_s;
typedef struct _MultiPacketInfo_s MultiPacketInfo;

void init_flexsea_payload_ptr_data(void);

//Read All
void rx_cmd_data_read_all_rw(uint8_t *buf, uint8_t *info);
void rx_cmd_data_read_all_rr(uint8_t *buf, uint8_t *info);
void tx_cmd_data_read_all_r(uint8_t *shBuf, uint8_t *cmd, uint8_t *cmdType, \
					uint16_t *len);
void tx_cmd_data_read_all_w(uint8_t *shBuf, uint8_t *cmd, uint8_t *cmdType, \
					uint16_t *len);

//User Data
void rx_cmd_data_user_rw(uint8_t *buf, uint8_t *info);
void rx_multi_cmd_data_user_rw(uint8_t *msgBuf, MultiPacketInfo *mInfo, uint8_t *responseBuf, uint16_t* responseLen);
void rx_cmd_data_user_rr(uint8_t *buf, uint8_t *info);
void rx_multi_cmd_data_user_rr(uint8_t *msgBuf, MultiPacketInfo *mInfo, uint8_t *responseBuf, uint16_t* responseLen);
void rx_cmd_data_user_w(uint8_t *buf, uint8_t *info);
void rx_multi_cmd_data_user_w(uint8_t *msgBuf, MultiPacketInfo *mInfo, uint8_t *responseBuf, uint16_t* responseLen);
void tx_cmd_data_user_r(uint8_t *shBuf, uint8_t *cmd, uint8_t *cmdType, \
						uint16_t *len);
void tx_cmd_data_user_w(uint8_t *shBuf, uint8_t *cmd, uint8_t *cmdType, \
						uint16_t *len, uint8_t select_w);

void copyUserWtoStack(struct user_data_s u);
void readUserRfromStack(struct user_data_s *u);
void ptx_cmd_data_user_r(uint8_t slaveId, uint16_t *numb, uint8_t *commStr);
void ptx_cmd_data_user_w(uint8_t slaveId, uint16_t *numb, uint8_t *commStr, \
							uint8_t select_w);

//****************************************************************************
// Definition(s):
//****************************************************************************

#define USER_RW_WVARS		10
#define USER_RW_R_VARS		6

//****************************************************************************
// Structure(s):
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#ifdef __cplusplus
}
#endif

#endif	//INC_FLEXSEA_CMD_DATA_H
