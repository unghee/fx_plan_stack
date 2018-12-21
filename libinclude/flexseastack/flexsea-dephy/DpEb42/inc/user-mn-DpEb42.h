/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-dephy' Exo Controllers
	Copyright (C) 2018 Dephy, Inc. <http://dephy.com/>
*****************************************************************************
	[Lead developer] Jean-Francois Duval, jfduval at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] user-mn-DpEb42: User code running on Mn
****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2018-02-06 | jfduval | Copied from DpEb31
	*
****************************************************************************/

#ifdef BOARD_TYPE_FLEXSEA_MANAGE

#ifndef INC_DPEB42_MN_H
#define INC_DPEB42_MN_H

#include <stdint.h>
//****************************************************************************
// Include(s)
//****************************************************************************

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

void init_DpEb42(void);
void DpEb42_fsm_1(void);
void DpEb42_fsm_2(void);
void setMotorPositionDP(int32_t);
void reset_dephy(void);

#ifdef BILATERAL
void bwc(void);
void genBilateralPacket(void);
uint8_t decodeBilateralPacket(void);
#endif //BILATERAL

//****************************************************************************
// Accessor(s)
//****************************************************************************


//****************************************************************************
// Definition(s):
//****************************************************************************

#define DP_FSM2_POWER_ON_DELAY		3500

//****************************************************************************
// Structure(s)
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

extern struct rigid_s dpRigid;
extern int32_t dp_mot_ang_zero;
extern int32_t dp_ank_ang_zero;
extern int32_t mot_ang_offset;

#endif	//INC_DPEB42_MN_H

#endif //BOARD_TYPE_FLEXSEA_EXECUTE
