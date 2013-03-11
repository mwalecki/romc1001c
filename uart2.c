#include "uart2.h"
#include "led.h"
#include "encoder.h"
#include "control.h"
#include "keyb.h"
#include "nf/nfv2.h"

extern PARAMS_St Params;
extern UART_St	Uart2;
extern REFERENCE_Un* Reference;
extern RECORD_St Record;
extern vs8 uart2TxBuffer[UART2_TxBufSz] __attribute__((space(dma)));
extern vs8 uart2RxBuffer[UART2_RxBufSz];

extern NF_STRUCT_ComBuf NFComBuf;

//##                                      #### ######## ################ UART2 Config
void UART2_Config(void)
{
	_U2RXR = 7;		// map UART2 Receiver to RP7
	_RP5R = 5;		// map UART2 Transmitter to RP5
	_TRISB6 = 0;	// RB6 as Output: UART2_EN
	_LATB6 = 0;		// Init as input

	Uart2.txBuf = uart2TxBuffer;
	Uart2.rxBuf = uart2RxBuffer;
	
	U2MODEbits.STSEL = 0;			// 1-stop bit
	U2MODEbits.PDSEL = 0;			// No Parity, 8-data bits
	U2MODEbits.ABAUD = 0;			// Autobaud Disabled
	U2BRG = U2BRGVAL;					// BAUD Rate Setting
	U2STAbits.UTXISEL0 = 1;			// Interrupt after all Tx operations completed
	U2STAbits.UTXISEL1 = 0;			                            
	U2STAbits.URXISEL  = 0;			// Interrupt after one RX character is received
	U2MODEbits.UARTEN   = 1;		// Enable UART
	U2STAbits.UTXEN     = 1;		// Enable UART Tx
	
	IFS1bits.U2TXIF = 0;
	IEC1bits.U2TXIE = 1;				// UART2 Transmitter Interrupt Enable
	IPC7bits.U2TXIP = 3;				// mid-range interrupt priority
	IFS1bits.U2RXIF = 0;
	IEC1bits.U2RXIE = 1;				// UART2 Receiver Interrupt Enable
	IPC7bits.U2RXIP = 3;				// mid-range interrupt priority
	IEC4bits.U2EIE = 0;				// UART2 Error Interrupt Enable
	
	U2STAbits.OERR=0;
	

}

//##                                      #### ######## ################ DMA0 Config
void DMA0_UART2TX_Config(void)
{
	DMA0REQ = 0x001F;					// Select UART2 Transmitter
	DMA0PAD = (volatile unsigned int) &U2TXREG;
	
	DMA0CONbits.AMODE = 0;	// Addressing: Register Indirect with Post-Increment
	DMA0CONbits.MODE  = 1;	// One-Shot Mode
	DMA0CONbits.DIR   = 1;	// RAM To Peripheral
	DMA0CONbits.SIZE  = 1;	// Byte-Size
	//DMA0CNT = 7;				// 8 DMA requests
	DMA0STA = __builtin_dmaoffset(uart2TxBuffer);	// Associated Buffer
	IFS0bits.DMA0IF  = 0;			// Clear DMA Interrupt Flag
	IEC0bits.DMA0IE  = 1;			// Enable DMA interrupt
}

//##                                      #### ######## ################ Interrupt Handlers

void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void)
{
	IFS1bits.U2TXIF = 0;				// Clear UART2 Transmitter Interrupt Flag
	IEC1bits.U2TXIE = 1;				// UART2 Transmitter Interrupt Enable
	IFS0bits.DMA0IF = 0;				// Clear the DMA0 Interrupt Flag;
}

void __attribute__ ((interrupt, no_auto_psv)) _U2RXInterrupt(void) 
{
	Uart2.rxBuf[Uart2.rxPt] = U2RXREG;
	if(Uart2.rxPt==0 && Uart2.rxBuf[0]!='*' && Uart2.rxBuf[0]!=':' && Uart2.rxBuf[0]!='#')
		Uart2.rxPt=0;
	else if(Uart2.rxBuf[0]!='#' && (Uart2.rxBuf[Uart2.rxPt]==10 || Uart2.rxBuf[Uart2.rxPt]==10))
	{
		Uart2.rxReady = 1;
		Uart2.rxPt = 0;
	}
	else if(Uart2.rxBuf[0]=='#' && Uart2.rxPt==UART2_BYTE_FRAME_SZ)
	{
		Uart2.rxPt = 0;
	}
	else if(Uart2.rxPt<UART2_RxBufSz-1)
	{
		Uart2.rxPt++;
	}
	else
	{
		Uart2.rxPt = 0;
	}	
	IFS1bits.U2RXIF = 0;
}

