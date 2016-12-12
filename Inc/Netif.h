/*
 * Netif.h
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#ifndef INC_NETIF_H_
#define INC_NETIF_H_

#include "TCPIPmem.h"
#include "IP.h"

/* Network interface Head file */

/* the max number of netif */
#define NET_IF_MAX_NUM	1
#define NETIF_HEAD_MAX_LEN	14	/* the max  phsical layer head len. eg ethernet head len */


/* netif is associated with each netwok device */
struct SNetIf
{
	struct SNetIf DT_XDATA *pNext;	/* point to the next netif */
	IP_ADDR IPAddr;			/* Ip address for this device */
	IP_ADDR NetMask;			/* Net mask for this device */
	IP_ADDR GateWay;			/* Gate way IP address fort this device */

	/* call input to get a packet from device to IP layer. This
	function maybe 'EtherInput()', and called periodically */
	void (DT_CODE * input)(struct SNetIf DT_XDATA * NetIf) REENTRANT_SIG;

	/* call output to put a packet from IP layer to device. After
	Ip layer selected a device, it use output to send this packet.
	MemHead contain a packet and Netif tell dirver which netif it
	is. NOTE:MemHead->pStart point to pIPHead*/
	BOOL (DT_CODE * output)(struct SMemHead DT_XDATA *MemHead,struct SNetIf DT_XDATA* NetIf,DWORD DestIP) REENTRANT_SIG;

	void DT_XDATA *Info;	/* information of this device. eg. MAC address. */
};
void NetIfInit() REENTRANT_MUL;
struct SNetIf DT_XDATA * NetIfAdd(DWORD IPAddr, DWORD NetMask,DWORD GateWay,
						 void (DT_CODE * input)(struct SNetIf DT_XDATA * NetIf) REENTRANT_SIG,
						 BOOL (DT_CODE * output)(struct SMemHead DT_XDATA *MemHead,struct SNetIf DT_XDATA* NetIf,DWORD DestIP) REENTRANT_SIG,
						 void DT_XDATA * Info) REENTRANT_MUL;
struct SNetIf DT_XDATA * NetIfFindRout(IP_ADDR IPAddr) REENTRANT_SIG;
struct SNetIf DT_XDATA * NetIfFindIP(IP_ADDR IPAddr) REENTRANT_MUL;
void NetIfTimer() REENTRANT_MUL;


#endif /* INC_NETIF_H_ */
