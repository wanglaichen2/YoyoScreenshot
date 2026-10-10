@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

rem YoyoScreenshot MSIX pack
rem Usage: pack_msix.bat [1.0.4.0]
rem Output: Output\YouYouJieTu_<ver>_x64.msix

for %%I in ("%~dp0..") do set "REPO=%%~fI"
set "SRC=%REPO%\Exec\Release\x64\YoyoScreenshot"
set "DST=%~dp0"
set "MSIXTPL=%DST%msix"
set "LAYOUT=%DST%msix_layout"
set "OUTDIR=%DST%Output"
set "APP_NAME=YouYouJieTu"
set "APP_VER=%~1"
if "%APP_VER%"=="" set "APP_VER=1.0.4.0"
if /I "%APP_VER:~0,1%"=="v" set "APP_VER=%APP_VER:~1%"

echo %APP_VER%| findstr /R "^[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*$" >nul
if errorlevel 1 (
  echo [ERROR] Version must be like 1.0.4.0  got %APP_VER%
  exit /b 1
)

set "MSIX=%OUTDIR%\%APP_NAME%_%APP_VER%_x64.msix"
set "PFX=%OUTDIR%\YouYouJieTu_Test.pfx"
set "PFX_PASS=YouYouJieTuTest"

if not exist "%SRC%\YoyoScreenshot.exe" (
  echo [ERROR] Missing "%SRC%\YoyoScreenshot.exe"
  echo         Run build_vs.sh Release x64 first
  exit /b 1
)
if not exist "%MSIXTPL%\AppxManifest.xml" (
  echo [ERROR] Missing "%MSIXTPL%\AppxManifest.xml"
  exit /b 1
)

echo [1/6] Sync from Exec ...
if not exist "%DST%resources" mkdir "%DST%resources"
copy /Y "%SRC%\YoyoScreenshot.exe" "%DST%" >nul
if exist "%SRC%\icon.ico" copy /Y "%SRC%\icon.ico" "%DST%" >nul
if exist "%REPO%\resources\icon.ico" copy /Y "%REPO%\resources\icon.ico" "%DST%" >nul
if exist "%SRC%\resources\icon.ico" copy /Y "%SRC%\resources\icon.ico" "%DST%resources\" >nul
if exist "%REPO%\resources\icon.ico" copy /Y "%REPO%\resources\icon.ico" "%DST%resources\" >nul
for %%F in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll libatomic-1.dll libgomp-1.dll libssp-0.dll libXCGUI.dll XCGUI.dll) do (
  if exist "%SRC%\%%F" copy /Y "%SRC%\%%F" "%DST%" >nul
)
if exist "%SRC%\Lang" xcopy /E /I /Y /Q "%SRC%\Lang" "%DST%Lang\" >nul
if exist "%REPO%\Lang" xcopy /E /I /Y /Q "%REPO%\Lang" "%DST%Lang\" >nul

echo [2/6] Build msix_layout ...
if exist "%LAYOUT%" rmdir /s /q "%LAYOUT%"
mkdir "%LAYOUT%"
mkdir "%LAYOUT%\Assets"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

copy /Y "%MSIXTPL%\AppxManifest.xml" "%LAYOUT%\AppxManifest.xml" >nul
powershell -NoProfile -ExecutionPolicy Bypass -File "%DST%msix\stamp_version.ps1" -ManifestPath "%LAYOUT%\AppxManifest.xml" -Version "%APP_VER%"
if errorlevel 1 (
  echo [ERROR] Failed to stamp Version=%APP_VER%
  exit /b 1
)
echo       Generate localized Start Menu strings ...
powershell -NoProfile -ExecutionPolicy Bypass -File "%DST%msix\gen_localized_strings.ps1" -OutDir "%LAYOUT%\Strings"
if errorlevel 1 (
  echo [ERROR] Failed to generate Strings\*\Resources.resw
  exit /b 1
)
xcopy /Y /Q "%MSIXTPL%\Assets\*" "%LAYOUT%\Assets\" >nul
copy /Y "%DST%YoyoScreenshot.exe" "%LAYOUT%\" >nul
if exist "%DST%icon.ico" copy /Y "%DST%icon.ico" "%LAYOUT%\" >nul
if exist "%DST%resources" xcopy /E /I /Y /Q "%DST%resources" "%LAYOUT%\resources\" >nul
if exist "%DST%Lang" xcopy /E /I /Y /Q "%DST%Lang" "%LAYOUT%\Lang\" >nul
for %%F in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll libatomic-1.dll libgomp-1.dll libssp-0.dll libXCGUI.dll XCGUI.dll) do (
  if exist "%DST%%%F" copy /Y "%DST%%%F" "%LAYOUT%\" >nul
)

