; MUI 1.67 compatible ------
!include "MUI.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"

!define PRODUCT_NAME "vRTM"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "Intel Corporation"
!define PRODUCT_WEB_SITE "http://www.intel.com"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_NAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; MUI end ------

Name "${PRODUCT_NAME}"
OutFile "vrtm-setup.exe"
InstallDir "$PROGRAMFILES\Intel\vRTM"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

var extFlag
var vcr10Flag
var vcr13Flag

; ------------------------------------------------------------------
; ***************************** PAGES ******************************
; ------------------------------------------------------------------

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
;!insertmacro MUI_PAGE_LICENSE ""
; Components page
;!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!define MUI_PAGE_CUSTOMFUNCTION_SHOW DirectoryPageShow
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_NOAUTOCLOSE
;!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
;!define MUI_FINISHPAGE_SHOWREADME ""
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"
; -------------------------------------------------------------------------
; ***************************** END OF PAGES ******************************
; -------------------------------------------------------------------------

; ----------------------------------------------------------------------------------
; ******************************** IN-BUILT FUNCTIONS ******************************
; ----------------------------------------------------------------------------------

; Built-in Function StrStr
!define StrStr "!insertmacro StrStr"
!macro StrStr ResultVar String SubString
  Push `${String}`
  Push `${SubString}`
  Call StrStr
  Pop `${ResultVar}`
!macroend

Function StrStr
/*After this point:
  ------------------------------------------
  $R0 = SubString (input)
  $R1 = String (input)
  $R2 = SubStringLen (temp)
  $R3 = StrLen (temp)
  $R4 = StartCharPos (temp)
  $R5 = TempStr (temp)*/

  ;Get input from user
  Exch $R0
  Exch
  Exch $R1
  Push $R2
  Push $R3
  Push $R4
  Push $R5

  ;Get "String" and "SubString" length
  StrLen $R2 $R0
  StrLen $R3 $R1
  ;Start "StartCharPos" counter
  StrCpy $R4 0

  ;Loop until "SubString" is found or "String" reaches its end
  ${Do}
    ;Remove everything before and after the searched part ("TempStr")
    StrCpy $R5 $R1 $R2 $R4

    ;Compare "TempStr" with "SubString"
    ${IfThen} $R5 == $R0 ${|} ${ExitDo} ${|}
    ;If not "SubString", this could be "String"'s end
    ${IfThen} $R4 >= $R3 ${|} ${ExitDo} ${|}
    ;If not, continue the loop
    IntOp $R4 $R4 + 1
  ${Loop}

/*After this point:
  ------------------------------------------
  $R0 = ResultVar (output)*/

  ;Remove part before "SubString" on "String" (if there has one)
  StrCpy $R0 $R1 `` $R4

  ;Return output to user
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Pop $R1
  Exch $R0
FunctionEnd
; ----------------------------------------------------------------------------------
; *************************** END OF IN-BUILT FUNCTIONS ****************************
; ----------------------------------------------------------------------------------

; ----------------------------------------------------------------------------------
; *************************** SECTION FOR INSTALLING *******************************
; ----------------------------------------------------------------------------------

