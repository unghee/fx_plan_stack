#ifdef BOARD_TYPE_FLEXSEA_MANAGE
#ifdef DEPHY

#ifndef INC_DEPHY_MN_H
#define INC_DEPHY_MN_H

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

void init_dephy(void);
void dephy_fsm_1(void);
void dephy_fsm_2(void);

//****************************************************************************
// Definition(s):
//****************************************************************************

//List of projects - Dephy only:
#define PRJ_DEPHY_NULL			0
#define PRJ_DEPHY_CYCLE_TESTER	1	//Automatic Cycle Tester
#define PRJ_DEPHY_DPEB31		2	//DpEb3.1 Exo (2017)
#define PRJ_DEPHY_DPEB42		3	//DpEb4.2 Exo (2018)

//Leg:
#define RIGHT					1	//(Master / Mn1)
#define LEFT					2	//(Slave / Mn2)

//Step 1) Select active project (from list):
//==========================================

#define ACTIVE_DEPHY_PROJECT		PRJ_DEPHY_DPEB42
#define ACTIVE_DEPHY_SUBPROJECT		RIGHT

//Step 2) Customize the enabled/disabled sub-modules:
//===================================================

//Automatic Cycle Tester
#if(ACTIVE_DEPHY_PROJECT == PRJ_DEPHY_CYCLE_TESTER)

	//Enable/Disable sub-modules:
	#define USE_USB
	#define USE_COMM			//Requires USE_RS485 and/or USE_USB
	#define USE_I2C_1			//3V3, IMU & Digital pot
	//#define USE_I2C_2			//3V3, Expansion
	#define USE_I2C_3			//Onboard, Regulate & Execute
	#define USE_IMU				//Requires USE_I2C_1
	#define USE_UART3			//Bluetooth
	#define USE_EEPROM			//Emulated EEPROM, onboard FLASH
	#define USE_WATCHDOG		//Independent watchdog (IWDG)

	//Runtime finite state machine (FSM):
	#define RUNTIME_FSM1		ENABLED
	#define RUNTIME_FSM2		ENABLED

#endif	//PRJ_DEPHY_CYCLE_TESTER

//DpEb3.1 Exo
#if(ACTIVE_DEPHY_PROJECT == PRJ_DEPHY_DPEB31)

	#if (HW_VER < 10)

		//Enable/Disable sub-modules:
		#define USE_USB
		#define USE_COMM			//Requires USE_RS485 and/or USE_USB
		#define USE_I2C_1			//3V3, IMU & Digital pot
		//#define USE_I2C_2			//3V3, Expansion
		#define USE_I2C_3			//Onboard, Regulate & Execute
		#define USE_IMU				//Requires USE_I2C_1
		#define USE_UART3			//Bluetooth
		#define USE_EEPROM			//Emulated EEPROM, onboard FLASH
		#define USE_WATCHDOG		//Independent watchdog (IWDG)
		//#define USE_SVM			//Support vector machine

		//Runtime finite state machine (FSM):
		#define RUNTIME_FSM1		ENABLED
		#define RUNTIME_FSM2		ENABLED

	#else

		//Enable/Disable sub-modules:
		#define USE_USB
		#define USE_COMM			//Requires USE_RS485 and/or USE_USB
		#define USE_I2C_1			//3V3, IMU & Digital pot
		//#define USE_I2C_2			//3V3, Expansion
		#define USE_I2C_3			//Onboard, Regulate & Execute
		#define USE_IMU				//Requires USE_I2C_1
		#define USE_UART3			//Bluetooth
		#define USE_EEPROM			//Emulated EEPROM, onboard FLASH
		//#define USE_WATCHDOG		//Independent watchdog (IWDG)
		//#define USE_SVM			//Support vector machine

		//Runtime finite state machine (FSM):
		#define RUNTIME_FSM1		ENABLED
		#define RUNTIME_FSM2		ENABLED

	#endif

	#define BILATERAL

	#if(ACTIVE_DEPHY_SUBPROJECT == RIGHT)

		#define EXO_SIDE	RIGHT
		#define BILATERAL_MASTER

	#elif(ACTIVE_DEPHY_SUBPROJECT == LEFT)

		#define EXO_SIDE	LEFT
		#define BILATERAL_SLAVE

	#else

		#error "PRJ_DEPHY_DPEB31 requires a subproject"

	#endif

#endif	//PRJ_DEPHY_DPEB31

//DpEb4.2 Exo
#if(ACTIVE_DEPHY_PROJECT == PRJ_DEPHY_DPEB42)

#include "user-mn-DpEb42.h"

	//Enable/Disable sub-modules:
	#define USE_USB
	#define USE_COMM			//Requires USE_RS485 and/or USE_USB
	#define USE_I2C_1			//3V3, IMU & Digital pot
	//#define USE_I2C_2			//3V3, Expansion
	#define USE_I2C_3			//Onboard, Regulate & Execute
	#define USE_IMU				//Requires USE_I2C_1
	#define USE_EEPROM			//Emulated EEPROM, onboard FLASH
	//#define USE_WATCHDOG		//Independent watchdog (IWDG)
	//#define USE_SVM			//Support vector machine

	#define PRJ_DEPHY_DPEB45	//DpEb4.5

	//Runtime finite state machine (FSM):
	#define RUNTIME_FSM1		ENABLED //STATE MACHINE
	#define RUNTIME_FSM2		ENABLED //COMM
	//#define	MANUAL_GUI_CONTROL

	#define BILATERAL

	#if(ACTIVE_DEPHY_SUBPROJECT == RIGHT)

		#define EXO_SIDE	RIGHT
		#define BILATERAL_MASTER
		#define USE_UART4			//Bluetooth #2

	#elif(ACTIVE_DEPHY_SUBPROJECT == LEFT)

		#define EXO_SIDE	LEFT
		#define BILATERAL_SLAVE
		#define USE_UART3			//Bluetooth #1

	#else

		#error "PRJ_DEPHY_DPEB42 requires a subproject"

	#endif

#endif	//PRJ_DEPHY_DPEB31

//****************************************************************************
// Structure(s)
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#endif	//DEPHY
#endif	//INC_USER_MN_H

#endif //BOARD_TYPE_FLEXSEA_MANAGE
