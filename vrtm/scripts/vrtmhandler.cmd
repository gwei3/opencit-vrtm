@echo off
setlocal enabledelayedexpansion

set DESC=vRTM
set NAME=vrtmhandler

REM ###################################################################################################
REM #Set environment specific variables here 
REM ###################################################################################################

REM set the vrtm home directory
REM set VRTM_HOME="C:\Program Files (x86)\Intel\vRTM"
for %%i in ("%~dp0..") do set "parentfolder=%%~fi"
set VRTM_HOME=%parentfolder%

set DAEMON=%VRTM_HOME%\scripts\%NAME%.cmd
set VRTM_BIN=%VRTM_HOME%\bin

REM @###################################################################################################

REM parsing the command arguments
set wcommand=%1
set cmdparams=
for /f "usebackq tokens=1*" %%i in (`echo %*`) DO @ set cmdparams=%%j
REM echo. Running command: %wcommand% with %cmdparams%

IF "%wcommand%"=="start" (
  call:vrtm_start
) ELSE IF "%wcommand%"=="stop" (
  call:vrtm_stop
) ELSE IF "%wcommand%"=="version" (
  call:vrtm_version
) ELSE (
  call:print_help
)
GOTO:EOF

REM functions
:vrtm_start
  echo. Starting vrtm....
  cd %VRTM_BIN%
  vrtmcore.exe
GOTO:EOF

:vrtm_stop
  echo. Stopping all vrtm processes....
  taskkill /IM vrtmcore.exe /F
GOTO:EOF

:vrtm_version
  type %VRTM_HOME%\vrtm.version
GOTO:EOF

:print_help
  echo. "Usage: $0 start|stop|version"
GOTO:EOF

endlocal