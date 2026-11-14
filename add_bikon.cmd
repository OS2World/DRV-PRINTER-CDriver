@echo off
rem Add BYIKON.SYS to CONFIG.SYS before the LAST section
find "REM [LAST section" < C:\CONFIG.SYS > C:\CONFIG.SYS.new
rem This is complex, let's just append to the file before the LAST section
copy C:\CONFIG.SYS C:\CONFIG.SYS.tmp
echo DEVICE=C:\OS2\BOOT\BYIKON.SYS /V >> C:\CONFIG.SYS