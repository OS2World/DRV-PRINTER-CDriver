/****************************************************************************

	OS/2 Device Driver module for IKON 10092 Plotter Adaptors.

	The entry point DriverStrategy() is called from the BIKONA.ASM 
	module with the address of the Driver Request Packet for further 
	processing.

	void _near _fastcall DriverStrategy( DRV_REQ     *pDrvReq	);

	Note:

	******************************************************************
	All statics should be initialized so as to force them into the
	_DATA segment, rather than the CONST or _BSS segments.  Data
	values that end up in the CONST or _BSS segments will be discarded
	after 'init' time.

	Any data defined after the L_LastData variable will be discarded
	at Init time.  If referenced during later Strategy or Interrupt
	calls, the system will crash with a protection violation.
	******************************************************************

	History:
	01-08-91 ras - SCR P0487, initial entry
	01-09-91 ras - SCR P0487, continue
	01-10-91 ras - SCR P0487, continue
	01-11-91 ras - SCR P0487, add text mode processing
	01-12-91 ras - SCR P0487, add move mode processing

		Copyright 1991, Byers Engineering Company
****************************************************************************/
#define INCL_DOS
#include <os2.h>				// OS/2   defines
#include <conio.h>				// I/O    defines
#include <memory.h>				// Memory defines
#include <stdlib.h>				// Standard Lib defines
#include <dos.h>				// DOS    defines
#include "cdriver.h"				// OS/2 C defines
#include "bikon.h"				// IKON   defines
#include "version.h"				// Version info

// verify compiled using LARGE model
#ifndef	M_I86LM
#error *** MUST COMPILE IN LARGE MODEL ONLY
#endif

// specify compilation options
#pragma	intrinsic( 	inp, outp, memcpy 	)
#pragma intrinsic(	_enable, _disable	)
#pragma intrinsic(	abs			)
#pragma	check_stack(	off			)

// declare functions for 'alloc_text'
       void  _near _fastcall DriverStrategy(	 DRV_REQ    *pDrvReq	);
static void  _near 	     L_ReqInit(		 DRV_REQ    *pDrvReq	);
static void  _near 	     L_ReqClose(	 DRV_REQ    *pDrvReq	);
static void  _near 	     L_InitAdaptor(	 IK_REQ     *pIkReq	);
static void  _near 	     L_WriteRaster(	 IK_REQ     *pIkReq	);
static void  _near 	     L_WriteText(	 IK_REQ     *pIkReq	);
static void  _near 	     L_WriteMove(	 IK_REQ     *pIkReq	);
static void  _near 	     L_WriteControl(	 IK_REQ     *pIkReq	);
static int   _near	     L_WaitReady(	 uint32	     qTimeout	);
static void  _far	     L_InterruptService( void			);
static void  _near	     L_StartControl(	 void			);
static void  _near	     L_StartDma(	 int	     nRasterFlag);
static void  _near 	     L_LastCode(	 void 			);

// force allocation of functions to _TEXT
#pragma	alloc_text( _TEXT, DriverStrategy	)
#pragma alloc_text( _TEXT, L_ReqInit		)
#pragma alloc_text( _TEXT, L_ReqClose		)
#pragma alloc_text( _TEXT, L_InitAdaptor	)
#pragma alloc_text( _TEXT, L_WriteRaster	)
#pragma alloc_text( _TEXT, L_WriteText		)
#pragma alloc_text( _TEXT, L_WriteMove		)
#pragma alloc_text( _TEXT, L_WriteControl	)
#pragma alloc_text( _TEXT, L_WaitReady		)
#pragma alloc_text( _TEXT, L_InterruptService	)
#pragma alloc_text( _TEXT, L_StartControl	)
#pragma alloc_text( _TEXT, L_StartDma		)
#pragma alloc_text( _TEXT, L_LastCode		)

// force allocation of init functions to _INITCODE (discardable after init)
#pragma alloc_text( _INITCODE, L_ReqInit	)
#pragma alloc_text( _INITCODE, L_ParseArgs	)
#pragma alloc_text( _INITCODE, L_Emit		)
#pragma alloc_text( _INITCODE, L_AddNum		)
#pragma alloc_text( _INITCODE, L_AddStr		)
#pragma alloc_text( _INITCODE, L_Slen		)
#pragma alloc_text( _INITCODE, L_Upc		)
#pragma alloc_text( _INITCODE, L_WaitKey	)

// declare init functions
static void  _near 	     L_ReqInit(		 DRV_REQ    *pDrvReq	);
static void  _near 	     L_ParseArgs(	 const char FAR *p	);
static void  _near 	     L_Emit(		 const char *msg	);
static int   _near 	     L_Slen(		 const char *s	);
static int   _near 	     L_Upc(		 int		c	);
static void  _near 	     L_AddNum(	 unsigned value, int hex );
static void  _near 	     L_AddStr(	 const char *s	);
static void  _near 	     L_WaitKey(	 void		);

// boot message prefix
#define PREFIX  "BYIKON: "

// options on the DEVICE= line
static int fVerbose, fQuiet, fWait, fBeep;

// static work areas: the driver runs with SS != DS, so nothing that a DOS
// call sees may live on the stack
static USHORT done, got;
static char   key;
static char   line[96];
static char   text[160];

// define Interrupt service modes, used by L_InterruptService()
// to determine next action to perform.  these values are
// used as a mask to determine which threads the interrupt
// routine should resume
#define	I_IDLE		0
#define	I_READY		0x0001
#define	I_RASTER	0x0002
#define	I_TEXT		0x0004
#define	I_MOVE		0x0008
#define	I_CONTROL	0x0010

//********************************************
// define local statics kept as part of driver
//********************************************
static	char		L_Id1[] = "BYERS IKON";	// id to find with Ramscope
static	uint32		L_qBufPhyAddr	   = 0; // Physical buffer address
static	uint16		L_nBufPhyOff	   = 0; //		   offset
static	uint16		L_nBufLineCnt	   = 0; // Raster lines left to write
static	uint16		L_nBufBytesPerLine = 0; //	  bytes per line
static	uint16		L_nControlFlags	   = 0; // Control flag values
static	int		L_nIntMode    = I_IDLE;	// Interrupts idle
static	int		L_nIntSimulate 	   = 0;	// non-zero for internal int
static	REG_STACK_USAGE(L_RegStack,   0,	// def interrupt stack usage
				    256,
				      0,
				      0,
				      0,
				      0 );
static	uint32		L_qIntServAddr = (uint32) &L_InterruptService;


//*****************************
// Define IKON Adaptor literals
//*****************************

//;define IKON I/O Port addresses
#define IO_BASE_ADDR		0x0300		//;base port address
//; output port offsets
#define IO_OUT_SETUP_OFF	0		//;write setup control
#define IO_OUT_REMOTE_OFF	1		//;write remote functions
#define IO_OUT_DATA_OFF		2		//;write programmed data
//; input port offsets
#define IO_IN_SETUP_OFF		0		//;read setup status
#define IO_IN_ISTAT_OFF		1		//;read interface status
#define IO_IN_DIAG_OFF		2		//;read diagnostic data
#define IO_IN_DSTAT_OFF		3		//;read device status
#define IO_IN_STRAP_OFF		4		//;read interface strap options

