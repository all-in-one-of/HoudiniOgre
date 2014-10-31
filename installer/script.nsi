; custom config

!define ProductName "HoudiniOgre9"
!define ProductVersion "1.4.1107"
!define Company "EDM Studio Inc"
!define Homepage "http://www.edmstudio.com"

!define DocSource "..\doc"
!define DllSource "..\lib"
!define ImgSource "."

;!define OgreSource "..\..\ogrenew\lib"
!define OgreSource "..\OgreSDK\bin\release"

!define UninstallerRegistryKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ProductName}"
!define UninstallerExecutableName "Uninst_HoudiniOgre.exe"


; nsis config
Name "${ProductName}"
OutFile "houdiniogre9-setup.exe"

SetCompressor lzma


; user vars

Var DirList
Var ActiveVersion


; UI config

#!define MUI_ICON "youricon.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_RIGHT

!define MUI_WELCOMEFINISHPAGE_BITMAP "welcomefinish.bmp"


!include MUI.nsh

;; installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${DocSource}\license.txt"
Page custom dirPRE dirLEAVE
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_LINK "EDM Studio Inc"
!define MUI_FINISHPAGE_LINK_LOCATION "${Homepage}"
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!insertmacro MUI_PAGE_FINISH

;; uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;; supported languages
!insertmacro MUI_LANGUAGE "english"


; installer

Section "install"
  SetOutPath "$INSTDIR\houdini\dso"
  File "${DllSource}\HoudiniOgre.dll"
  
  SetOutPath "$INSTDIR\houdini\config\Icons"
  File "${DocSource}\ROP_OgreExport.icon"
  
  SetOutPath "$INSTDIR\bin"
  File "${OgreSource}\OgreMain.dll"
  
  SetOutPath "$INSTDIR\mozilla\documents\nodes\out"
  File "${DocSource}\OgreExport.txt"
  
  ;SetOutPath "$INSTDIR\mozilla\help\content\helpcards\_icons\large\THOR"
  SetOutPath "$INSTDIR\mozilla\documents\icons\large"
  File "${DocSource}\ROP_OgreExport.png"

  SetOutPath "$INSTDIR"
  WriteUninstaller "${UninstallerExecutableName}"
  
  WriteRegStr HKLM "${UninstallerRegistryKey}" "DisplayName" "${ProductName}"
  WriteRegStr HKLM "${UninstallerRegistryKey}" "DisplayVersion" "${ProductVersion}"
  WriteRegStr HKLM "${UninstallerRegistryKey}" "DisplayIcon" "$OUTDIR\${UninstallerExecutableName},0"
  WriteRegStr HKLM "${UninstallerRegistryKey}" "Publisher" "${Company}"
  WriteRegStr HKLM "${UninstallerRegistryKey}" "URLInfoAbout" "${Homepage}"
  WriteRegStr HKLM "${UninstallerRegistryKey}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UninstallerRegistryKey}" "InstallSource" "$EXEDIR"
  WriteRegStr HKLM "${UninstallerRegistryKey}" "UninstallString" "$OUTDIR\${UninstallerExecutableName}"
  WriteRegStr HKLM "${UninstallerRegistryKey}" "ModifyPath" "$OUTDIR\${UninstallerExecutableName}"
SectionEnd

Function .onInit
  # prevent the installer from beeing executed twice a time
  System::Call "kernel32::CreateMutexA(i 0, i 0, t '$(^Name)') i .n ?e" # create a mutex
  Pop $0
  StrCmp $0 0 launch
  
  StrLen $0 "$(^Name)"
  IntOp $0 $0 + 1  
loop:
  FindWindow $1 '#32770' '' 0 $1 # search the installers window if already running
  IntCmp $1 0 +4
  System::Call /NOUNLOAD "user32.dll::GetWindowText(i r1, t .r2, i r0) i.n"
  StrCmp $2 "$(^Name)" 0 loop # check back for correct window text
  System::Call "user32::SetForegroundWindow(i r1) i.n" # bring it to front if found
Abort # then exit

launch:
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "dirpage.ini"
FunctionEnd

Function dirPRE
  ReadRegStr $ActiveVersion HKLM "Software\Side Effects Software" "ActiveVersion"
  StrCpy $R1 "0"
  loop:
  	EnumRegKey $0 HKLM "Software\Side Effects Software" $R1
  	StrCmp $0 "" end
    IntOp $R1 $R1 + 1
    StrCpy $1 $0 8
    StrCmp $1 "Houdini " 0 loop
    ReadRegStr $2 HKLM "Software\Side Effects Software\$0" "InstallPath"
    IfFileExists "$2\*.*" 0 loop
    ReadRegStr $3 HKLM "Software\Side Effects Software\$0" "Version"
    StrCpy $DirList "$DirList|v$3:  $2"
    Goto loop
  
  end:
  StrCpy $DirList $DirList "" 1
  ReadRegStr $2 HKLM "Software\Side Effects Software\Houdini $ActiveVersion" "InstallPath"
  StrCpy $ActiveVersion "v$ActiveVersion:  $2"
  
  !insertmacro MUI_HEADER_TEXT "Houdini Directory" "Please select the appropriate Houdini installation."
  !insertmacro MUI_INSTALLOPTIONS_WRITE "dirpage.ini" "Field 3" "Text" "$_CLICK"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "dirpage.ini" "Field 2" "Text" "${ProductName} can be installed to the following Houdini versions found on your system.\r\n"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "dirpage.ini" "Field 1" "State" "$ActiveVersion"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "dirpage.ini" "Field 1" "ListItems" "$DirList"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "dirpage.ini"
FunctionEnd

Function dirLEAVE
  !insertmacro MUI_INSTALLOPTIONS_READ "$0" "dirpage.ini" "Field 1" "State"
  StrLen $R0 $0
  StrCpy $R1 "0"
  loop:	
    IntOp $R1 $R1 + 1    
    StrCmp $R1 $R0 0 +2
    Abort
    StrCpy $1 $0 3 $R1
    StrCmp $1 ":  " end loop
  end:
  IntOp $R1 $R1 + 3
  StrCpy $INSTDIR $0 "" $R1
FunctionEnd


; uninstaller

Section "un.install"
  SetOutPath "$INSTDIR" 
  Delete "$OUTDIR\houdini\dso\HoudiniOgre.dll"
  Delete "$OUTDIR\houdini\config\Icons\ROP_OgreExport.icon"
  Delete "$OUTDIR\bin\OgreMain.dll"
  Delete "$OUTDIR\mozilla\help\content\helpcards\out\OgreExport.html"
  Delete "$OUTDIR\mozilla\help\content\helpcards\_icons\large\THOR\ROP_OgreExport.png"
  
  DeleteRegKey HKLM "${UninstallerRegistryKey}"
  
  Delete "$OUTDIR\${UninstallerExecutableName}"
SectionEnd

Function un.onInit
  # prevent the uninstaller from beeing executed twice a time
  System::Call "kernel32::CreateMutexA(i 0, i 0, t '$(^Name)') i .n ?e" # create a mutex
  Pop $0
  StrCmp $0 0 0 +2
Return
  
  StrLen $0 "$(^Name)"
  IntOp $0 $0 + 1  
loop:
  FindWindow $1 '#32770' '' 0 $1 # search the uninstallers window if already running
  IntCmp $1 0 +4
  System::Call /NOUNLOAD "user32.dll::GetWindowText(i r1, t .r2, i r0) i.n"
  StrCmp $2 "$(^Name)" 0 loop # check back for correct window text
  System::Call "user32::SetForegroundWindow(i r1) i.n" # bring it to front if found
Abort # then exit
FunctionEnd