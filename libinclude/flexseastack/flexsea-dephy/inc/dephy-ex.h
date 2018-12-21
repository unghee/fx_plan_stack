#ifdef BOARD_TYPE_FLEXSEA_EXECUTE

#ifndef INC_DEPHY_EX_H
#define INC_DEPHY_EX_H

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

void init_dephy(void);
void dephy_fsm(void);

//****************************************************************************
// Definition(s):
//****************************************************************************

//List of projects - Dephy only:
#define PRJ_DEPHY_NULL			0
#define PRJ_DEPHY_DPEB31		1	//DpEb3.1 Exo (2017)
#define PRJ_DEPHY_DPEB42		2	//DpEb4.2 Exo (2018)

//Leg:
#define RIGHT					1
#define LEFT					2

//Step 1) Select active project (from list):
//==========================================

#define ACTIVE_DEPHY_PROJECT		PRJ_DEPHY_DPEB42
#define ACTIVE_DEPHY_SUBPROJECT		RIGHT

//Step 2) Customize the enabled/disabled sub-modules:
//===================================================

//Similar to Simple Motor, but specialized for DpEb 3.1
#if(ACTIVE_DEPHY_PROJECT == PRJ_DEPHY_DPEB42)

	//Enable/Disable sub-modules:
	#define USE_RS485
	//#define USE_USB
	#define USE_COMM			//Requires USE_RS485 and/or USE_USB
	//#define USE_QEI
	//#define USE_TRAPEZ
	#define USE_I2C_0			//3V3, Onboard (Manage)
	#define USE_I2C_1			//5V, External (Angle sensor)
	//#define USE_STRAIN		//Requires USE_I2C_1
	#define USE_AS5047			//16-bit Position Sensor, SPI
	#define USE_AS5048B			//Joint angle sensor (I2C)
	#define USE_EEPROM			//
	#define USE_I2T_LIMIT		//I2t current limit

	//Motor type and commutation:
	#define MOTOR_COMMUT		COMMUT_SINE
	#define MOTOR_TYPE			MOTOR_BRUSHLESS
	#define MOTOR_ORIENTATION 	CLOCKWISE_ORIENTATION

	//Runtime finite state machine (FSM):
	#define RUNTIME_FSM			ENABLED

	//Encoders:
	#define ENC_CONTROL			ENC_AS5047
	#define ENC_COMMUT			ENC_AS5047
	#define ENC_DISPLAY			ENC_CONTROL
	
	#define CURRENT_ZERO		((int32)2048)

	//Slave ID:
	#define SLAVE_ID			FLEXSEA_EXECUTE_1

	//Project specific definitions:
	//...

#endif  //PROJECT_DPEB31

//****************************************************************************
// Structure(s)
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#endif	//INC_DEPHY_EX_H

#endif //BOARD_TYPE_FLEXSEA_EXECUTE
