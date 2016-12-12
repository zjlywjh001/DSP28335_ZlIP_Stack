/*
 * RTL8019.h
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#ifndef INC_RTL8019_H_
#define INC_RTL8019_H_

#include "TCPIPmem.h"

/* min and max packet size */
#define MIN_PACKET_SIZE 60		/* min length in earthnet */
#define MAX_PACKET_SIZE 1514

/* after hardware reset a delay is need. if your RTL8019 is reset
in program(not power up) a delay is adviced */
#define RTL_DELAY_AFTER_HARDWARE_RESET 30000

/* for example if you use a1-a5 as address lines ADDRESS_SHIFT should be 0x02 */
#define	ADDRESS_SHIFT 0x01//0x01

/* base address of RTL8019 */
#define RTL_BASE_ADDRESS 0x4900//0xb000


/* receive start page also transmit stop page. it will write in
register PSTART*/
#define RECEIVE_START_PAGE		0x4C

/* value in PSTOP, max is 0x60(refrence to rtl8019 datasheet),
agree with (web51.HWserver.com), disagree with laogu(www.laogu.com).
after test there is no problem set as 0x80*/
#define RECEIVE_STOP_PAGE		0x60

/* value in TPSR, two startpage of two block of trasmit buffer */
#define SEND_START_PAGE0		0x40
#define SEND_START_PAGE1		0x46
#define SEND_STOP_PAGE	RECEIVE_START_PAGE

/***************************
 *	define about registers *
 ***************************/

/* command register */
#define CR	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x00)	/* 7-6 bits pages; 5-3 bits commands; 2 trasmmite packet flag; 1-0 excute command */
#define CR_PAGE0				0x00					/* pages */
#define CR_PAGE1				0x40
#define CR_PAGE2				0x80
#define CR_PAGE3				0xA0
#define CR_REMOTE_READ			0x08					/* commands */
#define CR_REMOTE_WRITE			0x10
#define CR_SEND_PACKET			0x18
#define CR_ABORT_COMPLETE_DMA	0x20
#define CR_TXP					0x04					/* trasmmite packet flag */
#define CR_START_COMMAND		0x02					/* excute command */
#define CR_STOP_COMMAND			0x01

/* register about transmit/receive page */
#define PSTART_WPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x01)	/* receive start page register */
#define PSTART_RPAGE2	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x01)
#define PSTOP_WPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x02)	/* receive stop page register */
#define PSTOP_RPAGE2	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x02)
/*	bnry is the last page read, so next time you read from bnry+1. inital is startpage
	curr is the page going to write, so next time RTL write in to curr.init is startpage + 1.
	empty: curr = bnry + 1; full: curr = bnry( leave a page not write);
	when curr equal stop page RTL set it to startpage automotly*/
#define BNRY_WPAGE0		(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x03)	/* recieve next to read page register */
#define BNRY_RPAGE0		(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x03)
#define CURR_WPAGE1		(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x07)	/* recieve next to write page + 1 register */
#define CURR_RPAGE1		(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x07)
#define TPSR_WPAGE0		(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x04)	/* transmit page start register */
#define TPSR_RPAGE2		(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x04)
#define TBCRL_WPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x05)	/* transmit byte count high register */
#define TBCRH_WPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x06)	/* transmit byte count low register */

/* register about interrupt */
#define ISR_WPAGE0 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x07)	/* Interrupt Status Register. write 1 into corresponding bit clear the bit */
#define ISR_RPAGE0 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x07)
#define ISR_RESET_STATE_RECEIVE_OVER				0x80 /* bit 7 */
#define ISR_DMA_OPERATION_COMPLETE					0x40 /* bit 6 */
#define ISR_COUNT_SETED								0x20 /* bit 5. Set when remote DMA operation has been completed.*/
#define ISR_RECEIVE_BUFFER_EXHAUSTED				0x10 /* bit 4 */
#define ISR_TRANSMIT_ABORT_EXCESSIVE_COLLISION		0x08 /* bit 3 */
#define ISR_RECIVED_PACKET_WITH_ERROR				0x04 /* bit 2. CRC error Frame alignment error Missed packet */
#define ISR_TRANSMIT_NO_ERROR						0x02 /* bit 1. */
#define ISR_RECEIVE_NO_ERROR						0x01/* bit 0. */
#define IMR_WPAGE0 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0F)	/* Interrupt Mask Register. Set 1 enable the interrupt. power up all 0s */
#define IMR_RPAGE2 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0F)

