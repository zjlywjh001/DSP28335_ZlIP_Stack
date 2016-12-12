/*
 * TCP.c
 *
 *  Created on: 2016Äê11ÔÂ4ÈÕ
 *      Author: Jiaheng
 */

#include "GlobalDef.h"
#include "TCPIPmem.h"
#include "IP.h"
#include "Netif.h"
#include "TCP.h"

struct STCB DT_XDATA TCBPool[TCP_CONNECTION_MAX_NUM];
struct STCB DT_XDATA * DT_XDATA TCBFreeList;		/* free list */
struct STCB DT_XDATA * DT_XDATA TCBList;			/* tcb in use */

struct SPacketQueue DT_XDATA QPool[TCP_QUEUE_MAX_NUM];
struct SPacketQueue DT_XDATA * DT_XDATA QFreeList;

struct STCB DT_XDATA *TCPGetTCB() REENTRANT_SIG
{
	struct STCB DT_XDATA * pTCB;
	if((pTCB = TCBFreeList) != NULL)
	{
		TCBFreeList = TCBFreeList->pNext;
	}
	return pTCB;
}

void TCPInsertTCB(struct STCB DT_XDATA *pTCB) REENTRANT_SIG
{
	pTCB->pNext = TCBList;
	TCBList = pTCB;
}

struct SPacketQueue DT_XDATA * TCPGetQ() REENTRANT_SIG
{
	struct SPacketQueue DT_XDATA * pQ;
	if((pQ = QFreeList) != NULL)
	{
		QFreeList = QFreeList->pNext;
	}
	return pQ;
}

/* insert to the head of *ppQ. Q is a double link chain */
BOOL TCPInsertQ(struct SPacketQueue DT_XDATA * DT_XDATA * ppQ,struct SMemHead DT_XDATA *MemHead,
				DWORD Seq) REENTRANT_SIG
{
	struct SPacketQueue DT_XDATA *pNewQ;
	struct SPacketQueue DT_XDATA *pQ;

	/* allocate a queue to it */
	if((pNewQ = TCPGetQ()) == NULL)
		return FALSE;

	/* write content */
	pNewQ->Seq = Seq;
	pNewQ->MemHead = MemHead;

	/*
	 * link in the queue
	 */

	/* if is a empty queue */
	if(*ppQ == NULL)
	{
		*ppQ = pNewQ;

		/* pNext pPre point to itself when no others in queue */
		pNewQ->pNext = pNewQ;
		pNewQ->pPre  = pNewQ;
	}
	else
	{
		pQ = *ppQ;

		/* pNext link */
		pNewQ->pNext = pQ->pNext;
		pQ->pNext    = pNewQ;

		/* pPre link */
		pNewQ->pNext->pPre	= pNewQ;
		pNewQ->pPre			= pQ;
	}
	return TRUE;
}

/* move the last unit in queue outof queue,if the queue
is empty return FALSE.actrually last unit is *ppQ */
struct SPacketQueue DT_XDATA * TCPOutQ(struct SPacketQueue DT_XDATA * DT_XDATA * ppQ) REENTRANT_SIG
{
	struct SPacketQueue DT_XDATA *pQ;

	/* a empty queue? */
	if((pQ = *ppQ) == NULL)
		return NULL;

	/* after remove it, the queue is empty? */
	if(pQ->pNext == pQ)
		*ppQ = NULL;
	else
	{
		/* relink */
		pQ->pPre->pNext = pQ->pNext;
		pQ->pNext->pPre = pQ->pPre;

		/* and the queue head *ppQ point to pQ->pPre */
		*ppQ = pQ->pPre;
	}

	/* relaim it. link to QFreeList */
	pQ->pNext = QFreeList;
	QFreeList = pQ;
	return pQ;
}

void TCPInit() REENTRANT_MUL
{
	WORD i;

	/* move TCBPool to TCBFreeList */
	for(i = 0, TCBFreeList = NULL; i<TCP_CONNECTION_MAX_NUM; i++)
	{
		TCBPool[i].pNext = TCBFreeList;
		TCBFreeList = &TCBPool[i];
	}

	/* move QPool to QFreeList */
	for(i = 0, QFreeList = NULL; i<TCP_QUEUE_MAX_NUM; i++)
	{
		QPool[i].pNext = QFreeList;
		QFreeList = &QPool[i];
	}

	TCBList = NULL;
}


