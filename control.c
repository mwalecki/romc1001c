#include "control.h"
#include "nf/nfv2.h"

extern PARAMS_St Params;
extern REFERENCE_Un	ReferenceA;
extern REFERENCE_Un	ReferenceB;
extern REFERENCE_Un* Reference;
extern s16 referenceCurrent;
extern RECORD_St Record;
extern STATUS_St* Status;	// Pointer to current Status-write-to register
extern ENC_St Enc;

extern NF_STRUCT_ComBuf NFComBuf; 


tPID PositionPID;
fractional PositionAbcCoefficient[3] __attribute__ ((section (".xbss, bss, xmemory")));
fractional PositionControlHistory[3] __attribute__ ((section (".ybss, bss, ymemory")));
fractional PositionKCoeffs[] = {0,0,0};

tPID CurrentPID;
fractional CurrentAbcCoefficient[3] __attribute__ ((section (".xbss, bss, xmemory")));
fractional CurrentControlHistory[3] __attribute__ ((section (".ybss, bss, ymemory")));
fractional CurrentKCoeffs[] = {0,0,0};


void PositionPID_Config()
{
	PositionPID.abcCoefficients = &PositionAbcCoefficient[0];	/*Set up pointer to derived coefficients */
	PositionPID.controlHistory = &PositionControlHistory[0];	/*Set up pointer to controller history samples */
    PIDInit(&PositionPID);                        				/*Clear the controler history and the controller output */
	PositionKCoeffs[0] = Q15(PID_POS_P);	// Kp
	PositionKCoeffs[1] = Q15(PID_POS_I);	// Ki
	PositionKCoeffs[2] = Q15(PID_POS_D);	// Kd
    PIDCoeffCalc(&PositionKCoeffs[0], &PositionPID);
}

void CurrentPID_Config()
{
	CurrentPID.abcCoefficients = &CurrentAbcCoefficient[0];	/*Set up pointer to derived coefficients */
	CurrentPID.controlHistory = &CurrentControlHistory[0];	/*Set up pointer to controller history samples */
    PIDInit(&CurrentPID);                        				/*Clear the controler history and the controller output */
	CurrentKCoeffs[0] = Q15(PID_CURR_P);	// Kp
	CurrentKCoeffs[1] = Q15(PID_CURR_I);	// Ki
	CurrentKCoeffs[2] = Q15(PID_CURR_D);	// Kd
    PIDCoeffCalc(&CurrentKCoeffs[0], &CurrentPID);
}

inline void MOTOR_Control()	// Called in timer1 interrupt, timers.c
{
	static s32 deviation;
	
	switch(Params.mode)
	{
		case M_MANUAL:
			switch(Reference->dir)
			{
				case D_STOP:
					MOTOR_PwmSet(0);
					break;
				case D_FWD:
					MOTOR_PwmSet(MOTOR_MANUAL_DUTY);
					break;
				case D_REV:
					MOTOR_PwmSet(-MOTOR_MANUAL_DUTY);
					break;
			}
			break;
		case M_PWM:
			MOTOR_PwmSet(Reference->pwm);
			break;
		case M_POSITION:
			deviation =  ENC_Position() - Reference->position ;
				
			if(deviation > 1000)
				deviation = 1000;
			if(deviation < -1000)
				deviation = -1000;
			
			PositionPID.measuredOutput = (s16)deviation;
			PositionPID.controlReference = 0;
			
			PID(&PositionPID);
			if(PositionPID.controlOutput < -1000)
				PositionPID.controlOutput = -1000;
			else if(PositionPID.controlOutput > 1000)
				PositionPID.controlOutput = 1000;
			
			#ifdef PID_CASCADE
				referenceCurrent = MAX_CURRENT * (PositionPID.controlOutput);
			#else
				MOTOR_PwmSet(PositionPID.controlOutput);
			#endif
			
			if(deviation == 0)
				Params.dir = D_STOP;
			else if(deviation > 0)
				Params.dir = D_REV;
			else
				Params.dir = D_FWD;
		
			break;
		case M_CURRENT:
			referenceCurrent = Reference->current; 
			break;
		case M_TEST_CURRENT:
			#ifdef RECORD_CURRENT
				if(Record.index < (RECORD_SAMPLES / 2))
					referenceCurrent = Reference->current; 
				else
					referenceCurrent = 0; 
			#endif
			break;
		case M_ERROR:
			referenceCurrent = 0;
			MOTOR_PwmSet(0);
			break;
	}
	
	
	#ifdef RECORD_POSITION
	if(Record.index < RECORD_SAMPLES)
	{
		Record.measure[Record.index] = ENC_Position();
		#ifdef PID_CASCADE
			Record.output[Record.index] = MAX_CURRENT*PositionPID.controlOutput;
		#else
			Record.output[Record.index] = PositionPID.controlOutput;
		#endif
		Record.index++;
	}
	#endif
}	

