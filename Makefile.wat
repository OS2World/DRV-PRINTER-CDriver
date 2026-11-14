# Makefile.wat - build BYIKON.SYS (Byers IKON 10092 plotter driver) with Open Watcom 2
#
# Run from the project root:   wmake -f Makefile.wat
# or use compile_wat.cmd.
#
# Paths - change these to match your Watcom installation:
#   WATCOM   = C:\WATCOM   (or wherever Watcom 2 is installed)
#   HBLIB    = C:\WATCOM\lib286   (libraries)
#   HBLINC   = C:\WATCOM\h        (include files)
#   HBINC    = C:\WATCOM\h\os21x    (OS/2 1.x include files)
#
# If you use a different drive or directory, adjust the paths below.
#
NAME = bikon
OBJ  = bin
SRC  = src
INC  = src\include

# Watcom 2: the environment (WATCOM, PATH, INCLUDE) is set by compile_wat.cmd.
# If you run wmake by hand, set it first:
#     set WATCOM=C:\WATCOM
#     set PATH=C:\WATCOM\BINP;C:\WATCOM\BINW;%PATH%
#     set INCLUDE=

# Compiler and assembler flags
CC   = wcc
AS   = wasm
LINK = wlink

# Compiler flags - large model (ml), debug info (-d2), quiet (-q)
# -zc preserves case of external symbols (ASM uses mixed case)
CFLAGS = -bt=os2 -ml -5 -s -zdp -zff -zgf -zc -zl -zp1 -zq -d2 -d_PDD_ -i=$(INC) -i=$(%WATCOM)\h -i=$(%WATCOM)\h\os21x

# Assembler flags
ASFLAGS = -bt=os2 -d1 -zq

# Object files and headers
OBJS = $(OBJ)\bikona.obj $(OBJ)\bikonc.obj
HDRS = $(INC)\CDRIVER.H $(INC)\BYTYPES.H $(INC)\BIKON.H

all: $(OBJ)\$(NAME).sys .SYMBOLIC

$(OBJ)\$(NAME).sys: $(OBJS) $(SRC)\bldlevel.lnk
	@%create $(OBJ)\$(NAME).lnk
	@%append $(OBJ)\$(NAME).lnk system os2 dll initglobal
	@%append $(OBJ)\$(NAME).lnk option protmode
	@%append $(OBJ)\$(NAME).lnk name $(OBJ)\$(NAME).sys
	@%append $(OBJ)\$(NAME).lnk @$(SRC)\bldlevel.lnk
	@%append $(OBJ)\$(NAME).lnk option map=$(OBJ)\$(NAME).map
	@%append $(OBJ)\$(NAME).lnk option quiet
	@%append $(OBJ)\$(NAME).lnk library $(%WATCOM)\lib286\os2\os2.lib
	@%append $(OBJ)\$(NAME).lnk library $(%WATCOM)\lib286\os2\clibs.lib
	
	@%append $(OBJ)\$(NAME).lnk file $(OBJ)\bikona.obj
	@%append $(OBJ)\$(NAME).lnk file $(OBJ)\bikonc.obj
	wlink @$(OBJ)\$(NAME).lnk

$(OBJ)\bikona.obj: $(SRC)\BIKONA.ASM
	-@mkdir $(OBJ) > nul 2>&1
	$(AS) $(ASFLAGS) -fo=$@ $(SRC)\BIKONA.ASM

$(OBJ)\bikonc.obj: $(SRC)\BIKONC.C $(HDRS)
	-@mkdir $(OBJ) > nul 2>&1
	$(CC) $(CFLAGS) -fo=$@ $(SRC)\BIKONC.C

clean: .SYMBOLIC
	-@echo y | del $(OBJ)\*.* > nul 2>&1
	-@rmdir $(OBJ) > nul 2>&1
	-@del *.err > nul 2>&1