/* tcp check sum. return check sum. TCPSize = TCPDataSize + TCPHeadSize*/
WORD TCPCheckSum(struct SIPHead DT_XDATA * pIPHead,WORD TCPSize) REENTRANT_SIG
{
	DWORD sum = 0;
	WORD DT_XDATA * p;
	BYTE i;

	/* clac pseudo-header CheckSum. pseudo-header is:
	   source ip, destination ip, pad 8 bits, protocol, TCP lendth */
	sum = 0;

	/* source ip and dest ip */
	p = (WORD DT_XDATA *)(&(pIPHead->IPScrH));
	for(i=0; i < sizeof(DWORD)/sizeof(WORD)*2; i++,p=p+2)
		sum += MergeNetWord(*(p+1),*p);

	/* pad 8 and protocol */
	sum += pIPHead->Protocol;

	/* tcp lendth */
	sum += TCPSize;

	return CheckSum((WORD DT_XDATA *)((BYTE DT_XDATA *)pIPHead + IP_HEAD_MIN_LEN),TCPSize,sum);
}

/* this funtion should be called periodically */
void TCPTimer() REENTRANT_MUL
{
	struct STCB DT_XDATA *pTCB;

	/* go through all tcbs to see if any time out */
	for(pTCB = TCBList; pTCB != NULL; pTCB = pTCB->pNext)
	{
		/* delayed ack need send now? */
		if(pTCB->bNeedAck == TRUE)
		{
			if(pTCB->DelayAckTimer == 0)
			{
				/* send a ack. bNeedAck will set FLASE in TCPOutput*/
				TCPSendSeg(pTCB,TCPAllocate(0),TCP_ACK);
			}
			else
				pTCB->DelayAckTimer--;
		}

		/* TCP_STATE_LASTACK TCP_STATE_TIMEWAIT state time out? */
		if(pTCB->TCPState == TCP_STATE_LASTACK ||
			pTCB->TCPState == TCP_STATE_TIMEWAIT)
		{
			if(pTCB->LastAckTimer == 0)
			{
				pTCB->TCPState = TCP_STATE_CLOSED;

				/* release buf queue and call user close */
				TCPRelease(pTCB);
				/* let pTCB->close(); to be call when they send a fin when we at established */
			}
			else
				pTCB->LastAckTimer--;
		}

		/* if retransmit timer out? */
		if(pTCB->QUnacked != NULL)
		{
			if(pTCB->RetranTimer == 0)
			{
				/* retransmit,pStart set to tcpdata */
				IPOutput(pTCB->QUnacked->MemHead);

				/* timer restart and retransmit times inc */
				if(pTCB->RetranTimes == TCP_RETRAN_MAX_TIME)
				{
					pTCB->TCPState = TCP_STATE_CLOSED;

					/* closed by countpart shut down */
					TCPRelease(pTCB);
				}
				else
				{
					pTCB->RetranTimes++;
					pTCB->RetranTimer = TCP_RETRAN_TIME_OUT;
				}
			}
			else
				pTCB->RetranTimer--;
		}
	}
}
/* when a TCP close, send too much packet but no replay,
connection fail. TCPIP will call TCPRelease to release packet
and queue, but will not reclaim TCB. in other word user
can use this socket again. */
void TCPRelease(struct STCB DT_XDATA *pTCB) REENTRANT_SIG
{
	struct SPacketQueue DT_XDATA *pQ;

	/* reclaim Q, and free packet in queue */
	while(pQ = TCPOutQ(&(pTCB->QExceedSeq)))
		MemFree(pQ->MemHead);
	while(pQ = TCPOutQ(&(pTCB->QUnacked)))
		MemFree(pQ->MemHead);
	while(pQ = TCPOutQ(&(pTCB->QUnSend)))
		MemFree(pQ->MemHead);
}

