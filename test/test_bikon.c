/* test_bikon.c - Simple test program for BYIKON.SYS driver
 *
 * This program opens the BYIKON$$ device and attempts to query
 * the IKON adaptor configuration via IOCTL.
 */

#define INCL_DOS
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#include <os2.h>
#include <stdio.h>
#include <string.h>

/* IKON IOCTL definitions (from bikon.h) */
#define IK_CATEGORY     128
#define IK_FUNCTION     0

#define IK_INIT_QUERY   0x0001
#define IK_INIT_SET     0x0002
#define IK_WRITE_RASTER 0x0003
#define IK_WRITE_TEXT   0x0004
#define IK_WRITE_MOVE   0x0005
#define IK_WRITE_CONTROLS 0x0006

#define ERR_IK_NO_ADAPTOR      2003
#define ERR_IK_BAD_INTERRUPT   2004
#define ERR_IK_TIMEOUT         2005
#define ERR_IK_ALREADY_BUSY    2006
#define ERR_IK_BAD_PARM        2007

typedef struct {
    unsigned short nIoPort;
    unsigned short nInterrupt;
    unsigned short nDma;
} IK_CFG_PARM;

typedef struct {
    unsigned short nOpCode;
    unsigned long  qBufSize;
    unsigned long  qBufUsed;
    void far *     fpcBuf;
    unsigned long  qTimeout;
    unsigned short nStatus;
    unsigned short nFlags;
    unsigned short nBytesPerLine;
} IK_REQ;

int main(void) {
    HFILE hf;
    ULONG ulAction;
    APIRET rc;
    IK_REQ req;
    IK_CFG_PARM cfg;

    printf("BYIKON.SYS test program\n");
    printf("========================\n\n");

    /* Open the device */
    rc = DosOpen("BYIKON$$", &hf, &ulAction, 0, 0,
                 OPEN_ACTION_OPEN_IF_EXISTS,
                 OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE,
                 0);

    if (rc != 0) {
        printf("DosOpen failed: rc = %lu\n", rc);
        if (rc == ERROR_FILE_NOT_FOUND) {
            printf("Device BYIKON$$ not found. Is the driver loaded?\n");
        }
        return 1;
    }

    printf("Device opened successfully (handle = %u)\n", hf);

    /* Query IKON configuration */
    memset(&req, 0, sizeof(req));
    req.nOpCode = IK_INIT_QUERY;
    req.qBufSize = sizeof(cfg);
    req.fpcBuf = &cfg;

    rc = DosDevIOCTL(hf, IK_CATEGORY, IK_FUNCTION,
                     &req, sizeof(req), NULL,
                     NULL, 0, NULL);

    if (rc != 0) {
        printf("DosDevIOCTL failed: rc = %lu\n", rc);
    } else {
        printf("IOCTL returned: nStatus = %u\n", req.nStatus);
        if (req.nStatus == 0) {
            printf("IKON Configuration:\n");
            printf("  I/O Port:   0x%04X\n", cfg.nIoPort);
            printf("  Interrupt:  %u\n", cfg.nInterrupt);
            printf("  DMA:        %u\n", cfg.nDma);
        } else if (req.nStatus == ERR_IK_NO_ADAPTOR) {
            printf("No IKON adaptor detected (expected in VM)\n");
        } else {
            printf("Error code: %u\n", req.nStatus);
        }
    }

    /* Close the device */
    DosClose(hf);
    printf("\nTest complete.\n");
    return 0;
}