//;define Setup Register IN/OUT mask bits
#define IO_SETUP_HI_TEST_ENABLE	0x80		//;hi to enable internal test
#define IO_SETUP_HI_TEST_STROBE	0x40		//;hi to strobe test ready
#define IO_SETUP_HI_LATCH_RESET	0x20		//;hi to force device reset
#define IO_SETUP_HI_DMA_ENABLE	0x10		//;hi to start DMA requests
#define IO_SETUP_LO_DMA_DISABLE	0x00		//;lo to disable DMA
#define IO_SETUP_HI_STREAM_MODE	0x08		//;hi to enable stream mode
#define IO_SETUP_HI_INT_ENABLE	0x04		//;hi to enable interrupts
#define IO_SETUP_HI_SPP_MODE	0x02		//;hi for Versatec SPP mode
#define IO_SETUP_HI_PLOT_MODE	0x01		//;hi for Versatec Plot mode
#define IO_SETUP_LO_PRINT_MODE	0x00		//;lo for Versatec Print mode

//;define Remote Register mask bits
#define IO_REMOTE_HI_SOFT_ACK	0x80		//;hi to simulate device ack
#define IO_REMOTE_HI_RESET_ADP	0x40		//;hi to reset adaptor
#define IO_REMOTE_HI_RESET_INT	0x20		//;hi to reset interrupt flag
#define IO_REMOTE_HI_RESET_DEV	0x10		//;hi to reset device
#define IO_REMOTE_HI_CLEAR	0x08		//;hi to set Versa CLEAR-
#define IO_REMOTE_HI_FF		0x04		//;hi to set Versa FORM FEED-
#define IO_REMOTE_HI_EOT	0x02		//;hi to set Versa EOT-
#define IO_REMOTE_HI_TERM	0x01		//;hi to set Versa LINE TERM-

//;define Interface Status Register mask bits
#define IO_ISTAT_HI_READY	0x80		//;hi when adaptor,device ready
#define IO_ISTAT_HI_READY_DEV	0x40		//;hi when device ready
#define IO_ISTAT_HI_INTERRUPT	0x20		//;hi when interrupting
#define IO_ISTAT_LO_WORD	0x10		//;lo when 16 bit mode
#define IO_ISTAT_LO_SWAP	0x08		//;lo when byte swap
#define IO_ISTAT_BIT_TSEL	0x04		//;test pattern switch 1 status
#define IO_ISTAT_BIT_FPLT	0x02		//;test pattern switch 2 status
#define IO_ISTAT_LO_TEST	0x01		//;lo when adaptor testing

//;define Input Device Status Register mask bits
#define IO_DSTAT_LO_TTL		0x80		//;lo when TTL mode
#define IO_DSTAT_LO_DIF		0x40		//;lo when Differential mode
#define IO_DSTAT_LO_CENT	0x20		//;lo when Centronics mode
#define IO_DSTAT_HI_VRDY	0x10		//;hi when Versatec ready
#define IO_DSTAT_HI_CBSY	0x08		//;hi when Centronics busy
#define IO_DSTAT_HI_NOPAP	0x04		//;hi when no paper
#define IO_DSTAT_HI_ONLIN	0x02		//;hi when device on-line
#define IO_DSTAT_HI_FAULT	0x01		//;hi when Centronics fault

//;define Strap Status Register mask bits
#define IO_STRAP_LO_PULLDOWN	0x80		//;lo-pulldown,hi-pullup
#define IO_STRAP_DMA_MASK	0x70		//;DMA bits mask
#define IO_STRAP_DMA_SHIFT	4		//;DMA bits shift to 0
#define IO_STRAP_INT_MASK	0x0f		//;INT bits mask
#define IO_STRAP_INT_SHIFT	0		//;INT bits shift to 0

//**************************************************************
// define IKON I/O Port addresses (updated during IK_INIT_SETUP)
//**************************************************************

//;IKON Register addresses
static	uint16	L_nIoBase	    =	IO_BASE_ADDR;
static	uint16	L_nIoOutSetupAddr   =	IO_BASE_ADDR+IO_OUT_SETUP_OFF;
static	uint16	L_nIoOutRemoteAddr  =	IO_BASE_ADDR+IO_OUT_REMOTE_OFF;
static	uint16	L_nIoOutDataAddr    =	IO_BASE_ADDR+IO_OUT_DATA_OFF;
static	uint16	L_nIoInSetupAddr    =	IO_BASE_ADDR+IO_IN_SETUP_OFF;
static	uint16	L_nIoInIstatAddr    =	IO_BASE_ADDR+IO_IN_ISTAT_OFF;
static	uint16	L_nIoInDiagAddr     =	IO_BASE_ADDR+IO_IN_DIAG_OFF;
static	uint16	L_nIoInDstatAddr    =	IO_BASE_ADDR+IO_IN_DSTAT_OFF;
static	uint16	L_nIoInStrapAddr    =	IO_BASE_ADDR+IO_IN_STRAP_OFF;


//**************************************************************
// define IKON Interrupt controls (updated during IK_INIT_SETUP)
//**************************************************************
static	int		L_nIntNum	= 15;	// adaptor interrupt number
static	int		L_nIntInUse	=  0;	// non-zero when int in use

//***************************************************************
// define IKON DMA access controls (updated during IK_INIT_SETUP)
//***************************************************************

typedef	struct				// DMA control structure
  {	uint16		nAdrStatus;		//status      reg address
	uint16		nAdrMask;		//mask 	      reg address
	uint16		nAdrMode;		//mode        reg address
	uint16		nAdrReset;		//reset count reg address
	uint16		nAdrPage;		//page        reg address
	uint16		nAdrOff;		//offset      reg address
	uint16		nAdrCnt;		//count       reg address
	uint16		nValStatus;		//status done value
	uint16		nValMode;		//mode        value (read mem)
	uint16		nValSet;		//set   mask  value (disable)
	uint16		nValReset;		//reset mask  value (enable)
  } DMA_DEF;  

// define entry for current DMA channel values (updated during IK_INIT_SETUP)
static	uint16		L_nDmaNum	= 7;	// current DMA channel number
static	DMA_DEF		L_Dma			// current DMA controls
			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// define table of all DMA channel values
static	DMA_DEF		L_DmaVals[]		// table of all DMA controls
			= { { 0x08,0x0a,0x0b,0x0c,0x87,0x00,	// channel 0
			      0x01,0x01,0x08,0x04,0x00 },
			    { 0x08,0x0a,0x0b,0x0c,0x83,0x02,	//	   1
			      0x03,0x02,0x09,0x05,0x01 },
			    { 0x08,0x0a,0x0b,0x0c,0x81,0x04,	//	   2
			      0x05,0x04,0x0a,0x06,0x02 },
			    { 0x08,0x0a,0x0b,0x0c,0x82,0x06,	//	   3
			      0x07,0x08,0x0b,0x07,0x03 },
			    { 0xd0,0xd4,0xd6,0xd8,0x8f,0xc0,	//	   4
			      0xc2,0x01,0x08,0x04,0x00 },
			    { 0xd0,0xd4,0xd6,0xd8,0x8b,0xc4,	//	   5
			      0xc6,0x02,0x09,0x05,0x01 },
			    { 0xd0,0xd4,0xd6,0xd8,0x89,0xc8,	//	   6
			      0xca,0x04,0x0a,0x06,0x02 },
			    { 0xd0,0xd4,0xd6,0xd8,0x8a,0xcc,	//	   7
			      0xce,0x08,0x0b,0x07,0x03 },
			  };

