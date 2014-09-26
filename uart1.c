#include "uart1.h"
#include "uart2.h"
#include "led.h"
#include "encoder.h"
#include "control.h"
#include "keyb.h"
#include "nf/nfv2.h"

extern SYNCHRONIZER_St	DataSynchronizer;
extern PARAMS_St Params;
extern UART_St	Uart1, Uart2;
extern REFERENCE_Un ReferenceA;
extern REFERENCE_Un ReferenceB;
extern REFERENCE_Un* Reference;
extern RECORD_St Record;
extern ENC_St Enc;

extern STATUS_St StatusA __attribute__((space(dma)));
extern STATUS_St StatusB __attribute__((space(dma)));
extern STATUS_St* Status;	// Pointer to current Status-write-to register

extern vs8 uart1TxBuffer[UART1_TxBufSz] __attribute__((space(dma)));
uint8_t uart1TxPt;
extern uint8_t *standardStatusTxBuffer 		__attribute__((space(dma)));
extern uint8_t standardStatusTxBuffer1[] 	__attribute__((space(dma)));
extern uint8_t standardStatusTxBuffer2[] 	__attribute__((space(dma)));
extern uint8_t *standardStatusTxBytes;
extern uint8_t standardStatusTxBytes1;
extern uint8_t standardStatusTxBytes2;

const uint8_t StandardStatusCommArray[]	= {NF_COMMAND_ReadDrivesPosition, NF_COMMAND_ReadDrivesCurrent, NF_COMMAND_ReadDrivesStatus};
const uint8_t StandardStatusCommCnt		= 3;

extern fractional CurrentKCoeffs[];

vs8 uart1RxBuffer[UART1_RxBufSz];
vu16 statusReadyWait;

extern NF_STRUCT_ComBuf NFComBuf; 

void UART1_StandardStatusPrepare(void)
{
	// Update Communication Buffer
	NFComBuf.ReadDrivesPosition.data[0] = ENC_Position();
	NFComBuf.ReadDrivesCurrent.data[0] = Status->current;
	NFComBuf.ReadDrivesStatus.data[0] =
		(SW_EDGE1				? NF_DrivesStatus_LimitSwitchUp		: 0 ) |
		(SW_EDGE2				? NF_DrivesStatus_LimitSwitchDown	: 0 ) |
		(!SW_SYNC				? NF_DrivesStatus_SynchroSwitch		: 0 ) |
		(Enc.synchroZero		? NF_DrivesStatus_EncoderIndexSignal: 0 ) |
		(Enc.isSynchronized		? NF_DrivesStatus_Synchronized		: 0 ) |
		(PS_FAULT				? NF_DrivesStatus_PowerStageFault	: 0 ) |
		(Params.mode == M_ERROR	? NF_DrivesStatus_Error				: 0 ) |
		(Status->overcurrent	? NF_DrivesStatus_Overcurrent		: 0 );
	
	// Make Command Frame
	*standardStatusTxBytes = NF_MakeCommandFrame(&NFComBuf, standardStatusTxBuffer, StandardStatusCommArray, StandardStatusCommCnt, NFComBuf.myAddress);
	
}	

void UART1_StandardStatusSend(void)
{
	// Prepare for DMA Transfer
	if(standardStatusTxBuffer == standardStatusTxBuffer1)
	{
		standardStatusTxBuffer = standardStatusTxBuffer2;
		standardStatusTxBytes = &standardStatusTxBytes2;
		DMA3CNT = standardStatusTxBytes1;						// Number of DMA requests
		DMA3STA = __builtin_dmaoffset(standardStatusTxBuffer1);	// Associated Tx Buffer
	}	
	else
	{
		standardStatusTxBuffer = standardStatusTxBuffer1;
		standardStatusTxBytes = &standardStatusTxBytes1;
		DMA3CNT = standardStatusTxBytes2;						// Number of DMA requests
		DMA3STA = __builtin_dmaoffset(standardStatusTxBuffer2);	// Associated Tx Buffer
	}
	DMA3CONbits.CHEN  = 1;	// Re-enable DMA3 Channel
	DMA3REQbits.FORCE = 1;	// Manual mode: Kick-start the first transfer
}	

