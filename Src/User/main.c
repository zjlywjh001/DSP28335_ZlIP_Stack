/*
 * main.c
 */

#include "DSP2833x_Device.h"     // DSP2833x Headerfile Include File
#include "DSP2833x_Examples.h"   // DSP2833x Examples Include File
#include "GlobalDef.h"
#include "TCPIPmem.h"
#include "RTL8019.h"
#include "IP.h"
#include "etherif.h"
#include "ARP.h"
#include "Netif.h"
#include "TCP.h"

#define	  LedReg	(*((volatile  Uint16 *)0x41FF))

void OnReceive(void DT_XDATA * buf,WORD size)  REENTRANT_MUL;
void OnAcceptRecv(void DT_XDATA *buf,WORD size)  REENTRANT_MUL;
void OnAccept(socket DT_XDATA *pNewTCB)  REENTRANT_MUL;
void OnClose(socket DT_XDATA * pSocket)  REENTRANT_MUL;
void Timer()  REENTRANT_MUL;

interrupt void ISRTimer0(void);

#define DATA_SIZE 	0x500
BYTE DT_XDATA DataBlock[DATA_SIZE];
BYTE DT_XDATA str[]="Hello World.\n";
socket		DT_XDATA * DT_XDATA ExConn;
socket		DT_XDATA * DT_XDATA	ExAccept;
socket		DT_XDATA * DT_XDATA	ExListen;

BYTE recvflag = FALSE;
DWORD datasize = 0;

void OnReceive(void DT_XDATA * buf,WORD size)  REENTRANT_MUL   //CallBack Function On Receiving Data
{
	recvflag  = TRUE;
	MyMemCopy(DataBlock,buf,size);
	datasize = size;
}

void OnAcceptRecv(void DT_XDATA *buf,WORD size) REENTRANT_MUL  //CallBack Function On Server Receiving Data Using On Listen Mode Only
{
	// Do what you want when connection accepted
}

void OnAccept(socket DT_XDATA *pNewSocket) REENTRANT_MUL   //CallBack Function On Server Accepting a connection.
{
	ExAccept = pNewSocket;
	pNewSocket->recv = OnAcceptRecv;
	pNewSocket->close = OnClose;
}

void OnClose(socket DT_XDATA * pSocket) REENTRANT_MUL
{
	TCPClose(pSocket);	/* we close too */
}


int main(void) {
	
 	InitSysCtrl();
	InitXintf16Gpio();
	DINT;
	InitPieCtrl();

	IER = 0x0000;
	IFR = 0x0000;

 	InitPieVectTable();

 	//MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);  //use when run in flash
 	//InitFlash();

 	EALLOW;  // This is needed to write to EALLOW protected registers
	PieVectTable.TINT0 = &ISRTimer0;
	EDIS;    // This is needed to disable write to EALLOW protected registers
	InitCpuTimers();   // For this example, only initialize the Cpu Timers
	ConfigCpuTimer(&CpuTimer0, 150, 25000);

	IER |= M_INT1;

	PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

	EINT;   // Enable Global interrupt INTM
	ERTM;   // Enable Global realtime interrupt DBGM

	LedReg = 0xFF;

 	//struct SMemHead DT_XDATA *MemHead;
	struct SEtherDevice DT_XDATA DevRTL;
	BYTE	DT_XDATA EtherAddr[ETHER_ADDR_LEN] = {0x52,0x54,0x4c,0x30,0x2e,0x2f};  //MAC Addr
	IP_ADDR		IPAddr	= 0xC0A80B0B;	/* 192.168.11.11	*//* ca71e590:202.113.229.144 *//* ca711075:202.113.16.117 */
	IP_ADDR		NetMask	= 0xffffff00;	/* 255.255.255.0 */
	IP_ADDR		GateWay	= 0xC0A80B01;	/* 192.168.11.1  */

	NetIfInit();
	ARPInit();
	TCPInit();
	MemInit();
	RTLInit(EtherAddr);
	//RTLSendPacketTest();
	StartCpuTimer0();
	/* init Devcie struct and init this device */
	EtherDevInit(&DevRTL,EtherAddr,RTLSendPacket,RTLReceivePacket);


	/* add this device to NetIf */
	NetIfAdd(IPAddr,NetMask,GateWay,EtherInput,EtherOutput,&DevRTL);


	//Server Example
/*
 * 	ExListen = TCPSocket(IPAddr);
	ExAccept = NULL;
	TCPListen(ExListen,TCP_DEFAULT_PORT,OnAccept);

*/


	//Client Example
	ExConn = TCPSocket(IPAddr);
	if(TCPConnect(ExConn,0xC0A80B28,1001,5000,OnReceive,OnClose) != TRUE) /*  Remote IP 0xC0A80B28 - 192.168.11.40 remote Port 1001 Open LocalPort 5000*/
	{
		TCPAbort(ExConn); //Connect Failed.
	}

	while (ExConn->TCPState!=TCP_STATE_ESTABLISHED) ; //wait for connection established

	recvflag = FALSE;
	datasize = 0;
	while (1)
	{
		if (ExConn->TCPState==TCP_STATE_ESTABLISHED)
		{
			//TCPSend(ExConn,str,sizeof(str)); //send something
			//DELAY_US(500000);
			//DELAY_US(500000);  //Send data every second.
			if (recvflag == TRUE)
			{
				TCPSend(ExConn,DataBlock,datasize);
				recvflag = FALSE;
			}
		}
	}

	TCPClose(ExConn); // Close connection

	return 0;
}

interrupt void ISRTimer0(void)
{
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
    CpuTimer0Regs.TCR.bit.TIF=1;
    CpuTimer0Regs.TCR.bit.TRB=1;

    NetIfTimer();
    ARPTimer();
    TCPTimer();

}
