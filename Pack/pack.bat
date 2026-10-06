@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem ============================================================
rem  YoyoScreenshot pack: sync from Exec, then run Inno Setup ISCC
rem  1) Build:  ./build_vs.sh Release x64   or  ./build_mingw.sh Release x64
rem  2) Pack:   pack.bat [version]
rem  Output:    Output\YoyoScreenshot_<version>_x64_Setup.exe
rem ============================================================

for %%I in ("%~dp0..") do set "REPO=%%~fI"
set "SRC=%REPO%\Exec\Release\x64\YoyoScreenshot"
set "DST=%~dp0"

if not exist "%SRC%\YoyoScreenshot.exe" (
  echo [ERROR] Missing "%SRC%\YoyoScreenshot.exe"
  echo         Run: ./build_vs.sh Release x64
  echo           or: ./build_mingw.sh Release x64
  exit /b 1
)

echo [1/3] Sync from Exec ...
if not exist "%DST%resources" mkdir "%DST%resources"
copy /Y "%SRC%\YoyoScreenshot.exe" "%DST%" >nul
if exist "%SRC%\icon.ico" copy /Y "%SRC%\icon.ico" "%DST%" >nul
if exist "%REPO%\resources\icon.ico" copy /Y "%REPO%\resources\icon.ico" "%DST%" >nul
if exist "%SRC%\resources\icon.ico" copy /Y "%SRC%\resources\icon.ico" "%DST%resources\" >nul
if exist "%REPO%\resources\icon.ico" copy /Y "%REPO%\resources\icon.ico" "%DST%resources\" >nul
for %%F in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll libatomic-1.dll libgomp-1.dll libssp-0.dll libXCGUI.dll XCGUI.dll) do (
  if exist "%SRC%\%%F" copy /Y "%SRC%\%%F" "%DST%" >nul
)

echo [2/3] Find ISCC ...
set "ISCC="
if defined ISCC_PATH if exist "%ISCC_PATH%" set "ISCC=%ISCC_PATH%"
if "%ISCC%"=="" if defined INNO_SETUP if exist "%INNO_SETUP%\ISCC.exe" set "ISCC=%INNO_SETUP%\ISCC.exe"
if "%ISCC%"=="" if exist "%ProgramFiles%\Inno Setup 7\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 7\ISCC.exe"
if "%ISCC%"=="" if exist "%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe"
if "%ISCC%"=="" if exist "%LOCALAPPDATA%\Programs\Inno Setup 7\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 7\ISCC.exe"
if "%ISCC%"=="" if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if "%ISCC%"=="" if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if "%ISCC%"=="" if exist "%ProgramFiles(x86)%\Inno Setup 5\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 5\ISCC.exe"
if "%ISCC%"=="" if exist "%ProgramFiles%\Inno Setup 5\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 5\ISCC.exe"
if "%ISCC%"=="" if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if "%ISCC%"=="" if exist "C:\Inno Setup 6\ISCC.exe" set "ISCC=C:\Inno Setup 6\ISCC.exe"
if "%ISCC%"=="" if exist "D:\Inno Setup 6\ISCC.exe" set "ISCC=D:\Inno Setup 6\ISCC.exe"
if not "%ISCC%"=="" goto :have_iscc

where ISCC.exe >nul 2>&1
if errorlevel 1 goto :no_iscc
for /f "delims=" %%I in ('where ISCC.exe 2^>nul') do (
  set "ISCC=%%I"
  goto :have_iscc
)

:no_iscc
echo [ERROR] ISCC.exe not found.
echo         Install Inno Setup: https://jrsoftware.org/isinfo.php
echo         Or set INNO_SETUP to the install folder
echo         Or set ISCC_PATH to full path of ISCC.exe
exit /b 2

:have_iscc
echo       ISCC=%ISCC%

set "APP_VER=%~1"
if "%APP_VER%"=="" set "APP_VER=1.0.0"
if /I "%APP_VER:~0,1%"=="v" set "APP_VER=%APP_VER:~1%"

echo [3/3] Compile YoyoScreenshot.iss (version %APP_VER%) ...
"%ISCC%" "/DMyAppVersion=%APP_VER%" "%DST%YoyoScreenshot.iss"
if errorlevel 1 (
  echo [ERROR] ISCC failed.
  exit /b 3
)

echo.
echo Build OK: "%DST%Output\YoyoScreenshot_%APP_VER%_x64_Setup.exe"
exit /b 0
