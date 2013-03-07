#include <p33FJ64MC204.h>
#include <stdlib.h>

//##                                      #### ######## ################ Configuration Bits Setup
#ifndef BOOTLOADER
	_FICD(ICS_PGD1				// DEBUG: communicate on PGC1/EMUC1 and PGD1/EMUD1
		& JTAGEN_OFF);			// JTAG Disabled
	_FPOR(FPWRT_PWR128);		// Power On Reset Timer: 128ms
	_FWDT( FWDTEN_OFF );
	
	_FOSCSEL(FNOSC_PRIPLL);	// Select Primary oscillator (XT, HS, EC) w/ PLL
	_FOSC(FCKSM_CSECMD		// Enable Clock Switching, no Clock Monitor
		& IOL1WAY_OFF			// Single configuration for remappable I/O OFF
		& OSCIOFNC_ON			// OSC2 Pin function: Digital I/O
		& POSCMD_HS);			// Configure Primary Oscillator in High Speed mode
#else
	_FICD(JTAGEN_OFF & ICS_PGD1);
//	_FPOR(FPWRT_PWR128);
//	_FWDT( FWDTEN_OFF );
	
	_FOSCSEL(FNOSC_FRC & IESO_OFF);
	_FOSC(FCKSM_CSECMD & OSCIOFNC_ON & POSCMD_XT);
#endif //BOOTLOADER

//##                                      #### ######## ################ INCLUDES
#include "common.h"
#include "led.h"
#include "keyb.h"
#include "motor.h"
#include "adc.h"
#include "encoder.h"
#include "control.h"
#include "uart1.h"
#include "uart2.h"
#include "timers.h"
#include "reset.h"
#include "mycrc.h"
#include "nfv2.h"

//##                                      #### ######## ################ GLOBALS

u16 u16temp;

SYNCHRONIZER_St	DataSynchronizer;

PARAMS_St		Params;
s16				referenceCurrent;
volatile UART_St	Uart1;
volatile UART_St	Uart2;
vs8 uart2RxBuffer[UART2_RxBufSz];
STDOWNCNT_St	STDownCnt[ST_Downcounters];

REFERENCE_Un	ReferenceA;
REFERENCE_Un	ReferenceB;
REFERENCE_Un*	Reference;	// Pointer to current Reference-read-from register

RECORD_St 		Record;

vs8 		uart2TxBuffer[UART2_TxBufSz] __attribute__((space(dma)));
uint8_t 	*standardStatusTxBuffer 		__attribute__((space(dma)));
uint8_t 	standardStatusTxBuffer1[32] __attribute__((space(dma)));
uint8_t 	standardStatusTxBuffer2[32] __attribute__((space(dma)));
vs8 		uart1TxBuffer[UART1_TxBufSz] __attribute__((space(dma)));

uint8_t *standardStatusTxBytes;
uint8_t standardStatusTxBytes1;
uint8_t standardStatusTxBytes2;

STATUS_St	StatusA __attribute__((space(dma)));	// Status register
STATUS_St	StatusB __attribute__((space(dma)));	// Status register
STATUS_St*	Status;									// Pointer to current Status-write-to register

ENC_St Enc;
ADC_St ADC;

volatile NF_STRUCT_ComBuf	NFComBuf __attribute__((packed));

//##                                      #### ######## ################ CARD SPECIFIC PARAMETERS

u8 ledOn;
// 1001D Hardware Current Sensor Specification
// Axis name																		TFG		CONV													TFG		TRACK
// Driver card address				0		1		2		3		4		5		6		7		8		9		A		B		C		D		E		F
// Current sensor LEM				HX15-P	HX15-P	HX15-P	HX15-P	HX15-P	HX15-P	HX15-P	HX03-P	HX15-P	HX15-P	HX15-P	HX15-P	HX15-P	HX15-P	HX03-P	HX15-P	
// R50 [Ohm]						1k		1k		1k		1k		1k		0		0		1k		1k		1k		1k		1k		1k		0		0		1k
// R53 [Ohm]						1k		1k		1k		1k		1k		10k		10k		1k		1k		1k		1k		1k		1k		10k		10k		1k
const float	ADCVAL_TO_I_COEFF[]={	12.3,	12.3,	12.3,	12.3,	12.3,	0.332,	0.332,	12.3,	12.3,	12.3,	12.3,	12.3,	12.3,	0.332,	0.332,	12.3	};
const int	ADC_CURRENT_INSEN[]={	10,		10,		10,		10,		10,		10,		10,		10,		10,		10,		10,		10,		10,		10,		10,		10		};
const u8	ENC_AB_SWAP[]={			0,		0,		0,		0,		0,		0,		0,		0,		1,		0,		0,		0,		1,		1,		1,		0		};
const u8	ENC_I_POLARITY[]={		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0		};
const u8	SW3_POLARITY[]={		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		0,		1,		1,		0		};
const u8	SW1_SW2_SWAP[]={		0,		0,		0,		0,		0,		0,		1,		0,		1,		1,		1,		0,		0,		0,		0,		1		};


