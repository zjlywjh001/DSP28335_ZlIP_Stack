/*
 * icmp.h
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#ifndef INC_ICMP_H_
#define INC_ICMP_H_

#include "TCPIPmem.h"

#define ICMP_EN	1	/* if icmp is enabled */

#if ICMP_EN
#define ICMP_TYPE_QUERY	8
#define ICMP_TYPE_REPLY	0
struct SICMPEchoHead
{
	BYTE type;
	BYTE ICMPCode;
	BYTE CheckSumH;
	BYTE CheckSumL;
	BYTE idH;
	BYTE idL;
	BYTE seqH;
	BYTE seqL;
};
void ICMPInput(struct SMemHead DT_XDATA *MemHead)	REENTRANT_MUL;
#endif


#endif /* INC_ICMP_H_ */
