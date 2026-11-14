# BYIKON - Byers IKON 10092 Plotter Driver for OS/2

**Device:** BYIKON$$  
**Version:** 0.0.1  
**Date:** 14 Nov 2026  
**License:** BSD 3-Clause

## Overview

This is an OS/2 16-bit character device driver for the Byers IKON 10092 series of large-format raster plotters. The driver is written in C with an ASM stub and is compatible with Open Watcom 2.

## Features

- Character device interface (BYIKON$$)
- Single-open, level 2 request packet handling
- Default I/O base: 0x0300, IRQ: 15
- Supports raster data, text output, and control commands
- Large model compilation (required for far pointers)

## Requirements

- OS/2 2.x or later
- Open Watcom 2.x (WATCOM 2.0.1 verified)
- Byers IKON 10092 plotter hardware (or equivalent adaptor)

## Building

```cmd
compile_wat.cmd
```

Or manually:

```cmd
wmake -f Makefile.wat
```

Output: `bin\bikon.sys`

## Installation

Add to CONFIG.SYS:

```
DEVINFO=KBD,DEVICE,DEVICEINFO
DEVICE=BYIKON.SYS
```

**Note:** Test on a VM first. The driver will fail gracefully if no IKON adaptor is detected (returns error 2003: ERR_IK_NO_ADAPTOR).

## Testing

Since no IKON hardware is available in the VM, test by:

1. Installing the driver (without hardware)
2. Checking for the expected error message in log
3. Verifying BLDLEVEL information

## Boot Messages

The driver supports the following options:

- `/V` - Verbose mode (version + build date, hardware status)
- `/Q` - Quiet mode (no output)
- `/W` - Wait for keypress before continuing
- `/AA` - Announce with audible beep

All boot messages are prefixed with `BYIKON:` and wrapped at 79 columns.

## Project Structure

```
DRV-PRINTER-CDriver/
├── Makefile.wat      - Open Watcom 2 build file
├── compile_wat.cmd   - Build script with logging
├── README.md         - This file
├── LICENSE           - BSD 3-Clause license
├── src/              - Source code
│   ├── BIKONC.C      - Main driver implementation
│   ├── BIKONA.ASM    - Assembly stub (MASM syntax)
│   ├── BIKON.DEF     - Module definition (legacy)
│   └── include/      - Header files
│       ├── BIKON.H   - IKON-specific definitions
│       ├── BYTYPES.H - Basic types
│       └── CDRIVER.H - CDriver common definitions
├── doc/
│   ├── original/     - Original files preserved
│   ├── CHANGES.md    - Change log
│   └── PROVENANCE.md - License and origin
├── sample/           - Sample/test programs (TEST_IK.C)
├── test/             - Test scripts and utilities
├── bin/              - Build output
└── .gitignore        - Git ignore patterns
```

## Known Limitations

- No hardware testing performed (IKON 10092 adaptor not available in VM)
- DMA support not tested
- Assumes ISA-style port/I/O interface

## Related Projects

See IBMRA1 driver for reference implementation of boot messages and OS/2 driver patterns.

## Credits

Original driver (C) 1991 Byers Engineering Company.  
Ported to Open Watcom 2 by OS2World contributors.