#ifndef _CONTROL_H_
#define _CONTROL_H_

#include "common.h"
#include "encoder.h"
#include "adc.h"
#include "motor.h"
#include "timers.h"
#include <dsp.h>

void PositionPID_Config();
void CurrentPID_Config();

extern STDOWNCNT_St	STDownCnt[ST_Downcounters];
inline void StatusUpdate();
inline void MOTOR_Control();
inline void CurrentControl();
inline void ModeSwitch(u8 mode);
inline void PID_CoeffsUpdate(u8 force);

#endif