//##                                      #### ######## ################ UART1 Config
void UART1_Config(void)
{
	_U1RXR = 21;	// map UART1 Receiver to RP21
	_RP20R = 3;		// map UART1 Transmitter to RP20

	Uart1.txBuf = uart1TxBuffer;
	Uart1.rxBuf = uart1RxBuffer;
	
	U1MODEbits.STSEL = 1;			// 2-stop bits
	U1MODEbits.PDSEL = 0;			// No Parity, 8-data bits
	U1MODEbits.BRGH = 1;			// High Speed Mode
	U1MODEbits.ABAUD = 0;			// Autobaud Disabled
	U1BRG = U1BRGVAL;					// BAUD Rate Setting
	U1STAbits.UTXISEL0 = 0b10;			// Interrupt after all Tx operations completed
	//U1STAbits.UTXISEL1 = 0;			                            
	U1STAbits.URXISEL  = 0;			// Interrupt after one RX character is received
	U1MODEbits.UARTEN   = 1;		// Enable UART
	U1STAbits.UTXEN     = 1;		// Enable UART Tx
	
//	IFS0bits.U1TXIF = 0;
//	IEC0bits.U1TXIE = 1;				// UART1 Transmitter Interrupt Enable
	IPC3bits.U1TXIP = 3;				// mid-range interrupt priority
	IFS0bits.U1RXIF = 0;
	IPC2bits.U1RXIP = 7;				// high interrupt priority
	IEC0bits.U1RXIE = 1;				// UART1 Receiver Interrupt Enable
	IPC16bits.U1EIP = 5;				// UART1 Error Interrupt Enable
	IEC4bits.U1EIE = 1;				// UART1 Error Interrupt Enable
	
	U1STAbits.OERR=0;
	
	NF_ComBufReset(&NFComBuf);
	NFComBuf.myAddress = (u8)KEY_ADDRESS;
	
	standardStatusTxBuffer = standardStatusTxBuffer1;
	standardStatusTxBytes = &standardStatusTxBytes1;
}

//##                                      #### ######## ################ DMA3 Config
void DMA3_UART1TX_STATUS_Config(void)
{
	DMA3REQ = 0x0C;					// Select DMA Request Source: UART1 Transmitter
	DMA3PAD = (volatile unsigned int) &U1TXREG;
	
	DMA3CONbits.AMODE = 0;		// Addressing: Register Indirect with Post-Increment
	DMA3CONbits.MODE  = 1;		// One-Shot Mode
	DMA3CONbits.HALF  = 0;		// 
	DMA3CONbits.DIR   = 1;		// RAM To Peripheral
	DMA3CONbits.SIZE  = 1;		// Byte-Size
//	DMA3CNT = sizeof(StatusA)-1;				// Number of DMA requests
//	DMA3STA = __builtin_dmaoffset(&StatusA);	// Associated Buffer A
//	DMA3STB = __builtin_dmaoffset(&StatusB);	// Associated Buffer B
//	IFS0bits.DMA1IF  = 0;			// Clear DMA Interrupt Flag
//	IEC0bits.DMA1IE  = 1;			// Enable DMA interrupt
}


inline void UART1_SendStatus(void)
{
	if(Status == &StatusA)
	{
		Status = &StatusB;
		DMA3STA = __builtin_dmaoffset(&StatusA);
	}	
	else
	{
		Status = &StatusA;
		DMA3STA = __builtin_dmaoffset(&StatusB);
	}	

//	Status = &StatusA;
//	DMA3STA = __builtin_dmaoffset(&StatusA);
	
	DMA3CONbits.CHEN  = 1;	// Re-enable DMA3 Channel
	DMA3REQbits.FORCE = 1;	// Manual mode: Kick-start the first transfer
}

