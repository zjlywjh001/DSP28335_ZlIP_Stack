/*
 * ARP.c
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


BYTE DT_XDATA EtherAddrAny[ETHER_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
BYTE DT_XDATA EtherAddrUnknown[ETHER_ADDR_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00};

/* entry table */
struct SARPEntry DT_XDATA ARPTable[ARP_ENTRY_MAX_NUM];

/* arp init */
void ARPInit() REENTRANT_MUL
{
	BYTE i;

	/* set every unit in arp tabel invalid */
	for(i = 0; i < ARP_ENTRY_MAX_NUM; i++)
		ARPTable[i].time = 0;
}

/* construct a arp query packet and return it */
struct SMemHead DT_XDATA * ARPQuery(struct SNetIf DT_XDATA *NetIf,IP_ADDR DestIP) REENTRANT_SIG
{
	struct SMemHead DT_XDATA *MemHead;
	struct SEtherHead DT_XDATA *EtherHead;
	struct SARPPacket DT_XDATA *ARPPacket;

	/* allocate a packet mem */
	if((MemHead = MemAllocate(sizeof(struct SARPPacket) +
		sizeof(struct SEtherHead))) == NULL)
		return NULL;

	EtherHead = (struct SEtherHead DT_XDATA *)(MemHead->pStart);
	ARPPacket = (struct SARPPacket DT_XDATA *)(MemHead->pStart +
		sizeof(struct SEtherHead));

	/* fill Ether head */
	MyMemCopy(EtherHead->DestAddr,EtherAddrAny,ETHER_ADDR_LEN);
	MyMemCopy(EtherHead->ScrAddr ,
		((struct SEtherDevice DT_XDATA *)(NetIf->Info))->Addr,ETHER_ADDR_LEN);
	EtherHead->typeH = NetWord_H(htons(ETHER_TYPE_ARP));
	EtherHead->typeL = NetWord_L(htons(ETHER_TYPE_ARP));

	/* fill arp head */
	ARPPacket->HardWareAddrLen	= ARP_HARDWARE_ADDR_LEN_ETHER;
	ARPPacket->HardwareTypeH		= NetWord_H(htons(ARP_HARDWARE_TYPE_ETHER));
	ARPPacket->HardwareTypeL		= NetWord_L(htons(ARP_HARDWARE_TYPE_ETHER));
	ARPPacket->ProtocolAddrLen	= ARP_PROTOCOL_ADDR_LEN_IP;
	ARPPacket->ProtocolTypeH		= NetWord_H(htons(ARP_PROTOCOL_TYPE_IP));
	ARPPacket->ProtocolTypeL		= NetWord_L(htons(ARP_PROTOCOL_TYPE_IP));
	ARPPacket->typeH				= NetWord_H(htons(ARP_TYPE_ARP_REQUEST));
	ARPPacket->typeL				= NetWord_L(htons(ARP_TYPE_ARP_REQUEST));

	/* fill arp content */
	ARPPacket->IPDestAddrH	= NetDword_H(DestIP);
	ARPPacket->IPDestAddrh	= NetDword_h(DestIP);
	ARPPacket->IPDestAddrl	= NetDword_l(DestIP);
	ARPPacket->IPDestAddrL	= NetDword_L(DestIP);
	ARPPacket->IPScrAddrH	= NetDword_H(NetIf->IPAddr);
	ARPPacket->IPScrAddrh	= NetDword_h(NetIf->IPAddr);
	ARPPacket->IPScrAddrl	= NetDword_l(NetIf->IPAddr);
	ARPPacket->IPScrAddrL	= NetDword_L(NetIf->IPAddr);
	MyMemCopy(ARPPacket->EtherDestAddr,EtherAddrUnknown,ETHER_ADDR_LEN);
	MyMemCopy(ARPPacket->EtherScrAddr,
		((struct SEtherDevice DT_XDATA *)(NetIf->Info))->Addr,ETHER_ADDR_LEN);

	return MemHead;
}

/* deel with a input arp packet. if send a reply is needed return
this replay packet, oterhwise return NULL  */
struct SMemHead DT_XDATA *ARPInput(struct SMemHead DT_XDATA *MemHead, struct SNetIf DT_XDATA *NetIf) REENTRANT_MUL
{
	struct SEtherHead DT_XDATA *EtherHead;
	struct SARPPacket DT_XDATA *ARPPacket;

	EtherHead = (struct SEtherHead DT_XDATA *)(MemHead->pStart);
	ARPPacket = (struct SARPPacket DT_XDATA *)(MemHead->pStart +
		sizeof(struct SEtherHead));