/* fill a segment and send it,NOTE MemHead->pStart point to TCPData */
BOOL TCPSendSeg(struct STCB DT_XDATA *pTCB,struct SMemHead DT_XDATA *MemHead,BYTE TCPFlag) REENTRANT_SIG
{
	struct STCPHead DT_XDATA 	*pTCPHead;
	struct SIPHead  DT_XDATA 	*pIPHead;
	WORD SeqInc;

	/* mem insufficient? */
	if(MemHead == NULL)
		return FALSE;

	/* SeqMine increasment */
	if((TCPFlag & TCP_SYN) || (TCPFlag & TCP_FIN))
	{
		SeqInc = MemHead->pEnd - MemHead->pStart + 1;
	}
	else
	{
		SeqInc = MemHead->pEnd - MemHead->pStart;
	}

	pTCPHead = (struct STCPHead DT_XDATA *)(MemHead->pStart - sizeof(struct STCPHead));

	/* fill tcp header */
	pTCPHead->PortDestH		= NetWord_H(pTCB->PortDest);
	pTCPHead->PortDestL		= NetWord_L(pTCB->PortDest);
	pTCPHead->PortScrH		= NetWord_H(pTCB->PortScr);
	pTCPHead->PortScrL		= NetWord_L(pTCB->PortScr);
	pTCPHead->SeqH			= NetDword_H(htonl(pTCB->SeqMine));
	pTCPHead->Seqh			= NetDword_h(htonl(pTCB->SeqMine));
	pTCPHead->Seql			= NetDword_l(htonl(pTCB->SeqMine));
	pTCPHead->SeqL			= NetDword_L(htonl(pTCB->SeqMine));
	pTCPHead->AckSeqH		= NetDword_H(htonl(pTCB->SeqHis));
	pTCPHead->AckSeqh		= NetDword_h(htonl(pTCB->SeqHis));
	pTCPHead->AckSeql		= NetDword_l(htonl(pTCB->SeqHis));
	pTCPHead->AckSeqL		= NetDword_L(htonl(pTCB->SeqHis));
	pTCPHead->TCPHeadLen	= (BYTE)(((BYTE)sizeof(struct STCPHead)/4)<<4);
	pTCPHead->flag			= TCPFlag;
	pTCPHead->WndSizeH		= NetWord_H(htons(pTCB->WndMine = MemFreeSize()));
	pTCPHead->WndSizeL		= NetWord_L(htons(pTCB->WndMine = MemFreeSize()));
	pTCPHead->CheckSumH		= 0;
	pTCPHead->CheckSumL		= 0;
	pTCPHead->UrgentPointH	= 0;
	pTCPHead->UrgentPointL	= 0;

	/* fill some of IPHead. it will be used to calculate TCPChecksum
	and as augument passed to IPlayer */
	pIPHead = (struct SIPHead DT_XDATA *)((BYTE DT_XDATA *)pTCPHead - IP_HEAD_MIN_LEN);
	pIPHead->IPDestH					= NetDword_H(pTCB->IPDest);
	pIPHead->IPDesth					= NetDword_h(pTCB->IPDest);
	pIPHead->IPDestl					= NetDword_l(pTCB->IPDest);
	pIPHead->IPDestL					= NetDword_L(pTCB->IPDest);
	pIPHead->IPScrH					= NetDword_H(pTCB->IPScr);
	pIPHead->IPScrh					= NetDword_h(pTCB->IPScr);
	pIPHead->IPScrl					= NetDword_l(pTCB->IPScr);
	pIPHead->IPScrL					= NetDword_L(pTCB->IPScr);
	pIPHead->Protocol				= IP_PROTOCOL_TCP;
	pIPHead->TotalLenH				= NetWord_H(htons(MemHead->pEnd -
		MemHead->pStart + TCP_HEAD_MIN_LEN + IP_HEAD_MIN_LEN));	/* pStart point to TCP data */
	pIPHead->TotalLenL				= NetWord_L(htons(MemHead->pEnd -
			MemHead->pStart + TCP_HEAD_MIN_LEN + IP_HEAD_MIN_LEN));
	pTCPHead->CheckSumH = NetWord_H(htons(TCPCheckSum(pIPHead,MemHead->pEnd - MemHead->pStart + TCP_HEAD_MIN_LEN)));
	pTCPHead->CheckSumL = NetWord_L(htons(TCPCheckSum(pIPHead,MemHead->pEnd - MemHead->pStart + TCP_HEAD_MIN_LEN)));

	/* send packet */
	MemHead->pStart = (BYTE DT_XDATA *)pIPHead;	/* dec pStart */
	IPOutput(MemHead);

	/*
	 * renew tcb
	 */
	/* no ack need. because this packet will give a ack to him */
	pTCB->bNeedAck = FALSE;

	pTCB->SeqMine += SeqInc;

	/* if this packet contant data or is a FIN or SYN packet
	we write it to unacked queue */
	if(SeqInc != 0)
	{
		/* if the unacked queue is empty, start timer for this packet */
		if(pTCB->QUnacked == NULL)
		{
			pTCB->RetranTimer = TCP_RETRAN_TIME_OUT;
			pTCB->RetranTimes = 0;
		}

		TCPInsertQ(&(pTCB->QUnacked),MemHead,htonl(MergeNetDword(pTCPHead->SeqH,pTCPHead->Seqh,pTCPHead->Seql,pTCPHead->SeqL)));
	}
	else
	{
		MemFree(MemHead);
	}
	return TRUE;
}

/* judge his wnd if he can receive this packet. send call TCPSendSeg.
only send this seg completely return TRUE*/
BOOL TCPSendSegJudgeWnd(struct STCB DT_XDATA *pTCB,struct SMemHead DT_XDATA *MemHead) REENTRANT_MUL
{
	struct SMemHead DT_XDATA *NewMemHead;

	/* if WndHis is large enough to receive this packet send it.
	otherwise create a new packet and send part of Data. the remain
	going to transmit when WndHis refresh at TCPInput */
	if(MemHead->pEnd - MemHead->pStart > pTCB->WndHis)
	{
		/* part of Data need send */
		if(pTCB->WndHis > 0)
		{
			/* create a new MemHead */
			if((NewMemHead = TCPAllocate(pTCB->WndHis)) == NULL)
				return FALSE;

			/* copy part of data to new MemHead */
			MyMemCopy(NewMemHead->pStart,MemHead->pStart,pTCB->WndHis);

			/* delete this part from old MemHead */
			MemHead->pStart += pTCB->WndHis;

			/* send the NewMemHead */
			TCPSendSeg(pTCB,NewMemHead,TCP_ACK);

			return FALSE;
		}
		else
		{
			/* can't send any data now */
			return FALSE;
		}
	}
	else
	{
		TCPSendSeg(pTCB,MemHead,TCP_ACK);
		return TRUE;
	}
}

