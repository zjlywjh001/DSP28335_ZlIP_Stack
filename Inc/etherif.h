/*
 * etherif.h
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#ifndef INC_ETHERIF_H_
#define INC_ETHERIF_H_

#include "TCPIPmem.h"
#include "Netif.h"


/* net interface. fetch packet and send packet */
#define ETHER_TYPE_IP	0x0800
#define ETHER_TYPE_ARP  0x0806

#define ETHER_ADDR_LEN	6


/* the header of Ethernet packet */
struct SEtherHead
{
	BYTE	DestAddr[ETHER_ADDR_LEN];
	BYTE	ScrAddr[ETHER_ADDR_LEN];

	/* 16 bits.0800H IP, 0806H ARP, value less than 0x0600 used in
	IEEE802 to indicate the length of the packet*/
	BYTE typeH;
	BYTE typeL;

};

/* struct for every ethernet device */
struct SEtherDevice
{
	BYTE Addr[ETHER_ADDR_LEN];

	/* send by this device */
	BOOL (DT_CODE * send)(void DT_XDATA *buf, WORD size)  REENTRANT_SIG;

	/* get a packet from device buffer. returned packet is sorted
	in buffer pointed by SMemHead. If no packet return NULL */
	struct SMemHead DT_XDATA * (DT_CODE * recv)() REENTRANT_SIG;
};

BOOL EtherOutput(struct SMemHead DT_XDATA *MemHead,struct SNetIf DT_XDATA* NetIf,
		IP_ADDR DestIP) REENTRANT_SIG;
void EtherInput(struct SNetIf DT_XDATA * NetIf) REENTRANT_SIG;
void EtherDevInit(struct SEtherDevice DT_XDATA * pDevice, BYTE EtherAddr[],
				  BOOL (DT_CODE * send)(void DT_XDATA *buf, WORD size) REENTRANT_SIG,
				  struct SMemHead DT_XDATA *(DT_CODE * recv)() REENTRANT_SIG) REENTRANT_MUL;






#endif /* INC_ETHERIF_H_ */