Section InstallPrerequisites
  SetOverwrite ifnewer
  SetOutPath "$INSTDIR\prerequisites"
  File "..\vrtm\prerequisites\vcredist_10.exe"
  ${If} $vcr10Flag == 0
    MessageBox MB_ICONEXCLAMATION|MB_OKCANCEL|MB_DEFBUTTON1 "Microsoft Visual C++ 2010 Redistributable not found. It is recommended to continue with the installation for Intel CIT vRTM setup." /SD IDOK IDCANCEL end10
    nsExec::Exec '$INSTDIR\prerequisites\vcredist_10.exe'
  ${endif}

  end10:
  File "..\vrtm\prerequisites\vcredist_13.exe"
  ${If} $vcr13Flag == 0
    MessageBox MB_ICONEXCLAMATION|MB_OKCANCEL|MB_DEFBUTTON1 "Microsoft Visual C++ 2013 Redistributable not found. It is recommended to continue with the installation for Intel CIT vRTM setup." /SD IDOK IDCANCEL end13
    nsExec::Exec '$INSTDIR\prerequisites\vcredist_13.exe'
  ${endif}

  end13:
  File "..\vrtm\prerequisites\Ext2Fsd-0.62.exe"
  ${If} $extFlag == 0
    MessageBox MB_ICONEXCLAMATION|MB_OKCANCEL|MB_DEFBUTTON1 "Ext2Fsd driver not found. It is recommended to continue with the installation for Intel CIT vRTM setup." /SD IDOK IDCANCEL end
    nsExec::Exec '$INSTDIR\prerequisites\Ext2Fsd-0.62.exe'
  ${endif}
  end:
SectionEnd

Section Install
  SetOverwrite ifnewer
  SetOutPath "$INSTDIR\configuration"
  File "..\vrtm\configuration\vRTM.cfg"
  File "..\vrtm\configuration\vrtm_log.properties"
  File "..\vrtm\configuration\vrtm_proxylog.properties"
  SetOutPath "$INSTDIR\scripts"
  File "..\install\nocmd.vbs"
  File "..\install\initsvcsetup.cmd"
  File "..\vrtm\scripts\vrtm.cmd"
  File "..\vrtm\scripts\Mount-EXTVM.ps1"
  File "..\vrtm\scripts\mount_vm_image.sh"
  File "..\vrtm\scripts\preheat-guestmount.sh"
  SetOutPath "$INSTDIR\bin"
  File "..\vrtm\bin\log4cpp.dll"
  File "..\vrtm\bin\libxml2.dll"
  File "..\vrtm\bin\pthreadVC2.dll"
  File "..\vrtm\bin\vrtmchannel.dll"
  File "..\vrtm\bin\vrtmservice.exe"
  File "..\vrtm\bin\vrtmcore.exe"
  File "..\..\dcg_security-tboot-xm\imvm\bin\verifier.exe"
SectionEnd

Section AdditionalIcons
  CreateDirectory "$SMPROGRAMS\Intel"
  CreateDirectory "$SMPROGRAMS\Intel\vRTM"
  CreateShortCut "$SMPROGRAMS\Intel\vRTM\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\bin\vrtmcore.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"

  # Create System Environment Variable VRTM_HOME
  !define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
  !define env_hkcu 'HKCU "Environment"'
  WriteRegExpandStr ${env_hklm} VRTM_HOME $INSTDIR
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
SectionEnd

Section InstallService
  # Create vRTM service and tasks
  nsExec::Exec '$INSTDIR\scripts\initsvcsetup.cmd'
  Delete "$INSTDIR\scripts\initsvcsetup.cmd"
  Delete "$INSTDIR\scripts\nocmd.vbs"
SectionEnd

; ----------------------------------------------------------------------------------
; ************************** SECTION FOR UNINSTALLING ******************************
; ----------------------------------------------------------------------------------

