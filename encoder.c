#include "encoder.h"



void ENC_ParameterSetLoad(int paramSetNum)
{
	Enc.swapAB = ENC_AB_SWAP[paramSetNum];
	Enc.IPolarity = ENC_I_POLARITY[paramSetNum];
	Enc.sw3Polarity = SW3_POLARITY[paramSetNum];
	Enc.sw1sw2Swap = SW1_SW2_SWAP[paramSetNum];
}

void ENC_Config()
{
	AD1PCFGL |= 0b111110010;	// AN1,4,5,6,7,8 (RA1, RB2, RB3, RP16,17,18) pins as digital input
	_QEA1R = 16;		// map QEI1 A input to RP16
	_QEB1R = 17;		// map QEI1 B input to RP17
	_INDX1R = 4;		// map QEI1 Z input to RP18	########################

//	QEI1CONbits.QEIM   = 0b110;	// Quadrature Encoder Interface enabled (x4 mode)
								// with position counter reset by Index Pulse
	QEI1CONbits.QEIM   = 0b111;	// Quadrature Encoder Interface enabled (x4 mode)
								// with position counter reset by match
								
//	QEI1CONbits.POSRES = 1;	// Index Pulse resets Position Counter bit
	QEI1CONbits.POSRES = 0;	// Index Pulse resets Position Counter bit
	
	QEI1CONbits.PCDOUT = 0;	// Position Counter Direction Status Output Enable bit
//	QEI1CONbits.SWPAB  = 0;	// Phase A and B Input Swap Select bit
	QEI1CONbits.SWPAB  = Enc.swapAB;	// Phase A and B Input Swap Select bit
 
	
	QEI1CONbits.QEISIDL = 0;	// Continue Operation in Idle Mode
 
	//DFLT1CONbits.QECK = 0;	// QEA/B/Index Digital Filter Clock Divide 1:1
	//DFLT1CONbits.QEOUT = 0;	// QEA/B/Index Pin Digital Filter Output Enable bit
	
	DFLT1CONbits.QECK = 0b001;	// QEA/B/Index Digital Filter Clock Divide 1:4
	//DFLT1CONbits.QECK = 0b011;	// QEA/B/Index Digital Filter Clock Divide 1:16
	DFLT1CONbits.QEOUT = 1;	// QEA/B/Index Pin Digital Filter Output Enable bit
	
	DFLT1CONbits.CEID = 1;	// Count Error Interrupt Disable bit
 
	MAX1CNT = ENC_RES;

				
	_QEI1IP = 7;			// QEI1 Interrupt Priority High
	IFS3bits.QEI1IF = 0;	// QEI1 Interrupt Flag
	IEC3bits.QEI1IE = 0;	// QEI1 Interrupt Enable
	
	
	// ENC_I Change Notify Interrupt Enable
	_TRISC2 = 1;			// #### 			RC2 as Input: ENC_I
	_CN10IE = 1;			// enable interrupt
	//_CN10PUE = 1;			// enable pull-up resistor
	
	IFS1bits.CNIF = 0;	// clear Input Change Notification Interrupt Flag 
	IPC4bits.CNIP = 6;	// set Input Change Notification Interrupt Priority High
	IEC1bits.CNIE = 1;	// enable Input Change Notification Interrupt
}

void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void)
{
	if((Params.encZeroTrace != 0) && (SW_SYNC != 0) )
	{
		if(Enc.isSynchronized == 0)
		{
			//Enc.positionOffset = 0;
			//Enc.positionOffset = ENC_Position();
			Enc.oldPos1Cnt = 0;
			Enc.ovfCnt = 0;
			POS1CNT = 0;
			Enc.synchroZero = 1;
			Enc.isSynchronized = 1;
		}	
	}	
	else
		Enc.synchroZero = 0;
		
	IFS1bits.CNIF = 0;		// clear IF
}

void __attribute__ ((interrupt, no_auto_psv)) _QEI1Interrupt(void)  
{
//	if (QEI1CONbits.UPDN == 1)
//		Enc.ovfCnt++;
//	else
//		Enc.ovfCnt--;
//		
	IFS3bits.QEI1IF = 0;
}

s32 ENC_Position()
{
	u16 newPos1Cnt = POS1CNT;
	
	if( (Enc.oldPos1Cnt & (0b11<<14)) == (0b11<<14) && (newPos1Cnt & (0b11<<14)) == 0 )
		Enc.ovfCnt++;
	else if( (Enc.oldPos1Cnt & (0b11<<14)) == 0 && (newPos1Cnt & (0b11<<14)) == (0b11<<14) )
		Enc.ovfCnt--;
	
	Enc.oldPos1Cnt = newPos1Cnt;
	
	return ((Enc.ovfCnt*ENC_RES) + newPos1Cnt);// - Enc.positionOffset);
}	
