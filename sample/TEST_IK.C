// Module to test use of Byers IKON OS/2 Driver
//
//	01-09-91 ras - SCR P0487, initial entry
//	01-10-91 ras - SCR P0487, add output of FF control
//				  add output of raster data
//	01-12-91 ras - SCR P0487, add output of text data
//
#define INCL_DOS
#include <os2.h>				// OS/2 definitions
#include <stdio.h>				// Standard I/O defines
#include <stdlib.h>				// Standard Lib defines
#include "bytypes.h"				// Byers definitions
#include "bikon.h"				// Byers IKON defines

// verify compiled using LARGE model
#ifndef	M_I86LM
#error *** MUST COMPILE IN LARGE MODEL ONLY
#endif

// define local function prototypes

// define local statics
static	int		L_nHandle;		// BYSCAN$$ handle
static	IK_CFG_PARM	L_IkCfgP;		// Ikon Cfg Parameters
static	IK_REQ		L_IkReq;		// IoCTL struct
static	char		L_acRaster[16384];	// raster test data
static	char		L_acText[]		// text test data
			= { "This is a test of the output\n"
			    "of multiple lines of text\n"
			    "through the Ikon driver. \n"
			  };

//**************************************************************************
//
//	Main module
//
//**************************************************************************

// start function
int	main( argc, argv )

  int	argc;
  char *argv[];

  {	// define automatics
	int		nNdx;			// loop control
	int		nAction;		// DosOpen action
	int		nStatus;		// 	   status

	// setup buffer for write raster for vertical bars
	for ( nNdx = 0; nNdx < sizeof(L_acRaster); nNdx++ )
	{
	    // set data to 0x0f
	    L_acRaster[nNdx] = 0x0f;
	}

	// attempt to open device
	printf( "\nBYIKON$$ open attempted" );
	nStatus = DosOpen( "BYIKON$$",
			   &L_nHandle,
			   &nAction,
			   0L,
			   0,
			   FILE_OPEN,
			   OPEN_ACCESS_READWRITE
			   | OPEN_SHARE_DENYREADWRITE
			   | OPEN_FLAGS_FAIL_ON_ERROR
			   | OPEN_FLAGS_WRITE_THROUGH,
			   0L );
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to open BYIKON$$, error of %d", nStatus );
	    exit( 1 );
	}

	// setup structure for driver INIT QUERY access
	L_IkReq.nOpCode  = IK_INIT_QUERY;
	L_IkReq.qBufSize = sizeof(L_IkCfgP);
	L_IkReq.fpcBuf   = &L_IkCfgP;

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ INIT QUERY attempted" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to INIT QUERY IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// display data we got
	printf( "\nResults from IOCTL INIT Query" );
	printf( "\n  Status        %d",		L_IkReq.nStatus		  );
	printf( "\n  Flags         %d",		L_IkReq.nFlags		  );
	printf( "\n  Transfer size %ld",	L_IkReq.qBufUsed	  );
	printf( "\n  IoPort        %04x",	L_IkCfgP.nIoPort 	  );
	printf( "\n  Interrupt     %04x",	L_IkCfgP.nInterrupt   	  );
	printf( "\n  Dma           %04x",	L_IkCfgP.nDma	 	  );

	// force I/O addr to 0x0320
	L_IkCfgP.nIoPort = 0x0320;

	// setup structure for driver INIT SET access
	L_IkReq.nOpCode  = IK_INIT_SET;
	L_IkReq.qBufSize = sizeof(L_IkCfgP);
	L_IkReq.fpcBuf   = &L_IkCfgP;

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ INIT SET attempted" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to INIT SET IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// setup structure for driver WRITE RASTER access
	L_IkReq.nOpCode  	= IK_WRITE_RASTER;
	L_IkReq.nFlags	 	= 0;
	L_IkReq.qTimeout	= 120000L;
	L_IkReq.qBufSize 	= sizeof(L_acRaster);
	L_IkReq.nBytesPerLine 	= 64;
	L_IkReq.fpcBuf   	= &L_acRaster[0];

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ WRITE RASTER started" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to WRITE RASTER IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// setup structure for driver WRITE MOVE access
	L_IkReq.nOpCode  	= IK_WRITE_MOVE;
	L_IkReq.nFlags	 	= 0;
	L_IkReq.qTimeout	= 120000L;
	L_IkReq.qBufSize 	= 0;
	L_IkReq.nBytesPerLine 	= 100;
	L_IkReq.fpcBuf   	= NULL;

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ WRITE MOVE started" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to WRITE MOVE IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// setup structure for driver WRITE TEXT access
	L_IkReq.nOpCode  	= IK_WRITE_TEXT;
	L_IkReq.nFlags	 	= 0;
	L_IkReq.qTimeout	= 120000L;
	L_IkReq.qBufSize 	= sizeof(L_acText);
	L_IkReq.nBytesPerLine 	= 0;
	L_IkReq.fpcBuf   	= &L_acText[0];

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ WRITE TEXT started" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to WRITE TEXT IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// setup structure for driver WRITE RASTER access
	L_IkReq.nOpCode  	= IK_WRITE_RASTER;
	L_IkReq.nFlags	 	= 0;
	L_IkReq.qTimeout	= 120000L;
	L_IkReq.qBufSize 	= sizeof(L_acRaster);
	L_IkReq.nBytesPerLine 	= 64;
	L_IkReq.fpcBuf   	= &L_acRaster[0];

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ WRITE RASTER started" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to WRITE RASTER IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// setup structure for driver WRITE MOVE access
	L_IkReq.nOpCode  	= IK_WRITE_MOVE;
	L_IkReq.nFlags	 	= 0;
	L_IkReq.qTimeout	= 120000L;
	L_IkReq.qBufSize 	= 0;
	L_IkReq.nBytesPerLine 	= 200;
	L_IkReq.fpcBuf   	= NULL;

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ WRITE MOVE started" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to WRITE MOVE IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// setup structure for driver WRITE TEXT access
	L_IkReq.nOpCode  	= IK_WRITE_TEXT;
	L_IkReq.nFlags	 	= 0;
	L_IkReq.qTimeout	= 120000L;
	L_IkReq.qBufSize 	= sizeof(L_acText);
	L_IkReq.nBytesPerLine 	= 0;
	L_IkReq.fpcBuf   	= &L_acText[0];

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ WRITE TEXT started" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to WRITE TEXT IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// setup structure for driver WRITE CONTROLS access
	L_IkReq.nOpCode  = IK_WRITE_CONTROLS;
	L_IkReq.nFlags	 = 0;
	L_IkReq.nFlags	 = IK_FLAG_C_FF;
	L_IkReq.qBufSize = 0;
	L_IkReq.fpcBuf   = NULL;

	// perform DosDevIOCtl()
	printf( "\nBYIKON$$ WRITE CONTROLS started" );
	nStatus = DosDevIOCtl(  NULL,
				&L_IkReq,
				0,
				128,
				L_nHandle );

	// check IoCtl failed
	if ( nStatus != 0 )
	{   // report error
	    printf( "\nFailed to WRITE CONTROLS IOCTL, error of %d", nStatus );
	    exit( 1 );
	}

	// close the device
	DosClose( L_nHandle );

	// return to caller
	return( 0 );
  }
