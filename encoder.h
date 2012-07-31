#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "common.h"


extern PARAMS_St Params;
extern STATUS_St* Status;
extern ENC_St Enc;
extern const u8	ENC_AB_SWAP[];
extern const u8	ENC_I_POLARITY[];
extern const u8	SW3_POLARITY[];
extern const u8	SW1_SW2_SWAP[];

//#define ENC_RES	4000
#define ENC_RES	0xffff

#define SW_1		_RB3
#define SW_2		_RB2
#define SW_3		_RA1
#define SW_EDGE1 	((SW_1 && (!Enc.sw1sw2Swap)) || (SW_2 && Enc.sw1sw2Swap))
#define SW_EDGE2 	((SW_2 && (!Enc.sw1sw2Swap)) || (SW_1 && Enc.sw1sw2Swap))
#define SW_SYNC		(SW_3 ^ Enc.sw3Polarity)
#define ER_STOP			_LATC3
#define ER_STOP_TRIS	_TRISC3
#define ENC_I		(_RC2 ^ Enc.IPolarity)

#define ER_STOP_CONFIG	ER_STOP_TRIS=0

void ENC_ParameterSetLoad(int paramSetNum);
void ENC_Config();
void __attribute__ ((interrupt, no_auto_psv)) _QEI1Interrupt(void);
s32 ENC_Position();

#endif
