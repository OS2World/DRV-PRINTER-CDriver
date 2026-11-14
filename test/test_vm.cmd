@echo off
rem test_vm.cmd - Test script for BYIKON.SYS in VM
rem Run this in the OS/2 VM after installing the driver

echo =========================================
echo BYIKON.SYS Test Script
echo =========================================
echo.

echo 1. Checking if driver is loaded...
dd BYIKON$$

echo.
echo 2. Checking BLDLEVEL...
bldlevel \OS2\BOOT\BYIKON.SYS

echo.
echo 3. Running test program (if compiled)...
if exist test_bikon.exe (
    test_bikon.exe
) else (
    echo test_bikon.exe not found. Compile with:
    echo   wcc -bt=os2 -ml test\test_bikon.c
)

echo.
echo 4. Boot log check (last 20 lines)...
if exist \OS2\LOG\BOOT.LOG (
    tail -20 \OS2\LOG\BOOT.LOG | find "BYIKON"
) else (
    echo Boot log not found
)

echo.
echo =========================================
echo Test complete
echo =========================================