void __attribute__ ((interrupt, no_auto_psv)) _U2TXInterrupt(void) 
{
	//LED_Set(0xff);
	UART2_TX_EN = 0;	// Transmit Buffer empty, set RS-485 Transceiver for Receive
	IEC1bits.U2RXIE = 1;				// UART2 Receiver Interrupt Enable
	IFS1bits.U2TXIF = 0;				// Clear UART2 Transmitter Interrupt Flag
}

void __attribute__ ((interrupt, no_auto_psv)) _U2ErrInterrupt(void)
{
	IFS4bits.U2EIF = 0; // Clear the UART2 Error Interrupt Flag
}

void UART2_SendNBytes(char *buf, u8 n)
{
	unsigned int i;
	IEC1bits.U2RXIE = 0;				// UART2 Receiver Interrupt Disable
	IEC1bits.U2TXIE = 0;				// UART2 Transmitter Interrupt Disable
	UART2_TX_EN = 1;					// Set RS-485 Transceiver fo Transmit
	for(i=0;i<n;i++)
		uart2TxBuffer[i]=buf[i];
	DMA0CNT = n-1;				// n DMA requests
	DMA0CONbits.CHEN  = 1;	// Re-enable DMA0 Channel
				
	DMA0STA = __builtin_dmaoffset(uart2TxBuffer);	// Associated Buffer
	DMA0REQbits.FORCE = 1;	// Manual mode: Kick-start the first transfer
}

