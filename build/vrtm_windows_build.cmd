@echo off
setlocal enabledelayedexpansion

set me=%~n0
set pwd=%~dp0

set makensis="C:\NSIS\makensis.exe"

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
  echo. "%vrtm_home%\third party\libxml2\bin\%1\%2\libxml2.dll"
  copy "%vrtm_home%\third party\libxml2\bin\%1\%2\libxml2.dll" %vrtm_home%\bin /y
  echo. "%vrtm_home%\third party\log4cpp\bin\%1\%2\log4cpp.dll"
  copy "%vrtm_home%\third party\log4cpp\bin\%1\%2\log4cpp.dll" %vrtm_home%\bin /y
  echo. "%vrtm_home%\third party\pthread\dll\%1\pthreadVC2.dll"
  copy "%vrtm_home%\third party\pthread\dll\%1\pthreadVC2.dll" %vrtm_home%\bin /y
  
  mkdir "%vrtm_home%\prerequisites"
  echo. "%vrtm_home%\third party\Ext2Fsd-0.62.exe"
  copy "%vrtm_home%\third party\Ext2Fsd-0.62.exe" %vrtm_home%\prerequisites /y
  echo. "%vrtm_home%\third party\vcredist\vs2010\vcredist_%1.exe"
  copy "%vrtm_home%\third party\vcredist\vs2010\vcredist_%1.exe" %vrtm_home%\prerequisites\vcredist_10.exe /y
  echo. "%vrtm_home%\third party\vcredist\vs2013\vcredist_%1.exe"
  copy "%vrtm_home%\third party\vcredist\vs2013\vcredist_%1.exe" %vrtm_home%\prerequisites\vcredist_13.exe /y
GOTO:EOF

:vrtm_build
  echo. Building vrtm.... %1 %2
  cd
  call %vrtm_home%\src\vrtm_build.cmd %1 %2
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: vRTM build failed
	EXIT /b %ERRORLEVEL%
  )
GOTO:EOF

:vrtm_install
  echo. Creating vrtm installer....
  cd
  call %makensis% vrtm_installer.nsi
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: vRTM install failed
	EXIT /b %ERRORLEVEL%
  )
GOTO:EOF

:print_help
  echo. "Usage: $0 Platform Configuration"
GOTO:EOF

endlocal