/* send seg in unsend queue untill can't send any more. if send all
seg in queue return true */
BOOL TCPSendUnsendQ(struct STCB DT_XDATA *pTCB) REENTRANT_MUL
{
	/* send every packet in unsend queue if can */
	for(;pTCB->QUnSend != NULL;)
	{
		/* send it completely? */
		if(TCPSendSegJudgeWnd(pTCB,pTCB->QUnSend->MemHead) == TRUE)
		{
			/* delete it from unsend queue */
			TCPOutQ(&(pTCB->QUnSend));
		}
		else
		{
			/* only part of the seg is send */
			return FALSE;
		}
	}
	return TRUE;
}

/* call by TCPInput after judge this packet can be received.NOTE:MemHead-pStart point
to TCP head. TCPHead byte order is change in TCPInput */
void TCPRecvSeg(struct STCB DT_XDATA *pTCB,struct SMemHead DT_XDATA *MemHead) REENTRANT_SIG
{
	WORD TCPDataSize;
	struct STCB DT_XDATA *pNewTCB;
	struct SIPHead  DT_XDATA *pIPHead;
	struct STCPHead DT_XDATA *pTCPHead;

	pTCPHead = (struct STCPHead DT_XDATA *)(MemHead->pStart );
	pIPHead	 = (struct SIPHead  DT_XDATA *)(MemHead->pStart - IP_HEAD_MIN_LEN);

	/*
	 * begain renew tcb values
	 */

	/* dest windows size renew.*/
	pTCB->WndHis = MergeNetWord(pTCPHead->WndSizeH,pTCPHead->WndSizeL);

	/* after dest windows renew is it possible to send a packet in unsend queue now ?*/
	TCPSendUnsendQ(pTCB);

	/* His Sequence renew */
	TCPDataSize = ntohs(MergeNetWord(pIPHead->TotalLenH,pIPHead->TotalLenL)) - IP_HEAD_MIN_LEN
		- TCP_HEAD_LEN(pTCPHead);
	if((pTCPHead->flag & TCP_SYN)  || (pTCPHead->flag & TCP_FIN))
	{
		pTCB->SeqHis += TCPDataSize + 1;
	}
	else
	{
		pTCB->SeqHis += TCPDataSize;
	}

	/* NeedAck? */
	if(TCPDataSize != 0)
	{
		pTCB->bNeedAck = TRUE;
		pTCB->DelayAckTimer = TCP_DELAY_ACK_TIME_OUT;
	}

	/* if This packet acked packet in unacked queue */
	if((pTCPHead->flag & TCP_ACK) != 0)
	{
		while(pTCB->QUnacked != NULL &&
			TCP_SEQ_COMPARE(pTCB->QUnacked->Seq,MergeNetDword(pTCPHead->AckSeqH,pTCPHead->AckSeqh,pTCPHead->AckSeql,pTCPHead->AckSeqL)) < 0)
		{
			MemFree(pTCB->QUnacked->MemHead);
			TCPOutQ(&(pTCB->QUnacked));

			/* timer for retran restore */
			pTCB->RetranTimer = TCP_RETRAN_TIME_OUT;
			pTCB->RetranTimes = 0;
		}
	}

	/*
	 * deal refer to tcb.state and tcp flag
	 */
	switch(pTCB->TCPState)
	{
	case TCP_STATE_CLOSED:
		break;
	case TCP_STATE_LISTEN:
		/* syn: to TCP_STATE_SYNSENT, send syn+ack */
		if(pTCPHead->flag == TCP_SYN)
		{
			/* create a new tcb and it is going to deal with
			this connection. */
			if((pNewTCB = TCPSocket(htonl(pTCB->IPScr))) == NULL)
			{
				MemFree(MemHead);
				return;
			}

			/* insert this tcb to tcb list isn't need. because
			this tcb is already insert at TCPSocket()*/

			/* initial new tcb value*/
			pNewTCB->TCPState	= TCP_STATE_SYNRECVD;
			pNewTCB->IPDest		= MergeNetDword(pIPHead->IPScrH,pIPHead->IPScrh,pIPHead->IPScrl,pIPHead->IPScrL);
			pNewTCB->PortDest	= MergeNetWord(pTCPHead->PortScrH,pTCPHead->PortScrL);
			pNewTCB->PortScr	= MergeNetWord(pTCPHead->PortDestH,pTCPHead->PortDestL);

			pNewTCB->SeqHis = MergeNetDword(pTCPHead->SeqH,pTCPHead->Seqh,pTCPHead->Seql,pTCPHead->SeqL) + 1;	/* syn is use 1
													sequence */
			pNewTCB->WndHis =MergeNetWord(pTCPHead->WndSizeH,pTCPHead->WndSizeL);

			/* set accept function. when pNewTCB accept this
			connection call pTCB->accetp */
			pNewTCB->accept = pTCB->accept;

			/* send syn+ack */
			TCPSendSeg(pNewTCB,TCPAllocate(0),TCP_SYN | TCP_ACK);
		}
		break;
	case TCP_STATE_SYNRECVD:

		/* ack: to TCP_STATE_ESTABLISHED */
		if((pTCPHead->flag & TCP_ACK) != 0)
			pTCB->TCPState = TCP_STATE_ESTABLISHED;

		/* call accept. Let user to know and deal more */
		pTCB->accept(pTCB);

		break;
	case TCP_STATE_SYNSENT:
		switch(pTCPHead->flag)
		{
		case TCP_SYN:
			/* syn: to TCP_STATE_SYNRECVD send syn+ack */

			pTCB->TCPState = TCP_STATE_SYNRECVD;

			/* ackseq initial */
			pTCB->SeqHis = MergeNetDword(pTCPHead->SeqH,pTCPHead->Seqh,pTCPHead->Seql,pTCPHead->SeqL) + 1;	/* syn use 1 sequence */

			TCPSendSeg(pTCB,TCPAllocate(0),	TCP_SYN | TCP_ACK);
			break;
		case TCP_SYN | TCP_ACK:
			/* syn+ack: to TCP_STATE_ESTABLISHED send ack */

			pTCB->TCPState = TCP_STATE_ESTABLISHED;

			/* ackseq initial */
			pTCB->SeqHis = MergeNetDword(pTCPHead->SeqH,pTCPHead->Seqh,pTCPHead->Seql,pTCPHead->SeqL) + 1;	/* syn use 1 sequence */

			TCPSendSeg(pTCB,TCPAllocate(0),TCP_ACK);
			break;
		case TCP_RST | TCP_ACK:
			/* rst: to closed */
			pTCB->TCPState = TCP_STATE_CLOSED;
			TCPRelease(pTCB);
			break;
		}
		break;
	case TCP_STATE_ESTABLISHED:
		/* fin:to closewait send ack */
		if((pTCPHead->flag & TCP_FIN) != 0)
		{
			pTCB->TCPState = TCP_STATE_CLOSEWAIT;
			TCPSendSeg(pTCB,TCPAllocate(0),	TCP_ACK);

			/* call ->close() let user to know he want to close connection
			user should call TCPClose() to close the connection in ->close */
			pTCB->close(pTCB);
		}
		break;
	case TCP_STATE_CLOSEWAIT:
		/* he want to close, send a fin to close */
		pTCB->TCPState = TCP_STATE_LASTACK;
		pTCB->LastAckTimer = TCP_LASTACK_TIME_OUT;
		TCPSendSeg(pTCB,TCPAllocate(0),	TCP_FIN | TCP_ACK);
		break;
	case TCP_STATE_FINWAIT1:
		switch(pTCPHead->flag)
		{
		case TCP_FIN:
			/* fin: to TCP_STATE_CLOSING send ack */
			pTCB->TCPState = TCP_STATE_CLOSING;
			TCPSendSeg(pTCB,TCPAllocate(0),	TCP_ACK);
			break;
		case TCP_FIN | TCP_ACK:
			pTCB->TCPState = TCP_STATE_TIMEWAIT;
			pTCB->LastAckTimer = TCP_LASTACK_TIME_OUT;	/* start timer */
			TCPSendSeg(pTCB,TCPAllocate(0),	TCP_ACK);
			break;
		case TCP_ACK:
			pTCB->TCPState = TCP_STATE_FINWAIT2;
			break;
		}
		break;
	case TCP_STATE_CLOSING:
		/* ack:to TCP_STATE_CLOSED */
		if(pTCPHead->flag & TCP_ACK)
		{
			pTCB->TCPState = TCP_STATE_TIMEWAIT;
			pTCB->LastAckTimer = TCP_LASTACK_TIME_OUT;	/* start timer */
		}
		break;
	case TCP_STATE_FINWAIT2:
		if(pTCPHead->flag & TCP_FIN)
		{
			pTCB->TCPState = TCP_STATE_TIMEWAIT;
			pTCB->LastAckTimer = TCP_LASTACK_TIME_OUT;	/* start timer */
			TCPSendSeg(pTCB,TCPAllocate(0),	TCP_ACK);
		}
		break;
	}

	/*
	 * put tcp data to uper layer
	 */
	if(TCPDataSize != 0)
	{
		pTCB->recv(MemHead->pStart + TCP_HEAD_LEN(pTCPHead),TCPDataSize);
	}

	/*
	 * free this packet
	 */
	MemFree(MemHead);
}