inline void StatusUpdate()
{
	Status->position = ENC_Position();
	Status->sw1 = SW_EDGE1;
	Status->sw2 = SW_EDGE2;
	Status->swSynchr = !SW_SYNC;
	Status->synchroZero = Enc.synchroZero;
	Status->isSynchronized = Enc.isSynchronized;
	Status->powerStageFault = PS_FAULT;
	Status->error = (Params.mode == M_ERROR);
	
	NFComBuf.ReadDrivesPosition.data[0] = Status->position;
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
		
}	

inline void CurrentControl()	// Called in ADC interrupt, adc.c
{
	static s16 deviation;
	s16 s16temp;
	static s16 prevReferenceCurrent = 0;
	
	#define RECORD_TH 1000
	
	if((prevReferenceCurrent < RECORD_TH && referenceCurrent >= RECORD_TH) || (prevReferenceCurrent > -RECORD_TH && referenceCurrent <= -RECORD_TH))
		Record.index = 0;
	
	prevReferenceCurrent = referenceCurrent;
	
	if(referenceCurrent > 1000*CURRENT_LIMIT)
		s16temp = 1000*CURRENT_LIMIT;
	else if(referenceCurrent < -1000*CURRENT_LIMIT)
		s16temp = -1000*CURRENT_LIMIT;
	else
		s16temp = referenceCurrent;
	
	deviation = ADC_Current() - referenceCurrent;
	
	if(deviation > 1000)
		deviation = 1000;
	if(deviation < -1000)
		deviation = -1000;
	
	CurrentPID.measuredOutput = deviation;
	CurrentPID.controlReference = 0;
	
	PID(&CurrentPID);
	
	if(CurrentPID.controlOutput < -1000)
		CurrentPID.controlOutput = -1000;
	else if(CurrentPID.controlOutput > 1000)
		CurrentPID.controlOutput = 1000;
		
	MOTOR_PwmSet(CurrentPID.controlOutput);
	
	#ifdef RECORD_CURRENT
	if(Record.index < RECORD_SAMPLES)
	{
		Record.reference[Record.index] = referenceCurrent;
		Record.measure[Record.index] = ADC_Current();
		Record.output[Record.index] = CurrentPID.controlOutput;
		Record.index++;
	}
	#endif
}

inline void ModeSwitch(u8 mode)
{
	if(Params.mode == mode)
		return;
		
	Params.mode = mode;
	switch(mode)
	{
		case M_MANUAL:
			ReferenceA.largest = 0;
			ReferenceB.largest = 0;
		break;
		case M_POSITION:
			ReferenceA.position = ENC_Position();
			ReferenceB.position = ENC_Position();
		break;
		case M_ERROR:
			ER_STOP=1;
			ST_Reset(ST_ErStop);
			ReferenceA.largest = 0;
			ReferenceB.largest = 0;
		//	Enc.isSynchronized = 0;
		break;
		case M_BOOT:
			ER_STOP=1;
			ST_Reset(ST_ErStop);
			ReferenceA.largest = 0;
			ReferenceB.largest = 0;
		//	Enc.isSynchronized = 0;
			LED_Set(0b11111111, 0);
		break;
		default:
			ReferenceA.largest = 0;
			ReferenceB.largest = 0;
			Status->overcurrent = 0;
		break;
	}	
}	

inline void PID_CoeffsUpdate(u8 force)
{
	static fractional PosK[3], CurrK[3];
	if(PosK[0] != PositionKCoeffs[0] || PosK[1] != PositionKCoeffs[1] || PosK[2] != PositionKCoeffs[2] || force == 1)
	{
		PosK[0] = PositionKCoeffs[0];
		PosK[1] = PositionKCoeffs[1];
		PosK[2] = PositionKCoeffs[2];
		PIDCoeffCalc(&PositionKCoeffs[0], &PositionPID);
	}
	if(CurrK[0] != CurrentKCoeffs[0] || CurrK[1] != CurrentKCoeffs[1] || CurrK[2] != CurrentKCoeffs[2] || force == 1)
	{
	//	CurrentKCoeffs[0] = Q15(PID_CURR_P);	// Kp
	//	CurrentKCoeffs[1] = Q15(PID_CURR_I);	// Ki
	//	CurrentKCoeffs[2] = Q15(PID_CURR_D);	// Kd
		
		CurrK[0] = CurrentKCoeffs[0];
		CurrK[1] = CurrentKCoeffs[1];
		CurrK[2] = CurrentKCoeffs[2];
		
		PIDCoeffCalc(&CurrentKCoeffs[0], &CurrentPID);
	}	
}
