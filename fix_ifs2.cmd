@echo off
echo IFS=C:\OS2Additions\VBoxFS.ifs /Q > C:\CONFIG.SYS.new
rem This is a simple approach - just recreate the config.sys with the fixed line
rem We'll do it step by step
echo REM [Logical Config.Sys Sort, by Doug Bissett > C:\CONFIG.SYS.new
echo REM [Program version: 082 >> C:\CONFIG.SYS.new
echo REM [Database version: database version = 1.26 >> C:\CONFIG.SYS.new
echo REM [Userbase version: Not used >> C:\CONFIG.SYS.new
echo REM [Config.Sys Created on 17 Feb 2025 at 21:52:35 >> C:\CONFIG.SYS.new
echo. >> C:\CONFIG.SYS.new
echo REM [FIRST section >> C:\CONFIG.SYS.new
echo DLLBASING=OFF >> C:\CONFIG.SYS.new
echo EARLYMEMINIT=TRUE >> C:\CONFIG.SYS.new
echo DEVICE=C:\OS2\BOOT\UNICODE.SYS >> C:\CONFIG.SYS.new
echo. >> C:\CONFIG.SYS.new
echo REM [IFS section >> C:\CONFIG.SYS.new
echo IFS=C:\OS2\JFS.IFS /LW:5,20,4 /AUTOCHECK:* >> C:\CONFIG.SYS.new
echo IFS=C:\OS2\HPFS.IFS /CACHE:2048 /CRECL:4 /AUTOCHECK:* >> C:\CONFIG.SYS.new
echo IFS=C:\OS2\FAT32.IFS /cache:2048 /h /q /ac:* /largefiles >> C:\CONFIG.SYS.new
echo IFS=C:\OS2\BOOT\UDF.IFS /Q >> C:\CONFIG.SYS.new
echo IFS=C:\OS2\BOOT\CDFS.IFS /Q /W >> C:\CONFIG.SYS.new
echo IFS=C:\sys\bin\STUBFSD.IFS ISOFS >> C:\CONFIG.SYS.new
echo IFS=C:\Programs\NDFS\NDFS32.IFS /Q >> C:\CONFIG.SYS.new
echo CALL=C:\OS2\CACHEF32.EXE /f /p:2 /m:50000 /b:250 /d:5000 /S >> C:\CONFIG.SYS.new
echo REM RUN=C:\sys\bin\ISOFSDMN.EXE >> C:\CONFIG.SYS.new
echo IFS=C:\OS2Additions\VBoxFS.ifs /Q >> C:\CONFIG.SYS.new
echo SET NDFSDIR=C:\Programs\NDFS >> C:\CONFIG.SYS.new