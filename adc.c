#include "adc.h"
#include "control.h"

extern PARAMS_St Params;
extern RECORD_St Record;
extern STATUS_St* Status;	// Pointer to current Status-write-to register
extern ADC_St ADC;
// 3A = 250



void ADC_Config(void)
{
 // 12 bit, signed integer output format => range -2046..2045
 
		AD1CON1bits.FORM   	= 0b01;		// Data Output Format: Signed Integer
		AD1CON1bits.SSRC   	= 0b011;	// Sample Clock Source: PWM1 interval ends sampling and starts conversion
		AD1CON1bits.ASAM   	= 1;		// ADC Sample Control: Sampling begins immediately after conversion
		AD1CON1bits.AD12B  	= 1;		// 12-bit ADC operation
		
		AD1CON2bits.CSCNA 	= 1;		// Scan Input Selections for CH0+ during Sample A bit
//		AD1CON2bits.CHPS  	= 0;		// Converts CH0 - unimplemented when AD12B = 1
 
		AD1CON3bits.ADRC 	= 0;		// ADC Clock is derived from System Clock
		AD1CON3bits.ADCS 	= 100;		// ADC Conversion Clock, Tadc = (ADCS+1)*Tcy
 
		AD1CON2bits.SMPI    = 0;		// One ADC Channel scanned
 
		//AD1CSSH/AD1CSSL: A/D Input Scan Selection Register
 		AD1CSSLbits.CSS0 	= 1;		// Enable AN0 for channel scan
 
 		//AD1PCFGH/AD1PCFGL: Port Configuration Register
		AD1PCFGL			= 0xFFFF;
		AD1PCFGLbits.PCFG0 	= 0;		// AN0 as Analog Input
 
		IPC3bits.AD1IP 		= 6;		// High Priority
        IFS0bits.AD1IF 		= 0;		// Clear the A/D interrupt flag bit
        IEC0bits.AD1IE 		= 1;		// Enable A/D interrupt 
        AD1CON1bits.ADON 	= 1;		// Turn on the A/D converter
        
		Status->overcurrent = 0;
}

void ADC_Calibrate()
{
	u8 i;
	while(!	ADC.firstMeasurementDone)
		;
	ADC.initialOffset = ADC1BUF0;
}

void ADC_ParameterSetLoad(int paramSetNum)
{
	ADC.ADC2ICoeff = ADCVAL_TO_I_COEFF[paramSetNum];
	ADC.currentInsen = ADC_CURRENT_INSEN[paramSetNum];
}

inline int ADC_Current()
{
	int temp_i;
	temp_i = ADC1BUF0 - ADC.initialOffset;
	
	if(temp_i > ADC.currentInsen)
		temp_i -= ADC.currentInsen;
	else if(temp_i < -ADC.currentInsen)
		temp_i += ADC.currentInsen;
	else
		temp_i = 0;
	
	return (int)(ADC.ADC2ICoeff * temp_i);
	
	//return ADCVAL_TO_I_COEFF * (ADC1BUF0 - ADC_OFFSET);
}	

int ADC_CurrentFilter1()
{
	static int curr[] = {0,0,0,0,0,0,0,0};
	const int intTabLen = 8;
	int ret;
	int i;
	
	ret = 0;
	for(i=0; i<intTabLen-1; i++)
	{
		curr[i] = curr[i+1];
		ret += curr[i] / intTabLen;
	}
	curr[intTabLen-1] = ADC_Current();
	ret += curr[intTabLen-1] / intTabLen;
	
	return ret;
}	

int ADC_OvercurrentEmergencyStop()
{
	const int singleOvercurrentLimit = 50;
	static int singleOvercurrentCnt = 0;
	int motorCurrent;
	
	motorCurrent = ADC_CurrentFilter1();
	
	
	if(Params.maxCurrent != 0)
	{
		if(motorCurrent > Params.maxCurrent || motorCurrent < -Params.maxCurrent)
			singleOvercurrentCnt ++;
		else
			singleOvercurrentCnt = 0;
		
		
		if(singleOvercurrentCnt == singleOvercurrentLimit)
		{
			ModeSwitch(M_ERROR);
			Status->overcurrent = 1;
			singleOvercurrentCnt = 0;
		}	
	}
	return 0;
}

void __attribute__((interrupt, no_auto_psv)) _ADC1Interrupt(void)
{
	#ifdef PID_CASCADE
		if(Params.mode == M_CURRENT || Params.mode == M_TEST_CURRENT || Params.mode == M_POSITION)
	#else
		if(Params.mode == M_CURRENT || Params.mode == M_TEST_CURRENT)
	#endif	
		CurrentControl();
	
	Status->current = ADC_Current();
	ADC.firstMeasurementDone = 1;
    IFS0bits.AD1IF = 0;		// Clear the ADC1 Interrupt Flag
}
