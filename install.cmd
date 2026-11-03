@echo off
rem Installs the Q-SYS Design property handler to Program Files and registers it.
rem Upgrades in place. Elevates itself if needed.
rem   install.cmd          interactive: offers to restart Explorer
rem   install.cmd /quiet   no prompts, no Explorer restart
setlocal
net session >nul 2>&1 || (
    powershell -NoProfile -Command "Start-Process -FilePath $env:ComSpec -ArgumentList '/c \"\"%~f0\" %*\"' -Verb RunAs"
    exit /b
)

set "DEST=%ProgramFiles%\Q-SYS Design Property Handler"
rem Release zip: files sit next to this script. Source tree: they are in bin\.
set "SRC=%~dp0"
if not exist "%SRC%QsysPropertyHandler.dll" set "SRC=%~dp0bin\"
if not exist "%SRC%QsysPropertyHandler.dll" (echo QsysPropertyHandler.dll not found next to install.cmd or in bin\. & goto fail)

if not exist "%DEST%" mkdir "%DEST%" || goto fail
rem An installed copy may be loaded in Explorer; it cannot be overwritten but it can be renamed.
del /q "%DEST%\*.old" >nul 2>&1
if exist "%DEST%\QsysPropertyHandler.dll" move /y "%DEST%\QsysPropertyHandler.dll" "%DEST%\QsysPropertyHandler.%RANDOM%.old" >nul
copy /y "%SRC%QsysPropertyHandler.dll" "%DEST%\" >nul || goto fail
if exist "%SRC%qsysver.exe" copy /y "%SRC%qsysver.exe" "%DEST%\" >nul
copy /y "%~dp0uninstall.cmd" "%DEST%\" >nul

regsvr32 /s "%DEST%\QsysPropertyHandler.dll" || (echo regsvr32 failed. & goto fail)
echo Installed to "%DEST%".

if /i "%~1"=="/quiet" exit /b 0
choice /m "Restart Explorer now so open windows pick it up"
if not errorlevel 2 (taskkill /f /im explorer.exe >nul & start explorer.exe)
pause
exit /b 0

:fail
if /i not "%~1"=="/quiet" pause
exit /b 1
