/*
 * icmp.c
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */



#include "GlobalDef.h"
#include "TCPIPmem.h"
#include "IP.h"
#include "icmp.h"

#if ICMP_EN
/* MemHead->pStart is point to ICMP head */
void ICMPInput(struct SMemHead DT_XDATA *MemHead)	REENTRANT_MUL
{
	IP_ADDR ipaddr;
	struct SIPHead DT_XDATA *pIPHead;

	/* which type of icmp */
	switch(((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->type)
	{
	case ICMP_TYPE_QUERY:

		/* chage type */
		((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->type = ICMP_TYPE_REPLY;

		/* adjust checksum. refer to lwip: if change type from 8 to 0,
		for checksum, that is increasing 0x8000 and add flowed hight bit
		to bit 0.*/
		if (MergeNetWord(((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumH,((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumL) >= htons(0xffff - (((WORD)ICMP_TYPE_QUERY) << 8)))
		{
			WORD checksum = MergeNetWord(((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumH,((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumL) + htons(((WORD)ICMP_TYPE_QUERY) << 8) + 1;
			((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumH = NetWord_H(checksum);
			((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumL = NetWord_L(checksum);

		}
		else
		{
			WORD checksum = MergeNetWord(((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumH,((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumL) + htons(((WORD)ICMP_TYPE_QUERY) << 8);
			((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumH = NetWord_H(checksum);
			((struct SICMPEchoHead DT_XDATA *)(MemHead->pStart))->CheckSumL = NetWord_L(checksum);

		}

		/*
		 *send this packet back, first fill some of IPHead field
		 */

		/* dec pStart and get ip head */
		pIPHead = (struct SIPHead DT_XDATA *)(MemHead->pStart -= IP_HEAD_MIN_LEN);

		/* exchange ipdest and ipscr */
		ipaddr = MergeNetDword(pIPHead->IPDestH,pIPHead->IPDesth,pIPHead->IPDestl,pIPHead->IPDestL);
		pIPHead->IPDestH = pIPHead->IPScrH;
		pIPHead->IPDesth = pIPHead->IPScrh;
		pIPHead->IPDestl = pIPHead->IPScrl;
		pIPHead->IPDestL = pIPHead->IPScrL;
		pIPHead->IPScrH  = NetDword_H(ipaddr);
		pIPHead->IPScrh  = NetDword_h(ipaddr);
		pIPHead->IPScrl  = NetDword_l(ipaddr);
		pIPHead->IPScrL  = NetDword_L(ipaddr);


		/* ip total length not change */

		/* protocol */
		pIPHead->Protocol = IP_PROTOCOL_ICMP;

		IPOutput(MemHead);

		/* whether send success or not free this packet */
		MemFree(MemHead);

		break;
	default:
		MemFree(MemHead);
	}
}

#endif
