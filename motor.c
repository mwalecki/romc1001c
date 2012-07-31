#include "motor.h"



void MOTOR_Config()
{
	
	_TRISB13 = 0;	// RB13 as Output: MOTOR_L2
	_TRISB15 = 0;	// RB15 as Output: MOTOR_L1
	
	P1TCONbits.PTMOD = 0b00;	// PWM time base: free running mode
	P1TCONbits.PTCKPS = 1;	// PWM time base input clock prescaler: 1:4
	P1TCONbits.PTOPS = 0;	// PWM time base output clock postscaler: 1:1
	// PxTPER = F_CY / (F_PWM * (PxTMR Prescaler)) - 1
	// F_CY = 10MHz, F_PWM = 1kHz
	_PTPER = 500;
	PWM1CON1bits.PMOD1 = 1;	// PWM I/O pin pair 1 is in the Independent PWM Output mode
	PWM1CON1bits.PMOD2 = 1;	// PWM I/O pin pair 2 is in the Independent PWM Output mode
	PWM1CON1bits.PMOD3 = 1;	// PWM I/O pin pair 3 is in the Independent PWM Output mode
	PWM1CON1bits.PEN1H = 1;	// PWM1H pin is enabled for PWM output
	PWM1CON1bits.PEN2H = 1;	// PWM2H pin is enabled for PWM output
	PWM1CON1bits.PEN3H = 1;	// PWM3H pin is enabled for PWM output (SYNCH signal)
	PWM1CON1bits.PEN1L = 0;	// PWM1L pin disabled, I/O pin becomes general purpose I/O
	PWM1CON1bits.PEN2L = 0;	// PWM2L pin disabled, I/O pin becomes general purpose I/O
	PWM1CON1bits.PEN3L = 0;	// PWM3L pin disabled, I/O pin becomes general purpose I/O
	
	PWM1CON2bits.IUE = 1;	// Immediate update of PWM enabled

	P1OVDCONbits.POVD1H = 1;	// Output on PWMx I/O pin is controlled by the PWM generator
	P1OVDCONbits.POVD2H = 1;	// Output on PWMx I/O pin is controlled by the PWM generator
	P1OVDCONbits.POVD3H = 1;	// Output on PWMx I/O pin is controlled by the PWM generator
	P1OVDCONbits.POVD1L = 0;	// Output on PWMx I/O pin is controlled by the value in the corresponding POUTxH:POUTxL bit
	P1OVDCONbits.POVD2L = 0;	// Output on PWMx I/O pin is controlled by the value in the corresponding POUTxH:POUTxL bit
	P1OVDCONbits.POVD3L = 0;	// Output on PWMx I/O pin is controlled by the value in the corresponding POUTxH:POUTxL bit
	
	P1DC1 = 0;	// Duty cycle initialization
	P1DC2 = 0;	// Duty cycle initialization
	P1DC3 = 500;	// Duty cycle initialization

	P1SECMPbits.SEVTDIR = 0;	// A Special Event Trigger occurs when the PWM time base is counting upward
	PWM1CON2bits.SEVOPS = 0;	// Select PWM Special Event Trigger Output Postscale value to 1:1
	P1SECMPbits.SEVTCMP = 0;	// Assign special event compare value
	
	P1TCONbits.PTEN = 1;		// Enabling PWM Pulse Generation
 
	P1TCONbits.PTEN = 1;
}

void MOTOR_PwmSet(int output)
{
	static u8 fail;
	unsigned int out;

	ADC_OvercurrentEmergencyStop();

	output = -output;	// Polaryzacja silnika
	
	if(SW_EDGE1)
	{
		if(fail == 0)
		{
			ModeSwitch(M_ERROR);
			fail = 1;
		}
		if(output > 0)
			output = 0;
	}	
	else if(SW_EDGE2)
	{
		if(fail == 0)
		{
			ModeSwitch(M_ERROR);
			fail = 1;
		}
		if(output < 0)
			output = 0;
	}	
	else
		fail = 0;
		
	
	if(output == 0)
	{
		MOTOR_H1_DUTY = 0;
		MOTOR_H2_DUTY = 0;
		MOTOR_L1 = 0;
		MOTOR_L2 = 0;
		Params.dir = D_STOP;
		
		P1SECMPbits.SEVTCMP = 0;
	}
	else if(output > 0)
	{
		MOTOR_H2_DUTY = 0;
		MOTOR_L1 = 0;
		
		out = output;
		MOTOR_H1_DUTY = out;
		MOTOR_L2 = 1;
		Params.dir = D_FWD;
		
		P1SECMPbits.SEVTCMP = (out >> 1);
	}
	else
	{
		output = -output;
		
		MOTOR_H1_DUTY = 0;
		MOTOR_L2 = 0;
		
		out = output;
		MOTOR_H2_DUTY = out;
		MOTOR_L1 = 1;
		Params.dir = D_REV;
		
		P1SECMPbits.SEVTCMP = (out >> 1);
	}
}
