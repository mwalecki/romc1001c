#ifndef _MBI5167_H_
#define _MBI5167_H_

#include "common.h"

//##                                      #### ######## ################ MBI5167 port map:

#define LED_CLK				_LATA8
#define LED_SDI				_LATA9
#define LED_LE				_LATA7
#define LED_TRIS_CLK		_TRISA8
#define LED_TRIS_SDI		_TRISA9
#define LED_TRIS_LE			_TRISA7

void LED_Proc();
void LED_Config();
void LED_Set(u8 newSet);

#endif	// ifndef _MBI5167_H_
