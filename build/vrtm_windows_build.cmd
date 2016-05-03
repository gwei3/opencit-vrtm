@echo off
setlocal enabledelayedexpansion

set me=%~n0
set pwd=%~dp0
set "vrtm_build_home=%pwd%"

REM set makensis="C:\Program Files (x86)\NSIS\\makensis.exe"
IF "%NSIS_HOME%"=="" (
  set "makensis=C:\Program Files (x86)\NSIS\makensis.exe"
) ELSE (
  set "makensis=%NSIS_HOME%\makensis.exe"
)

set vrtm_home=%pwd%\..\vrtm

IF "%1"=="" (
  call:print_help
) ELSE IF "%2"=="" (
  call:print_help
) ELSE (
  call:vrtm_install %1 %2
)
GOTO:EOF

:vrtm_install
  echo. Creating vrtm installer.... %1 %2
  call:thirdparty_build %1 %2
  call:vrtm_build %1 %2
  call:vrtm_install
GOTO:EOF

:thirdparty_build
  echo. Building third party libraries.... %1 %2
  cd
  mkdir "%vrtm_home%\bin"
  echo. "%vrtm_home%\third party\libxml2\bin\%1\%2\libxml2.dll"
  copy "%vrtm_home%\third party\libxml2\bin\%1\%2\libxml2.dll" "%vrtm_home%\bin" /y
  echo. "%vrtm_home%\third party\log4cpp\bin\%1\%2\log4cpp.dll"
  copy "%vrtm_home%\third party\log4cpp\bin\%1\%2\log4cpp.dll" "%vrtm_home%\bin" /y
  echo. "%vrtm_home%\third party\pthread\dll\%1\pthreadVC2.dll"
  copy "%vrtm_home%\third party\pthread\dll\%1\pthreadVC2.dll" "%vrtm_home%\bin" /y
  
  mkdir "%vrtm_home%\prerequisites"
  echo. "%vrtm_home%\third party\Ext2Fsd-0.62.exe"
  copy "%vrtm_home%\third party\Ext2Fsd-0.62.exe" "%vrtm_home%\prerequisites" /y
  echo. "%vrtm_home%\third party\vcredist\vs2010\vcredist_%1.exe"
  copy "%vrtm_home%\third party\vcredist\vs2010\vcredist_%1.exe" "%vrtm_home%\prerequisites\vcredist_10.exe" /y
  echo. "%vrtm_home%\third party\vcredist\vs2013\vcredist_%1.exe"
  copy "%vrtm_home%\third party\vcredist\vs2013\vcredist_%1.exe" "%vrtm_home%\prerequisites\vcredist_13.exe" /y
GOTO:EOF

:vrtm_build
  echo. Building vrtm.... %1 %2
  cd %vrtm_build_home%
  cd
  call "%vrtm_home%\src\vrtm_build.cmd" %1 %2
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: vRTM build failed
	call:ExitBatch
	REM EXIT /b %ERRORLEVEL%
  )
GOTO:EOF

:vrtm_install
  echo. Creating vrtm installer....
  cd %vrtm_build_home%
  cd  
  IF EXIST "%makensis%" (
	echo. "%makensis% exists"
	call "%makensis%" vrtm_installer.nsi
	) ELSE (
	echo. "Neither makensis.exe found at default location nor environment variable pointing to NSIS_HOME exists."
	echo. "If NSIS not installed please install it and add NSIS_HOME environment variable in system variables"
	call:ExitBatch
	REM EXIT /b 1
  )  
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: vRTM installer creation failed
	call:ExitBatch
	REM EXIT /b %ERRORLEVEL%
  )
GOTO:EOF

:print_help
  echo. "Usage: $0 Platform Configuration"
GOTO:EOF

:ExitBatch - Cleanly exit batch processing, regardless how many CALLs
if not exist "%temp%\ExitBatchYes.txt" call :buildYes
call :CtrlC <"%temp%\ExitBatchYes.txt" 1>nul 2>&1
:CtrlC
cmd /c exit -1073741510

:buildYes - Establish a Yes file for the language used by the OS
pushd "%temp%"
set "yes="
copy nul ExitBatchYes.txt >nul
for /f "delims=(/ tokens=2" %%Y in (
  '"copy /-y nul ExitBatchYes.txt <nul"'
) do if not defined yes set "yes=%%Y"
echo %yes%>ExitBatchYes.txt
popd
exit /b

endlocal