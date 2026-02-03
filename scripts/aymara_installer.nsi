!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "WinMessages.nsh"

!define APP_NAME "AymaraLang"
!define APP_EXE "aymc.exe"
!define APP_PUBLISHER "AymaraLang"
!define APP_URL "https://github.com/your-org/aymaraLang"

!ifdef APP_VERSION
!define APP_VERSION "${APP_VERSION}"
!else
!define APP_VERSION "0.1.0"
!endif

!ifdef OUTPUT_EXE
OutFile "${OUTPUT_EXE}"
!else
OutFile "AymaraLang-Setup-${APP_VERSION}.exe"
!endif

!ifdef STAGING_DIR
!define STAGING_DIR "${STAGING_DIR}"
!else
!define STAGING_DIR "..\\dist\\windows-installer"
!endif

InstallDir "$PROGRAMFILES64\\${APP_NAME}"
InstallDirRegKey HKLM "Software\\${APP_NAME}" "InstallDir"
RequestExecutionLevel admin

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\\Contrib\\Graphics\\Icons\\modern-install.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "Spanish"

Var MissingDeps

Function StrContains
  Exch $1
  Exch
  Exch $0
  Push $2
  Push $3
  Push $4
  StrCpy $2 0
  StrLen $3 $1
  ${Do}
    StrCpy $4 $0 $3 $2
    ${IfThen} $4 == $1 ${|} Pop $4
      Pop $3
      Pop $2
      Exch $0
      Exch $1
      Push 1
      Return ${|}
    ${|}
    IntOp $2 $2 + 1
    StrLen $4 $0
    ${IfThen} $2 >= $4 ${|} ExitDo ${|}
  ${Loop}
  Pop $4
  Pop $3
  Pop $2
  Exch $0
  Exch $1
  Push 0
FunctionEnd

Function CheckTool
  Exch $0
  Exch
  Exch $1
  Push $2
  SearchPath $2 $0
  ${If} $2 == ""
    StrCpy $MissingDeps "$MissingDeps\r\n- $1"
  ${EndIf}
  Pop $2
  Pop $1
  Pop $0
FunctionEnd

Function CheckDependencies
  StrCpy $MissingDeps ""
  Push "cmake.exe"
  Push "CMake"
  Call CheckTool
  Push "nasm.exe"
  Push "NASM"
  Call CheckTool
  SearchPath $0 "g++.exe"
  SearchPath $1 "clang.exe"
  ${If} $0 == ""
    ${If} $1 == ""
      StrCpy $MissingDeps "$MissingDeps\r\n- MinGW-w64 (g++) o LLVM (clang)"
    ${EndIf}
  ${EndIf}

  ${If} $MissingDeps != ""
    MessageBox MB_YESNO|MB_ICONEXCLAMATION \
      "Faltan dependencias de compilación en el sistema:$MissingDeps\r\n\r\n¿Deseas instalarlas ahora?" \
      IDYES InstallDependencies IDNO SkipInstall
    InstallDependencies:
      ExecWait 'powershell -ExecutionPolicy Bypass -File "$INSTDIR\\scripts\\install_deps_windows.ps1"'
      Goto Done
    SkipInstall:
      MessageBox MB_OK|MB_ICONINFORMATION \
        "Puedes instalarlas manualmente más tarde con el script: $INSTDIR\\scripts\\install_deps_windows.ps1"
    Done:
  ${EndIf}
FunctionEnd

Function AddToPath
  Exch $0
  Push $1
  Push $2
  ReadRegStr $1 HKCU "Environment" "Path"
  ${If} $1 == ""
    StrCpy $1 $0
  ${Else}
    Push $1
    Push $0
    Call StrContains
    Pop $2
    ${If} $2 == 0
      StrCpy $1 "$1;$0"
    ${EndIf}
  ${EndIf}
  WriteRegExpandStr HKCU "Environment" "Path" $1
  System::Call 'USER32::SendMessageTimeoutW(i 0xffff, i ${WM_SETTINGCHANGE}, i 0, w "Environment", i 0, i 1000, *i .r0)'
  Pop $2
  Pop $1
  Pop $0
FunctionEnd

Section "AymaraLang Compiler" SEC_CORE
  SetOutPath "$INSTDIR"
  WriteRegStr HKLM "Software\\${APP_NAME}" "InstallDir" "$INSTDIR"
  File /r "${STAGING_DIR}\\bin"
  File /r "${STAGING_DIR}\\runtime"
  File /r "${STAGING_DIR}\\scripts"
  File "..\\README.md"
  File "..\\LICENSE"

  Call CheckDependencies

  CreateDirectory "$SMPROGRAMS\\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\\${APP_NAME}\\AymaraLang Readme.lnk" "$INSTDIR\\README.md"
SectionEnd

Section "Agregar a PATH (usuario actual)" SEC_PATH
  Push "$INSTDIR\\bin"
  Call AddToPath
SectionEnd

Section "Desinstalar" SEC_UNINSTALL
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\\Uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\\${APP_NAME}\\AymaraLang Readme.lnk"
  RMDir "$SMPROGRAMS\\${APP_NAME}"
  Delete "$INSTDIR\\README.md"
  Delete "$INSTDIR\\LICENSE"
  RMDir /r "$INSTDIR\\bin"
  RMDir /r "$INSTDIR\\runtime"
  RMDir /r "$INSTDIR\\scripts"
  Delete "$INSTDIR\\Uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "Software\\${APP_NAME}"
SectionEnd
