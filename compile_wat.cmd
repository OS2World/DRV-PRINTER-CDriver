@echo off
rem ---------------------------------------------------------------------------
rem compile_wat.cmd - build BYIKON.SYS with Open Watcom 2
rem Usage:  compile_wat.cmd [clean]
rem Output: bin\bikon.sys, bin\bikon.map
rem Log:    compile.log
rem ---------------------------------------------------------------------------
if "%1"=="clean" goto :clean

echo ===== Building BYIKON.SYS with Open Watcom 2 =====

rem Set up Watcom environment
set WATCOM=C:\WATCOM
set PATH=C:\WATCOM\BINP;C:\WATCOM\BINW;%PATH%
set INCLUDE=
set EDPATH=C:\WATCOM\EDDAT

wmake -h -f Makefile.wat
if errorlevel 1 goto :failed

if not exist bin\bikon.sys goto :failed

echo.
dir bin\bikon.sys
echo.
echo ===== BUILD OK =====
goto :end

:clean
if exist compile.log del compile.log
if exist bin rmdir /s /q bin
echo ===== clean done =====
goto :end

:failed
echo ===== BUILD FAILED =====
dir /s bin 2>nul
echo compile.log (if exists):
type compile.log 2>nul
goto :end

:end