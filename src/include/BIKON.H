/****************************************************************************

	"bikon.h"

	Ikon 10092 Adaptor definitions for use by Byers OS/2 modules

	history:
	01-08-91 ras - SCR P0487, initial entry
	01-09-91 ras - SCR P0487, continue

		Copyright 1991, Byers Engineering Company
****************************************************************************/

// define structure to communicate with driver
typedef	struct
  {	uint16		nOpCode;		// request opcode
	uint16		nStatus;		// completion status
	uint16		nFlags;			// request control/status
	uint16		nBytesPerLine;		// bytes data per 'line'
	uint32		qTimeout;		// request timeout (msecs)
	uint32		qBufSize;		// buffer size
	uint32		qBufUsed;		// buffer used
	void far       *fpcBuf;			// buffer ptr
  } IK_REQ;

// define structure to contain driver configuration parameters
typedef	struct
  { 	uint16		nIoPort;		// Adaptor I/O port base
	uint16		nInterrupt;		//	   Interrupt vector
	uint16		nDma;			//	   DMA channel
  } IK_CFG_PARM;

// define client req error codes
#define	ERR_IK_BAD_PARM     	2001
#define	ERR_IK_TIMEOUT		2002
#define	ERR_IK_NO_ADAPTOR	2003
#define	ERR_IK_BAD_INTERRUPT	2004
#define	ERR_IK_ALREADY_BUSY	2005

// define IK_REQ opcodes
#define	IK_INIT_QUERY		0
#define	IK_INIT_SET		1
#define	IK_WRITE_RASTER		3
#define	IK_WRITE_TEXT		4
#define	IK_WRITE_MOVE		5
#define	IK_WRITE_CONTROLS	6

// define IK_REQ .nFlags Control values
#define	IK_FLAG_C_LT		0x0001
#define	IK_FLAG_C_EOT		0x0002
#define	IK_FLAG_C_FF		0x0004
#define	IK_FLAG_C_CLR		0x0008

// define IK_REQ status values
#define	IK_STATUS_UNKNOWN	1		// error, status is unknown

// define default timeout values
#define	IK_TIMEOUT_DEFAULT	10000L		// default to 10 seconds
