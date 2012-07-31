#include "led.h"
#include "timers.h"


extern PARAMS_St Params;
extern REFERENCE_Un* Reference;
extern SYNCHRONIZER_St	DataSynchronizer;

void LED_Proc()
{
	static uint8_t blink = 0;
	uint8_t disp = 0;
	
	blink = 1-blink;
	
	if(Params.mode == M_ERROR)
	{
		if(blink)
			disp |= 0b11110000;
	}
	else if(Params.mode == M_MANUAL)
		switch(Reference->dir)
		{
			case D_STOP:
				disp |= 0b10100000;
				break;
			case D_FWD:
				if(blink)
					disp |= 0b10000000;
				break;
			case D_REV:
				if(blink)
					disp |= 0b00100000;
				break;
		}	
	else if(Params.mode == M_PWM)
	{
		if(Reference->pwm == 0){
			disp |= 0b01010000;
		}	
		else if(Reference->pwm > 0){
			if(blink)
				disp |= 0b01000000;
		}		
		else{
			if(blink)
				disp |= 0b00010000;
		}		
	}	
	else if(Params.mode == M_CURRENT)
	{
		if(Reference->current == 0){
			disp |= 0b01010000;
		}	
		else if(Reference->current > 0){
			if(blink)
				disp |= 0b01000000;
		}		
		else{
			if(blink)
				disp |= 0b00010000;
		}		
	}	
	else if(Params.mode == M_POSITION)
	{
		if(Params.dir == D_STOP){
			disp |= 0b01010000;
		}	
		else if(Params.dir == D_FWD){
			if(blink)
				disp |= 0b01000000;
		}		
		else{
			if(blink)
				disp |= 0b00010000;
		}		
	}	
	else if(Params.mode == M_BOOT)
	{
		disp |= 0b11111111;
	}
	
	if(blink)
		disp |= 0b00000001;
	
	if(DataSynchronizer.synchronized)
		disp |= 0b00000010;
	
	LED_Set(disp);
}

void LED_Config()
{
	LED_TRIS_CLK = 0;
	LED_TRIS_SDI = 0;
	LED_TRIS_LE = 0;
	LED_CLK = 0;
	LED_SDI = 0;
	LED_LE = 0;
	return;
}

void LED_Set(u8 newSet)
{
	static unsigned char oldSet;
	unsigned char i;
	
	if(newSet == oldSet)
		return;
		
	oldSet = newSet;
	
	for (i=0; i<8; i++)
	{
		LED_SDI = (newSet & 0x01);
		LED_CLK = 1;
		newSet = newSet >> 1;
		LED_CLK = 0;
	}
	LED_LE = 1;
	Nop();
	Nop();
	LED_LE = 0;
}