void DMA1_UART1TX_Config(void)
{
	DMA1REQ = 0x0C;					// Select UART1 Transmitter
	DMA1PAD = (volatile unsigned int) &U1TXREG;
	
	DMA1CONbits.AMODE = 0;	// Addressing: Register Indirect with Post-Increment
	DMA1CONbits.MODE  = 1;	// One-Shot Mode
	DMA1CONbits.DIR   = 1;	// RAM To Peripheral
	DMA1CONbits.SIZE  = 1;	// Byte-Size
	//DMA1CNT = 7;				// 8 DMA requests
	DMA1STA = __builtin_dmaoffset(uart1TxBuffer);	// Associated Buffer
//	IFS0bits.DMA1IF  = 0;			// Clear DMA Interrupt Flag
//	IEC0bits.DMA1IE  = 1;			// Enable DMA interrupt
}

//##                                      #### ######## ################ Interrupt Handlers

//void __attribute__((interrupt, no_auto_psv)) _DMA1Interrupt(void)
//{
//	IFS0bits.U1TXIF = 0;				// Clear UART1 Transmitter Interrupt Flag
//	IEC0bits.U1TXIE = 1;				// UART1 Transmitter Interrupt Enable
//	IFS0bits.DMA1IF = 0;				// Clear the DMA1 Interrupt Flag;
//}

