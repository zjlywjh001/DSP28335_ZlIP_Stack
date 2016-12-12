/*
 * IP.h
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#ifndef INC_IP_H_
#define INC_IP_H_

#include "TCPIPmem.h"

/* IP */
//#define IP_ENABLE_FORWARD	/* enable packet come in an forward act as a router */
#define IP_VERSION_4	4

#define IP_HEAD_MIN_LEN	20	/* ip head len with no option */

#define IP_INITIAL_LIFE 64

#define IP_PROTOCOL_TCP		0x06
#define IP_PROTOCOL_ICMP	0x01

/* caculate ip version of a packet */
#define IP_VERSION(pIPHead) (pIPHead->Ver_HeadLen>>4)

/* caculate ip head len of a packet.head len in Ver_HeadLen
is mulitple of 4 byte */
#define IP_HEAD_LEN(pIPHead) ((pIPHead->Ver_HeadLen & 0x0f)*4)

typedef DWORD IP_ADDR;

struct SIPHead
{
	/* Version 4 bits, HeadLength 4 bits. typical value is 0x45 */
	BYTE Ver_HeadLen;

	/* Precedence(priority) 3 bits, Delay, Throughput, Reliability
	, reserved 2 bits. typical value 0x00 */
	BYTE ServeType;

	BYTE TotalLenH;				/* all size of IP packet inlcude IPHead. 16 bits */
	BYTE TotalLenL;
	BYTE FragmentIDH;			/* 16 bits */
	BYTE FragmentIDL;

	/* Reserved 1 bit, May be fragmented 1 bit, Last fragment 1 bit,
	Fragment offset 13 bits. typical 0x00 */
#define	IP_FRAGMENT_OFFSET_MASK	0x1FFF
	BYTE FragmentFlag_OffsetH;
	BYTE FragmentFlag_OffsetL;

	BYTE LifeLength;			/* ttl */
	BYTE Protocol;				/* eg. IP_PROTOCAL_TCP*/
	BYTE CheckSumH;				/* 16 bits */
	BYTE CheckSumL;
	BYTE IPScrH;				/* 32 bits */
	BYTE IPScrh;
	BYTE IPScrl;
	BYTE IPScrL;
	BYTE IPDestH;
	BYTE IPDesth;
	BYTE IPDestl;
	BYTE IPDestL;
};

WORD CheckSum(WORD DT_XDATA * buff,WORD size,DWORD InSum) REENTRANT_SIG;
void IPInput(struct SMemHead DT_XDATA *MemHead) REENTRANT_MUL;
BOOL IPOutput(struct SMemHead DT_XDATA * MemHead) REENTRANT_SIG;




#endif /* INC_IP_H_ */
