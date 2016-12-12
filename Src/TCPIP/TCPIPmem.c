/*
 * TCPIPmem.c
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#include "GlobalDef.h"
#include "TCPIPmem.h"

/* buf for all tcpip packet */
static BYTE DT_XDATA TCPIPBuf[TCPIP_BUF_SIZE];
static struct SMemHead DT_XDATA * DT_XDATA MemHeadEnd;	/* end of the chain */
static struct SMemHead DT_XDATA * DT_XDATA MemHeadStart;

static WORD DT_XDATA FreeSize;	/* free memsize can use */

void MemInit() REENTRANT_MUL
{
	MemHeadEnd = (struct SMemHead DT_XDATA *)(TCPIPBuf + TCPIP_BUF_SIZE);
	MemHeadStart = (struct SMemHead DT_XDATA *)TCPIPBuf;

	/* at inital there is only a buf block. value in
	mem head is as following */
	MemHeadStart->pNext = MemHeadEnd;
	MemHeadStart->pPre  = MemHeadStart;
	MemHeadStart->used = FALSE;

	FreeSize = TCPIP_BUF_SIZE - sizeof(struct SMemHead);
}

/* allocate a buffer size of size, and set pStart to the start of buffer,pEnd
to end of buffer.Also decrease FreeSize. */
struct SMemHead DT_XDATA *MemAllocate(WORD size) REENTRANT_SIG
{
	struct SMemHead DT_XDATA *MemHead;
	struct SMemHead DT_XDATA *NewMemHead;
	WORD MemPlayloadSize;

	/* search though the mem blocks */
	for(MemHead = MemHeadStart; MemHead != MemHeadEnd; MemHead = MemHead->pNext)
	{
		/* if unused and mem playload size > size, select it. */
		if(MemHead->used == FALSE &&
			(MemPlayloadSize = (BYTE DT_XDATA *)(MemHead->pNext) - (BYTE DT_XDATA *)MemHead - sizeof(struct SMemHead)) >= size)
		{
			/* if MemPalyloadSize - size > sizeof(struct SMemHead) create
			a new SMemHead at the excess memory leaving for later usage */
			if(MemPlayloadSize - size > sizeof(struct SMemHead))
			{
				NewMemHead = (struct SMemHead DT_XDATA *)((BYTE DT_XDATA *)MemHead + sizeof(struct SMemHead)+ size );

				/* link into link chain */
				NewMemHead->pNext = MemHead->pNext;
				MemHead->pNext    = NewMemHead;
				NewMemHead->pPre  = MemHead;
				if(NewMemHead->pNext != MemHeadEnd)
				{
					NewMemHead->pNext->pPre = NewMemHead;
				}

				/* set new mem as unused */
				NewMemHead->used = FALSE;

				/* decrease FreeSize: playload of MemHead and the head of NewMemHead */
				FreeSize -= (BYTE DT_XDATA *)(MemHead->pNext) - (BYTE DT_XDATA *)MemHead;
			}
			else
			{
				/* decrease: playload of MemHead */
				FreeSize -= (BYTE DT_XDATA *)(MemHead->pNext) - (BYTE DT_XDATA *)MemHead - sizeof(struct SMemHead);
			}


			/* set pStart */
			MemHead->pStart = (BYTE DT_XDATA *)MemHead + sizeof(struct SMemHead);

			MemHead->pEnd	= MemHead->pStart + size;

			/* set as used */
			MemHead->used = TRUE;

			return MemHead;
		}
	}
	return NULL;
}

void MemFree(struct SMemHead DT_XDATA * MemHead) REENTRANT_SIG
{
	struct SMemHead DT_XDATA * MemHeadMergePre;
	struct SMemHead DT_XDATA * MemHeadMergeNext;

	/* set used flag to false */
	MemHead->used = FALSE;

	/* inc FreeSize the size of playload of 'MemHead'*/
	FreeSize += (BYTE DT_XDATA *)(MemHead->pNext) - (BYTE DT_XDATA *)MemHead - sizeof(struct SMemHead);

	/*
	 * if pNext or pPre is a unused memblock, merge with it
	 */

	/* find the two MemHead going to merge */
	if(MemHead->pPre->used == FALSE)
	{
		MemHeadMergePre = MemHead->pPre;	/* note: if MemHead == TCPBuf, MemHead->pPre == MemHead, but it is not a problom */
	}
	else
	{
		MemHeadMergePre = MemHead;
	}
	if(MemHead->pNext != MemHeadEnd && MemHead->pNext->used == FALSE)
	{
		MemHeadMergeNext = MemHead->pNext;

		/* MemHead of 'MemHead->pNext' will be free. Free size inc*/
		FreeSize += sizeof(struct SMemHead);
	}
	else
	{
		MemHeadMergeNext = MemHead;
	}

	/* merge is necessary? */
	if(MemHeadMergePre != MemHeadMergeNext)
	{
		/* merge. that is del MemHeadMergeNext from the chain */
		MemHeadMergePre->pNext = MemHeadMergeNext->pNext;
		if(MemHeadMergeNext->pNext != MemHeadEnd)
			MemHeadMergeNext->pNext->pPre = MemHeadMergePre;

		/* will MemHead of 'MemHead' will be free? */
		if(MemHead != MemHeadMergePre)
			FreeSize += sizeof(struct SMemHead);

	}
}

WORD MemFreeSize() REENTRANT_SIG
{
	return FreeSize;
}

