#ifndef _MOTOR_H_
#define _MOTOR_H_

#include "common.h"
#include "encoder.h"
#include "adc.h"

#define MOTOR_PWM_PERIOD	2000
#define MOTOR_MANUAL_DUTY	150

#define MOTOR_H1_DUTY	P1DC1
#define MOTOR_H2_DUTY	P1DC2
#define MOTOR_L2			_LATB13
#define MOTOR_L1			_LATB15

#define PS_FAULT		_RB11

extern PARAMS_St Params;

void MOTOR_Config();
void MOTOR_PwmSet(int output);

#endif
