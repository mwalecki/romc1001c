#ifndef _TIMERS_H_
#define _TIMERS_H_

#include "common.h"

#define ST_StatusLed	0
#define ST_Period0			250 // all values in [ms]
#define ST_SendStatus	1
#define ST_Period1			250
#define ST_ErStop		2
#define ST_Period2			1000
#define ST_Downcounters	3

#define ST_SetPeriod(n,i)	{STDownCnt[n].period = i; STDownCnt[n].val=i;}
#define ST_Reset(n)			STDownCnt[n].val=STDownCnt[n].period

#define US_TO_PR34						5
#define STATUS_WRITE_TO_READ_PRECED_US	1000
#define MAX_CORR_PREC_CNT				10
#define PREC_CNT_HIST_L					4
#define PREC_CNT_HIST_H					6

void TIMER1_Config();
void TIMER2_Config();
void TIMER3_Config();
void TIMER4_Config();
inline void TIMER4_MeasureStatusReadInterval();
void __attribute__((interrupt,no_auto_psv)) _T1Interrupt( void );
void __attribute__((interrupt,no_auto_psv)) _T2Interrupt( void );
void SYSTICK_Config(void);

#endif
