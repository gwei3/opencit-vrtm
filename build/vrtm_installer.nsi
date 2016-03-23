; MUI 1.67 compatible ------
!include "MUI.nsh"

!define PRODUCT_NAME "vRTM"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "Intel Corporation"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_NAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
;!insertmacro MUI_PAGE_LICENSE ""
; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
;!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_NOAUTOCLOSE
;!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
;!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "vrtm-setup.exe"
InstallDir "$PROGRAMFILES\Intel\vRTM"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

; These are the programs that are needed by vRTM.
Section -Prerequisites
  MessageBox MB_ICONEXCLAMATION|MB_OKCANCEL|MB_DEFBUTTON1 "$(^Name) has some pre-requisites, it is recommended to continue with Installing Pre-requisites" /SD IDOK IDCANCEL endPre
    SetOutPath "$INSTDIR\prerequisites"
    File "..\vrtm\prerequisites\Ext2Fsd-0.62.exe"
	File "..\vrtm\prerequisites\vcredist_x86_10.exe"
	File "..\vrtm\prerequisites\vcredist_x86_13.exe"

    ExecWait "$INSTDIR\prerequisites\vcredist_x86_13.exe"
    ExecWait "$INSTDIR\prerequisites\vcredist_x86_10.exe"
    ExecWait "$INSTDIR\prerequisites\Ext2Fsd-0.62.exe"
  endPre:
SectionEnd

Section "vrtmcore" SEC01
  SetOutPath "$INSTDIR\configuration"
  File "..\vrtm\configuration\vRTM.cfg"
  File "..\vrtm\configuration\vrtm_log.properties"
  File "..\vrtm\configuration\vrtm_proxylog.properties"

  SetOutPath "$INSTDIR\scripts"
  File "..\vrtm\scripts\vrtm.cmd"
  File "..\vrtm\scripts\nocmd.vbs"
  File "..\vrtm\scripts\initsvcsetup.cmd"
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

  CreateDirectory "$INSTDIR\log"
  CreateDirectory "$INSTDIR\temp"
  CreateDirectory "$SMPROGRAMS\Intel"
  CreateDirectory "$SMPROGRAMS\Intel\vRTM"
  CreateShortCut "$SMPROGRAMS\Intel\vRTM\vRTM.lnk" "$INSTDIR\bin\vrtmcore.exe"
  CreateShortCut "$SMPROGRAMS\Intel\vRTM\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\vrtmcore.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\vrtmcore.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  
  # Create System Environment Variable VRTM_HOME
  !define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
  !define env_hkcu 'HKCU "Environment"'
  WriteRegExpandStr ${env_hklm} VRTM_HOME $INSTDIR
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
SectionEnd

Section -StartService
  # Create vRTM service and tasks
  ExecWait '$INSTDIR\initsvcsetup.cmd'
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "Installs the vrtmcore components"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  # Remove vRTM service and tasks
  nsExec::Exec 'cmd /k schtasks /end /tn vRTM /f'
  nsExec::Exec 'cmd /k schtasks /delete /tn vRTM /f'
  nsExec::Exec 'sc stop vRTM'
  nsExec::Exec 'sc delete vRTM'

  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\log\vrtm.log"
  Delete "$INSTDIR\log\vrtm_listener.log"
  Delete "$INSTDIR\temp\rpcoreservice__DT_XX99"
  Delete "$INSTDIR\bin\log4cpp.dll"
  Delete "$INSTDIR\bin\libxml2.dll"
  Delete "$INSTDIR\bin\pthreadVC2.dll"
  Delete "$INSTDIR\bin\vrtmchannel.dll"
  Delete "$INSTDIR\bin\vrtmservice.exe"
  Delete "$INSTDIR\bin\vrtmcore.exe"
  Delete "$INSTDIR\bin\verifier.exe"
  Delete "$INSTDIR\scripts\vrtm.cmd"
  Delete "$INSTDIR\scripts\nocmd.vbs"
  Delete "$INSTDIR\scripts\initsvcsetup.cmd"
  Delete "$INSTDIR\scripts\Mount-EXTVM.ps1"
  Delete "$INSTDIR\scripts\mount_vm_image.sh"
  Delete "$INSTDIR\scripts\preheat-guestmount.sh"
  Delete "$INSTDIR\configuration\vRTM.cfg"
  Delete "$INSTDIR\configuration\vrtm_log.properties"
  Delete "$INSTDIR\configuration\vrtm_proxylog.properties"
  Delete "$INSTDIR\prerequisites\Ext2Fsd-0.62.exe"
  Delete "$INSTDIR\prerequisites\vcredist_x86_10.exe"
  Delete "$INSTDIR\prerequisites\vcredist_x86_13.exe"

  Delete "$SMPROGRAMS\Intel\vRTM\Uninstall.lnk"
  Delete "$SMPROGRAMS\Intel\vRTM\vRTM.lnk"
  RMDir "$SMPROGRAMS\Intel\vRTM"
  RMDir "$SMPROGRAMS\Intel"

  RMDir "$INSTDIR\log"
  RMDir "$INSTDIR\temp"
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