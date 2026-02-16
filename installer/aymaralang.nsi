; ============================
; AymaraLang - NSIS Installer
; ============================

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinMessages.nsh"
!include "WordFunc.nsh"

!define PRODUCT_NAME "AymaraLang"
!define PRODUCT_PUBLISHER "AymaraLang"
!define PRODUCT_WEB_SITE "https://aymaralang.local"
!define PRODUCT_VERSION "0.1.0"

!ifdef OUTPUT_FILE
  OutFile "${OUTPUT_FILE}"
!else
  OutFile "AymaraLang-Setup.exe"
!endif

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
InstallDir "$PROGRAMFILES64\AymaraLang"
InstallDirRegKey HKLM "Software\${PRODUCT_NAME}" "InstallDir"
RequestExecutionLevel admin
ShowInstDetails show
ShowUninstDetails show

; If you don't have this file yet, comment both lines
Icon "..\assets\logo.ico"
UninstallIcon "..\assets\logo.ico"

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME

; If ..\LICENSE does not exist, comment the next line
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "Spanish"

InstType "Default"

; ----------------------------
; Sections
; ----------------------------

Section "Core (required)" SEC_CORE
  SectionIn RO
  SetRegView 64

  SetOutPath "$INSTDIR"
  File "..\dist\README.md"
  File "..\dist\LICENSE"
  File "..\assets\logo.ico"

  SetOutPath "$INSTDIR\bin"
  File /r "..\dist\bin\*"

  SetOutPath "$INSTDIR\share"
  File /r "..\dist\share\*"

  WriteRegStr HKLM "Software\${PRODUCT_NAME}" "InstallDir" "$INSTDIR"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "NoRepair" 1

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  Call EnsureVCRedist
  Call AddToPath
  Call RegisterAymFileAssociation
SectionEnd

Section "AymaraLang Command Prompt Shortcut" SEC_SHORTCUT
  SetRegView 64
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\AymaraLang Command Prompt.lnk" "$SYSDIR\cmd.exe" '/k "set PATH=$INSTDIR\bin;%PATH%&&title AymaraLang Command Prompt"' "$INSTDIR\bin\aymc.exe"
SectionEnd

Section "Uninstall"
  SetRegView 64

  Call un.RemoveAymFileAssociation
  Call un.RemoveFromPath

  Delete "$SMPROGRAMS\${PRODUCT_NAME}\AymaraLang Command Prompt.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\include"
  RMDir /r "$INSTDIR\lib"
  RMDir /r "$INSTDIR\share"
  Delete "$INSTDIR\aym.exe"
  Delete "$INSTDIR\aymc.exe"

  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\LICENSE"
  RMDir /r "$INSTDIR"

  DeleteRegKey HKLM "Software\${PRODUCT_NAME}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
SectionEnd

; ----------------------------
; Functions
; ----------------------------

Function .onInit
  Call EnsureNasm
  Call EnsureGcc
FunctionEnd

Function EnsureVCRedist
  SetRegView 64

  StrCpy $0 0
  ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
  ReadRegStr   $1 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Version"
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
  SetRegView 64

  IfFileExists "..\installer\VC_redist.x64.exe" 0 +3
  ExecWait '"..\installer\VC_redist.x64.exe" /install /quiet /norestart'
  Return

  MessageBox MB_ICONSTOP "No se encontró VC_redist.x64.exe en installer."
  Abort
FunctionEnd

Function EnsureNasm
  Push "nasm.exe"
  Push "nasm (MSYS2)"
  Call RequireToolInPath
FunctionEnd

Function EnsureGcc
  Push "gcc.exe"
  Push "gcc (MSYS2)"
  Call RequireToolInPath
FunctionEnd

Function RequireToolInPath
  Exch $1 ; etiqueta visible
  Exch
  Exch $0 ; ejecutable
  Push $2

  ExecWait '"$SYSDIR\where.exe" $0' $2
  ${If} $2 != 0
    MessageBox MB_ICONSTOP "$1 no está en PATH. Instala MSYS2 (con nasm y gcc) y agrega su binario a PATH antes de continuar."
    Abort
  ${EndIf}

  Pop $2
  Pop $0
  Pop $1
FunctionEnd

Function AddToPath
  SetRegView 64
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  StrCpy $1 "$INSTDIR\bin"

  StrCmp $0 "" 0 +2
    StrCpy $0 "$1"
  StrCmp $0 "$1" +2 0
    StrCpy $0 "$0;$1"

  WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$0"
  Call RefreshEnv
FunctionEnd

Function RegisterAymFileAssociation
  SetRegView 64
  WriteRegStr HKLM "Software\Classes\.aym" "" "AymaraLang.Source"
  WriteRegStr HKLM "Software\Classes\AymaraLang.Source" "" "AymaraLang Source File"
  WriteRegStr HKLM "Software\Classes\AymaraLang.Source\DefaultIcon" "" "$INSTDIR\logo.ico,0"
  WriteRegStr HKLM "Software\Classes\AymaraLang.Source\shell\open\command" "" '"$INSTDIR\bin\aymc.exe" "%1"'
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0x0000, p 0, p 0)'
FunctionEnd

; ---------- Uninstall-only versions (must be un.*) ----------

Function un.RemoveAymFileAssociation
  SetRegView 64
  DeleteRegKey HKLM "Software\Classes\AymaraLang.Source"
  ReadRegStr $0 HKLM "Software\Classes\.aym" ""
  StrCmp $0 "AymaraLang.Source" 0 done
  DeleteRegKey HKLM "Software\Classes\.aym"
done:
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0x0000, p 0, p 0)'
FunctionEnd

Function un.RemoveFromPath
  SetRegView 64
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  StrCmp $0 "" done

  StrCpy $1 "$INSTDIR\bin"
  StrLen $2 $1
  StrLen $3 $0
  IntOp $4 $3 - $2
  IntCmp $4 1 done done 0

  StrCpy $5 $0 $2 $4
  StrCmp $5 $1 0 done

  IntOp $6 $4 - 1
  StrCpy $7 $0 1 $6
  StrCmp $7 ";" 0 done

  StrCpy $0 $0 $6
  WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$0"
  Call un.RefreshEnv

done:
FunctionEnd

Function RefreshEnv
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
FunctionEnd

Function un.RefreshEnv
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
FunctionEnd