/*	DCR data configuration Register.
	typical set is 11001000 = 0xC8.
	bit 7 always 1.
	bit 6-5 FT1 FT0 = 10
	bit 4 Auto-initalize Remote = Send packet Command not executed = 0
	bit 3 Loopback Select = Normal = 1
	bit 2 This bit must be set to zero
	bit 1 Byte Order = 8086 CPU = 0
	bit 0 Word Transfer Select = byte wide DMA transfer not word = 0 */
#define DCR_WPAGE0 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0E)
#define DCR_RPAGE2 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0E)

/*	TCR Transmit Configuration Register
	typical set is 11100000 = 0xE0
	bit 7-5 always 1
	bit 4 Collision Offset Enable = 0
	bit 3 AutoTransmit Disable = normal operation = 0
	bit 2-1 Lookback = normal operation = 00
	bit 0 CRC generator and Checker enabled. */
#define TCR_WPAGE0 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0D)
#define TCR_RPAGE2 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0D)

/*	RCR Receive Configuration Register
	typical set is 11001110 = 0xCE, not receive  broad cast packet set as =0xC8
	bit 7-6 always 1
	bit 5 mon buffer to memory or not = no mon = 0
	bit 4 all address packet recieve = packet with specified address accepted = 0
	bit 3 muliticast destination address accepted? = yes = 1
	bit 2 packets with broadcast destination address are accepted? = yes = 1
	bit 1 packets with length fewer than 64 bytes are accepted? = yes = 1
	bit 0 packets with receive errors are accepted? = no = 0 */
#define RCR_WPAGE0 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0C)
#define RCR_RPAGE2 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0C)

/* RSR Receive State Register */
#define RSR_RPAGE0 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0C)
#define RSR_MULTI_BROADCAST_PACKET	0x20	/* current receive packet is multi or broadcast packet */
#define RSR_RECEIVE_NO_ERROR 0x01

/* registers aboute REMOTE DMA operation(read/write RTL8019 ram) */
#define REMOTE_DMA_PORT (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x10)	/* remote DMA port. any of 0x10 - 0x17. read/write RAM of rtl8019 from this port */
#define RSARH_WPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x09)	/* remote start address register. high byte */
#define RSARL_WPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x08)	/* remote start address register. low byte */
#define RBCRH_WPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0B)	/* remote byte count registers. high byte */
#define RBCRL_WPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x0A)	/* remote byte count registers. low byte */

/* registers about LOCAL DMA */
#define CLDAH_RPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x02)	/* Current Local DMA Adress high byte. typical use of this two register is to reading CURR */
#define CLDAL_RPAGE0	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x01)	/* Current Local DMA Adress low byte */

/* reset port */
#define RESET_PORT (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x1F) /* any of 0x18 - 0x1F. write to this port reset the rtl8019 */

/* phisical adress(MAC address) register */
#define PRA0_WPAGE1	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x01)
#define PRA1_WPAGE1	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x02)
#define PRA2_WPAGE1	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x03)
#define PRA3_WPAGE1	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x04)
#define PRA4_WPAGE1	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x05)
#define PRA5_WPAGE1	(RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x06)

/*	RTL config register 1
	bit7: IRQEN:set IRQ low level enable or high level enable
	bit6-4: IRQS2-0: which IRQ pins is selected as interrupt
	bit3-0: which base address is selected */
#define CONFIG1_RPAGE3 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x04)
#define CONFIG1_WPAGE3 (RTL_BASE_ADDRESS + ADDRESS_SHIFT * 0x04)

void RTLInit(BYTE LocalMACAddr[]) REENTRANT_MUL;
BOOL RTLSendPacket(BYTE DT_XDATA * buffer,WORD size) REENTRANT_SIG;
struct SMemHead DT_XDATA * RTLReceivePacket() REENTRANT_SIG;
void RTLSendPacketTest();



#endif /* INC_RTL8019_H_ */