void __attribute__ ((interrupt, no_auto_psv)) _U1RXInterrupt(void) 
{
	uint8_t commArray[10];
	uint8_t commCnt;
	uint8_t bytesToSend;
	
	// While hardware receive buffer not empty...
	while(U1STAbits.URXDA != 0)
	{
		// Add received char to a software receive buffer
		Uart1.rxBuf[Uart1.rxPt] = U1RXREG;
		
		if(Uart1.rxPt == 2)
			TIMER4_MeasureStatusReadInterval();
		// Try to do interpretation
		// And if correct frame with more than 0 data bytes received...
		if(NF_Interpreter(&NFComBuf, Uart1.rxBuf, &Uart1.rxPt, commArray, &commCnt) > 0){
			// If SetDrivesMode command received...
			if(NFComBuf.SetDrivesMode.updated != 0){
				switch(NFComBuf.SetDrivesMode.data[0]){
					case NF_DrivesMode_ERROR:
						ModeSwitch(M_ERROR);	
						break;
					case NF_DrivesMode_MANUAL:
						ModeSwitch(M_MANUAL);	
						break;
					case NF_DrivesMode_PWM:
						// If there is no error...
						if((Params.mode != M_ERROR) && !SW_EDGE1 && !SW_EDGE2){
							ModeSwitch(M_PWM);
							Params.encZeroTrace = 0;
						}	
						break;
					case NF_DrivesMode_CURRENT:
						// If there is no error...
						if((Params.mode != M_ERROR) && !SW_EDGE1 && !SW_EDGE2){
							ModeSwitch(M_CURRENT);
							Params.encZeroTrace = 0;
						}	
						break;
					case NF_DrivesMode_SYNC_PWM0:
						// If there is no error...
						if((Params.mode != M_ERROR) && !SW_EDGE1 && !SW_EDGE2){
							ModeSwitch(M_PWM);
							if(Params.encZeroTrace == 0 && Enc.isSynchronized == 1)
								Enc.isSynchronized = 0;		// Allow resynchronization
							Params.encZeroTrace = 1;
						}
						break;
					case NF_DrivesMode_SYNC_CURRENT0:
						// If there is no error...
						if((Params.mode != M_ERROR) && !SW_EDGE1 && !SW_EDGE2){
							ModeSwitch(M_CURRENT);
							if(Params.encZeroTrace == 0 && Enc.isSynchronized == 1)
								Enc.isSynchronized = 0;		// Allow resynchronization
							Params.encZeroTrace = 1;
						}
						break;
					default:
						break;
				}
				NFComBuf.SetDrivesMode.updated = 0;
			}	
			
			// If SetDrivesPWM command received...
			if(NFComBuf.SetDrivesPWM.updated != 0){
				// If there is no error...
				if((Params.mode == M_PWM) && !SW_EDGE1 && !SW_EDGE2){
					if(Reference == &ReferenceA)
					{
						ReferenceB.pwm = NFComBuf.SetDrivesPWM.data[0];
						Reference = &ReferenceB;
					}	
					else
					{
						ReferenceA.pwm = NFComBuf.SetDrivesPWM.data[0];
						Reference = &ReferenceA;
					}
				}
				NFComBuf.SetDrivesPWM.updated = 0;
			}
			// If SetDrivesCurrent command received...
			if(NFComBuf.SetDrivesCurrent.updated != 0){
				// If there is no error...
				if((Params.mode == M_CURRENT) && !SW_EDGE1 && !SW_EDGE2){
					if(Reference == &ReferenceA)
					{
						ReferenceB.current = NFComBuf.SetDrivesCurrent.data[0];
						Reference = &ReferenceB;
					}	
					else
					{
						ReferenceA.current = NFComBuf.SetDrivesCurrent.data[0];
						Reference = &ReferenceA;
					}
				}
				NFComBuf.SetDrivesCurrent.updated = 0;
			}	
			
			// If SetDrivesMaxCurrent command received...
			if(NFComBuf.SetDrivesMaxCurrent.updated != 0){
				Params.maxCurrent = NFComBuf.SetDrivesMaxCurrent.data[0];
				NFComBuf.SetDrivesMaxCurrent.updated = 0;
			}	
			
			// If SetDrivesMisc command received...
			if(NFComBuf.SetDrivesMisc.updated != 0){
				// Set / Reset Synchronized
				if(NFComBuf.SetDrivesMisc.data[0] & NF_DrivesMisc_ResetSynchronized)
					Enc.isSynchronized = 0;
				else if(NFComBuf.SetDrivesMisc.data[0] & NF_DrivesMisc_SetSynchronized)
					Enc.isSynchronized = 1;
				NFComBuf.SetDrivesMisc.updated = 0;
			}	
			
			// If SetCurrentRegulator command received...
			if(NFComBuf.SetCurrentRegulator.updated != 0){
				CurrentKCoeffs[0] = NFComBuf.SetCurrentRegulator.data[0].p;
				CurrentKCoeffs[1] = NFComBuf.SetCurrentRegulator.data[0].i;
				CurrentKCoeffs[2] = NFComBuf.SetCurrentRegulator.data[0].d;
				PID_CoeffsUpdate(1);
				NFComBuf.SetCurrentRegulator.updated = 0;
			}
			
			// If Interpreter found a query to be answered...
			if(commCnt > 0){
				uint8_t stndardStatusRequest = 1;
				int i = 0;
			/*	for(; i < commCnt; i++){
					if((commArray[i] != NF_COMMAND_ReadDrivesPosition) && (commArray[i] != NF_COMMAND_ReadDrivesCurrent) && (commArray[i] != NF_COMMAND_ReadDrivesStatus)){
						stndardStatusRequest = 0;
						break;
					}	
				}*/
				stndardStatusRequest = 0;
			//	if(stndardStatusRequest == 1){	// 16.08.2012
			//	//	while(1){
			//	//		if(DataSynchronizer.statusReady == 1 || DataSynchronizer.synchronized == 0){
			//				UART1_StandardStatusSend();
			//				DataSynchronizer.statusReady = 0;
			//	//			break;
			//	//		}	
			//	//	}
			//	}
				if(stndardStatusRequest == 1 && DataSynchronizer.statusReady == 1){	// 17.08.2012
					UART1_StandardStatusSend();
					DataSynchronizer.statusReady = 0;
				}
				else{
					// Update status structure
					StatusUpdate();
					// Make a response frame (response == with my address)
					bytesToSend = NF_MakeCommandFrame(&NFComBuf, (uint8_t*)(Uart1.txBuf), (const uint8_t*)commArray, commCnt, NFComBuf.myAddress);
					//UART1_SendNBytes((char*)(Uart1.txBuf), bytesToSend);
					UART1_TransferTxBuffer(bytesToSend);
				}
				//sprintf(Uart2.txBuf, "%04d %04d %d\r\n", PR3, TMR3, DataSynchronizer.synchronized);
				//UART2_SendNBytes(&DataSynchronizer, 1);
			}
		}
	}		
	// Clear Interrupt Flag
	IFS0bits.U1RXIF = 0;
}

