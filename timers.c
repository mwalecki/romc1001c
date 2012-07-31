#include "timers.h"
#include "control.h"
#include "uart1.h"

extern SYNCHRONIZER_St	DataSynchronizer;
extern STDOWNCNT_St STDownCnt[ST_Downcounters];
extern REFERENCE_Un* Reference;

void TIMER1_Config()
{
	T1CON = 0;              // Timer reset
 	IFS0bits.T1IF = 0;      // Reset Timer1 interrupt flag
	IPC0bits.T1IP = 1;      // Timer1 Interrupt priority level LOW
 	IEC0bits.T1IE = 1;      // Enable Timer1 interrupt
 	TMR1 = 0;  	
	PR1 = 40000;           // Timer1 period register for T=1ms
	T1CONbits.TON = 1;      // Enable Timer1 and start the counter
}

void TIMER2_Config()
{
	T2CON = 0;              // Timer reset
 	IFS0bits.T2IF = 0;      // Reset Timer interrupt flag
	IPC1bits.T2IP = 6;      // Timer Interrupt priority - same as ADC IP
							// to avoid position and current regulators interference
 	IEC0bits.T2IE = 1;      // Enable Timer interrupt
 	TMR2 = 1010;  	
	PR2 = 40000;           // Timer1 period register for T=1ms
	T2CONbits.TON = 1;      // Enable Timer1 and start the counter
}

// Timer3
// T3 Overflow Interrupt triggers write of up-to-date parameters to Status buffer.
// T3 period is set based on mean Status inquiry interval.
// T3 phase is shifted to provide Status bufer write precedence of Status inquiry at 500us.
void TIMER3_Config()
{
	T3CON = 0;              // Timer reset
	T3CONbits.TCKPS = 0b01;		// Clock Prescaler
							//  1:256	0b11
							//  1:64	0b10
							//  1:8		0b01
							//  1:1		0b00
 	IFS0bits.T3IF = 0;      // Timer interrupt flag Reset 
	IPC2bits.T3IP = 6;      // Timer Interrupt priority level
 	IEC0bits.T3IE = 1;      // Timer interrupt Enable
 	TMR3 = 2020;  			// Initial Value
	PR3 = 2000 * US_TO_PR34;			// Timer period register for T=2ms
	T3CONbits.TON = 1;      // Enable Timer and start the counter
}

// Timer4
// T4 is used to measure interval between Status inquiries.
// No interrupt generated.
void TIMER4_Config()
{
	T4CON = 0;              // Timer reset
	T4CONbits.TCKPS = 0b01;		// Clock Prescaler
							//  1:256	0b11
							//  1:64	0b10
							//  1:8		0b01
							//  1:1		0b00
 	IFS1bits.T4IF = 0;      // Timer interrupt flag Reset 
	IPC6bits.T4IP = 1;      // Timer Interrupt priority level=1, Low Priority
 	IEC1bits.T4IE = 0;      // Timer interrupt Enable
 	TMR4 = 3030;  			// Initial Value
	PR4 = 10000 * US_TO_PR34;			// Timer period register for T=10ms
	T4CONbits.TON = 1;      // Enable Timer and start the counter
}

inline void TIMER4_MeasureStatusReadInterval()
{
	volatile static u16 lastRead, currentRead, measure, t3ValAtStatusRead;
	volatile static u8 ovfFlag, t3ValCorrectCnt;
	
	currentRead = TMR4;
	ovfFlag = IFS1bits.T4IF;
	t3ValAtStatusRead = TMR3;	
	DataSynchronizer.lastPrecedenceUs = t3ValAtStatusRead / US_TO_PR34;
	
 	if(ovfFlag == 1)		// Timer4 overflow occured at least once => ignore measure.
 	{
	 	IFS1bits.T4IF = 0;	// Timer interrupt flag Reset 
	}	
	else
	{
		measure = currentRead - lastRead;
		if (measure > 1500 * US_TO_PR34 && measure < 2500 * US_TO_PR34)
		{
			if (measure < PR3)
				PR3 --;
			else if (measure > PR3)
				PR3 ++;
		}
	}	

	
	if(DataSynchronizer.lastPrecedenceUs < (STATUS_WRITE_TO_READ_PRECED_US - 5))
		TMR3 += 5*US_TO_PR34;
	else if(DataSynchronizer.lastPrecedenceUs > (STATUS_WRITE_TO_READ_PRECED_US + 5))
		TMR3 -= 5*US_TO_PR34;
	
	if(DataSynchronizer.lastPrecedenceUs > (STATUS_WRITE_TO_READ_PRECED_US - 50)
	&& DataSynchronizer.lastPrecedenceUs < (STATUS_WRITE_TO_READ_PRECED_US + 50))
	{
		if(DataSynchronizer.correctPrecedCnt < MAX_CORR_PREC_CNT)
			DataSynchronizer.correctPrecedCnt ++;
	}	
	else
	{
		if(DataSynchronizer.correctPrecedCnt > 0)
			DataSynchronizer.correctPrecedCnt --;
	}
	
	if(DataSynchronizer.correctPrecedCnt > PREC_CNT_HIST_H)
		DataSynchronizer.synchronized = 1;
	else if(DataSynchronizer.correctPrecedCnt < PREC_CNT_HIST_L)
		DataSynchronizer.synchronized = 0;
	
	
	
	
	lastRead = currentRead;
}	

void __attribute__((interrupt,no_auto_psv)) _T1Interrupt( void )
{
	vu8 i=0;
	for(; i<ST_Downcounters; i++)
	{
		if(STDownCnt[i].val != 0)
			STDownCnt[i].val--;
		else
		{
			STDownCnt[i].val = STDownCnt[i].period;
			STDownCnt[i].tick = 1;
		}
	}
	
 	TMR1=  0;  
	IFS0bits.T1IF = 0;	// reset Timer 1 interrupt flag
}

void __attribute__((interrupt,no_auto_psv)) _T2Interrupt( void )
{
	MOTOR_Control();
	IFS0bits.T2IF = 0;	// reset Timer 2 interrupt flag
}

void SYSTICK_Config(void)
{
	STDownCnt[0].period = ST_Period0;
	STDownCnt[1].period = ST_Period1;
	STDownCnt[2].period = ST_Period2;
}


// Timer3
// T3 Overflow Interrupt triggers write of up-to-date parameters to Status buffer.
void __attribute__((interrupt,no_auto_psv)) _T3Interrupt( void )
{
//	StatusUpdate();
	UART1_StandardStatusPrepare();
	
	if(DataSynchronizer.statusReady == 1) {
		if(DataSynchronizer.correctPrecedCnt > 0)
			DataSynchronizer.correctPrecedCnt --;
		else
			DataSynchronizer.synchronized = 0;
	}		
	
	DataSynchronizer.statusReady = 1;
	IFS0bits.T3IF = 0;	// reset Timer interrupt flag
}