	/* which type of arp */
	switch(ntohs(MergeNetWord(ARPPacket->typeH,ARPPacket->typeL)))
	{
	case ARP_TYPE_ARP_REQUEST:

		/* if arp request to local host */
		if(MergeNetDword(ARPPacket->IPDestAddrH,ARPPacket->IPDestAddrh,ARPPacket->IPDestAddrl,ARPPacket->IPDestAddrL) == NetIf->IPAddr)
		{
			/* send arp replay */

			/* fill Ether head */
			MyMemCopy(EtherHead->DestAddr,ARPPacket->EtherScrAddr,ETHER_ADDR_LEN);
			MyMemCopy(EtherHead->ScrAddr,
				((struct SEtherDevice DT_XDATA *)(NetIf->Info))->Addr,ETHER_ADDR_LEN);
			EtherHead->typeH = NetWord_H(htons(ETHER_TYPE_ARP));
			EtherHead->typeL = NetWord_L(htons(ETHER_TYPE_ARP));

			/* copy source part to dest part. include Ether addr and
			Ip addr */
			MyMemCopy(ARPPacket->EtherDestAddr,ARPPacket->EtherScrAddr,
				(sizeof(IP_ADDR) + ETHER_ADDR_LEN));

			/* fill source part. include Ether addr and Ip addr*/
			ARPPacket->IPDestAddrH = ARPPacket->IPScrAddrH;
			ARPPacket->IPDestAddrh = ARPPacket->IPScrAddrh;
			ARPPacket->IPDestAddrl = ARPPacket->IPScrAddrl;
			ARPPacket->IPDestAddrL = ARPPacket->IPScrAddrL;

			ARPPacket->IPScrAddrH	= NetDword_H(NetIf->IPAddr);
			ARPPacket->IPScrAddrh	= NetDword_h(NetIf->IPAddr);
			ARPPacket->IPScrAddrl	= NetDword_l(NetIf->IPAddr);
			ARPPacket->IPScrAddrL	= NetDword_L(NetIf->IPAddr);
			MyMemCopy(ARPPacket->EtherScrAddr,
				((struct SEtherDevice DT_XDATA *)(NetIf->Info))->Addr,ETHER_ADDR_LEN);

			/* arp type */
			ARPPacket->typeH = NetWord_H(htons(ARP_TYPE_ARP_REPLY));
			ARPPacket->typeL = NetWord_L(htons(ARP_TYPE_ARP_REPLY));

			return MemHead;
		}
		break;

	case ARP_TYPE_ARP_REPLY:
		/* add to arp table */
		ARPAddEntry(ARPPacket);
		break;
	}

	/* for any case except ARP_TYPE_ARP_REQUEST for this IP,
	arp packet is released */
	MemFree(MemHead);

	/* no packet need send */
	return NULL;
}

/* add a entry to arp table */
void ARPAddEntry(struct SARPPacket DT_XDATA *ARPPacket) REENTRANT_MUL
{
	BYTE i;
	WORD MinTime;
	BYTE iToReplace;	/* index of entry going to be replace */

	/* find a free entry */
	for(i = 0; i<ARP_ENTRY_MAX_NUM; i++)
	{
		if(ARPTable[i].time == 0)
		{
			iToReplace = i;
			break;
		}
	}

	/* if no free entry, find the oldest entry */
	if(i == ARP_ENTRY_MAX_NUM)
	{
		for(i = 0, MinTime = ARP_ENTRY_TIME_OUT, iToReplace = 0;
			i<ARP_ENTRY_MAX_NUM; i++)
		{
			if(MinTime > ARPTable[i].time)
			{
				MinTime = ARPTable[i].time;
				iToReplace = i;
			}
		}
	}

	/* replace the entry */
	MyMemCopy(ARPTable[iToReplace].EtherAddr,ARPPacket->EtherScrAddr,
		ETHER_ADDR_LEN);
	ARPTable[iToReplace].IPAddr = MergeNetDword(ARPPacket->IPScrAddrH,ARPPacket->IPScrAddrh,ARPPacket->IPScrAddrl,ARPPacket->IPScrAddrL);

	/* start timer */
	ARPTable[iToReplace].time = ARP_ENTRY_TIME_OUT;
}

/* find IPAddr in arptable copy it to EtherAddr. if can't find return
false */
BOOL ARPFind(BYTE EtherAddr[],IP_ADDR IPAddr) REENTRANT_SIG
{
	BYTE i;
	for(i = 0; i<ARP_ENTRY_MAX_NUM; i++)
	{
		/* check only valid entry */
		if(ARPTable[i].time != 0)
		{
			if(ARPTable[i].IPAddr == IPAddr)
			{
				MyMemCopy(EtherAddr,ARPTable[i].EtherAddr,ETHER_ADDR_LEN);

				return TRUE;
			}
		}
	}
	return FALSE;
}

void ARPTimer() REENTRANT_MUL
{
	BYTE i;

	/* decrease every entry timer */
	for(i = 0; i<ARP_ENTRY_MAX_NUM; i++)
	{
		/* check only valid entry */
		if(ARPTable[i].time != 0)
		{
			ARPTable[i].time--;
		}
	}
}

