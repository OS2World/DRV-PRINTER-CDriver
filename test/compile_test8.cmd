@echo off
set WATCOM=C:\WATCOM
set PATH=C:\WATCOM\BINP;C:\WATCOM\BINW;%PATH%
set INCLUDE=C:\WATCOM\h;C:\WATCOM\h\os21x
D:
cd \Projects\Drivers\DRV-PRINTER-CDriver\test
echo Current dir:
cd
wcc -bt=os2 -ml test_bikon.c
if errorlevel 1 goto failed
wlink system os2v2 file test_bikon.obj name test_bikon.exe lib clib.lib lib os2386.lib option stack=4096
if errorlevel 1 goto failed
echo Test program compiled: test_bikon.exe
goto end

:failed
echo Compilation failed

:end