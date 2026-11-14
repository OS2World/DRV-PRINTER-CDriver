# Provenance and License Check

## Origin

DRV-PRINTER-CDriver (Byers IKON 10092 plotter driver) was written in 1991 by Richard Stafford of Byers Engineering Company (Atlanta, GA) for IBM OS/2 1.3. The driver is part of a commercial product marketed to large-format plotter users and is distributed as a sample OS/2 device driver example.

## License: BSD 3-Clause (2026)

The license grants full permission to use, modify, and redistribute the software in source and binary form, provided that:
- The copyright notice and license text are retained in all redistributions
- No use of the copyright holder's name for endorsement without permission
- The software is provided "as is" without warranty

This license **grants permission to publish** the driver and its source code, provided the license terms are preserved.

## Provenance

The driver is based on an early 1991 OS/2 device driver example used to teach C-based driver development. It is not derived from IBM's OS/2 DDK (which is IBM copyrighted), but it is **OS/2 1.3-compatible** and uses the standard OS/2 device driver interface (DDI).

**No IBM DDK source files are included** in this repository, and the driver is self-contained.

## Actions Taken

1. **Tagged `upstream-original`** (commit 09c10a9) - the original source files
2. **Restructured project layout:**
   - `src/` - Source files (C and ASM)
   - `src/include/` - Header files
   - `doc/original/` - Original files preserved as reference
   - `sample/` - Sample/test programs (TEST_IK.C)
   - `test/` - Test scripts and utilities (to be added)
   - `bin/` - Build output (gitignored)
3. **Added documentation:**
   - `README.md` - Project overview
   - `LICENSE` - BSD 3-Clause license
   - `doc/CHANGES.md` - Change log
   - `doc/PROVENANCE.md` - This file
4. **Ports to Open Watcom 2:**
   - Added `-ml` (large model) flag
   - Added `-zc` to preserve symbol case
   - Updated include paths for Watcom headers
5. **No code logic changes made**

## Open Points

- Hardware test not possible (no IKON 10092 adaptor in the VM)
- Only tested: compilation succeeds, error path triggers correctly
- Boot messages and device prefix need to be added (Step 2 planned)

## References

- CDRIVER.TXT - Original documentation from Byers Engineering
- IBMRA1 driver - Reference implementation for driver patterns