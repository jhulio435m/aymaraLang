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
  File /r /x "bin\*" /x "llvm-backend\*" "..\dist\*"

  SetOutPath "$INSTDIR\bin"
  File /r "..\dist\bin\*"

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
SectionEnd

Section "Add AymaraLang to PATH" SEC_PATH
  SectionIn 1
  SetRegView 64
  Call AddToPath
SectionEnd

Section "Install LLVM Backend" SEC_LLVM
  SetRegView 64
  SetOutPath "$INSTDIR\llvm-backend"

  ; No fallar si la carpeta no existe o está vacía (p.ej. llvm-backend agregado en repo)
  IfFileExists "..\llvm-backend\*.*" 0 +2
    File /r "..\llvm-backend\*.*"
SectionEnd

Section "AymaraLang Command Prompt Shortcut" SEC_SHORTCUT
  SetRegView 64
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\AymaraLang Command Prompt.lnk" "$SYSDIR\cmd.exe" '/k "set PATH=$INSTDIR\bin;%PATH%&&title AymaraLang Command Prompt"' "$INSTDIR\bin\aymc.exe"
SectionEnd

Section "Uninstall"
  SetRegView 64

  Call un.RemoveFromPath

  Delete "$SMPROGRAMS\${PRODUCT_NAME}\AymaraLang Command Prompt.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR\llvm-backend"
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\share"

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

; ---------- PATH helpers (no StrFunc) ----------

; Returns remainder starting at match, or "" if not found
; IN:  stack: haystack, needle
; OUT: stack: remainder
Function StrStr
  Exch $R1 ; needle
  Exch
  Exch $R0 ; haystack
  Push $R2
  Push $R3
  Push $R4

  StrLen $R2 $R1
  StrCpy $R3 0
loop:
  StrCpy $R4 $R0 $R2 $R3
  StrCmp $R4 $R1 found
  StrCmp $R4 "" notfound
  IntOp $R3 $R3 + 1
  Goto loop

found:
  StrCpy $R0 $R0 "" $R3
  Goto done

notfound:
  StrCpy $R0 ""

done:
  Pop $R4
  Pop $R3
  Pop $R2
  Exch $R0
  Exch $R1
FunctionEnd

; Replace first occurrence of needle in haystack with repl
; IN: stack: haystack, needle, repl
; OUT: stack: result
Function StrReplaceOnce
  Exch $R2 ; repl
  Exch
  Exch $R1 ; needle
  Exch 2
  Exch $R0 ; haystack
  Push $R3
  Push $R4
  Push $R5
  Push $R6

  Push $R0
  Push $R1
  Call StrStr
  Pop $R3
  StrCmp $R3 "" nochange

  StrLen $R4 $R0
  StrLen $R5 $R3
  IntOp $R6 $R4 - $R5

  StrCpy $R4 $R0 $R6
  StrLen $R5 $R1
  StrCpy $R5 $R3 "" $R5

  StrCpy $R0 "$R4$R2$R5"
  Goto done

nochange:
  ; unchanged

done:
  Pop $R6
  Pop $R5
  Pop $R4
  Pop $R3
  Exch $R0
  Exch $R1
  Exch $R2
FunctionEnd

; Collapse ;; -> ; repeatedly
; IN: stack: string
; OUT: stack: string
Function CollapseDoubleSemicolons
  Exch $R0
  Push $R1

loop2:
  Push $R0
  Push ";;"
  Call StrStr
  Pop $R1
  StrCmp $R1 "" done2

  Push $R0
  Push ";;"
  Push ";"
  Call StrReplaceOnce
  Pop $R0
  Goto loop2

done2:
  Pop $R1
  Exch $R0
FunctionEnd

Function AddToPath
  SetRegView 64

  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  StrCpy $1 "$INSTDIR\bin"

  ; normalize ;...;
  StrCpy $2 ";$0;"
  StrCpy $3 ";$1;"

  Push $2
  Push $3
  Call StrStr
  Pop $4

  ; if already present -> done
  StrCmp $4 "" +2
    Goto done

  ; append (handle empty)
  StrCmp $0 "" 0 +2
    StrCpy $0 "$1"
  StrCmp $0 "" +2 0
    StrCpy $0 "$0;$1"

  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$0"
  Call RefreshEnv

done:
FunctionEnd

; ---------- Uninstall-only versions (must be un.*) ----------

Function un.StrStr
  Exch $R1
  Exch
  Exch $R0
  Push $R2
  Push $R3
  Push $R4

  StrLen $R2 $R1
  StrCpy $R3 0
loopu:
  StrCpy $R4 $R0 $R2 $R3
  StrCmp $R4 $R1 foundu
  StrCmp $R4 "" notfoundu
  IntOp $R3 $R3 + 1
  Goto loopu

foundu:
  StrCpy $R0 $R0 "" $R3
  Goto doneu

notfoundu:
  StrCpy $R0 ""

doneu:
  Pop $R4
  Pop $R3
  Pop $R2
  Exch $R0
  Exch $R1
FunctionEnd

Function un.StrReplaceOnce
  Exch $R2
  Exch
  Exch $R1
  Exch 2
  Exch $R0
  Push $R3
  Push $R4
  Push $R5
  Push $R6

  Push $R0
  Push $R1
  Call un.StrStr
  Pop $R3
  StrCmp $R3 "" nochangeu

  StrLen $R4 $R0
  StrLen $R5 $R3
  IntOp $R6 $R4 - $R5

  StrCpy $R4 $R0 $R6
  StrLen $R5 $R1
  StrCpy $R5 $R3 "" $R5

  StrCpy $R0 "$R4$R2$R5"
  Goto doneu2

nochangeu:
  ; unchanged

doneu2:
  Pop $R6
  Pop $R5
  Pop $R4
  Pop $R3
  Exch $R0
  Exch $R1
  Exch $R2
FunctionEnd

Function un.CollapseDoubleSemicolons
  Exch $R0
  Push $R1

loopu2:
  Push $R0
  Push ";;"
  Call un.StrStr
  Pop $R1
  StrCmp $R1 "" doneu3

  Push $R0
  Push ";;"
  Push ";"
  Call un.StrReplaceOnce
  Pop $R0
  Goto loopu2

doneu3:
  Pop $R1
  Exch $R0
FunctionEnd

Function un.RemoveFromPath
  SetRegView 64

  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  StrCpy $1 "$INSTDIR\bin"

  StrCmp $0 "" done

  StrCpy $2 ";$0;"
  StrCpy $3 ";$1;"

  Push $2
  Push $3
  Call un.StrStr
  Pop $4
  StrCmp $4 "" done

  Push $2
  Push $3
  Push ";"
  Call un.StrReplaceOnce
  Pop $2

  ; trim edges
  StrCpy $0 $2 "" 1
  StrLen $5 $0
  IntOp $5 $5 - 1
  StrCpy $6 $0 1 $5
  StrCmp $6 ";" 0 +2
    StrCpy $0 $0 $5

  Push $0
  Call un.CollapseDoubleSemicolons
  Pop $0

  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$0"
  Call un.RefreshEnv

done:
FunctionEnd

Function RefreshEnv
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
FunctionEnd

Function un.RefreshEnv
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
FunctionEnd
