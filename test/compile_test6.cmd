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
echo Test program compiled: test_bikon.exe
goto end

:failed
echo Compilation failed

:end