/* tcp packet in.*/
void TCPInput(struct SMemHead DT_XDATA *MemHead) REENTRANT_SIG
{
	struct SIPHead  DT_XDATA *pIPHead;
	struct STCPHead DT_XDATA *pTCPHead;
	struct STCB		DT_XDATA *pNewTCB;
	struct STCB	DT_XDATA *pTCB;

	pTCPHead = (struct STCPHead DT_XDATA *)(MemHead->pStart);
	pIPHead	 = (struct SIPHead  DT_XDATA *)(MemHead->pStart - sizeof(struct SIPHead));

	/*
	 * is check sum ok?
	 */
	if(TCPCheckSum(pIPHead,ntohs(MergeNetWord(pIPHead->TotalLenH,pIPHead->TotalLenL)) - IP_HEAD_MIN_LEN) != 0)
	{
		MemFree(MemHead);
		return;
	}

	/*
	 * is there a connection can accept this tcp packet?
	 */

	/* if is syn packet. a tcb in listen can accept it. */
	if(pTCPHead->flag == TCP_SYN)
	{
		for(pTCB = TCBList; pTCB != NULL; pTCB = pTCB->pNext)
		{
			if(pTCB->TCPState == TCP_STATE_LISTEN &&
				pTCB->PortScr == MergeNetWord(pTCPHead->PortDestH,pTCPHead->PortDestL))
			{
				break;
			}
		}
	}
	else
	{
		/* search active connections. TCBState must not the
		closed and listen state */
		for(pTCB = TCBList; pTCB != NULL; pTCB = pTCB->pNext)
		{
			/* and the source ip, dest ip, source port, dest port
				must equal */
			if(pTCB->PortScr == MergeNetWord(pTCPHead->PortDestH,pTCPHead->PortDestL) &&
				pTCB->PortDest == MergeNetWord(pTCPHead->PortScrH,pTCPHead->PortScrL) &&
				pTCB->TCPState != TCP_STATE_LISTEN &&
				pTCB->TCPState != TCP_STATE_CLOSED &&
				pTCB->IPScr  == MergeNetDword(pIPHead->IPDestH,pIPHead->IPDesth,pIPHead->IPDestl,pIPHead->IPDestL) &&
				pTCB->IPDest == MergeNetDword(pIPHead->IPScrH,pIPHead->IPScrh,pIPHead->IPScrl,pIPHead->IPScrL))
				break;

		}
	}

	/* can't find, and send a rst */
	if(pTCB == NULL)
	{
		/* allocate a temp tcb for use */
		pNewTCB = TCPSocket(ntohl(MergeNetDword(pIPHead->IPDestH,pIPHead->IPDesth,pIPHead->IPDestl,pIPHead->IPDestL)));
		pNewTCB->IPDest 	= MergeNetDword(pIPHead->IPScrH,pIPHead->IPScrh,pIPHead->IPScrl,pIPHead->IPScrL);
		pNewTCB->PortDest 	= MergeNetWord(pTCPHead->PortScrH,pTCPHead->PortScrL);
		pNewTCB->PortScr	= MergeNetWord(pTCPHead->PortDestH,pTCPHead->PortDestL);

		/* set MemFree DataSize to 0 */
		MemHead->pStart = MemHead->pEnd;

		TCPSendSeg(pNewTCB,TCPAllocate(0),TCP_ACK | TCP_RST);

		MemFree(MemHead);
		TCPAbort(pNewTCB);
		return;
	}

	/*
	 *  is it a expected packet?
	 */
	/* first change all necessary part to host order */
 #ifndef HOST_ORDER_AS_NET
	BYTE Backup[4];
	Backup[0] = pTCPHead->AckSeqL;
	Backup[1] = pTCPHead->AckSeql;
	Backup[2] = pTCPHead->AckSeqh;
	Backup[3] = pTCPHead->AckSeqH;
	pTCPHead->AckSeqH	= Backup[0];
	pTCPHead->AckSeqh	= Backup[1];
	pTCPHead->AckSeql	= Backup[2];
	pTCPHead->AckSeqL	= Backup[3];
	Backup[0] = pTCPHead->SeqL;
	Backup[1] = pTCPHead->Seql;
	Backup[2] = pTCPHead->Seqh;
	Backup[3] = pTCPHead->SeqH;
	pTCPHead->SeqH		= Backup[0];
	pTCPHead->Seqh		= Backup[1];
	pTCPHead->Seql		= Backup[2];
	pTCPHead->SeqL		= Backup[3];
	Backup[0] = pTCPHead->WndSizeH;
	pTCPHead->WndSizeH	= pTCPHead->WndSizeL;
	pTCPHead->WndSizeL = Backup[0];
#endif

	/* if it is the first packet from him, don't check sequence.
	   in connection case: a syn or syn+ack packet. in listen case
	   : a syn packet. so pass all packet contain syn flag */
	if((pTCPHead->flag & TCP_SYN) == 0)
	{
		/* sequence ok? */
		if(pTCB->SeqHis != MergeNetDword(pTCPHead->SeqH,pTCPHead->Seqh,pTCPHead->Seql,pTCPHead->SeqL))
		{
			/* if this a packet fall within rev window */
			if(TCP_SEQ_COMPARE(MergeNetDword(pTCPHead->SeqH,pTCPHead->Seqh,pTCPHead->Seql,pTCPHead->SeqL),pTCB->SeqHis) > 0
				&& TCP_SEQ_COMPARE(MergeNetDword(pTCPHead->SeqH,pTCPHead->Seqh,pTCPHead->Seql,pTCPHead->SeqL),pTCB->SeqHis) < pTCB->WndMine)
			{
				/* write it to QExceedSeq for late receive */
				TCPInsertQ(&(pTCB->QExceedSeq),MemHead,MergeNetDword(pTCPHead->SeqH,pTCPHead->Seqh,pTCPHead->Seql,pTCPHead->SeqL));
				return;
			}
			else
			{
				/* packet fall outof window, drop it. and send a ack back, because
				this is probably ocurr when our pre send ack is lose.*/
				MemFree(MemHead);
				TCPSendSeg(pTCB,TCPAllocate(0),TCP_ACK);
				return;
			}
		}/* else sequence equal. ok */
	}/* else is syn packet */

	/* deal incoming packet */
	TCPRecvSeg(pTCB,MemHead);

	/* if seg in ExceedSeq can receive now? */
	while(pTCB->QExceedSeq != NULL &&
		pTCB->SeqHis == pTCB->QExceedSeq->Seq)
	{
		TCPRecvSeg(pTCB,pTCB->QExceedSeq->MemHead);
		TCPOutQ(&(pTCB->QExceedSeq));
	}
}

