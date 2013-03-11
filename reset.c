#include "reset.h"
#include "nf/nfv2.h"

extern NF_STRUCT_ComBuf NFComBuf; 

int RESET_Status_To_Str(char* buf)
{
	char* bufPt;
	bufPt = buf;
	if(RCONbits.TRAPR != 0)
	{
		RCONbits.TRAPR = 0;
		sprintf(bufPt, "[TRAPR]");
		bufPt += 7;
	}	
	if(RCONbits.IOPUWR != 0)
	{
		RCONbits.IOPUWR = 0;
		sprintf(bufPt, "[IOPUWR]");
		bufPt += 8;
	}	
	if(RCONbits.CM != 0)
	{
		RCONbits.CM = 0;
		sprintf(bufPt, "[CM]");
		bufPt += 4;
	}	
	if(RCONbits.EXTR != 0)
	{
		RCONbits.EXTR = 0;
		sprintf(bufPt, "[EXTR]");
		bufPt += 6;
	}	
	if(RCONbits.SWR != 0)
	{
		RCONbits.SWR = 0;
		sprintf(bufPt, "[SWR]");
		bufPt += 5;
	}
	if(RCONbits.WDTO != 0)
	{
		RCONbits.WDTO = 0;
		sprintf(bufPt, "[WDTO]");
		bufPt += 6;
	}
	if(RCONbits.SLEEP != 0)
	{
		RCONbits.SLEEP = 0;
		sprintf(bufPt, "[SLEEP]");
		bufPt += 7;
	}	
	if(RCONbits.IDLE != 0)
	{
		RCONbits.IDLE = 0;
		sprintf(bufPt, "[IDLE]");
		bufPt += 6;
	}	
	if(RCONbits.BOR != 0)
	{
		RCONbits.BOR = 0;
		sprintf(bufPt, "[BOR]");
		bufPt += 5;
	}	
	if(RCONbits.POR != 0)
	{
		RCONbits.POR = 0;
		sprintf(bufPt, "[POR]");
		bufPt += 5;
	}
		
	if(bufPt == buf)		// No RESET flag
	{
		sprintf(bufPt, "--");
		bufPt += 2;
	}	
	sprintf(bufPt, "\r\n");
	
	return 0;
}	

int UART1_Status_To_Str(char* buf)
{
	char* bufPt;
	bufPt = buf;
	if(U1STAbits.UTXISEL1 != 0)
	{
		sprintf(bufPt, "[UTXISEL1]");
		bufPt += 10;
	}	
	if(U1STAbits.UTXINV != 0)
	{
		sprintf(bufPt, "[UTXINV]");
		bufPt += 8;
	}	
	if(U1STAbits.UTXISEL0 != 0)
	{
		sprintf(bufPt, "[UTXISEL0]");
		bufPt += 10;
	}	
	if(U1STAbits.UTXBRK != 0)
	{
		sprintf(bufPt, "[UTXBRK]");
		bufPt += 8;
	}	
	if(U1STAbits.UTXEN != 0)
	{
		sprintf(bufPt, "[UTXEN]");
		bufPt += 7;
	}	
	if(U1STAbits.UTXBF != 0)
	{
		sprintf(bufPt, "[UTXBF]");
		bufPt += 7;
	}	
	if(U1STAbits.TRMT != 0)
	{
		sprintf(bufPt, "[TRMT]");
		bufPt += 6;
	}	
	if(U1STAbits.URXISEL1 != 0)
	{
		sprintf(bufPt, "[URXISEL1]");
		bufPt += 10;
	}	
	if(U1STAbits.URXISEL0 != 0)
	{
		sprintf(bufPt, "[URXISEL0]");
		bufPt += 10;
	}	
	if(U1STAbits.ADDEN != 0)
	{
		sprintf(bufPt, "[ADDEN]");
		bufPt += 7;
	}	
	if(U1STAbits.RIDLE != 0)
	{
		sprintf(bufPt, "[RIDLE]");
		bufPt += 7;
	}	
	if(U1STAbits.PERR != 0)
	{
		sprintf(bufPt, "[PERR]");
		bufPt += 6;
		U1STAbits.PERR = 0;
	}	
	if(U1STAbits.FERR != 0)
	{
		sprintf(bufPt, "[FERR]");
		bufPt += 6;
		U1STAbits.FERR = 0;
	}	
	if(U1STAbits.OERR != 0)
	{
		sprintf(bufPt, "[OERR]");
		bufPt += 6;
		U1STAbits.OERR = 0;
	}	
	if(U1STAbits.URXDA != 0)
	{
		sprintf(bufPt, "[URXDA]");
		bufPt += 7;
	}	
		
	if(bufPt == buf)		// No flag
	{
		sprintf(bufPt, "--");
		bufPt += 2;
	}	
	sprintf(bufPt, "\r\n");
	
	return 0;
}	

int NF_ComBuf_Status_To_Str(char* buf)
{
	char* bufPt;
	bufPt = buf;
	if(U1STAbits.UTXISEL1 != 0)
	{
		sprintf(bufPt, "[UTXISEL1]");
		bufPt += 10;
	}	
		
	if(bufPt == buf)		// No flag
	{
		sprintf(bufPt, "--");
		bufPt += 2;
	}	
	sprintf(bufPt, "\r\n");
	
	return 0;
}	
