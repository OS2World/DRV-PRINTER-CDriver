/****************************************************************************

	"ddriver.h"

	Definitions for construction of a 'C' OS/2 Device Driver

	history:
	12-21-90 ras - SCR P0487, initial entry
	12-22-90 ras - SCR P0487, add HlpSetIRQ, UnSetIRQ, EOI, RegisterStack
				  add HlpQueueInit, Read, Write, Flush
				  add HlpBlock, Run, 
				  add HlpPullReqPkt, PushReqPkt
	12-23-90 ras - SCR P0487, add HlpVirtToPhys
	12-24-90 ras - SCR P0487, add HlpLock,UnLock,AllocGDTSel,
					 PhysToGDTSel

		Copyright 1990, Byers Engineering Company
****************************************************************************/

// define some type shorthand
#define	int8	signed	 char		// signed    8 bit int
#define	uint8	unsigned char		// unsigned  8 bit int
#define	int16		 short		// signed   16 bit int
#define	uint16	unsigned short		// unsigned 16 bit int
#define	int32		 long		// signed   32 bit int
#define	uint32	unsigned long		// unsigned 32 bit int

// define literals for Driver Request types
#define	DREQ_INIT		0x00
#define	DREQ_MEDIA_CHECK	0x01
#define	DREQ_BUILD_BPB		0x02
#define	DREQ_READ		0x04
#define	DREQ_ND_READ_NO_WAIT	0x05
#define	DREQ_INPUT_STATUS	0x06
#define	DREQ_INPUT_FLUSH	0x07
#define	DREQ_WRITE		0x08
#define	DREQ_WRITE_VERIFY	0x09
#define	DREQ_OUTPUT_STATUS	0x0a
#define	DREQ_OUTPUT_FLUSH	0x0b
#define	DREQ_OPEN		0x0d
#define	DREQ_CLOSE		0x0e
#define	DREQ_REMOVABLE_MEDIA	0x0f
#define	DREQ_IOCTL		0x10
#define	DREQ_RESET_MEDIA	0x11
#define	DREQ_GET_LOG_DRIVE_MAP	0x12
#define	DREQ_SET_LOG_DRIVE_MAP	0x13
#define	DREQ_DEINSTALL		0x14
#define	DREQ_PARTITION_DISK	0x16
#define	DREQ_GET_LOG_UNIT_MAP	0x17
#define	DREQ_NCACHE_READ	0x18
#define	DREQ_NCACHE_WRITE	0x19
#define	DREQ_NCACHE_WRITE_VER	0x1a
#define	DREQ_PREPARE_SHUTDOWN	0x1c

// define structures for device driver communications
typedef struct				// Request Header
  {	uint8		bLen;			// length of header
	uint8		bUnit;			// unit code for block devices
	uint8		bCmd;			// device driver command
	uint16		nStatus;		// return status
	uint8		bReserved1[4];		// reserved
	uint32		qLinkage;		// queue linkage
    union
      {	union				// 00: INIT specific
	{ struct
	  { uint8	bReserved2;		// unused on input
	    uint32	qDevHlpAddr;		// DevHlp address
	    uint32	qInitArgAddr;		// Initialization arg addr
	  } In;
	  struct
	  { uint8	bNumUnits;		// blk dev, num units used
	    uint16	nCodeEnd;		// offset of code end
	    uint16	nDataEnd;		// offset of data end
	    uint32	qBpbAddr;		// blk dev, addr of bpb list
	  } Out;
	} Init;
	struct				// 04: READ specific
	{ uint8		bMediaDescriptor;	// media descriptor byte
	  uint32	qBufAddr;		// transfer buffer address
	  uint16	nTransferCnt;		// transfer count
	  uint32	qStartSector;		// blk dev, starting sector
	  uint16	nFileNumber;		// blk dev, file channel
	  uint16	nAdviceCode;		// blk dev, lev2 advice code
	} Read;
	struct				// 08: WRITE specific
	{ uint8		bMediaDescriptor;	// media descriptor byte
	  uint32	qBufAddr;		// transfer buffer address
	  uint16	nTransferCnt;		// transfer count
	  uint32	qStartSector;		// blk dev, starting sector
	  uint16	nFileNumber;		// blk dev, file channel
	} Write;
	struct				// 09: WRITE_VERIFY specific
	{ uint8		bMediaDescriptor;	// media descriptor byte
	  uint32	qBufAddr;		// transfer buffer address
	  uint16	nTransferCnt;		// transfer count
	  uint32	qStartSector;		// blk dev, starting sector
	  uint16	nFileNumber;		// blk dev, file channel
	} WriteVerify;
	struct				// 0d: OPEN specific
	{ uint16	nFileNumber;		// blk dev, file channel
	} Open;
	struct				// 0d: CLOSE specific
	{ uint16	nFileNumber;		// blk dev, file channel
	} Close;
      	struct				// 10: IOCTL specific
	{ uint8		bCategory;		// category code
	  uint8		bFunction;		// function code
	  void	far    *fpParm;			// ptr to Parm buffer
	  void	far    *fpData;			// ptr to Data buffer
	  uint16	nFileNumber;		// blk dev, file channel
	} IoCtl;
      };
  } DRV_REQ;