//***************************************************
// define local statics discarded after initilization
//***************************************************
static	char	L_LastData      = 0;		// last dataseg loc to save
static	char	L_acMsg[] 	= "\r\nByers OS/2 IKON 10092(7) Driver"
				  "\r\nVersion 0.0.1";

/****************************************************************************

	void _near _fastcall DriverStrategy( DRV_REQ     *pDrvReq	)

	This function is the primary entry point for device driver services.
	It is called from the .ASM entry point module with the address of
	the Driver Request packet.  It should perform whatever services
	are required by the packet, set the packet status and return.

	Called with:
		pDrvReq		ptr to Driver Request packet

	Returns:
		none

	Note:

	This module should reside in the primary code segment _TEXT
	declared by the .ASM stub.  All driver functions can be
	accomplished within this function, or by calling other
	functions also declared to exist within the _TEXT segment
	by the above 'alloc_text' pragma.

	If some driver functions are to be accomplished by calling other
	functions, they can be allocated into 'high' memory segments by
	not including them in the 'alloc_text' pragma.  The .DEF file
	should declare IOPL for any such segments that are to be
	preserved after 'init' time.  Note that 'high' memory functions
	can not be called in 'real' mode, only in 'protected' mode.

	Segments loaded into 'high' memory are swappable unless locked
	by the 'init' processing.
	
		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
void _near _fastcall	DriverStrategy(	DRV_REQ     *pDrvReq	)

  {	// define automatics
	IK_REQ	       *pIkReq;			// IKON Request ptr

	// clear status word for default 'no error' indication
	pDrvReq->nStatus = 0;

	// process by type of request packet
	switch ( pDrvReq->bCmd )
	{
      				//********************************
	  case DREQ_INIT:	// CONFIG.SYS Initialize Driver
      				//********************************
	    // call Init routine
	    L_ReqInit( pDrvReq );

	    // stop case
	    break;

      				//********************************
	  case DREQ_OPEN:	// OPEN command
      				//********************************
	    // nothing to do, driver declared as 'one at a time'
	    // so don't need to worry about exclusivity
	    break;


      				//********************************
	  case DREQ_CLOSE:	// CLOSE command
      				//********************************
	    // terminate any plotter activities
	    L_ReqClose( pDrvReq );

	    // stop case
	    break;


      				//********************************
	  case DREQ_IOCTL:	// IOCTL command
      				//********************************
	    // verify Category 128, Function 0 in Request Packet
	    if (    pDrvReq->IoCtl.bCategory != 128
	    	 || pDrvReq->IoCtl.bFunction != 0 )
	    {   // nope, don't know this one, indicate it
	    	pDrvReq->nStatus = 0x8103;

		// stop case
		break;
	    }

	    // extract ptr to IKON Request data
	    // should point to a IK_REQ structure
	    pIkReq = pDrvReq->IoCtl.fpParm;

	    // verify r/w accessibility of IK_REQ structure
	    HlpVerifyAccess( pIkReq, sizeof(IK_REQ), 1 );

	    // default status to bad parameters
	    pIkReq->qBufUsed = 0;
	    pIkReq->nStatus  = ERR_IK_BAD_PARM;

	    // process by type of Client Request
	    switch ( pIkReq->nOpCode )
	    {
	      				//********************************
	      case IK_INIT_QUERY:	// Query value of IK_CFG_PARM
	      				//********************************
		// IK_REQ should point to a IK_CFG_PARM
		// check buffer claims big enough
		if ( pIkReq->qBufSize < sizeof(IK_CFG_PARM) )
		{   // nope, stop case
		    break;
		}

		// verify accessibility of it
		HlpVerifyAccess( pIkReq->fpcBuf,
				 sizeof(IK_CFG_PARM),
				 1 );

		// copy data from local values to their buffer
		((IK_CFG_PARM *) (pIkReq->fpcBuf))->nIoPort    = L_nIoBase;
		((IK_CFG_PARM *) (pIkReq->fpcBuf))->nInterrupt = L_nIntNum;
		((IK_CFG_PARM *) (pIkReq->fpcBuf))->nDma       = L_nDmaNum;

		// set params to show successful transfer
		pIkReq->qBufUsed = sizeof( IK_CFG_PARM );
		pIkReq->nStatus  = 0;

	      	// stop case
		break;

	      				//********************************
	      case IK_INIT_SET:		// Setup value of IK_CFG_PARM
	      				//********************************
		// IK_REQ should point to a IK_CFG_PARM
		// check buffer claims big enough
		if ( pIkReq->qBufSize < sizeof(IK_CFG_PARM) )
		{   // nope, stop case
		    break;
		}

		// verify accessibility of it
		HlpVerifyAccess( pIkReq->fpcBuf,
				 sizeof(IK_CFG_PARM),
				 1 );

		// set params to show amt transferred
		pIkReq->qBufUsed = sizeof( IK_CFG_PARM );

		// perform remainder of adaptor initialization
		// set 'nStatus' value appropriately
		L_InitAdaptor( pIkReq );

	      	// stop case
		break;

	      				//*************************
	    case IK_WRITE_RASTER:	// Write raster from IK_REQ
	      				//*************************
		// IK_REQ.fpcBuf   	points to raster data
		//	 .qBufSize 	is raster byte count
		//	 .nBytesPerLine	is used to determine line count

		// write raster data, will set status, etc. appropriately
		L_WriteRaster( pIkReq );

		// stop case
		break;

	      				//***********************
	    case IK_WRITE_TEXT:		// Write text from IK_REQ
	      				//***********************
		// IK_REQ.fpcBuf   points to text data
		//	 .qBufSize is text byte count

		// verify accessibility of text data
		HlpVerifyAccess( pIkReq->fpcBuf,
				 (uint16) pIkReq->qBufSize,
				 1 );

		// write text data, will set status, etc. appropriately
		L_WriteText( pIkReq );

		// stop case
		break;

	      				//*****************************
	    case IK_WRITE_MOVE:		// Write paper move from IK_REQ
	      				//*****************************
		// IK_REQ.nBytesPerLine contains paper move count

		// write paper move data, will set status, etc.
		L_WriteMove( pIkReq );

		// stop case
		break;

	      				//***********************************
	    case IK_WRITE_CONTROLS:	// Write plotter controls from IK_REQ
	      				//***********************************
		// IK_REQ.nFlags contains plotter control mask to write

		// write plotter control data, will set status, etc.
		L_WriteControl( pIkReq );

		// stop case
		break;

	    } // end IoCtl case

	    // stop IoCtl case
	    break;
	    

			//****************
	  default:	// unknown command
			//****************
	    // set error
	    pDrvReq->nStatus = 0x8103;

	    // stop default case
	    break;
	}

	// set 'done' bit in status
	pDrvReq->nStatus |= 0x0100;


	// return to caller
	return;
  }

/****************************************************************************

	static void _near L_WriteRaster(	IK_REQ     *pIkReq	)

	This function starts output of raster data from the pIkReq data.

	Called with:
		pIkReq			ptr to IK_REQ structure

	Returns:
		none, returns status in pIkReq

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _near L_WriteRaster(	IK_REQ     *pIkReq	)

  {	// define automatics
	int		nStatus;		// Hlp function status
	uint32		qLockHandle;		// client buffer lock
	uint32		qTimeout;		// timeout value

	// verify not already doing this
	if ( L_nBufLineCnt != 0 )
	{   // whoops, can't start another one yet
	    pIkReq->nStatus  = ERR_IK_ALREADY_BUSY;
	    pIkReq->qBufUsed = 0;

	    // return to caller
	    return;
	}

	// verify params (does not check for short last line)
	if (    pIkReq->nBytesPerLine == 0
	     || pIkReq->nBytesPerLine  > (uint16) pIkReq->qBufSize )
	{   // can't have empty lines of data
	    pIkReq->nStatus  = ERR_IK_BAD_PARM;
	    pIkReq->qBufUsed = 0;

	    // return to caller
	    return;
	}

	// lock the client data buffer (assumes zero offset)
	qLockHandle = HlpLock( FP_SEG(pIkReq->fpcBuf), 0, 0 );

	// verify accessibility of raster data
	HlpVerifyAccess( pIkReq->fpcBuf,
			 (uint16) pIkReq->qBufSize,
			 1 );

	// get physical address of buffer, set offset to zero
	L_qBufPhyAddr = HlpVirtToPhys( (uint32) pIkReq->fpcBuf );
	L_nBufPhyOff  = 0;

	// setup the bytes per line value
	L_nBufBytesPerLine = pIkReq->nBytesPerLine;

	// compute how many lines of data we will output
	// note that we are restricting to a 64k buffer
	// so the math will be done without library call
	L_nBufLineCnt  = (uint16) pIkReq->qBufSize / L_nBufBytesPerLine;

	// compute timeout value
	qTimeout = (pIkReq->qTimeout == 0)
			? IK_TIMEOUT_DEFAULT
			: pIkReq->qTimeout;

	// block on adaptor being ready to run
	if ( L_WaitReady(qTimeout) != 0 )
	{   // never came ready, set timeout error
	    pIkReq->nStatus  = ERR_IK_TIMEOUT;
	    pIkReq->qBufUsed = 0;

	    // unlock the callers data
	    HlpUnLock( qLockHandle );

	    // clear the count in process value
	    L_nBufLineCnt = 0;

	    // return to caller
	    return;
	}

	// set interrupt mode to raster, perform simulated 
	// interrupt to start raster data running
	_disable();
	L_nIntMode     |= I_RASTER;
	L_nIntSimulate  = 1;
	L_InterruptService();
	_enable();

	// block on raster data transfer until interrupt time
	while ( 1 )
	{
	    // disable interrupts for check that we are done
	    _disable();

	    // check if all raster lines written
	    if ( L_nBufLineCnt == 0 )
	    {   // yep, re-enable interrupts
	    	_enable();

		// stop wait loop
		break;
	    }

	    // still have lines of data to write, block thread
	    // waiting on a long composed of the virtual address
	    // of a piece of our data plus the type of call
	    nStatus = HlpBlock( (uint32) &L_qIntServAddr+I_RASTER,
				qTimeout,
				0 );

	    // check unusual (0x0100) or timeout (0x4000) return
	    if (    (nStatus & 0x0100) != 0
	         || (nStatus & 0x4000) == 0 )
	    {   // timeout return, indicate no data written and timeout
		pIkReq->nStatus  = ERR_IK_TIMEOUT;
		pIkReq->qBufUsed = 0;

		// clear Interrupt control bit, raster line count
		L_nIntMode    &= ~I_RASTER;
		L_nBufLineCnt  = 0;

		// release lock
		HlpUnLock( qLockHandle );

		// return to caller
		return;
	    }

	}

	// all raster lines written, clear Interrupt control bit
	L_nIntMode &= ~I_RASTER;

	// release client data buffer lock
	HlpUnLock( qLockHandle );

	// set status, count for successful completion
	pIkReq->nStatus  = 0;
	pIkReq->qBufUsed = pIkReq->qBufSize;

	// return to caller without error
	return;
  }

/****************************************************************************

	static void _near L_WriteText(	IK_REQ     *pIkReq	)

	This function starts output of text data from the pIkReq data.
	The buffer is not considered to be composed of 'lines' of data,
	but is output to the device as one string of data.  Multiple
	lines of data must be indicated by a 'LF' character with the
	data stream.  The IkReq->nBytesPerLine is consequently not used.

	A single 'Line Terminate' control will be written at the end
	of the data stream.

	Called with:
		pIkReq			ptr to IK_REQ structure

	Returns:
		none, returns status in pIkReq

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _near L_WriteText(	IK_REQ     *pIkReq	)

  {	// define automatics
	int		nStatus;		// Hlp function status
	uint32		qLockHandle;		// client buffer lock
	uint32		qTimeout;		// timeout value

	// verify not already busy
	if ( L_nBufLineCnt != 0 )
	{   // whoops, can't start another one yet
	    pIkReq->nStatus  = ERR_IK_ALREADY_BUSY;
	    pIkReq->qBufUsed = 0;

	    // return to caller
	    return;
	}

	// verify params
	if ( pIkReq->qBufSize == 0 )
	{   // can't have empty buffer
	    pIkReq->nStatus  = ERR_IK_BAD_PARM;
	    pIkReq->qBufUsed = 0;

	    // return to caller
	    return;
	}

	// lock the client data buffer (assumes zero offset)
	qLockHandle = HlpLock( FP_SEG(pIkReq->fpcBuf), 0, 0 );

	// verify accessibility of text data
	HlpVerifyAccess( pIkReq->fpcBuf,
			 (uint16) pIkReq->qBufSize,
			 1 );

	// get physical address of buffer, set offset to zero
	L_qBufPhyAddr = HlpVirtToPhys( (uint32) pIkReq->fpcBuf );
	L_nBufPhyOff  = 0;

	// setup the bytes per line value, indicating only one line
	L_nBufBytesPerLine = (uint16) pIkReq->qBufSize;

	// set lines of data count to 1
	L_nBufLineCnt = 1;

	// compute timeout value
	qTimeout = (pIkReq->qTimeout == 0)
			? IK_TIMEOUT_DEFAULT
			: pIkReq->qTimeout;

	// block on adaptor being ready to run
	if ( L_WaitReady(qTimeout) != 0 )
	{   // never came ready, set timeout error
	    pIkReq->nStatus  = ERR_IK_TIMEOUT;
	    pIkReq->qBufUsed = 0;

	    // unlock the callers data
	    HlpUnLock( qLockHandle );

	    // clear the count in process value
	    L_nBufLineCnt = 0;

	    // return to caller
	    return;
	}

	// set interrupt mode to text, perform simulated 
	// interrupt to start text data running
	_disable();
	L_nIntMode     |= I_TEXT;
	L_nIntSimulate  = 1;
	L_InterruptService();
	_enable();

	// block on text data transfer until interrupt time
	while ( 1 )
	{
	    // disable interrupts for check that we are done
	    _disable();

	    // check if the text line written
	    if ( L_nBufLineCnt == 0 )
	    {   // yep, re-enable interrupts
	    	_enable();

		// stop wait loop
		break;
	    }

	    // still have lines of data to write, block thread
	    // waiting on a long composed of the virtual address
	    // of a piece of our data plus the type of call
	    nStatus = HlpBlock( (uint32) &L_qIntServAddr+I_TEXT,
				qTimeout,
				0 );

	    // check unusual (0x0100) or timeout (0x4000) return
	    if (    (nStatus & 0x0100) != 0
	         || (nStatus & 0x4000) == 0 )
	    {   // timeout return, indicate no data written and timeout
		pIkReq->nStatus  = ERR_IK_TIMEOUT;
		pIkReq->qBufUsed = 0;

		// clear Interrupt control bit, raster line count
		L_nIntMode    &= ~I_TEXT;
		L_nBufLineCnt  = 0;

		// release lock
		HlpUnLock( qLockHandle );

		// return to caller
		return;
	    }

	}

	// all text data written, clear Interrupt control bit
	L_nIntMode &= ~I_TEXT;

	// release client data buffer lock
	HlpUnLock( qLockHandle );

	// set status, count for successful completion
	pIkReq->nStatus  = 0;
	pIkReq->qBufUsed = pIkReq->qBufSize;

	// return to caller without error
	return;
  }

/****************************************************************************

	static void _near L_WriteMove(	IK_REQ     *pIkReq	)

	This function starts output of paper moves from the pIkReq data.
	The paper moves are accomplished by outputting a string of
	Line Terminate control functions.  No data is expected to be
	supplied from the incoming module, except that the number of
	raster lines to move paper is supplied in the nBytesPerLine.

	Called with:
		pIkReq			ptr to IK_REQ structure

	Returns:
		none, returns status in pIkReq

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _near L_WriteMove(	IK_REQ     *pIkReq	)

  {	// define automatics
	int		nStatus;		// Hlp function status
	uint32		qLockHandle;		// client buffer lock
	uint32		qTimeout;		// timeout value

	// verify not already busy
	if ( L_nBufLineCnt != 0 )
	{   // whoops, can't start another one yet
	    pIkReq->nStatus  = ERR_IK_ALREADY_BUSY;
	    pIkReq->qBufUsed = 0;

	    // return to caller
	    return;
	}

	// verify params
	if ( pIkReq->nBytesPerLine == 0 )
	{   // can't have empty lines to move request
	    pIkReq->nStatus  = ERR_IK_BAD_PARM;
	    pIkReq->qBufUsed = 0;

	    // return to caller
	    return;
	}

	// setup the bytes per line value to zero
	L_nBufBytesPerLine = 0;


	// set lines of data count from the IkReq->nBytesPerLine field
	L_nBufLineCnt = pIkReq->nBytesPerLine;

	// compute timeout value
	qTimeout = (pIkReq->qTimeout == 0)
			? IK_TIMEOUT_DEFAULT
			: pIkReq->qTimeout;

	// block on adaptor being ready to run
	if ( L_WaitReady(qTimeout) != 0 )
	{   // never came ready, set timeout error
	    pIkReq->nStatus  = ERR_IK_TIMEOUT;
	    pIkReq->qBufUsed = 0;

	    // unlock the callers data
	    HlpUnLock( qLockHandle );

	    // clear the count in process value
	    L_nBufLineCnt = 0;

	    // return to caller
	    return;
	}

	// set interrupt mode to Move, perform simulated 
	// interrupt to start move operations running
	_disable();
	L_nIntMode     |= I_MOVE;
	L_nIntSimulate  = 1;
	L_InterruptService();
	_enable();

	// block on Move operations until interrupt time
	while ( 1 )
	{
	    // disable interrupts for check that we are done
	    _disable();

	    // check if the text line written
	    if ( L_nBufLineCnt == 0 )
	    {   // yep, re-enable interrupts
	    	_enable();

		// stop wait loop
		break;
	    }

	    // still have lines of data to write, block thread
	    // waiting on a long composed of the virtual address
	    // of a piece of our data plus the type of call
	    nStatus = HlpBlock( (uint32) &L_qIntServAddr+I_MOVE,
				qTimeout,
				0 );

	    // check unusual (0x0100) or timeout (0x4000) return
	    if (    (nStatus & 0x0100) != 0
	         || (nStatus & 0x4000) == 0 )
	    {   // timeout return, indicate no data written and timeout
		pIkReq->nStatus  = ERR_IK_TIMEOUT;
		pIkReq->qBufUsed = 0;

		// clear Interrupt control bit, raster line count
		L_nIntMode    &= ~I_MOVE;
		L_nBufLineCnt  = 0;

		// release lock
		HlpUnLock( qLockHandle );

		// return to caller
		return;
	    }

	}

	// all text data written, clear Interrupt control bit
	L_nIntMode &= ~I_MOVE;

	// release client data buffer lock
	HlpUnLock( qLockHandle );

	// set status, count for successful completion
	pIkReq->nStatus  = 0;
	pIkReq->qBufUsed = pIkReq->qBufSize;

	// return to caller without error
	return;
  }

/****************************************************************************

	static void _near L_WriteControl(	IK_REQ     *pIkReq	)

	This function outputs plotter controls from the pIkReq data.

	Called with:
		pIkReq			ptr to IK_REQ structure

	Returns:
		none, returns status in pIkReq

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _near L_WriteControl(	IK_REQ     *pIkReq	)

  {	// define automatics
	uint32		qTimeout;		// timeout value

	// verify not already doing this
	if ( L_nBufLineCnt != 0 )
	{   // whoops, can't start another one yet
	    pIkReq->nStatus  = ERR_IK_ALREADY_BUSY;
	    pIkReq->qBufUsed = 0;

	    // return to caller
	    return;
	}

	// verify params
	if ( (pIkReq->nFlags & (  IK_FLAG_C_LT
			        | IK_FLAG_C_EOT
			        | IK_FLAG_C_FF
			        | IK_FLAG_C_CLR)) == 0 )
	{   // can't have empty control flags
	    pIkReq->nStatus  = ERR_IK_BAD_PARM;
	    pIkReq->qBufUsed = 0;

	    // return to caller
	    return;
	}

	// setup the control value
	L_nControlFlags = pIkReq->nFlags;

	// compute timeout value
	qTimeout = (pIkReq->qTimeout == 0)
			? IK_TIMEOUT_DEFAULT
			: pIkReq->qTimeout;

	// block on adaptor being ready to run
	if ( L_WaitReady(qTimeout) != 0 )
	{   // never came ready, set timeout error
	    pIkReq->nStatus  = ERR_IK_TIMEOUT;
	    pIkReq->qBufUsed = 0;

	    // clear the control values
	    L_nControlFlags = 0;

	    // return to caller
	    return;
	}

	// note: we do not block from here on, assumption is that
	// if the adaptor is ready, we can start a control operation
	// and then return back to the caller

	// set interrupt mode to control, perform simulated 
	// interrupt to start control value running
	_disable();
	L_nIntMode     |= I_CONTROL;
	L_nIntSimulate  = 1;
	L_InterruptService();
	_enable();

	// set status, count for successful completion
	pIkReq->nStatus  = 0;
	pIkReq->qBufUsed = pIkReq->qBufSize;

	// return to caller without error
	return;
  }

/****************************************************************************

	static int _near L_WaitReady(	uint32	qTimeout	)

	This function is used to test the adaptor for Ready status, and
	if not already ready, to enable interrupts and block on the
	adaptor becoming ready.

	Called with:
		qTimeout		max time (msecs) to wait on device

	Returns:
		0		adaptor is  ready
		1			not ready

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// start function
static int  _near L_WaitReady(	uint32	qTimeout	)

  {	// define automatics
	int		nStatus;		// Block status

	// check if adaptor (and device) are ready
	if ( (inp(L_nIoInIstatAddr) & IO_ISTAT_HI_READY) != 0 )
	{   // is already ready, return to caller, ready status
	    return( 0 );
	}

	// not ready, disable interrupts while we
	// setup the adaptor interrupt control
	_disable();

	// enable the adaptor interrupt control
	outp( L_nIoOutSetupAddr,
	      inp(L_nIoInSetupAddr) | IO_SETUP_HI_INT_ENABLE );

	// set the simulated interrupt mode to wake us on ready
	L_nIntMode |= I_READY;

	// block on ISR addr + Ready status, qTimeout wait
	// thread will resume with interrupts enabled, either
	// on timeout, or by HlpRun from Interrupt Service
	nStatus = HlpBlock( (uint32) &L_qIntServAddr+I_READY,
			    qTimeout,
			    0 );

	// clear wait ready flag
	L_nIntMode &= ~I_READY;

	// check status once more (probably potential for race
	// condition here if more than one thread is allowed
	// into the driver at a time)
	if ( (inp(L_nIoInIstatAddr) & IO_ISTAT_HI_READY) != 0 )
	{   // is ready, return to caller, ready status
	    return( 0 );
	}

	// not ready, timed out, return to caller with error
	return( 1 );
  }

/****************************************************************************

	static void _far L_InterruptService(	void	)

	This function handles hardware interrupts from the Scanner.

	Called with:
		none

	Returns:
		none

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _far L_InterruptService(	void	)

  {	// define automatics
	int		nAdpStat;		// adaptor status value

	// get adaptor status value
	nAdpStat = inp( L_nIoInIstatAddr );

	// if adaptor interrupting, clear it
	if ( (nAdpStat & IO_ISTAT_HI_INTERRUPT) != 0 )
	{   // adaptor generated interrupt, clear it
	    outp( L_nIoOutRemoteAddr, IO_REMOTE_HI_RESET_INT );
	}

	// if device ready, check active operations
	// (use 'while' so can use break to stop)
	while ( (nAdpStat & IO_ISTAT_HI_READY) != 0 )
	{   // device is ready, do first requested operation

	    // check for Adaptor Ready wait
	    if ( (L_nIntMode & I_READY) != 0 )
	    {   // wanted notification when adaptor ready
		// release waiting thread (from L_WaitReady())
		HlpRun( (uint32) &L_qIntServAddr+I_READY );

		// stop loop, any others must wait
		break;
	    }

	    // check for Raster Control Lines
	    if (    (L_nIntMode & I_CONTROL) != 0
	         && L_nControlFlags	     != 0 )
	    {   // want to output control data
	    	L_StartControl();

		// clear output control data flag
		L_nIntMode &= ~I_CONTROL;

		// continue with next entry, device will
		// still be 'ready' enough to setup for 
		// Raster, Text, or Move output preparation
	    }

	    // check for Raster Line Output
	    if (    (L_nIntMode & I_RASTER) != 0
	    	 && L_nBufLineCnt	    != 0 )
	    {   // want to output raster data
		L_StartDma( I_RASTER );

		// set flag for need Line Terminate control data
		// at the next interrupt process
		L_nIntMode 	|= I_CONTROL;
		L_nControlFlags |= IK_FLAG_C_LT;

		// stop while
		break;
	    }

	    // check for Text Data Output
	    if (    (L_nIntMode & I_TEXT) != 0
	    	 && L_nBufLineCnt	  != 0 )
	    {   // want to output text data
		L_StartDma( I_TEXT );

		// set flag for need Line Terminate control data
		// at the next interrupt process
		L_nIntMode 	|= I_CONTROL;
		L_nControlFlags |= IK_FLAG_C_LT;

		// stop while
		break;
	    }

	    // check for Move Mode Output
	    if (    (L_nIntMode & I_MOVE) != 0
	    	 && L_nBufLineCnt	  != 0 )
	    {   // want to output move mode data, do so by
	    	// setting the Line Terminate Control flag
		// and performing a L_StartControl()
		L_nControlFlags |= IK_FLAG_C_LT;
		L_StartControl();

		// stop while
		break;
	    }

	    // all options tested, stop loop
	    break;
	}

	// if a Hardware interrupt, acknowledge it in the PIC
	if ( L_nIntSimulate == 0 )
	{   // acknowledge interrupt in PIC
	    HlpEOI( L_nIntNum );
	}
	else
	{   // simulated interrupt, clear flag
	    L_nIntSimulate = 0;
	}

	// return to caller
	return;
  }

/****************************************************************************

	static void _near L_StartDma(	int	nRasterFlag	)

	This function is used to startup a raster mode data transfer from
	the memory buffer to the adaptor using DMA.

	Note:
	This module assumes that interrupts have been disabled before
	calling.  This assumption is necessary during manipulation of
	the DMA channel registers.

	This module also assumes that the adaptor is in a ready state.

	Called with:
		nRasterFlag		I_RASTER for raster mode data
					I_TEXT   for text   mode data

	Returns:
		none

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// start function
static void _near L_StartDma(	int	nRasterFlag	)

  {	// define automatics
	uint16		nDmaCnt;		// dma output count
	uint32		qPhyAddr;		// dma physical buffer loc

//DEBUG, need to handle various byte orders in buffered data
//	 and the swap mode and dma channel (word or byte) 
//	 that the adaptor is using

	// Note: It is assumed that the Adaptor and Device are 'ready'
	// at this point.

	// set plotter into proper mode (PLOT or TEXT)
	if ( nRasterFlag == I_RASTER )
	{   // raster mode, set PLOT bit
	    outp( L_nIoOutSetupAddr,
	          inp(L_nIoInSetupAddr) | IO_SETUP_HI_PLOT_MODE );
	}
	else
	{   // text mode, reset PLOT bit
	    outp( L_nIoOutSetupAddr,
	    	  inp(L_nIoInSetupAddr) & ~IO_SETUP_HI_PLOT_MODE );
	}

	//******************
	// setup DMA channel
	//******************

	// disable DMA channel operation
	outp( L_Dma.nAdrMask,	L_Dma.nValSet	);

	// set DMA mode to READ MEMORY
	outp( L_Dma.nAdrMode,	L_Dma.nValMode	);

	// calc physical address of buffer, update offset for next buffer
	qPhyAddr       = L_qBufPhyAddr + L_nBufPhyOff;
	L_nBufPhyOff += L_nBufBytesPerLine;

	// setup DMA buffer address in WORD mode
	// it looks screwy:
	//	1. reset logic
	//	2. output lo byte of hi 16 bits
	//	3. output lo byte of (phy addr divided by 2) (words)
	//	4. output hi byte of lo 16 bit (phy addr divided by 2)
	outp( L_Dma.nAdrReset,	0 );
	outp( L_Dma.nAdrPage,	(uint8) ((qPhyAddr >> 16) & 0xff) );
	outp( L_Dma.nAdrOff,	(uint8) ((qPhyAddr >>  1) & 0xff) );
	outp( L_Dma.nAdrOff,	(uint8) ((qPhyAddr >>  9) & 0xff) );

	// set DMA transfer count (cnt is 1 less than desired moves in words)
	nDmaCnt = (L_nBufBytesPerLine >> 1) - 1;
	outp( L_Dma.nAdrCnt, (uint8) ( nDmaCnt       & 0xff) );
	outp( L_Dma.nAdrCnt, (uint8) ((nDmaCnt >> 8) & 0xff) );

	// start DMA operation
	outp( L_Dma.nAdrMask, 	L_Dma.nValReset );

	// start IKON dma mode with adaptor interrupts enabled
	// in the current PLOT or TEXT mode
	outp( L_nIoOutSetupAddr,
	      inp(L_nIoInSetupAddr) 
	      | IO_SETUP_HI_DMA_ENABLE
	      | IO_SETUP_HI_INT_ENABLE );

	// return to caller
	return;
  }

/****************************************************************************

	static void _near L_StartControl(	void	)

	This function is used to startup a Control Mode output to the
	plotter, primarily Line Terminate, Form Feed, etc.

	Called with:
		none

	Returns:
		none

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// start function
static void _near L_StartControl(	void	)

  {
	// Note: It is assumed that the Adaptor and Device are 'ready'
	// at this point.

	// output control flag word to port
	outp( L_nIoOutRemoteAddr, 
	      L_nControlFlags & (  IK_FLAG_C_LT
	      			 | IK_FLAG_C_EOT
				 | IK_FLAG_C_FF
				 | IK_FLAG_C_CLR ) );

	// test if LT set and Line Count not zero
	if (    (L_nControlFlags & IK_FLAG_C_LT) != 0
	     && L_nBufLineCnt			  > 0 )
	{   // yep, must be completing an output line, adjust count
	    L_nBufLineCnt--;

	    // if raster line count now zero, need to unblock thread
	    if ( L_nBufLineCnt == 0 )
	    {   // yep, it is waiting, release the proper thread
		// note that only one of the following three should 
		// actually be set at a time

		// test for Raster mode output
	    	if ( (L_nIntMode & I_RASTER) != 0 )
		{   // raster mode threads
		    HlpRun( (uint32) &L_qIntServAddr+I_RASTER );
		}

		// test for Text mode output
	    	if ( (L_nIntMode & I_TEXT) != 0 )
		{   // text mode threads
		    HlpRun( (uint32) &L_qIntServAddr+I_TEXT );
		}

		// test for Move mode output
	    	if ( (L_nIntMode & I_MOVE) != 0 )
		{   // move mode threads
		    HlpRun( (uint32) &L_qIntServAddr+I_MOVE );
		}
	    }
	}

	// clear the control flag word bits
	L_nControlFlags &= ~(  IK_FLAG_C_LT
	      		     | IK_FLAG_C_EOT
			     | IK_FLAG_C_FF
			     | IK_FLAG_C_CLR );

	// return to caller
	return;
  }

/****************************************************************************

	static void _near L_InitAdaptor( IK_REQ *pIkReq	)

	This function performs initialization of the Vidar Adaptor.  It
	expects the I/O Port to be defined in the IK_CFG_PARM structure 
	pointed to by the IK_REQ.  If an Ikon adaptor appears to be at
	the appropriate address, it will fill in the Interrupt and DMA
	members of the caller's IK_CFG_PARM structure.

	Called with:
		pIkReq		ptr to IK_REQ structure
		none

	Returns:
		none, errors returned to pIkReq->nStatus

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _near L_InitAdaptor( IK_REQ *pIkReq	)

  {	// define automatics
	int		nStrap;			// adaptor strapping values

	// extract I/O, precompute adaptor port addresses
	L_nIoBase	   = ((IK_CFG_PARM *) (pIkReq->fpcBuf))->nIoPort;
	L_nIoOutSetupAddr  = L_nIoBase + IO_OUT_SETUP_OFF;
	L_nIoOutRemoteAddr = L_nIoBase + IO_OUT_REMOTE_OFF;
	L_nIoOutDataAddr   = L_nIoBase + IO_OUT_DATA_OFF;
	L_nIoInSetupAddr   = L_nIoBase + IO_IN_SETUP_OFF;
	L_nIoInIstatAddr   = L_nIoBase + IO_IN_ISTAT_OFF;
	L_nIoInDiagAddr	   = L_nIoBase + IO_IN_DIAG_OFF;
	L_nIoInSetupAddr   = L_nIoBase + IO_IN_SETUP_OFF;
	L_nIoInStrapAddr   = L_nIoBase + IO_IN_STRAP_OFF;

	// reset adaptor interface logic
	outp( L_nIoOutRemoteAddr, IO_REMOTE_HI_RESET_ADP );

	// verify latched function register is all zero
	// the previous reset adaptor should have cleared it
	if ( inp(L_nIoInSetupAddr) != 0 )
	{   // nope, must not be an IKON card!
	    // so set status error indicator
	    pIkReq->nStatus = ERR_IK_NO_ADAPTOR;

	    // and return to caller
	    return;
	}

	// get Interrupt, DMA channels from ports on card
	nStrap    = inp( L_nIoInStrapAddr );
	L_nIntNum = (nStrap & IO_STRAP_INT_MASK) >> IO_STRAP_INT_SHIFT;
	L_nDmaNum = (nStrap & IO_STRAP_DMA_MASK) >> IO_STRAP_DMA_SHIFT;

	// tell caller what we found
	((IK_CFG_PARM *) (pIkReq->fpcBuf))->nInterrupt = L_nIntNum;
	((IK_CFG_PARM *) (pIkReq->fpcBuf))->nDma       = L_nDmaNum;

	// setup the DMA access control structure
	memcpy( &L_Dma, &L_DmaVals[L_nDmaNum], sizeof(L_Dma) );

	// allocate an interrupt vector
	if ( HlpSetIRQ( (uint16) L_qIntServAddr,
			L_nIntNum,
			0 ) != 0 )
	{   // unable to allocate interrupt vector, return error
	    pIkReq->nStatus = ERR_IK_BAD_INTERRUPT;
	    return;
	}

	// interrupt allocated, set flag
	L_nIntInUse = 1;

	// no errors, clear status
	pIkReq->nStatus = 0;

	// return to caller
	return;
  }

/****************************************************************************

	static void _near L_ReqClose(	DRV_REQ *pDrvReq	)

	This function performs termination of the Vidar Adaptor.

	Called with:
		pDrvReq		ptr to Driver Request Pkt

	Returns:
		none

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _near L_ReqClose(	DRV_REQ *pDrvReq	)

  {
	// disable interrupts at the adaptor
	outp( L_nIoOutSetupAddr,
	      inp(L_nIoInSetupAddr) & ~IO_SETUP_HI_INT_ENABLE );

	// if in use, terminate interrupt vector
	if ( L_nIntInUse == 1 )
	{   // yep, release it
	    HlpUnSetIRQ( L_nIntNum );

	    // clear the flag
	    L_nIntInUse = 0;
	}

	// return to caller
	return;
  }

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/****************************************************************************

	static void _near 	L_LastCode(	void	)

	This function serves the purpose of marking the end of the
	code which should be included in the _TEXT segment.  Functions
	placed in the _TEXT segment after this point (and also defined
	in the 'alloc_text' pragma) will be discarded after 'init' time.

	Called with:
		none

	Returns:
		none

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _near 	L_LastCode(	void	)

  {
	// return to caller
	return;
  }

/****************************************************************************

	---- small helpers (no C library in a device driver) ----------------------
****************************************************************************/

