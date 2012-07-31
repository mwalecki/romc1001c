#ifndef _UART1_H_
#define _UART1_H_

#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#define U1BAUDRATE 1000000              
//#define U1BRGVAL   ((FCY/U1BAUDRATE)/4)-1
#define U1BRGVAL   10

#define UART1_TxBufSz		256
#define UART1_RxBufSz		256
#define UART1_INTERP_BUF_SZ	256
#define UART1_BYTE_FRAME_SZ	1

#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_
//Interpreter:
#define _GROUP(buf, grName, grNameLen)									if(strncmp(buf, grName, grNameLen)==0){
#define _ENDGROUP														}
#define _GET_SET_MEMBER(buf, varName, memName, memNameLen, grNameLen)	if(strncmp(buf+grNameLen, memName, memNameLen)==0){if(buf[(grNameLen+memNameLen)]=='?'){/*ltoa(varName, buf, 10);*/UART1_SendNBytes(buf, strlen(buf));}else{varName=atol(buf+(grNameLen+memNameLen+1));}}
#define _IF_MEMBER_THEN(buf, memName, memNameLen, grNameLen)			if(strncmp(buf+grNameLen, memName, memNameLen)==0)
#endif



void UART1_StandardStatusPrepare(void);
void UART1_StandardStatusSend(void);
void UART1_Proc();
inline void UART1_TransferTxBuffer(u8 n);
void UART1_SendNBytes(char *buf, u8 n);
void DMA3_UART1TX_STATUS_Config(void);
void __attribute__((interrupt, no_auto_psv)) _DMA3Interrupt(void);
inline void UART1_SendStatus(void);
void DMA1_UART1TX_Config(void);
void UART1_Config(void);
void __attribute__ ((interrupt, no_auto_psv)) _U1ErrInterrupt(void);


#endif