BOOL TCPSendEx(struct STCB DT_XDATA * pTCB,struct SMemHead DT_XDATA *MemHead) REENTRANT_MUL
{
	/* if state is "closed, listen, syn recvd, syn sent",
	need connected first */
	if(pTCB->TCPState <= TCP_STATE_SYNSENT)
	{
		MemFree(MemHead);
		return FALSE;
	}

	/* if unsend queue is empty */
	if(pTCB->QUnSend == NULL)
	{
		/* if this packet send completely? */
		if(TCPSendSegJudgeWnd(pTCB,MemHead) == FALSE)
		{
			/* insert the remain for later sending */
			return TCPInsertQ(&(pTCB->QUnSend),MemHead,0);
		}
		else
			return TRUE;
	}
	else
		return TCPInsertQ(&(pTCB->QUnSend),MemHead,0);
}

BOOL TCPSend(struct STCB DT_XDATA * pTCB,void DT_XDATA *buf,WORD DataSize) REENTRANT_MUL
{
	struct SMemHead DT_XDATA *MemHead;

	/* allocate */
	if((MemHead = TCPAllocate(DataSize)) == NULL)
		return FALSE;

	/* copy */
	MyMemCopy(MemHead->pStart,buf,DataSize);

	return TCPSendEx(pTCB,MemHead);
}