// string length
static int _near L_Slen( const char *s )
{
    int n = 0;
    while( s[n] )
        n++;
    return n;
}

// uppercase a character
static int _near L_Upc( int c )
{
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

// Print one message: "BYIKON: text", wrapped at 79 columns, prefix on every line.
static void _near L_Emit( const char *msg )
{
    int len, n, cut, i, k;
    const int prefix = sizeof( PREFIX ) - 1;
    const int width  = 79 - prefix - 2;

    if( fQuiet )
        return;
    len = L_Slen( msg );
    while( len > 0 ) {
        n = len;
        if( n > width ) {
            cut = width;
            while( cut > 0 && msg[cut] != ' ' )
                cut--;
            n = cut ? cut : width;
        }
        k = 0;
        for( i = 0; i < prefix; i++ )
            line[k++] = PREFIX[i];
        for( i = 0; i < n; i++ )
            line[k++] = msg[i];
        line[k++] = '\r';
        line[k++] = '\n';
        DosWrite( 1, (void FAR *)line, k, (USHORT FAR *)&done );
        msg += n;
        len -= n;
        if( len > 0 && *msg == ' ' ) {
            msg++;
            len--;
        }
    }
}

// append the text of a value to text[]
static void _near L_AddNum( unsigned value, int hex )
{
    static const char digits[] = "0123456789ABCDEF";
    char tmp[8];
    int  n = 0, i, base = hex ? 16 : 10;

    do {
        tmp[n++] = digits[value % base];
        value /= base;
    } while( value );
    i = L_Slen( text );
    if( hex ) {
        text[i++] = '0';
        text[i++] = 'x';
    }
    while( n )
        text[i++] = tmp[--n];
    text[i] = 0;
}

static void _near L_AddStr( const char *s )
{
    int i = L_Slen( text ), j = 0;

    while( s[j] && i < (int)sizeof( text ) - 1 )
        text[i++] = s[j++];
    text[i] = 0;
}

/* DEVICE=...\BYIKON.SYS [/V] [/Q] [/W] [/AA]
   Unknown options and extra words are ignored, as in the original driver. */
static void _near L_ParseArgs( const char FAR *p )
{
    if( p == 0 )
        return;

    while( *p == ' ' || *p == '\t' )
        p++;
    while( *p && *p != ' ' && *p != '\t' )
        p++;                                  /* skip the driver file name */

    for( ;; ) {
        while( *p == ' ' || *p == '\t' )
            p++;
        if( *p == 0 || *p == '\r' || *p == '\n' )
            break;
        if( *p == '/' || *p == '-' ) {
            p++;
            if( L_Upc( p[0] ) == 'A' && L_Upc( p[1] ) == 'A' ) {
                fBeep = 1;
                p += 2;
            } else {
                switch( L_Upc( *p ) ) {
                case 'V':
                    fVerbose = 1;
                    break;
                case 'Q':
                    fQuiet = 1;
                    break;
                case 'W':
                    fWait = 1;
                    break;
                default:
                    break;                    /* unknown options are ignored */
                }
                if( *p )
                    p++;
            }
        } else {
            while( *p && *p != ' ' && *p != '\t' )
                p++;                          /* ignore other arguments */
        }
    }
}

// diagonal test: only /W, no other options
// temporary diagnostic: print what the console read returns
static void _near L_WaitKey( void )
{
    USHORT rc;

    L_Emit( "Press any key to continue." );
    rc = DosRead( 0, (void FAR *)&key, 1, (USHORT FAR *)&got );
    if( fVerbose ) {
        text[0] = 0;
        L_AddStr( "Console read: rc " );
        L_AddNum( rc, 0 );
        L_AddStr( ", got " );
        L_AddNum( got, 0 );
        L_Emit( text );
    }
}

/****************************************************************************

	static void _near L_ReqInit( DRV_REQ *pDrvReq	)

	This function performs config.sys load time driver initialization.
	It is code that can be discarded after initialization.

	Called with:
		pDrvReq		ptr to Driver Request Pkt

	Returns:
		none

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define function
static void _near L_ReqInit( DRV_REQ *pDrvReq	)

  {	// define automatics
	uint16		nWrLen;			// DosWrite length
	unsigned char status;
	int ok;

	// save DevHlp address
	Drv_qDevHlpAddr = pDrvReq->Init.In.qDevHlpAddr;

	/* the arguments live in the INIT packet, which the output fields overwrite;
	   so read them before writing the output fields */
	L_ParseArgs( (const char FAR *)pDrvReq->Init.In.qInitArgAddr );

	// save code and data offset values in request header
	pDrvReq->Init.Out.nCodeEnd = (uint16) &L_LastCode;
	pDrvReq->Init.Out.nDataEnd = (uint16) &L_LastData;

	// init the Bpb address field
	pDrvReq->Init.Out.qBpbAddr = 0;

	// check for IKON adaptor at default I/O base
	status = (unsigned char)inp( IO_BASE_ADDR + IO_IN_ISTAT_OFF );
	if( status == 0xFF ) {
	    ok = 0;                               /* nothing at I/O base */
	} else {
	    // reset adaptor interface logic
	    outp( IO_BASE_ADDR + IO_OUT_REMOTE_OFF, IO_REMOTE_HI_RESET_ADP );
	    // verify latched function register is all zero
	    ok = (inp( IO_BASE_ADDR + IO_IN_SETUP_OFF ) == 0);
	}

	if( ok ) {
	    if( fVerbose ) {
	        text[0] = 0;
	        L_AddStr( "BYIKON.SYS version " VER_STRING " (" VER_DATE ")." );
	        L_Emit( text );
	        text[0] = 0;
	        L_AddStr( "IKON adaptor found at I/O base 0x" );
	        L_AddNum( IO_BASE_ADDR, 1 );
	        L_AddStr( " (status " );
	        L_AddNum( status, 1 );
	        L_AddStr( ")." );
	        L_Emit( text );
	    }
	    if( fWait && !fQuiet )
	        L_WaitKey();
	    if( fBeep )
	        DosBeep( 1000, 120 );
	} else {
	    text[0] = 0;
	    L_AddStr( "BYIKON.SYS could not be loaded. " );
	    if( status == 0xFF ) {
	        L_AddStr( "No IKON adaptor was found at I/O base 0x" );
	        L_AddNum( IO_BASE_ADDR, 1 );
	        L_AddStr( " (port reads 0xFF)." );
	    } else {
	        L_AddStr( "The IKON adaptor at I/O base 0x" );
	        L_AddNum( IO_BASE_ADDR, 1 );
	        L_AddStr( " did not accept the reset sequence." );
	    }
	    L_Emit( text );
	    if( fWait && !fQuiet )
	        L_WaitKey();
	}

	// register our interrupt stack usage
	HlpRegisterStack( (uint16) &L_RegStack );

	if( ok )
	    pDrvReq->nStatus |= 0x0100;  // RPDONE
	else
	    pDrvReq->nStatus |= 0x8100 | 0x0001;  // RPDONE | RPERR | RPERR_GENERAL

	// return to caller
	return;
  }
