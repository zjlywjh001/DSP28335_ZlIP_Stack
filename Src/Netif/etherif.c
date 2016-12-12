/*
 * etherif.c
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#include "GlobalDef.h"
#include "TCPIPmem.h"
#include "IP.h"
#include "etherif.h"
#include "ARP.h"
#include "Netif.h"


/* call output to put a packet from IP layer to device. After
	Ip layer selected a device, it use output to send this packet.
	MemHead contain a packet and Netif tell dirver which netif it
	is. NOTE:MemHead->pStart point to pIPHead
return:
	TRUE: send successfuly.*/
BOOL EtherOutput(struct SMemHead DT_XDATA *MemHead,struct SNetIf DT_XDATA* NetIf,
		IP_ADDR DestIP) REENTRANT_SIG
{
	DWORD NextIP;	/* next host to receive the packet in rout */
	struct SEtherHead DT_XDATA * pEtherHead;
	struct SMemHead DT_XDATA *p;

	pEtherHead = (struct SEtherHead DT_XDATA *)(MemHead->pStart - sizeof(struct SEtherHead));

	/* if DestIP in this subnet ... */
	if((NetIf->NetMask & NetIf->IPAddr) == (NetIf->NetMask & DestIP))
		NextIP = DestIP;
	else
		NextIP = NetIf->GateWay;

	/* find Ether addr of NextIP */
	if(ARPFind(pEtherHead->DestAddr,NextIP) == FALSE)
	{
		/* send a arp query */
		if((p = ARPQuery(NetIf,NextIP)) != NULL)
		{
			((struct SEtherDevice DT_XDATA *)(NetIf->Info))->send(
				p->pStart,sizeof(struct SARPPacket) +
				sizeof(struct SEtherHead));

			MemFree(p);
		}
	}
	else
	{
		/* fill ehter header, DestAddr already filled in ARPFind */
		MyMemCopy(pEtherHead->ScrAddr,
			((struct SEtherDevice DT_XDATA *)(NetIf->Info))->Addr,ETHER_ADDR_LEN);

		pEtherHead->typeH = NetWord_H(htons(ETHER_TYPE_IP));
		pEtherHead->typeL = NetWord_L(htons(ETHER_TYPE_IP));

		/* send the packet. packet lenth is less than MemHead size */
		return ((struct SEtherDevice DT_XDATA *)(NetIf->Info))->send(
			pEtherHead,(WORD)(MemHead->pEnd - (BYTE DT_XDATA *)pEtherHead));
	}
	return FALSE;
	/* free MemHead when it is acked in tcp model */
}

/* this function is called periodically.Get a packet from specific
device. If there is a packet, call NetIf->Input to do more */
void EtherInput(struct SNetIf DT_XDATA * NetIf) REENTRANT_SIG
{
	struct SMemHead DT_XDATA *MemHead;
	struct SEtherHead DT_XDATA *pEtherHead;
	struct SMemHead DT_XDATA *p;

	/* if there is a packet to deal with */
	while((MemHead = ((struct SEtherDevice DT_XDATA *)(NetIf->Info))->recv())
		!= NULL)
	{
		/* Note, pStart point to EtherHead */
		pEtherHead = (struct SEtherHead DT_XDATA *)(MemHead->pStart);
		/* which packet type */
		switch(ntohs(MergeNetWord(pEtherHead->typeH,pEtherHead->typeL)))
		{
		case ETHER_TYPE_IP:
			/* before pass to IP layer, let MemHead->pStart point
			to IP header */
			MemHead->pStart += sizeof(struct SEtherHead);

			/* pass to IP layer for more dealing */
			IPInput(MemHead);
			break;

		case ETHER_TYPE_ARP:
			if((p = ARPInput(MemHead,NetIf)) != NULL)
			{
				/* a arp reply need to be send */
				((struct SEtherDevice DT_XDATA *)(NetIf->Info))->send(
					p->pStart,sizeof(struct SARPPacket)
					+ sizeof(struct SEtherHead));

				MemFree(p);
			}
			/* 'MemHead' is freed in ARPInput() */
			break;
		default:

			/* unknown packet type free */
			MemFree(MemHead);
		}
	}
}
/* ethernet device init */
void EtherDevInit(struct SEtherDevice DT_XDATA * pDevice, BYTE EtherAddr[],
				  BOOL (DT_CODE * send)(void DT_XDATA *buf, WORD size)  REENTRANT_SIG,
				  struct SMemHead DT_XDATA *(DT_CODE * recv)()  REENTRANT_SIG) REENTRANT_MUL
{
	MyMemCopy(pDevice->Addr,EtherAddr,ETHER_ADDR_LEN);
	pDevice->recv = recv;
	pDevice->send = send;
}