echo [3/6] Find MakeAppx / MakePri / SignTool ...
set "MAKEAPPX="
set "MAKEPRI="
set "SIGNTOOL="
for %%V in (10.0.26100.0 10.0.22621.0 10.0.22000.0 10.0.19041.0) do (
  if "!MAKEAPPX!"=="" if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\MakeAppx.exe" set "MAKEAPPX=%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\MakeAppx.exe"
  if "!MAKEPRI!"=="" if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\MakePri.exe" set "MAKEPRI=%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\MakePri.exe"
  if "!SIGNTOOL!"=="" if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\SignTool.exe" set "SIGNTOOL=%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\SignTool.exe"
)
if "%MAKEAPPX%"=="" (
  where MakeAppx.exe >nul 2>&1
  if not errorlevel 1 (
    for /f "delims=" %%I in ('where MakeAppx.exe 2^>nul') do (
      set "MAKEAPPX=%%I"
      goto :have_makeappx
    )
  )
)
:have_makeappx
if "%MAKEAPPX%"=="" (
  echo [ERROR] MakeAppx.exe not found. Install Windows SDK.
  exit /b 2
)
echo       MAKEAPPX=%MAKEAPPX%
if not "%MAKEPRI%"=="" echo       MAKEPRI=%MAKEPRI%
if not "%SIGNTOOL%"=="" echo       SIGNTOOL=%SIGNTOOL%

echo [4/6] MakePri resources.pri (localized names + unplated icons) ...
if "%MAKEPRI%"=="" (
  echo [ERROR] MakePri.exe not found. Localized Start Menu names require MakePri / Windows SDK.
  exit /b 2
)
"%MAKEPRI%" createconfig /cf "%LAYOUT%\priconfig.xml" /dq lang-zh-CN_scale-100_contrast-standard /o >nul
if errorlevel 1 (
  echo [ERROR] MakePri createconfig failed.
  exit /b 2
)
"%MAKEPRI%" new /pr "%LAYOUT%" /cf "%LAYOUT%\priconfig.xml" /mn "%LAYOUT%\AppxManifest.xml" /of "%LAYOUT%\resources.pri" /o
if errorlevel 1 (
  echo [ERROR] MakePri new failed - cannot build localized DisplayName package.
  if exist "%LAYOUT%\priconfig.xml" del /f /q "%LAYOUT%\priconfig.xml"
  exit /b 2
)
echo       resources.pri OK
if exist "%LAYOUT%\priconfig.xml" del /f /q "%LAYOUT%\priconfig.xml"

echo [5/6] MakeAppx pack ...
"%MAKEAPPX%" pack /d "%LAYOUT%" /p "%MSIX%" /o
if errorlevel 1 (
  echo [ERROR] MakeAppx failed.
  exit /b 3
)

echo [6/6] Sign with local test cert ...
if "%SIGNTOOL%"=="" (
  echo       SignTool not found - skip sign.
  goto :done
)

if not exist "%PFX%" (
  echo       Creating self-signed test PFX ...
  powershell -NoProfile -ExecutionPolicy Bypass -File "%DST%msix\make_test_pfx.ps1" -OutPfx "%PFX%" -Password "%PFX_PASS%"
  if errorlevel 1 (
    echo [WARN] Failed to create test PFX - leave unsigned.
    goto :done
  )
)

"%SIGNTOOL%" sign /fd SHA256 /a /f "%PFX%" /p "%PFX_PASS%" "%MSIX%"
if errorlevel 1 (
  echo [WARN] Sign failed - unsigned package still present.
  goto :done
)
echo       Signed OK.

:done
echo.
echo Build OK: "%MSIX%"
echo.
echo Install sideload:
echo   Add-AppxPackage -Path "%MSIX%"
echo.
echo Note: Partner Center requires a unique package full name; bump Version when re-uploading.
exit /b 0
