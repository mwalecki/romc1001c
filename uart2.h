#ifndef _UART2_H_
#define _UART2_H_

#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define U2BAUDRATE 38400              
#define U2BRGVAL   ((FCY/U2BAUDRATE)/16)-1

// :BOOT0\r\n = 3a;42;4f;4f;54;30;0d;0a	; 3520bps
// :07:BOOT\r\n = 3a;30;37;3a;42;4f;4f;54;0d;0a ; 38400bps [Latest version]

#define UART2_TX_EN	_LATB6

#define UART2_TxBufSz		32
#define UART2_RxBufSz		32
#define UART2_INTERP_BUF_SZ	32
#define UART2_BYTE_FRAME_SZ	4

#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_
//Interpreter:
#define _GROUP(buf, grName, grNameLen)									if(strncmp(buf, grName, grNameLen)==0){
#define _ENDGROUP														}
#define _GET_SET_MEMBER(buf, varName, memName, memNameLen, grNameLen)	if(strncmp(buf+grNameLen, memName, memNameLen)==0){if(buf[(grNameLen+memNameLen)]=='?'){/*ltoa(varName, buf, 10);*/while(--i != 0);UART2_SendNBytes(buf, strlen(buf));}else{varName=atol(buf+(grNameLen+memNameLen+1));}}
#define _IF_MEMBER_THEN(buf, memName, memNameLen, grNameLen)			if(strncmp(buf+grNameLen, memName, memNameLen)==0)
#endif

void UART2_Proc();
void UART2_SendNBytes(char *buf, u8 n);
void DMA0_UART2TX_Config(void);
void UART2_Config(void);
void __attribute__ ((interrupt, no_auto_psv)) _U2ErrInterrupt(void);


#endif
