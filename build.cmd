@echo off
rem Builds bin\QsysPropertyHandler.dll and bin\qsysver.exe (x64, static CRT).
rem Needs Visual Studio 2022 or its Build Tools with the "Desktop development with C++" workload.
setlocal
set ROOT=%~dp0
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VSWHERE%" (echo vswhere.exe not found - install VS 2022 Build Tools. & exit /b 1)
for /f "usebackq delims=" %%i in (`call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSDIR=%%i
if not defined VSDIR (echo No MSVC x64 toolset found. & exit /b 1)
rem vcvars64 itself looks vswhere up on PATH.
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

if not exist "%ROOT%bin" mkdir "%ROOT%bin"
if not exist "%ROOT%obj" mkdir "%ROOT%obj"

rem A registered handler stays loaded in Explorer, which locks the DLL. A loaded DLL
rem can still be renamed, so move it aside and link a fresh one in its place.
if exist "%ROOT%bin\QsysPropertyHandler.dll" (
    del /q "%ROOT%obj\*.old" >nul 2>&1
    move /y "%ROOT%bin\QsysPropertyHandler.dll" "%ROOT%obj\QsysPropertyHandler.%RANDOM%.old" >nul
)

set CFLAGS=/nologo /W4 /O2 /MT /EHsc /std:c++17 /DUNICODE /D_UNICODE /Fo"%ROOT%obj\\"

rc /nologo /fo "%ROOT%obj\QsysPropertyHandler.res" "%ROOT%src\QsysPropertyHandler.rc" || exit /b 1
rc /nologo /fo "%ROOT%obj\qsysver.res" "%ROOT%src\qsysver.rc" || exit /b 1

cl %CFLAGS% /LD "%ROOT%src\QsysPropertyHandler.cpp" "%ROOT%obj\QsysPropertyHandler.res" /Fe"%ROOT%bin\QsysPropertyHandler.dll" ^
   /link /DEF:"%ROOT%src\QsysPropertyHandler.def" ole32.lib advapi32.lib shell32.lib || exit /b 1
cl %CFLAGS% "%ROOT%src\qsysver.cpp" "%ROOT%obj\qsysver.res" /Fe"%ROOT%bin\qsysver.exe" || exit /b 1
del /q "%ROOT%bin\*.lib" "%ROOT%bin\*.exp" 2>nul
echo Built %ROOT%bin
