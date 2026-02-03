!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinMessages.nsh"
!include "StrFunc.nsh"
!include "WordFunc.nsh"

!define PRODUCT_NAME "AymaraLang"
!define PRODUCT_PUBLISHER "AymaraLang"
!define PRODUCT_WEB_SITE "https://aymaralang.local"

!ifexist "..\\VERSION.txt"
  !searchparse /file "..\\VERSION.txt" "" PRODUCT_VERSION
!else
  !define PRODUCT_VERSION "0.1.0"
!endif

!ifdef OUTPUT_FILE
  OutFile "${OUTPUT_FILE}"
!else
  OutFile "AymaraLang-Setup.exe"
!endif

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
InstallDir "$PROGRAMFILES64\\AymaraLang"
InstallDirRegKey HKLM "Software\\${PRODUCT_NAME}" "InstallDir"
RequestExecutionLevel admin
SetRegView 64
ShowInstDetails show
ShowUninstDetails show

!ifexist "..\\assets\\logo.ico"
  !define MUI_ICON "..\\assets\\logo.ico"
  !define MUI_UNICON "..\\assets\\logo.ico"
!endif

!ifexist "..\\assets\\banner.bmp"
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_RIGHT
  !define MUI_HEADERIMAGE_BITMAP "..\\assets\\banner.bmp"
!endif

!ifexist "..\\assets\\dialog.bmp"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "..\\assets\\dialog.bmp"
!endif

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!ifexist "..\\LICENSE"
  !insertmacro MUI_PAGE_LICENSE "..\\LICENSE"
!endif
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "Spanish"

InstType "Default"

Section "Core (required)" SEC_CORE
  SectionIn RO

  SetOutPath "$INSTDIR"
  File /r /x "bin\\*" /x "llvm-backend\\*" "..\\dist\\*"

  SetOutPath "$INSTDIR\\bin"
  File /r "..\\dist\\bin\\*"

  WriteRegStr HKLM "Software\\${PRODUCT_NAME}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}" "UninstallString" "\"$INSTDIR\\Uninstall.exe\""
  WriteRegDWORD HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}" "NoRepair" 1

  WriteUninstaller "$INSTDIR\\Uninstall.exe"

  Call EnsureVCRedist
SectionEnd

Section "Add AymaraLang to PATH" SEC_PATH
  SectionIn 1
  Call AddToPath
SectionEnd

Section "Install LLVM Backend" SEC_LLVM
  SetOutPath "$INSTDIR\\llvm-backend"
  IfFileExists "..\\dist\\llvm-backend\\*" 0 +2
  File /r "..\\dist\\llvm-backend\\*"
SectionEnd

Section "AymaraLang Command Prompt Shortcut" SEC_SHORTCUT
  CreateDirectory "$SMPROGRAMS\\${PRODUCT_NAME}"
  CreateShortcut "$SMPROGRAMS\\${PRODUCT_NAME}\\AymaraLang Command Prompt.lnk" "$COMSPEC" "/k \"set PATH=$INSTDIR\\bin;%PATH%&&title AymaraLang Command Prompt\"" "$INSTDIR\\bin\\aymc.exe"
SectionEnd

Section "Uninstall"
  Call un.RemoveFromPath

  Delete "$SMPROGRAMS\\${PRODUCT_NAME}\\AymaraLang Command Prompt.lnk"
  RMDir "$SMPROGRAMS\\${PRODUCT_NAME}"

  Delete "$INSTDIR\\Uninstall.exe"
  RMDir /r "$INSTDIR\\llvm-backend"
  RMDir /r "$INSTDIR\\bin"
  RMDir /r "$INSTDIR\\share"
  Delete "$INSTDIR\\README.md"
  Delete "$INSTDIR\\LICENSE"
  RMDir /r "$INSTDIR"

  DeleteRegKey HKLM "Software\\${PRODUCT_NAME}"
  DeleteRegKey HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}"
SectionEnd

Function EnsureVCRedist
  StrCpy $0 0
  ReadRegDWORD $0 HKLM "SOFTWARE\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x64" "Installed"
  ReadRegStr $1 HKLM "SOFTWARE\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x64" "Version"
  StrCpy $2 "14.30.30704.0"

  ${If} $0 != 1
    Call InstallVCRedist
    Return
  ${EndIf}

  ${If} $1 == ""
    Call InstallVCRedist
    Return
  ${EndIf}

  ${VersionCompare} $1 $2 $3
  ${If} $3 < 0
    Call InstallVCRedist
  ${EndIf}
FunctionEnd

Function InstallVCRedist
  IfFileExists "..\\installer\\VC_redist.x64.exe" 0 +3
  ExecWait '"..\\installer\\VC_redist.x64.exe" /install /quiet /norestart'
  Return
  MessageBox MB_ICONSTOP "No se encontró VC_redist.x64.exe en installer."
  Abort
FunctionEnd

Function AddToPath
  ReadRegExpandStr $0 HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path"
  StrCpy $1 "$INSTDIR\\bin"
  ${StrStr} $2 $0 $1
  ${If} $2 == ""
    ${If} $0 == ""
      StrCpy $0 $1
    ${Else}
      StrCpy $0 "$0;$1"
    ${EndIf}
    WriteRegExpandStr HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path" "$0"
    Call RefreshEnv
  ${EndIf}
FunctionEnd

Function un.RemoveFromPath
  ReadRegExpandStr $0 HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path"
  StrCpy $1 "$INSTDIR\\bin"
  StrCpy $2 $0

  ${StrRep} $2 $2 ";$1" ""
  ${StrRep} $2 $2 "$1;" ""
  ${StrRep} $2 $2 "$1" ""
  ${StrRep} $2 $2 ";;" ";"

  StrCpy $3 $2 1
  ${If} $3 == ";"
    StrCpy $2 $2 "" 1
  ${EndIf}

  StrLen $4 $2
  ${If} $4 > 0
    StrCpy $3 $2 1 -1
    ${If} $3 == ";"
      StrCpy $2 $2 -1
    ${EndIf}
  ${EndIf}

  WriteRegExpandStr HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path" "$2"
  Call RefreshEnv
FunctionEnd

Function RefreshEnv
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
FunctionEnd
