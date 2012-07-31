//Interpreter:
#define _GROUP(buf, grName, grNameLen)									if(strncmp((char*)buf, grName, grNameLen)==0){
#define _ENDGROUP														}
#define _GET_SET_MEMBER(buf, varName, memName, memNameLen, grNameLen)	if(strncmp((char*)buf+grNameLen, memName, memNameLen)==0){if(buf[(grNameLen+memNameLen)]=='?'){/*ltoa(varName, buf, 10);*/while(--i != 0);UART2_SendNBytes(buf, strlen(buf));}else{varName=atol((char*)buf+(grNameLen+memNameLen+1));}}
#define _IF_MEMBER_THEN(buf, memName, memNameLen, grNameLen)			if(strncmp((char*)buf+grNameLen, memName, memNameLen)==0)