//void __attribute__ ((interrupt, no_auto_psv)) _U1TXInterrupt(void) 
//{
//	//LED_Set(0xff);
//	//UART1_TX_EN = 0;	// Transmit Buffer empty, set RS-485 Transceiver for Receive
//	IEC0bits.U1RXIE = 1;				// UART1 Receiver Interrupt Enable
//	IFS0bits.U1TXIF = 0;				// Clear UART1 Transmitter Interrupt Flag
//}

void __attribute__ ((interrupt, no_auto_psv)) _U1ErrInterrupt(void)
{
	if(U1STAbits.OERR)
	{
		Uart1.rxPt = 0;
		U1STAbits.OERR=0;
	}
	IFS4bits.U1EIF = 0; // Clear the UART1 Error Interrupt Flag
}

void UART1_SendNBytes(char *buf, u8 n)
{
	unsigned int i;
	for(i=0;i<n;i++)
		uart1TxBuffer[i]=buf[i];
	DMA1CNT = n-1;				// n DMA requests
	DMA1CONbits.CHEN  = 1;	// Re-enable DMA1 Channel
				
	DMA1STA = __builtin_dmaoffset(uart1TxBuffer);	// Associated Buffer
	DMA1REQbits.FORCE = 1;	// Manual mode: Kick-start the first transfer
}

inline void UART1_TransferTxBuffer(u8 n)
{
	DMA1CNT = n-1;				// n DMA requests
	DMA1CONbits.CHEN  = 1;	// Re-enable DMA1 Channel
				
	DMA1STA = __builtin_dmaoffset(uart1TxBuffer);	// Associated Buffer
	DMA1REQbits.FORCE = 1;	// Manual mode: Kick-start the first transfer
}

void UART1_Proc()
{
	static char buf[UART1_INTERP_BUF_SZ];
	if(Uart1.rxReady != 1)
		return;
	strncpy(buf, (const char*)Uart1.rxBuf, UART1_INTERP_BUF_SZ);
	Uart1.rxReady = 0;
	_GROUP(buf, "*IDN?", 5)
		sprintf(buf,"MW1001 IRP/Sarkogripper\r\n");
		UART1_SendNBytes(buf, strlen(buf));
	_ENDGROUP
	else
	_GROUP(buf, ":MODE", 5)
		_IF_MEMBER_THEN(buf, ":MAN", 4, 5)
			ModeSwitch(M_MANUAL);	
		else
		_IF_MEMBER_THEN(buf, ":PWM", 4, 5)
			ModeSwitch(M_PWM);
		else
		_IF_MEMBER_THEN(buf, ":CUR", 4, 5)
			ModeSwitch(M_CURRENT);
		else
		_IF_MEMBER_THEN(buf, ":POS", 4, 5)
			ModeSwitch(M_POSITION);
	_ENDGROUP
	else
	_GROUP(buf, ":PWM", 4)
		_GET_SET_MEMBER(buf, Reference->pwm, ":SET", 4, 4)
		#ifdef RECORD
			Record.index = 0;
		#endif
	_ENDGROUP
	else
	_GROUP(buf, ":CUR", 4)
		_GET_SET_MEMBER(buf, Reference->current, ":SET", 4, 4)
		#ifdef RECORD
			Record.index = 0;
		#endif
	_ENDGROUP
	else
	_GROUP(buf, ":POS", 4)
		_GET_SET_MEMBER(buf, Reference->position, ":SET", 4, 4)
		#ifdef RECORD
			Record.index = 0;
		#endif
	_ENDGROUP
	else
	_GROUP(buf, ":ENC", 4)
		_IF_MEMBER_THEN(buf, ":POS?", 5, 4)
		{
			sprintf(buf,"%lu\r\n", ENC_Position());
			UART1_SendNBytes(buf, strlen(buf));
		}	
	_ENDGROUP
	else
	_GROUP(buf, ":XXX", 4)
		PID_CoeffsUpdate(1);
	_ENDGROUP
}



			
