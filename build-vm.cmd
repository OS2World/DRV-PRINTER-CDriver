@echo off
set WATCOM=C:\WATCOM
set PATH=C:\WATCOM\BINP;C:\WATCOM\BINW;%PATH%
set INCLUDE=
set EDPATH=C:\WATCOM\EDDAT
cd D:\Projects\Drivers\DRV-PRINTER-CDriver
wmake -h -f Makefile.wat