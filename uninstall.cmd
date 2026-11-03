@echo off
rem Unregisters the Q-SYS Design property handler and removes it from Program Files.
rem Elevates itself if needed.
rem   uninstall.cmd          interactive: offers to restart Explorer so the DLL can be deleted
rem   uninstall.cmd /quiet   no prompts, no Explorer restart (files are removed if not in use)
setlocal
net session >nul 2>&1 || (
    powershell -NoProfile -Command "Start-Process -FilePath $env:ComSpec -ArgumentList '/c \"\"%~f0\" %*\"' -Verb RunAs"
    exit /b
)

set "DEST=%ProgramFiles%\Q-SYS Design Property Handler"

rem install.cmd puts a copy of this script in DEST. cmd re-reads a running batch file,
rem so deleting DEST from under it fails; hand over to a temp copy instead. Invoking
rem a batch file without "call" never returns, so this file is not read again.
if /i "%~dp0"=="%DEST%\" (
    copy /y "%~f0" "%TEMP%\qsys-property-handler-uninstall.cmd" >nul
    "%TEMP%\qsys-property-handler-uninstall.cmd" %*
)

if exist "%DEST%\QsysPropertyHandler.dll" (
    regsvr32 /s /u "%DEST%\QsysPropertyHandler.dll" || echo regsvr32 /u failed; continuing.
) else (
    echo No installed copy found in "%DEST%".
)
echo Unregistered.

if /i not "%~1"=="/quiet" (
    choice /m "Restart Explorer now so the DLL is released"
    if not errorlevel 2 (taskkill /f /im explorer.exe >nul & start explorer.exe & timeout /t 2 /nobreak >nul)
)

cd /d "%TEMP%"
rd /s /q "%DEST%" 2>nul
if exist "%DEST%\QsysPropertyHandler.dll" (
    echo The DLL is still in use. Sign out and back in, then delete "%DEST%".
) else (
    echo Removed "%DEST%".
)
if /i not "%~1"=="/quiet" pause
exit /b 0
