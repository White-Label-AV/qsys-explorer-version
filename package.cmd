@echo off
rem Builds, tests and zips a release: dist\QsysPropertyHandler-<version>-x64.zip (+ .sha256).
rem The version comes from src\version.h.
setlocal
set ROOT=%~dp0

call "%ROOT%build.cmd" || exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%tests\run-tests.ps1" || (echo Tests failed; not packaging. & exit /b 1)

for /f "tokens=3" %%v in ('findstr /c:"#define QPH_VERSION_MAJOR" "%ROOT%src\version.h"') do set MAJ=%%v
for /f "tokens=3" %%v in ('findstr /c:"#define QPH_VERSION_MINOR" "%ROOT%src\version.h"') do set MIN=%%v
for /f "tokens=3" %%v in ('findstr /c:"#define QPH_VERSION_PATCH" "%ROOT%src\version.h"') do set PAT=%%v
set NAME=QsysPropertyHandler-%MAJ%.%MIN%.%PAT%-x64
set STAGE=%ROOT%obj\%NAME%

if exist "%STAGE%" rd /s /q "%STAGE%"
mkdir "%STAGE%"
for %%f in (bin\QsysPropertyHandler.dll bin\qsysver.exe install.cmd uninstall.cmd README.md CHANGELOG.md LICENSE) do (
    copy /y "%ROOT%%%f" "%STAGE%\" >nul || (echo Missing %%f & exit /b 1)
)

if not exist "%ROOT%dist" mkdir "%ROOT%dist"
powershell -NoProfile -Command ^
  "$z='%ROOT%dist\%NAME%.zip'; Compress-Archive -Path '%STAGE%\*' -DestinationPath $z -Force;" ^
  "$h=(Get-FileHash $z -Algorithm SHA256).Hash.ToLower(); Set-Content -Encoding ascii ($z+'.sha256') ($h+'  %NAME%.zip'); Write-Host $h" || exit /b 1
echo Packaged %ROOT%dist\%NAME%.zip
