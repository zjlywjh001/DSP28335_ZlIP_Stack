/*
 * ARP.h
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#ifndef INC_ARP_H_
#define INC_ARP_H_

#include "TCPIPmem.h"
#include "etherif.h"
#include "IP.h"
/* arp */

/* arppacket->HardwareType */
#define ARP_HARDWARE_TYPE_ETHER 0x0001

/* arppacket->ProtocolType */
#define ARP_PROTOCOL_TYPE_IP	0x0800

#define ARP_HARDWARE_ADDR_LEN_ETHER	ETHER_ADDR_LEN
#define ARP_PROTOCOL_ADDR_LEN_IP	4

/* arppacket->type */
#define ARP_TYPE_ARP_REQUEST	0x0001
#define ARP_TYPE_ARP_REPLY		0x0002
#define ARP_TYPE_RARP_REQUEST	0x0003
#define ARP_TYPE_RARP_REPLY		0x0004

#define ARP_ENTRY_MAX_NUM		4		/* must <= 255 */
#define ARP_ENTRY_TIME_OUT		0xFFFF	/* the time for refresh a entry */

/* arp entry */
struct SARPEntry
{
	IP_ADDR		IPAddr;
	BYTE	EtherAddr[ETHER_ADDR_LEN];

	/* decrease every time trick. when it hit to 0,
	remove it from arp entry tabel. if time = 0 indicate
	this entry is invalid*/
	WORD time;
};

/* arp packet struct */
struct SARPPacket
{
	/* header */
	BYTE HardwareTypeH;
	BYTE HardwareTypeL;
	BYTE ProtocolTypeH;
	BYTE ProtocolTypeL;
	BYTE HardWareAddrLen;
	BYTE ProtocolAddrLen;
	BYTE typeH;				/* refer to arp type */
	BYTE typeL;

	/* arp content */
	BYTE	EtherScrAddr[ETHER_ADDR_LEN];
	BYTE		IPScrAddrH;
	BYTE		IPScrAddrh;
	BYTE		IPScrAddrl;
	BYTE		IPScrAddrL;
	BYTE	EtherDestAddr[ETHER_ADDR_LEN];
	BYTE		IPDestAddrH;
	BYTE		IPDestAddrh;
	BYTE		IPDestAddrl;
	BYTE		IPDestAddrL;
};


void ARPInit() REENTRANT_MUL;
struct SMemHead DT_XDATA * ARPQuery(struct SNetIf DT_XDATA *NetIf,IP_ADDR DestIP) REENTRANT_SIG;
struct SMemHead DT_XDATA *ARPInput(struct SMemHead DT_XDATA *MemHead, struct SNetIf DT_XDATA *NetIf) REENTRANT_MUL;
void ARPAddEntry(struct SARPPacket DT_XDATA *ARPPacket)  REENTRANT_MUL;
BOOL ARPFind(BYTE EtherAddr[],IP_ADDR IPAddr)  REENTRANT_SIG;
void ARPTimer()  REENTRANT_MUL;



#endif /* INC_ARP_H_ */
