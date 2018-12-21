/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-dephy' Exo Controllers
	Copyright (C) 2018 Dephy, Inc. <http://dephy.com/>
*****************************************************************************
	[Lead developper] Luke Mooney, lmooney at dephy dot com.
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

#ifndef INC_WALKING_CONTROLLER_H
#define INC_WALKING_CONTROLLER_H

//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"
#include "exoDef.h"

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

void initWalkingController(void);
void walkingControllerFSM(void);
void refreshValues(void);
void resetWalkingController(void);
void rwTestFSM1(void);
void rwTestFSM2(void);

//****************************************************************************
// Accessor(s)
//****************************************************************************

int64_t get_st(void);
int32_t get_mot_ang(void);
int32_t get_mot_vel(void);

//****************************************************************************
// Definition(s):
//****************************************************************************

//****************************************************************************
// Structure(s)
//****************************************************************************
struct gait_data_s
{
	int16_t trans_ratio;
	int16_t ank_ang;
	int16_t ank_from_mot_ang;
	int16_t	mot_ang;
	int16_t mot_from_ank_ang;
	int16_t shk_ang;
	int16_t shk_vel;
	int16_t max_ppf_ank_ang;
	int16_t min_stn_shk_ang;
	int16_t min_swg_ank_ang;
	int16_t t_stn;
	int16_t ank_trq;
	int16_t ank_pow;
	int16_t pos_eng;
	int8_t  wlk_ste;
	int16_t exit_ppf_flag;


};

//****************************************************************************
// Shared variable(s)
//****************************************************************************

extern int32_t mot_from_ank_ang;
extern wcState_t stateDpEb42;
extern struct gait_data_s gait_data;

#endif	//INC_WALKING_CONTROLLER_H

#endif //BOARD_TYPE_FLEXSEA_EXECUTE