// define Character Queue structure
#define	CHR_QUEUE(name,size)	struct { uint16	nSize;		\
					 uint16 nOut;		\
					 uint16 nCnt;		\
					 uint8	bBuf[size];	\
				       } name = {size};

// define Register Stack Usage structure
#define	REG_STACK_USAGE(name,flag,num,cli,sti,eoi,nest)		\
		struct { uint16	nSize;				\
			 uint16 nFlags;				\
			 uint16 nUseIRQ;			\
			 uint16 nUseCLI;			\
			 uint16 nUseSTI;			\
			 uint16 nUseEOI;			\
			 uint16 nNest;				\
		       } name = {14,flag,num,cli,sti,eoi,nest};

// define Publics available in the .ASM stub
extern	uint32	Drv_qDevHlpAddr;		// device helper addr

// define Functions available in the .ASM stub
extern	void   far _pascal HlpVerifyAccess(	void far *fpMem,
						uint16	  nLen,
						uint16	  nType		);
extern	uint32 far _pascal HlpPhysToUVirt(	uint32	  qPhyAddr,
						uint16	  nLen,
						uint16	  nType,
						uint16	  nTagType	);
extern	int    far _pascal HlpSetIRQ(		uint16	  nOffset,
						uint16	  nIntNum,
						uint16	  nShareFlag	);
extern	int    far _pascal HlpRegisterStack(	uint16	  nOffset	);
extern	void   far _pascal HlpUnSetIRQ(		uint16	  nIntNum	);
extern	void   far _pascal HlpEOI(		uint16	  nIntNum	);
extern	void   far _pascal HlpQueueInit(	uint16	  nOffset	);
extern	void   far _pascal HlpQueueFlush(	uint16	  nOffset	);
extern	int    far _pascal HlpQueueRead(	uint16	  nOffset	);
extern	int    far _pascal HlpQueueWrite(	uint16	  nOffset,
						int	  nChr		);
extern	uint16 far _pascal HlpBlock(		uint32	  qEventId,
						uint32	  qTimeout,
						uint16	  nIntFlag	);
extern	void   far _pascal HlpRun(		uint32	  qEventId	);
extern	uint32 far _pascal HlpPullReqPkt(	uint16	  nOffset	);
extern	void   far _pascal HlpPushReqPkt(	uint16	  nOffset,
						uint32	  qPktAddr	);
extern	uint32 far _pascal HlpVirtToPhys(	uint32	  qPktAddr	);
extern	uint32 far _pascal HlpLock(		uint16	  nSelector,
						uint16	  nType,
						uint16	  nWait		);
extern	int    far _pascal HlpUnLock(		uint32	  qHandle	);
extern	int    far _pascal HlpAllocGDTSel(	uint32	  qSelAddr,
						uint16	  nCnt		);
extern	int    far _pascal HlpPhysToGDTSel(	uint32	  qPhysAddr,
						uint16	  nLength,
						uint16	  nSelector	);
