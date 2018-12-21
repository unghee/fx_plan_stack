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
	[Lead developer] Jean-Francois (JF) Duval, jfduval at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] flexsea_global_structs: contains all the data structures
	used across the project
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2016-12-08 | jfduval | Initial release
	*
****************************************************************************/

#ifndef INC_FLEXSEA_DEPHY_STRUCT_H
#define INC_FLEXSEA_DEPHY_STRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include <stdint.h>
//#include <flexsea_global_structs.h>

//****************************************************************************
// Definition(s):
//****************************************************************************

//****************************************************************************
// Structure(s):
//****************************************************************************

struct bwcPacket_s
{
	uint32_t timestamp;
	int16_t val[10];
	uint8_t cnt;
	uint8_t rxStatus;
};

/*
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
};
*/

//****************************************************************************
// Shared variable(s)
//****************************************************************************

//extern struct dual_utt_s utt;
extern struct bwcPacket_s bwcPacketTx, bwcPacketRx;

//****************************************************************************
// Prototype(s):
//****************************************************************************

void initializeDephyStructs(void);

#ifdef __cplusplus
}
#endif

#endif	//INC_FLEXSEA_DEPHY_STRUCT_H
