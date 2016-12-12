/*
 * TCPIPmem.h
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#ifndef INC_TCPIPMEM_H_
#define INC_TCPIPMEM_H_

/* mem manage especial for TCPIP packet */


#define TCPIP_BUF_SIZE	0x2000

/* use pNext and pPre mem blocks are linked in chain */
struct SMemHead
{
	struct SMemHead DT_XDATA *pNext;	/* next mem block */
	struct SMemHead DT_XDATA *pPre;	/* previous mem block */
	BOOL used;				/* if in using */
	BYTE DT_XDATA *pStart;			/* the start address of valid data */
	BYTE DT_XDATA *pEnd;
};

void MemInit() REENTRANT_MUL;
struct SMemHead DT_XDATA *MemAllocate(WORD size) REENTRANT_SIG;
void MemFree(struct SMemHead DT_XDATA * MemHead) REENTRANT_SIG;
WORD MemFreeSize() REENTRANT_SIG;



#endif /* INC_TCPIPMEM_H_ */