Section Uninstall
  # Remove vRTM service and tasks
  nsExec::Exec 'cmd /k schtasks /end /tn vRTM /f'
  nsExec::Exec 'cmd /k schtasks /delete /tn vRTM /f'
  nsExec::Exec 'sc stop vRTM'
  nsExec::Exec 'sc delete vRTM'

  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\rpcoreservice__DT_XX99"
  Delete "$INSTDIR\bin\log4cpp.dll"
  Delete "$INSTDIR\bin\libxml2.dll"
  Delete "$INSTDIR\bin\pthreadVC2.dll"
  Delete "$INSTDIR\bin\vrtmchannel.dll"
  Delete "$INSTDIR\bin\vrtmservice.exe"
  Delete "$INSTDIR\bin\vrtmcore.exe"
  Delete "$INSTDIR\bin\verifier.exe"
  Delete "$INSTDIR\scripts\vrtm.cmd"
  Delete "$INSTDIR\scripts\Mount-EXTVM.ps1"
  Delete "$INSTDIR\scripts\mount_vm_image.sh"
  Delete "$INSTDIR\scripts\preheat-guestmount.sh"
  Delete "$INSTDIR\configuration\vRTM.cfg"
  Delete "$INSTDIR\configuration\vrtm_log.properties"
  Delete "$INSTDIR\configuration\vrtm_proxylog.properties"
  Delete "$INSTDIR\prerequisites\Ext2Fsd-0.62.exe"
  Delete "$INSTDIR\prerequisites\vcredist_10.exe"
  Delete "$INSTDIR\prerequisites\vcredist_13.exe"
  Delete "$SMPROGRAMS\Intel\vRTM\Uninstall.lnk"

  RMDir "$SMPROGRAMS\Intel\vRTM"
  RMDir "$SMPROGRAMS\Intel"
  RMDir "$INSTDIR\bin"
  RMDir "$INSTDIR\scripts"
  RMDir "$INSTDIR\configuration"
  RMDir "$INSTDIR\prerequisites"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

  # Remove System Environment Variable VRTM_HOME
  DeleteRegValue ${env_hklm} VRTM_HOME
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
  SetAutoClose true
SectionEnd
; ----------------------------------------------------------------------------------
; ********************* END OF INSTALL/UNINSTALL SECTIONS **************************
; ----------------------------------------------------------------------------------

; ----------------------------------------------------------
; ********************* FUNCTIONS **************************
; ----------------------------------------------------------

Function .onInit
  !ifdef IsSilent
    SetSilent silent
  !endif

  ${If} ${RunningX64}
    StrCpy $2 "Name like '%%Microsoft Visual C++ 2010  x64%%'"
    StrCpy $3 "Name like '%%Microsoft Visual C++ 2013 x64%%'"
  ${Else}
    StrCpy $2 "Name like '%%Microsoft Visual C++ 2010  x86%%'"
    StrCpy $3 "Name like '%%Microsoft Visual C++ 2013 x86%%'"
  ${EndIf}

  nsExec::ExecToStack 'wmic product where "$2" get name'
  Pop $0
  Pop $1
  ${If} ${RunningX64}
    ${StrStr} $0 $1 "Microsoft Visual C++ 2010  x64"
  ${Else}
    ${StrStr} $0 $1 "Microsoft Visual C++ 2010  x86"
  ${EndIf}
  StrCmp $0 "" notfound10
    StrCpy $vcr10Flag 1
    Goto done10
  notfound10:
    StrCpy $vcr10Flag 0

  done10:
  nsExec::ExecToStack 'wmic product where "$3" get name'
  Pop $0
  Pop $1
  ${If} ${RunningX64}
    ${StrStr} $0 $1 "Microsoft Visual C++ 2013 x64"
  ${Else}
    ${StrStr} $0 $1 "Microsoft Visual C++ 2013 x86"
  ${EndIf}
  StrCmp $0 "" notfound13
    StrCpy $vcr13Flag 1
    Goto done13
  notfound13:
    StrCpy $vcr13Flag 0

  done13:
  ClearErrors
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\services\Ext2Fsd" "DisplayName"
  ${If} ${Errors}
    StrCpy $extFlag 0
  ${Else}
    StrCpy $extFlag 1
  ${EndIf}
FunctionEnd

Function un.onInit
;  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
;  Abort
FunctionEnd

Function un.onUninstSuccess
;  HideWindow
;  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function DirectoryPageShow
	FindWindow $R0 "#32770" "" $HWNDPARENT
	GetDlgItem $R1 $R0 1019
	EnableWindow $R1 0
	GetDlgItem $R1 $R0 1001
	EnableWindow $R1 0
FunctionEnd
; ----------------------------------------------------------
; ****************** END OF FUNCTIONS **********************
; ----------------------------------------------------------