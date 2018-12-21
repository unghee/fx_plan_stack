/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-user' User projects
	Copyright (C) 2018 Dephy, Inc. <http://dephy.com/>
*****************************************************************************
	[Lead developper] Luke Mooney, lmooney at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] exo-def: constants and macros for the DpEb4.2 project
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2018-02-06 | jfduval | Copied from DpEb31
****************************************************************************/

#ifndef INC_EXO_DEF42_H
#define INC_EXO_DEF42_H

//****************************************************************************
// Include(s)
//****************************************************************************

#include "dephy-mn.h"

//****************************************************************************
// Macro(s):
//****************************************************************************

//****************************************************************************
// Definition(s):
//****************************************************************************


//#define SHADOW_CONTROLLER		//Enable to put the Exo in Shadow mode (stalking controller)
//#define DATA_COLLECT_NO_MOTOR	//Enable to prevent motion
//#define SPRING_CONTROLLER

//Gait states:
typedef enum {	AG_SWING = 0,
				AG_DORSIFLEX,
				AG_PLANTARFLEX,
				AG_STEP_COUNTED,
				AG_UNDEFINED
				} gaitState_t;

//Walking Controller States:
typedef enum {	WC_STARTUP = 0,
				WC_CALIB0,
				WC_NOMOTOR,
				WC_STALK,
				WC_HOLDPOS,
				WC_SKIP1,
				WC_EARLY_STANCE,
				WC_LATE_STANCE,
				WC_PEAK_DORSIFLEX,
				WC_PLANTARFLEX,
				WC_PLANTARFLEX_SLOWDOWN,
				WC_CONTROL_DORSIFLEX,
				WC_ERROR1,
				WC_ERROR2
				} wcState_t;

//State machine constants:
#define STARTUP_DELAY				6000
#define STALK_EXO_MOT_OFFS			6000
#define STALK_EXO_RAMP_T			500
#define MIN_NUM_STEPS				10

//Motor and system constants:
#define MOT_ENC_FULL_ROT			16384
#define MOT_ENC_HALF_ROT			8192
#define EXO_CTRL_I_KP				100
#define EXO_CTRL_I_KI				32
#define ANK_TORQUE_TO_MA			70
#define EXO_CTRL_P_KP				200

//Exo Safety:
#define ES_STARTUP_DELAY			2000
#define ES_MIN_VB					42000
#define ES_CURR_LIM_POS				40000
#define ES_CURR_LIM_NEG				-35000
#define ES_CURR_CNT_TRESH			100
#define ES_VOLT_CNT_TRESH			500

//Tension String:
#define TS_MOT_VOLT					1000
#define TS_MIN_TIME					500
#define TS_MIN_VEL_CNT				100

//Swing:
#define SW_OFFSET_AT_BEG_APPROACH	6000				//offset_at_beg_approach
#define SW_OFFSET_AT_END_APPROACH	1000

//Plantarflex slow down:
#define PFSD_CURRENT				-20000

//Assign signs based on Exo side:

#if(EXO_SIDE == LEFT)
	#ifdef PRJ_DEPHY_DPEB45
		#define SIGN_JOINT_ANGLE			(1)
	#else
		#define SIGN_JOINT_ANGLE			(-1)
	#endif

	#define SIGN_MOTOR_VOLTAGE			(1)
	#define SIGN_MOTOR_CURRENT			(1)
	#define SIGN_MOTOR_ANGLE			(1)
	#define SIGN_JOINT_VELOCITY			(SIGN_JOINT_ANGLE)
	#define SIGN_MOTOR_VELOCITY			(SIGN_MOTOR_ANGLE)
	#define SIGN_ACCL_Y					(1)
	#define SIGN_GYRO_Z					(1)
	#define SIGN_GYRO_X					(1)

#elif(EXO_SIDE == RIGHT)
	#ifdef PRJ_DEPHY_DPEB45
		#define SIGN_JOINT_ANGLE			(-1)
	#else
		#define SIGN_JOINT_ANGLE			(1)
	#endif

	#define SIGN_MOTOR_VOLTAGE			(-1) //positive in PF
	#define SIGN_MOTOR_CURRENT			(-1) //positive in PF
	#define SIGN_MOTOR_ANGLE			(-1) //
	#define SIGN_JOINT_VELOCITY			(SIGN_JOINT_ANGLE)
	#define SIGN_MOTOR_VELOCITY			(SIGN_MOTOR_ANGLE)
	#define SIGN_ACCL_Y					(-1)
	#define SIGN_GYRO_Z					(-1)
	#define SIGN_GYRO_X					(-1)

#endif

//****************************************************************************
// Structure(s)
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************



#endif //INC_EXO_DEF42_H
