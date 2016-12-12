/*
 * IP.c
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */


#include "GlobalDef.h"
#include "TCPIPmem.h"
#include "IP.h"
#include "icmp.h"
#include "Netif.h"
#include "TCP.h"


/* Check sum calulation. data in buff, size, InSum is initial sum */
WORD CheckSum(WORD DT_XDATA * buff,WORD size,DWORD InSum) REENTRANT_SIG
{
	/* TO DO:in packet memory high part of short is in low memory. add all data in
	form of 16 bits get a result of 32 bits, add high 16 bits to low 16 bits two
	times.  get a 16 bits result then complement it. */

	DWORD cksum = InSum;

	/* sum all word except the last odd byte(if size is a odd num) */
	WORD DT_XDATA * EndBuf = buff + size;
	while(buff < EndBuf-1)
	{
		/* net order is equeal as host order in mirochip, so no need to change */
		cksum += MergeNetWord(*(buff+1),*(buff));
		buff = buff + 2;
	}

	/**((WORD xdata *)CheckSumInParam) = size;
	*((WORD xdata *)(CheckSumInParam+2)) = buff;
	asmAddCheckSum();
	cksum = CheckSumOutParm;
	*/

	/* if has last odd byte. use this byte as the high part of 16 bits, and add. */
	if((size & 0x0001) != 0)
		cksum += ((*buff)<<8) & 0xff00;

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >>16);
	return (WORD)(~cksum);
}

/* IP input process */
void IPInput(struct SMemHead DT_XDATA *MemHead) REENTRANT_MUL
{
	struct SIPHead DT_XDATA *pIPHead;
	struct SNetIf  DT_XDATA *pNetIf;		/* for search netif list */

	pIPHead = (struct SIPHead DT_XDATA *)(MemHead->pStart);

	/* check ip version */
	if(IP_VERSION(pIPHead) != IP_VERSION_4)
	{
		MemFree(MemHead);
		return;
	}

	/* if checksum is ok */
	if(CheckSum((WORD DT_XDATA *)pIPHead,(WORD)IP_HEAD_LEN(pIPHead),0) != 0)
	{
		MemFree(MemHead);
		return;
	}

	/* ip packet with options is not supported */
	if(IP_HEAD_LEN(pIPHead) != IP_HEAD_MIN_LEN)
	{
		MemFree(MemHead);
		return;
	}

	/* ip packet fragmented is not supported */
	if((ntohs(MergeNetWord(pIPHead->FragmentFlag_OffsetH,pIPHead->FragmentFlag_OffsetL)) & IP_FRAGMENT_OFFSET_MASK)!= 0)
	{
		MemFree(MemHead);
		return;
	}


	/* if this packet for us. check all the netif. if a host
	has tow device(tow ip). This packet may come from one device
	but send for the IP of the other deviec. In this case we should
	not drop or forward this packet */

	/* if this packet is not for us. forward it */
	if((pNetIf = NetIfFindIP(MergeNetDword(pIPHead->IPDestH,pIPHead->IPDesth,pIPHead->IPDestl,pIPHead->IPDestL))) == NULL)
	{
		#ifdef IP_ENABLE_FORWARD	/* if act as a router */
		/* We should decrease the IPHead->ttl */
		if(pIPHead->LifeLength != 0)
		{
			pIPHead->LifeLength--;

			/* recaculate IP head checksum. there is a easy method
			to recaculate, leave for later version improvment */
			CheckSum((WORD DT_XDATA *)pIPHead,(WORD)IP_HEAD_LEN(pIPHead),0);

			/* find a rout( a interface ) */
			if((pNetIf = NetIfFindRout(pIPHead->IPDest)) != NULL)
			{
				/* forward. send it through this interface. if return FALSE, we
				do not care, the soure of the packet will deel with it. */
				pNetIf->output(MemHead,pNetIf,pIPHead->IPDest);
			}
		}
		#endif

		MemFree(MemHead);
		return;
	}
	else
	{
		/* MemHead->pStart set to point uper layer */
		MemHead->pStart += sizeof(struct SIPHead);

		/* pass to the uper layer */
		switch(pIPHead->Protocol)
		{
		case IP_PROTOCOL_TCP:
			TCPInput(MemHead);
			break;
#if	ICMP_EN
		case IP_PROTOCOL_ICMP:
			ICMPInput(MemHead);
			break;
#endif
		default:
			MemFree(MemHead);
		}
	}
}

/* out put a ip packet,NOTE:MemHead->pStart point to IPHead.
IPScr IPDest Protocol TotalLen is already filled at uper layer.
To do so TCPCheckSum is easy to generate and pass augument to
IPOutput is easyer.
return :
	TURE: send the packt successful. */
BOOL IPOutput(struct SMemHead DT_XDATA * MemHead) REENTRANT_SIG
{
	struct SNetIf  DT_XDATA *pNetIf;
	struct SIPHead DT_XDATA *pIPHead;
	WORD tCheckSum;

	pIPHead = (struct SIPHead DT_XDATA *)(MemHead->pStart);

	/* found a rout */
	if((pNetIf = NetIfFindRout(MergeNetDword(pIPHead->IPDestH,pIPHead->IPDesth,pIPHead->IPDestl,pIPHead->IPDestL))) != NULL)
	{
		/* fill IP head */
		pIPHead->CheckSumH				= 0;
		pIPHead->CheckSumL				= 0;
		pIPHead->FragmentFlag_OffsetH	= 0;
		pIPHead->FragmentFlag_OffsetL	= 0;
		pIPHead->FragmentIDH				= 0;
		pIPHead->FragmentIDL				= 0;
		pIPHead->LifeLength				= IP_INITIAL_LIFE;
		pIPHead->ServeType				= 0;
		pIPHead->Ver_HeadLen			= (IP_VERSION_4 << 4) + IP_HEAD_MIN_LEN/4;

		/* checksum */
		tCheckSum = CheckSum((WORD DT_XDATA *)pIPHead,(WORD)IP_HEAD_LEN(pIPHead),0);
		pIPHead->CheckSumH = NetWord_H(htons(tCheckSum));
		pIPHead->CheckSumL = NetWord_L(htons(tCheckSum));

		/* output it */
		return pNetIf->output(MemHead,pNetIf,MergeNetDword(pIPHead->IPDestH,pIPHead->IPDesth,pIPHead->IPDestl,pIPHead->IPDestL));
	}
	else
		return FALSE;
	/* 'MemHead' freeing is at tcp model when it is acked */
}

