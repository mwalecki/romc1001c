#ifndef _COMMON_H_
#define _COMMON_H_

#include <p33FJ64MC204.h>
#include <inttypes.h>
#define OLD_STM32LIB_USED
#include "stm32f10x_type.h"

//############################################# PID SECTION ##############

// 
#define MAX_CURRENT 30
#define CURRENT_LIMIT 5

//#define PID_CASCADE

#ifdef PID_CASCADE	// Position PID with output to current controller
#define PID_POS_P	0.9700
#define PID_POS_I	0.0010
#define PID_POS_D	0.0000
#else				// Position PID with direct PWM output
#define PID_POS_P	0.8000
#define PID_POS_I	0.0100
#define PID_POS_D	0.0000
#endif
					// Current PID
#define PID_CURR_P	0.0600
#define PID_CURR_I	0.0500
#define PID_CURR_D	0.0000

//########################################### DEBUG SECTION ##############

//#define INTERNAL_OSC

//#define UART_STATUS

#define RECORD_CURRENT
//#define RECORD_POSITION

#define RECORD_SAMPLES	1000

//####
#ifdef RECORD_CURRENT 
	#define RECORD
#else
	#ifdef RECORD_POSITION 
		#define RECORD
	#endif
#endif

//########################################################################

#define FOSC	81142857
#define FCY		(FOSC/2)
//				81142857 / 2 = 40571428,5

// MRROC++ commands
#define COMMAND_MODE_PWM		0x00
#define COMMAND_MODE_CURRENT	0x01
#define COMMAND_MODE_POSITION	0x02
#define COMMAND_SET_PARAM		0x0f

// command parameters
#define COMMAND_PARAM_SYNCHRO	0x10

// SET_PARAM command parameters
#define PARAM_SYNCHRONIZED		0x10
#define PARAM_MAXCURRENT		0x20
#define PARAM_PID_POS_P			0x30
#define PARAM_PID_POS_I			0x40
#define PARAM_PID_POS_D			0x50
#define PARAM_PID_CURR_P		0x60
#define PARAM_PID_CURR_I		0x70
#define PARAM_PID_CURR_D		0x80
#define PARAM_DRIVER_MODE		0x90

#define COMMAND_MASK_MODE		0x0f
#define COMMAND_MASK_PARAM		0xf0

// Params.mode
#define M_MANUAL	0
#define M_POSITION	1
#define M_CURRENT	2
#define M_PWM		3
#define M_ERROR		4
#define M_BOOT		5
// Params.dir
#define D_STOP		0
#define D_FWD		1
#define D_REV		-1
// 
#define vu8	volatile unsigned char

typedef union{
	u8 synchronized;
	u8 driver_mode;
	u16 maxCurrent;
	u16 pidCoeff;
	u16 largest;
} PARAM_Un;

typedef union{
	s8	dir;
	s16	pwm;
	s16	current;
	s16 speed;
	s32 position;
	s32 largest;
	u8	buf[4];
} REFERENCE_Un;

typedef struct{
	u8	mode;
	u8	dir;
	u8	ledBlink;
	u8	ledOn;
	u8	coeffValid;
	u8	encZeroTrace;
	s16	maxCurrent;
} PARAMS_St;

typedef struct{
	s16 measure[RECORD_SAMPLES];
	s16 output[RECORD_SAMPLES];
	u16 index;
} RECORD_St;

typedef struct{
	vs8 *rxBuf;
	vs8 *txBuf;
	vu8 rxPt, txPt;
	vu8 rxReady	:1;
	vu8 txReady	:1;
} UART_St;

typedef struct{
	vs32 ovfCnt;
	vs32 positionOffset;
	vu8 synchroZero;
	vu8 isSynchronized;
	vu8 swapAB;
	vu8 IPolarity;
	vu8 sw3Polarity;
	vu8 sw1sw2Swap;
	u16 oldPos1Cnt;
} ENC_St;

typedef struct{
	vu32 val;
	vu32 period;
	vu8 tick;
} STDOWNCNT_St;

typedef struct{
	u8 startByte;
	vu8 sw1:1;
	vu8 sw2:1;
	vu8 swSynchr:1;
	vu8 synchroZero:1;
	vu8 powerStageFault:1;
	vu8 overcurrent:1;
	vu8 error:1;
	vu8 isSynchronized:1;
	vs16 current;
	vs32 position;
} STATUS_St;

typedef struct{
	u8 firstMeasurementDone;
	u16 initialOffset;
	float ADC2ICoeff;
	int currentInsen;
} ADC_St;

typedef struct{
	vu8 statusReady		:1;
	vu8 synchronized	:1;
	vu16 lastPrecedenceUs;
	vu8 correctPrecedCnt;
} SYNCHRONIZER_St;

#endif
