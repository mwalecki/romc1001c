#ifndef _KEYB_H_
#define _KEYB_H_

#include "common.h"

#define KEY_B1		_RB8
#define KEY_B2		_RB9
#define KEY_B3		_RC6
#define KEY_UP	 	KEY_B1
#define KEY_DOWN 	KEY_B2
#define KEY_MODE	KEY_B3

#define KEY_ADDR0	_RC7
#define KEY_ADDR1	_RA10
#define KEY_ADDR2	_RC9
#define KEY_ADDR3	_RC8
#define KEY_ADDRESS	(0x0f & ~(KEY_ADDR0 | (KEY_ADDR1 << 1) | (KEY_ADDR2 << 2) | (KEY_ADDR3 << 3)))

void KEYB_Proc();
void KEYB_Config();

#endif