BOOL TCPConnect(struct STCB DT_XDATA * pTCB, DWORD DestIP, WORD DestPort,WORD LocalPort,
						void (DT_CODE * recv)(void DT_XDATA * buf,WORD size) REENTRANT_MUL,
						void (DT_CODE * close)(struct STCB DT_XDATA * pSocket) REENTRANT_MUL) REENTRANT_SIG
{
	/* is it in closed state? */
	if(pTCB->TCPState != TCP_STATE_CLOSED)
		return FALSE;

	/* tcb renew */
	pTCB->IPDest	= htonl(DestIP);
	pTCB->PortDest  = htons(DestPort);
	pTCB->PortScr = htons(LocalPort);
	pTCB->recv	 = recv;
	pTCB->close	 = close;

	/* send syn */
	if(TCPSendSeg(pTCB,TCPAllocate(0),TCP_SYN) == TRUE)
	{
		pTCB->TCPState = TCP_STATE_SYNSENT;

		/* wait for establish */
		while(TRUE)
		{
			switch(pTCB->TCPState)
			{
			case TCP_STATE_ESTABLISHED:
				return TRUE;
			case TCP_STATE_CLOSED:
				/* 1. if receive a rst packet from him
				   2. retransmittimes exceed sreshold */
				return FALSE;
			}
		}
	}
	else
		return FALSE;
}
/* call this func to innitiate closing a connection. connection
will not close unless peer send a fin also.*/
void TCPClose(struct STCB DT_XDATA *pTCB) REENTRANT_MUL
{
	if(pTCB->TCPState != TCP_STATE_CLOSED)
	{
		switch(pTCB->TCPState)
		{
		case TCP_STATE_LISTEN:
			/* close right now */
			pTCB->TCPState = TCP_STATE_CLOSED;
			break;
		case TCP_STATE_SYNRECVD:
			/* close when peer send a fin */
			pTCB->TCPState = TCP_STATE_FINWAIT1;
			TCPSendSeg(pTCB,TCPAllocate(0),TCP_FIN | TCP_ACK);
			break;
		case TCP_STATE_SYNSENT:
			/* close right now */
			pTCB->TCPState = TCP_STATE_CLOSED;
			TCPRelease(pTCB);
			break;
		case TCP_STATE_ESTABLISHED:
			/* close when peer send a fin */
			pTCB->TCPState = TCP_STATE_FINWAIT1;
			TCPSendSeg(pTCB,TCPAllocate(0),TCP_FIN | TCP_ACK);
			break;
		case TCP_STATE_CLOSEWAIT:
			/* close when lastack time out */
			pTCB->TCPState = TCP_STATE_LASTACK;
			pTCB->LastAckTimer = TCP_LASTACK_TIME_OUT;
			TCPSendSeg(pTCB,TCPAllocate(0),TCP_FIN | TCP_ACK);
			break;
		}
	}
}