//##                                      #### ######## ################ PROTOTYPES
void CLOCK_Config() 
{
	#ifdef INTERNAL_OSC
		PLLFBD 				=	41;	// M = 40
		CLKDIVbits.PLLPOST 	=	0;	// N1 = 2
		CLKDIVbits.PLLPRE 	=	0;	// N2 = 2
		OSCTUN 				= 	0;
		RCONbits.SWDTEN 	= 	0;
	 
		// Clock switch to incorporate PLL
		__builtin_write_OSCCONH(0x01);		// Initiate Clock Switch to 
											// FRC with PLL (NOSC=0b001)
		__builtin_write_OSCCONL(0x01);		// Start clock switching
	 
		// disable two next lines when emulating in proteus VMS
		while (OSCCONbits.COSC != 0b001);	// Wait for Clock switch to occur	
		while(OSCCONbits.LOCK != 1) {};
		
		return;
	#endif //INTERNAL_OSC
	PLLFBD = 69;						// M = PLLFBD+2		49
	CLKDIVbits.PLLPRE = 5;				// N1 = 4					3
	CLKDIVbits.PLLPOST = 0;				// N2 = 2					0
	__builtin_write_OSCCONH(0b011);		// Unlock and Initiate Clock Switch to Posc with PLL (NOSC=0b011)
	__builtin_write_OSCCONL(0x01);		// Start clock switching
	while(OSCCONbits.COSC != 0b011);	// Wait for Clock switch to occur	
	while(OSCCONbits.LOCK != 1) {};		// Wait for PLL to lock
}





	

	

int main()
{
	char buf[100];
	
	Params.mode = M_MANUAL;
//	Enc.ovfCnt = 100;
	
	
	CLOCK_Config();
	
	crcInit();
	KEYB_Config();
	LED_Config();
	DMA0_UART2TX_Config();
	DMA1_UART1TX_Config();
	DMA3_UART1TX_STATUS_Config();
	UART1_Config();
	UART2_Config();
	MOTOR_Config();
	ADC_ParameterSetLoad(KEY_ADDRESS);
	ADC_Config();
	ENC_ParameterSetLoad(KEY_ADDRESS);
	ENC_Config();
	TIMER1_Config();	// SysTick
	TIMER2_Config();	// MotorControl & PID
	TIMER3_Config();	// Status Update
	TIMER4_Config();	// Status Inquiry Interval Measure
	SYSTICK_Config();
	ER_STOP_CONFIG;
	
	PositionPID_Config();
	CurrentPID_Config();
	
	PID_CoeffsUpdate(1);
	
	ADC_Calibrate();

	StatusA.startByte	= '#';
	StatusB.startByte	= '#';	
	Status = &StatusA;
	
	Reference = &ReferenceA;

	
	//sprintf(buf,"%02d: Startup ", KEY_ADDRESS);
	//RESET_Status_To_Str(buf+12);
	//UART2_SendNBytes(buf, strlen(buf));
	
	
	// Activate emergency stop at startup
	// in case of accidental reset
	ER_STOP=1;
	ST_Reset(ST_ErStop);
	
	while(1)
	{
		if(STDownCnt[ST_StatusLed].tick == 1)
		{
			LED_Proc();
			STDownCnt[ST_StatusLed].tick = 0;
		}
		if(STDownCnt[ST_SendStatus].tick == 1)	// T == 250ms
		{
			#ifdef UART_STATUS
				//sprintf(buf,"%ld\t%d\r\n", ENC_Position(), ADC_Current());
				//UART2_SendNBytes(buf, strlen(buf));
				UART2_SendNBytes(Uart1.rxBuf, 6);
				STDownCnt[ST_SendStatus].tick = 0;
			#endif
			
			PID_CoeffsUpdate(0);		// Calls PIDCoeffCalc() only if K values change (don't force update)
		}
		if(STDownCnt[ST_ErStop].tick == 1)
		{
			if(Params.mode == M_BOOT)
			{
				asm("reset");
			}	
			ER_STOP=0;
			STDownCnt[ST_ErStop].tick = 0;
		}
		UART1_Proc();
		UART2_Proc();
		KEYB_Proc();
	}
}