void UART2_Proc()
{
	u16 i=1000, iter;
	u8 addrTemp=0;
	
	static char interpBuf[UART2_INTERP_BUF_SZ];
	char* iBufPt;
	if(Uart2.rxReady != 1)
		return;
	strncpy(interpBuf, (const char*)Uart2.rxBuf, UART2_INTERP_BUF_SZ);
	Uart2.rxReady = 0;
	
	addrTemp = 10*(interpBuf[1]-0x30)+(interpBuf[2]-0x30);				// Address decoder
	if(interpBuf[0]==':' && ((u8)addrTemp == (u8)KEY_ADDRESS || (u8)addrTemp == ADDR_BROADCAST))
		iBufPt = interpBuf +3;
	else
		return;
	
	_GROUP(iBufPt, ":IDN?", 5)
		sprintf(interpBuf,"MW1001 IRP6/Sarkogripper %s \r\n", __DATE__);
		while(--i != 0);
		UART2_SendNBytes(interpBuf, strlen(interpBuf));
	_ENDGROUP
	else
	_GROUP(iBufPt, ":ADDR?", 6)
		sprintf(interpBuf,"%d\r\n", KEY_ADDRESS);
		while(--i != 0);
		UART2_SendNBytes(interpBuf, strlen(interpBuf));
	_ENDGROUP
	else
	_GROUP(iBufPt, ":BOOT", 5)
		ModeSwitch(M_BOOT);
	_ENDGROUP
	else
	_GROUP(iBufPt, ":RST", 4)
		asm("reset");
	_ENDGROUP
	else
	_GROUP(iBufPt, ":MODE", 5)
		_IF_MEMBER_THEN(iBufPt, ":MAN", 4, 5)
			ModeSwitch(M_MANUAL);	
		else
		_IF_MEMBER_THEN(iBufPt, ":PWM", 4, 5)
			ModeSwitch(M_PWM);
		else
		_IF_MEMBER_THEN(iBufPt, ":CUR", 4, 5)
			ModeSwitch(M_CURRENT);
		else
		_IF_MEMBER_THEN(iBufPt, ":POS", 4, 5)
			ModeSwitch(M_POSITION);
		else
		_IF_MEMBER_THEN(iBufPt, ":TCUR", 5, 5)
			ModeSwitch(M_TEST_CURRENT);
	_ENDGROUP
	else
	_GROUP(iBufPt, ":PWM", 4)
		_GET_SET_MEMBER(iBufPt, Reference->pwm, ":SET", 4, 4)
		#ifdef RECORD
			Record.index = 0;
		#endif
	_ENDGROUP
	else
	_GROUP(iBufPt, ":CUR", 4)
		_GET_SET_MEMBER(iBufPt, Reference->current, ":SET", 4, 4)
		#ifdef RECORD
			Record.index = 0;
		#endif
	_ENDGROUP
	else
	_GROUP(iBufPt, ":POS", 4)
		_GET_SET_MEMBER(iBufPt, Reference->position, ":SET", 4, 4)
		#ifdef RECORD
			Record.index = 0;
		#endif
	_ENDGROUP
	else
	_GROUP(iBufPt, ":ENC", 4)
		_IF_MEMBER_THEN(iBufPt, ":POS?", 5, 4)
		{
			while(--i != 0);
			sprintf(interpBuf,"%lu\r\n", ENC_Position());
			UART2_SendNBytes(interpBuf, strlen(interpBuf));
		}	
	_ENDGROUP
	else
	_GROUP(iBufPt, ":STAT", 5)
		_IF_MEMBER_THEN(iBufPt, ":RES?", 4, 5)
		{
			RESET_Status_To_Str(interpBuf);
			UART2_SendNBytes(interpBuf, strlen(interpBuf));
		}
		else
		_IF_MEMBER_THEN(iBufPt, ":UART?", 5, 5)
		{
			UART1_Status_To_Str(interpBuf);
			UART2_SendNBytes(interpBuf, strlen(interpBuf));
		}	
	_ENDGROUP
	else
	_GROUP(iBufPt, ":NFCB", 5)
		_IF_MEMBER_THEN(iBufPt, ":ADDR?", 6, 5)
		{
			while(--i != 0);
			sprintf(interpBuf,"%d\r\n", NFComBuf.myAddress);
			UART2_SendNBytes(interpBuf, strlen(interpBuf));
		}
	_ENDGROUP
	else
	_GROUP(iBufPt, ":XXX", 4)
		PID_CoeffsUpdate(1);
	_ENDGROUP
	else
	_GROUP(iBufPt, ":REC", 4)
		_IF_MEMBER_THEN(iBufPt, ":REF?", 5, 4)
		{
			while(--i != 0);
			for(iter=0; iter<RECORD_SAMPLES; iter++){
				i = 10000;
				while(--i != 0);
				sprintf(interpBuf,"%d; ", Record.reference[iter]);
				UART2_SendNBytes(interpBuf, strlen(interpBuf));
			}
			i = 10000;
			while(--i != 0);
			sprintf(interpBuf,"\r\n");
			UART2_SendNBytes(interpBuf, strlen(interpBuf));
		}
		_IF_MEMBER_THEN(iBufPt, ":MEAS?", 6, 4)
		{
			while(--i != 0);
			for(iter=0; iter<RECORD_SAMPLES; iter++){
				i = 10000;
				while(--i != 0);
				sprintf(interpBuf,"%d; ", Record.measure[iter]);
				UART2_SendNBytes(interpBuf, strlen(interpBuf));
			}
			i = 10000;
			while(--i != 0);
			sprintf(interpBuf,"\r\n");
			UART2_SendNBytes(interpBuf, strlen(interpBuf));
		}
		else
		_IF_MEMBER_THEN(iBufPt, ":OUT?", 5, 4)
		{
			while(--i != 0);
			for(iter=0; iter<RECORD_SAMPLES; iter++){
				i = 10000;
				while(--i != 0);
				sprintf(interpBuf,"%d; ", Record.output[iter]);
				UART2_SendNBytes(interpBuf, strlen(interpBuf));
			}
			i = 10000;
			while(--i != 0);
			sprintf(interpBuf,"\r\n");
			UART2_SendNBytes(interpBuf, strlen(interpBuf));
		}
	_ENDGROUP
	else
	_GROUP(iBufPt, ":LED", 4)
		_IF_MEMBER_THEN(iBufPt, " ON", 3, 4)
			Params.ledOverride = 1;
		else
		_IF_MEMBER_THEN(iBufPt, " OFF", 4, 4)
			Params.ledOverride = 0;
		else
		_IF_MEMBER_THEN(iBufPt, ":SET", 4, 4){
			if(Params.ledOverride)
				LED_Set( 	((iBufPt[9+0]=='0') ? 0 : (1<<7) )| 
							((iBufPt[9+1]=='0') ? 0 : (1<<6) )| 
							((iBufPt[9+2]=='0') ? 0 : (1<<5) )| 
							((iBufPt[9+3]=='0') ? 0 : (1<<4) )| 
							((iBufPt[9+4]=='0') ? 0 : (1<<3) )| 
							((iBufPt[9+5]=='0') ? 0 : (1<<2) )| 
							((iBufPt[9+6]=='0') ? 0 : (1<<1) )| 
							((iBufPt[9+7]=='0') ? 0 : (1<<0) ));
		}
		else
		_IF_MEMBER_THEN(iBufPt, ":TAB", 4, 4){
			if(Params.ledOverride)
				LED_Set(iBufPt[9 + (u8)KEY_ADDRESS]);
		}					
	_ENDGROUP
}



			
