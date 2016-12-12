/*
 * GlobalDef.h
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#ifndef INC_GLOBALDEF_H_
#define INC_GLOBALDEF_H_

#include "DSP2833x_Device.h"     // DSP2833x Headerfile Include File
#include "DSP2833x_Examples.h"   // DSP2833x Examples Include File

typedef unsigned char	BYTE;
typedef unsigned int   	WORD;
typedef unsigned long	DWORD;
typedef unsigned char	BOOL;

typedef signed long		SDWORD;

#ifndef HOST_ORDER_AS_NET
//#	define HOST_ORDER_AS_NET
#endif

#define	DT_CODE
#define	DT_XDATA
#define	REENTRANT_SIG
#define	REENTRANT_MUL


#ifndef TRUE
#	define TRUE 1
#endif

#ifndef FALSE
#	define FALSE 0
#endif

#ifndef NULL
#	define NULL 0
#endif

#ifndef HOST_ORDER_AS_NET
DWORD ntohl(DWORD in);
WORD ntohs(WORD in);
#else
#	define	ntohl(in)	in
#	define	ntohs(in)	in
#endif

#define htonl(in) ntohl(in)
#define htons(in) ntohs(in)

#define NetWord_H(in) ((WORD)in&0x00FF)
#define NetWord_L(in) (((WORD)in&0xFF00)>>8)
#define NetDword_H(in) ((DWORD)in&0x000000FF)
#define NetDword_h(in) (((DWORD)in&0x0000FF00)>>8)
#define NetDword_l(in) (((DWORD)in&0x00FF0000)>>16)
#define NetDword_L(in) (((DWORD)in&0xFF000000)>>24)

void MyMemCopy(BYTE DT_XDATA *buf1,BYTE DT_XDATA *buf2,WORD size)  REENTRANT_SIG;
Uint32 MergeNetDword(BYTE H,BYTE h,BYTE l,BYTE L);
Uint16 MergeNetWord(BYTE H,BYTE L);


#endif /* INC_GLOBALDEF_H_ */
