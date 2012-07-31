#include "keyb.h"
#include "uart2.h"
#include "control.h"

extern PARAMS_St Params;
extern REFERENCE_Un* Reference;

void KEYB_Proc()
{	
	if(!KEY_MODE)							// if MODE pressed
	{
		if(Params.mode==M_ERROR)
			ModeSwitch(M_MANUAL);
	}	
	else
	{
		if(Params.mode == M_MANUAL)
		{
			if(!KEY_UP && KEY_DOWN)			// if UP pressed
			{
				Reference->dir = D_FWD;
			}
			else if(KEY_UP && !KEY_DOWN)	// if DOWN pressed
			{
				Reference->dir = D_REV;
			}
			else
			{
				Reference->dir = D_STOP;
			}
		}	
	}
}

void KEYB_Config()
{
	_TRISB8 = 1;			// #### 					RB8 as Input: B1
//	_CN22IE = 1;			// enable interrupt
	_CN22PUE = 1;			// enable pull-up resistor
	_TRISB9 = 1;			// #### 					RB9 as Input: B2
//	_CN21IE = 1;			// enable interrupt
	_CN21PUE = 1;			// enable pull-up resistor
	_TRISC6 = 1;			// #### 					RC6 as Input: B3
//	_CN18IE = 1;			// enable interrupt
	_CN18PUE = 1;			// enable pull-up resistor
	
//	IFS1bits.CNIF = 0;	// clear Input Change Notification Interrupt Flag 
//	IPC4bits.CNIP = 1;	// set Input Change Notification Interrupt Priority
//	IEC1bits.CNIE = 1;	// enable Input Change Notification Interrupt

}