BOOL TCPListen(struct STCB DT_XDATA *pTCB,WORD ScrPort,
			   void (DT_CODE * accept)(struct STCB DT_XDATA *pNewTCB) REENTRANT_MUL) REENTRANT_MUL
{
	struct STCB DT_XDATA *pTCBt;

	/* is it in closed state? */
	if(pTCB->TCPState != TCP_STATE_CLOSED)
		return FALSE;

	ScrPort = htons(ScrPort);

	/* is there any other socket already listen in this port? */
	for(pTCBt = TCBList; pTCBt != NULL; pTCBt = pTCBt->pNext)
	{
		if(pTCBt->PortScr == ScrPort)
			return FALSE;
	}

	/* renew tcb */
	pTCB->PortScr = ScrPort;
	pTCB->TCPState = TCP_STATE_LISTEN;
	pTCB->accept   = accept;

	return TRUE;
}

struct STCB DT_XDATA * TCPSocket(IP_ADDR ScrIP) REENTRANT_SIG
{
	struct STCB DT_XDATA * pTCB;
	struct STCB DT_XDATA * pTCBt;
	WORD MaxScrPort;	/* max port number in use */

	/* get a tcb */
	if((pTCB = TCPGetTCB()) == NULL)
	{
		return NULL;
	}

	/* allocate a scrport. that is number of
	the highest	number of port in use add 1 */
	for(pTCBt = TCBList,MaxScrPort = TCP_DEFAULT_PORT;
		pTCBt != NULL; pTCBt = pTCBt->pNext)
	{
		if(ntohs(pTCBt->PortScr) > MaxScrPort)
			MaxScrPort = ntohs(pTCBt->PortScr);
	}
	pTCB->PortScr = htons((WORD)(MaxScrPort + 1));

	/* other tcb set */
	pTCB->TCPState	= TCP_STATE_CLOSED;
	pTCB->IPScr		= htonl(ScrIP);
	pTCB->WndMine	= MemFreeSize();
	pTCB->bNeedAck	= FALSE;
	pTCB->QExceedSeq	= NULL;
	pTCB->QUnacked		= NULL;
	pTCB->QUnSend		= NULL;

	/* Insert int tcb */
	TCPInsertTCB(pTCB);

	return pTCB;
}

/* reclaim TCB */
void TCPAbort(struct STCB DT_XDATA *pTCB) REENTRANT_SIG
{
	struct STCB DT_XDATA *pTCBt;

	TCPRelease(pTCB);

	/*
	 * search through the tcb list and delete it from list
	 */
	/* if it is the hear of the list */
	if(TCBList == pTCB)
	{
		TCBList = TCBList->pNext;
	}
	else
	{
		/* else search start from the second one */
		for(pTCBt = TCBList; pTCBt != NULL; pTCBt = pTCBt->pNext)
		{
			if(pTCBt->pNext == pTCB)
			{
				pTCBt->pNext = pTCB->pNext;
				break;
			}
		}
	}

	/* reclaim it. link it to TCBFreelist */
	pTCB->pNext = TCBFreeList;
	TCBFreeList = pTCB;
}
/* this func is called by user to allocate a packet and pstart
point to TCPplayload */
struct SMemHead DT_XDATA *TCPAllocate(WORD size) REENTRANT_SIG
{
	struct SMemHead DT_XDATA * MemHead;
	if((MemHead = MemAllocate((WORD)(ALL_HEAD_SIZE + size))) == NULL)
		return NULL;
	else
	{
		/* point to the tcp data */
		MemHead->pStart += ALL_HEAD_SIZE;
		return MemHead;
	}
}

