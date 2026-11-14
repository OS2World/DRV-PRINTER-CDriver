@echo off
set WATCOM=C:\WATCOM
set PATH=C:\WATCOM\BINP;C:\WATCOM\BINW;%PATH%
set INCLUDE=
cd D:\Projects\Drivers\DRV-PRINTER-CDriver\test
wcc -bt=os2 -ml test_bikon.c
if errorlevel 1 goto failed
echo Test program compiled: test_bikon.exe
goto end

:failed
echo Compilation failed

:end