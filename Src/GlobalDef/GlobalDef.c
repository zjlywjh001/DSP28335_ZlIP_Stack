/*
 * GlobalDef.c
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#include "GlobalDef.h"

#ifndef HOST_ORDER_AS_NET
DWORD ntohl(DWORD in) REENTRANT_SIG
{
	DWORD out;
	out = (in<<24) | ((in<<8)&(0x00FF0000)) | ((in>>8)&(0x0000FF00)) | (in>>24) ;
	return out;
}

WORD ntohs(WORD in) REENTRANT_SIG
{
	WORD out;
	out = (in<<8) | (in>>8);
	return out;
}
#endif

/* MemCopy offered by normal C lib */
void MyMemCopy(BYTE DT_XDATA *buf1,BYTE DT_XDATA *buf2,WORD size) REENTRANT_SIG
{
	BYTE DT_XDATA * EndBuf;
	for(EndBuf = buf1 + size; EndBuf != (BYTE DT_XDATA *)buf1;)
	{
		*buf1++ = *buf2++;
	}
}


Uint32 MergeNetDword(BYTE H,BYTE h,BYTE l,BYTE L)
{
	DWORD out;
	out = htonl(((DWORD)H<<24)|((DWORD)h<<16)|((DWORD)l<<8)|(DWORD)L);
	return out;
}


Uint16 MergeNetWord(BYTE H,BYTE L)
{
	BYTE out;
	out = htons(((DWORD)H<<8)&0xFF00|(DWORD)L);
	return out;
}
