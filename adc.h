#ifndef _ADC_H_
#define _ADC_H_

#include "common.h"


//const double C_ADCVAL_TO_I_COEFF = -15.0;
//#define ADCVAL_TO_I_COEFF -15 //LEM HX15-P
//#define ADC_CURRENT_INSEN	10
/*
LEM HX15-P
R50 = 1kOhm
R53 = 1kOhm
R55 = R52 = 6,2k Ohm
3.00A == 250
*/

//#define C_ADCVAL_TO_I_COEFF	-1.280
//#define ADCVAL_TO_I_COEFF 	-1
//#define ADC_CURRENT_INSEN	10
/*
LEM HX03-P
R50 = 0 Ohm
R53 = 10k Ohm
R55 = R52 = 6,2k Ohm
2.40A == 2015
*/

extern const float	ADCVAL_TO_I_COEFF[];
extern const int	ADC_CURRENT_INSEN[];

void ADC_Config(void);
void ADC_Calibrate();
void ADC_ParameterSetLoad(int paramSetNum);
inline int ADC_Current();
int ADC_CurrentFilter1();
int ADC_OvercurrentEmergencyStop();
void __attribute__((interrupt, no_auto_psv)) _ADC1Interrupt(void);

#endif
