/*
 * Netif.c
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#include "GlobalDef.h"
#include "TCPIPmem.h"
#include "IP.h"
#include "Netif.h"

/* You can get a new netif from NetIfPool */
static struct SNetIf DT_XDATA NetIfPool[NET_IF_MAX_NUM];

/* list hearder of free Netifs buffer */
static struct SNetIf DT_XDATA * DT_XDATA NetIfFreeList;

/* list header of Netifs */
static struct SNetIf DT_XDATA * DT_XDATA NetIfList;

void NetIfInit() REENTRANT_MUL
{
	WORD count;

	/* inti NetIfList */
	NetIfList = NULL;

	/* inti FreeList, chain up NetIfPool */
	for(count = 0, NetIfFreeList = NULL; count < NET_IF_MAX_NUM; count++)
	{
		NetIfPool[count].pNext = NetIfFreeList;
		NetIfFreeList = &NetIfPool[count];
	}
}

/* add a netif to list, return NULL if insufficient mem */
struct SNetIf DT_XDATA * NetIfAdd(DWORD IPAddr, DWORD NetMask,DWORD GateWay,
						 void (DT_CODE * input)(struct SNetIf DT_XDATA * NetIf) REENTRANT_SIG,
						 BOOL (DT_CODE * output)(struct SMemHead DT_XDATA *MemHead,struct SNetIf DT_XDATA* NetIf,DWORD DestIP) REENTRANT_SIG,
						 void DT_XDATA * Info) REENTRANT_MUL
{
	struct SNetIf DT_XDATA * NetIf;

	/* get netif from free list */
	NetIf = NetIfFreeList;
	NetIfFreeList = NetIfFreeList->pNext;

	/* if add more than NET_IF_MAX_NUM return NULL */
	if(NetIf == NULL)
		return NULL;
	else
	{
		NetIf->IPAddr	= htonl(IPAddr);
		NetIf->NetMask	= htonl(NetMask);
		NetIf->GateWay	= htonl(GateWay);
		NetIf->input	= input;
		NetIf->output	= output;
		NetIf->Info		= Info;

		/* add to the head of the list */
		NetIf->pNext = NetIfList;
		NetIfList = NetIf;

		return NetIf;
	}
}

/* find a netif which NetIf->NetMask & NetIf->NetAddr ==
NetIf->NetMask & IPAddr */
struct SNetIf DT_XDATA * NetIfFindRout(IP_ADDR IPAddr) REENTRANT_SIG
{
	struct SNetIf DT_XDATA *NetIf;
	for(NetIf = NetIfList; NetIf != NULL; NetIf = NetIf->pNext)
	{
		if((NetIf->NetMask & NetIf->IPAddr) == (NetIf->NetMask & IPAddr))
			return NetIf;
	}

	/* if can't find any suitable Netif, return NetIfList. That is the
	first netif is the default Netif*/
	return NetIfList;
}

struct SNetIf DT_XDATA * NetIfFindIP(IP_ADDR IPAddr) REENTRANT_MUL
{
	struct SNetIf DT_XDATA *pNetIf;
	for(pNetIf = NetIfList; pNetIf != NULL; pNetIf = pNetIf->pNext)
	{
		if(pNetIf->IPAddr == IPAddr)
			break;
	}
	return pNetIf;
}

/* net inteftimer. use to packup packet from every net interface */
void NetIfTimer() REENTRANT_MUL
{
	struct SNetIf DT_XDATA * NetIf;

	/* if netif has data to come in */
	for(NetIf = NetIfList; NetIf != NULL; NetIf = NetIf->pNext)
		NetIf->input(NetIf);
}
