/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-dephy' Exo Controllers
	Copyright (C) 2018 Dephy, Inc. <http://dephy.com/>
*****************************************************************************
	[Lead developer] Luke Mooney, lmooney at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] analyze-Gait: gait parameters
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2018-02-06 | jfduval | Copied from DpEb31
****************************************************************************/

#ifndef INC_CORRFUN_H
#define INC_CORRFUN_H

//****************************************************************************
// Include(s)
//****************************************************************************

#ifdef BOARD_TYPE_FLEXSEA_MANAGE

#include "main.h"
#include "flexsea_global_structs.h"

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************


	
//****************************************************************************
// Definition(s):
//
//****************************************************************************
// Structure(s)

struct corr_lin_s
{
	int16_t n;
	float x;
	float x2;
	float y;
	float y2;
	float xy;
	float R2;
	float a0;
	float a1;
};

void add_corr_lin_pt (struct corr_lin_s *, float, float);
void init_corr_lin (struct corr_lin_s *);
float get_corr_lin_est (struct corr_lin_s *, float);


#endif //BOARD_TYPE_FLEXSEA_PLAN

#endif //INC_ANALYZE_GAIT_H
