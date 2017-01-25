@echo off
REM #####################################################################
REM This script build the third party libraries on windows platform
REM #####################################################################
setlocal enabledelayedexpansion

set me=%~n0
set pwd=%~dp0

set vcvarsall="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
set VsDevCmd="C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\VsDevCmd.bat"

IF "%~1"=="" (
  call:print_help
) ELSE IF "%~2"=="" (
  call:print_help
) ELSE IF "%~3"=="" (
  call:print_help
) ELSE (
  set libxml_home=%1
  set log4cpp_home=%2
  set pthread_home=%3

  set /p dest_home=Enter the location to copy build files : 
  call:libxml_build
)
GOTO:EOF

:libxml_build
  echo. Building libxml....
  mkdir "%dest_home%\third party\libxml2"
  mkdir "%dest_home%\third party\libxml2\bin\x86\Debug"
  mkdir "%dest_home%\third party\libxml2\bin\x86\Release"
  mkdir "%dest_home%\third party\libxml2\bin\x64\Debug"
  mkdir "%dest_home%\third party\libxml2\bin\x64\Release"
  mkdir "%dest_home%\third party\libxml2\lib\x86\Debug"
  mkdir "%dest_home%\third party\libxml2\lib\x86\Release"
  mkdir "%dest_home%\third party\libxml2\lib\x64\Debug"
  mkdir "%dest_home%\third party\libxml2\lib\x64\Release"

  set working_dir=%libxml_home%\win32
  cd %working_dir%
  cd
  cscript configure.js iconv=no
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: Libxml configurations could not be set
	call:ExitBatch
	REM EXIT /b %ERRORLEVEL%
  )
  call:libxml_build_util Release x86
  call:libxml_build_util Release x64

  cscript configure.js iconv=no debug=yes
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: Libxml debug configurations could not be set
	call:ExitBatch
	REM EXIT /b %ERRORLEVEL%
  )
  call:libxml_build_util Debug x86
  call:libxml_build_util Debug x64
  cd
  xcopy %libxml_home%\include "%dest_home%\third party\libxml2\include" /s /i /y
  call:log4cpp_build
GOTO:EOF

:log4cpp_build
  echo. Building log4cpp....
  mkdir "%dest_home%\third party\log4cpp"
  mkdir "%dest_home%\third party\log4cpp\bin\x86\Debug"
  mkdir "%dest_home%\third party\log4cpp\bin\x86\Release"
  mkdir "%dest_home%\third party\log4cpp\bin\x64\Debug"
  mkdir "%dest_home%\third party\log4cpp\bin\x64\Release"
  mkdir "%dest_home%\third party\log4cpp\lib\x86\Debug"
  mkdir "%dest_home%\third party\log4cpp\lib\x86\Release"
  mkdir "%dest_home%\third party\log4cpp\lib\x64\Debug"
  mkdir "%dest_home%\third party\log4cpp\lib\x64\Release"

  set working_dir=%log4cpp_home%\msvc10
  cd %working_dir%
  cd
  call %VsDevCmd%
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: Visual Studio Dev Env could not be set
	call:ExitBatch
	REM EXIT /b %ERRORLEVEL%
  )
  cd
  call:log4cpp_build_util Debug x86
  call:log4cpp_build_util Debug x64
  call:log4cpp_build_util Release x86
  call:log4cpp_build_util Release x64
  cd
  xcopy %log4cpp_home%\include "%dest_home%\third party\log4cpp\include" /s /i /y
  call:pthread_build
GOTO:EOF

:pthread_build
  mkdir "%dest_home%\third party\pthread"
  mkdir "%dest_home%\third party\pthread\dll\x86"
  mkdir "%dest_home%\third party\pthread\dll\x64"
  mkdir "%dest_home%\third party\pthread\lib\x86"
  mkdir "%dest_home%\third party\pthread\lib\x64"

  xcopy %pthread_home%\include "%dest_home%\third party\pthread\include" /s /i /y
  copy %pthread_home%\dll\x86\pthreadVC2.dll "%dest_home%\third party\pthread\dll\x86" /y
  copy %pthread_home%\dll\x64\pthreadVC2.dll "%dest_home%\third party\pthread\dll\x64" /y
  copy %pthread_home%\lib\x86\pthreadVC2.lib "%dest_home%\third party\pthread\lib\x86" /y
  copy %pthread_home%\lib\x64\pthreadVC2.lib "%dest_home%\third party\pthread\lib\x64" /y
GOTO:EOF

:libxml_build_util
  setlocal
  echo. inside libxml_build_util %1 %2
  IF "%2"=="x86" (
    echo. calling with x86 option
    call %vcvarsall% x86
	IF NOT %ERRORLEVEL% EQU 0 (
	  echo. %me%: Visual Studio x86 configuration could not be set
	  call:ExitBatch
	  REM EXIT /b %ERRORLEVEL%
	)
  ) ELSE (
    echo. calling with x86_amd64 option
	call %vcvarsall% x86_amd64
	IF NOT %ERRORLEVEL% EQU 0 (
	  echo. %me%: Visual Studio x64 configuration could not be set
	  call:ExitBatch
	  REM EXIT /b %ERRORLEVEL%
	)
  )

  nmake /f Makefile.msvc
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: Build Failed
	call:ExitBatch
	REM EXIT /b %ERRORLEVEL%
  ) ELSE (
    copy bin.msvc\libxml2.dll "%dest_home%\third party\libxml2\bin\%2\%1" /y
    copy bin.msvc\libxml2.lib "%dest_home%\third party\libxml2\lib\%2\%1" /y
  )
  endlocal
GOTO:EOF

:log4cpp_build_util
  setlocal
  echo. inside log4cpp_build_util %1 %2
  IF "%2"=="x86" (
    echo. calling with Win32 option
    msbuild msvc10.sln /property:Configuration=%1;Platform=Win32
	IF NOT %ERRORLEVEL% EQU 0 (
	  echo. %me%: Build Failed
	  call:ExitBatch
	  REM EXIT /b %ERRORLEVEL%
	) ELSE (
      copy log4cpp\%1\log4cpp.dll "%dest_home%\third party\log4cpp\bin\%2\%1" /y
      copy log4cpp\%1\log4cpp.lib "%dest_home%\third party\log4cpp\lib\%2\%1" /y
	)
  ) ELSE (
    echo. calling with x64 option
    msbuild msvc10.sln /property:Configuration=%1;Platform=%2
    IF NOT %ERRORLEVEL% EQU 0 (
	  echo. %me%: Build Failed
	  call:ExitBatch
	  REM EXIT /b %ERRORLEVEL%
	) ELSE (
      copy log4cpp\%2\%1\log4cpp.dll "%dest_home%\third party\log4cpp\bin\%2\%1" /y
      copy log4cpp\%2\%1\log4cpp.lib "%dest_home%\third party\log4cpp\lib\%2\%1" /y
	)
  )
  endlocal
GOTO:EOF

:print_help
  echo. "Usage: $0 libxml_dir log4cpp_dir pthread_dir"
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