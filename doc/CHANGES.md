# BYIKON Driver Changes

## Version 0.0.1 (14 Nov 2026) - Initial Open Watcom 2 Port

### Step 1: Build the driver with Open Watcom 2 (no logic change)

**Files changed:**
- `src/BIKONA.ASM` - Changed all `PUBLIC` symbols to uppercase to match C compiler symbol naming
- `src/BIKONC.C` - Added `-zc` flag to preserve case (in Makefile.wat)

**Build modifications:**
- Created `Makefile.wat` with Open Watcom 2 flags
- Added `compile_wat.cmd` with environment setup
- Created `src/bldlevel.lnk` with version info
- Removed `option def=` directive (not supported by wlink for this use case)

**Binary produced:**
- `bin/bikon.sys` - 4900 bytes
- NE format 16-bit OS/2 character device driver
- BLDLEVEL: OS2World:0.0.1

### Notes

The original driver was written for Microsoft C. Porting to Open Watcom 2 required:

1. **Large model (`-ml`)** - Required because the original used MSC `-AL` (large model). Small model compilation gave `E1091 *** MUST COMPILE IN LARGE MODEL ONLY`

2. **Symbol case** - Watcom's default symbol mangling didn't match MASM's `PUBLIC` declarations. Added `-zc` flag to preserve case.

3. **Symbol names** - Changed assembly `public` names from mixed case to uppercase to match Watcom's default output naming.

4. **Link script** - The original used a DEF file which wlink handled differently. Switched to inline link commands with bldlevel.lnk included.

5. **Header includes** - Updated include paths to use Watcom's OS/2 1.x headers (`%WATCOM%\h\os21x`)