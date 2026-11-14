@echo off
echo DEVICE=C:\OS2\BOOT\BYIKON.SYS /V > C:\CONFIG.SYS.new
find /v "BYIKON" < C:\CONFIG.SYS >> C:\CONFIG.SYS.new
copy C:\CONFIG.SYS.new C:\CONFIG.SYS
del C:\CONFIG.SYS.new