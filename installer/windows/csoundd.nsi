#######################################################################
# C S O U N D   5   N U L L S O F T   I N S T A L L E R   S C R I P T
# By Michael Gogins <michael.gogins@gmail.com>
#
# If this script is compiled with the /DFLOAT option,
# the installer will install the 'float samples' version of Csound;
# by default, the installer will install the 'double samples' version.
# If this script is compiled with the /DNONFREE option,
# the installer will install non-free software (CsoundVST and vst4cs);
# by default, the installer will omit all non-free software.
#######################################################################

!include MUI2.nsh
!include WinMessages.nsh

#######################################################################
# DEFINITIONS
#######################################################################

!define PRODUCT "Csound"
!define PROGRAM "Csound5.11.rc1"
!echo "Building installer for: ${PROGRAM}"
!ifdef FLOAT
!ifdef NONFREE
!define VERSION "gnu-win32-vst-f"
!else
!define VERSION "gnu-win32-f"
!endif
!echo "Building installer for single-precision samples."
!define OPCODEDIR_ENV "OPCODEDIR"
!define OPCODEDIR_VAL "plugins"
!else
!ifdef NONFREE
!define VERSION "gnu-win32-vst-d"
!else
!define VERSION "gnu-win32-d"
!endif
!echo "Building installer for double-precision samples."
!define OPCODEDIR_ENV "OPCODEDIR64"
!define OPCODEDIR_VAL "plugins64"
!endif
!ifdef NONFREE
!echo "Building installer with VST software."
!else
!echo "Building installer without VST software."
!endif

!define ALL_USERS
!ifdef ALL_USERS
!define WriteEnvStr_RegKey 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
!else
!define WriteEnvStr_RegKey 'HKCU "Environment"'
!endif

!define PYTHON_VERSION 2.6

!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_NODESC

#######################################################################
# GENERAL
#######################################################################

Name "${PRODUCT}"
OutFile "${PROGRAM}-${VERSION}.exe"
InstallDir "$PROGRAMFILES\${PRODUCT}"
# Get installation folder from registry if Csound is already installed.
InstallDirRegKey HKCU "Software\${PRODUCT}" ""

#######################################################################
# VARIABLES
#######################################################################

Var MUI_TEMP
Var STARTMENU_FOLDER

#######################################################################
# FUNCTIONS
#######################################################################

# WriteEnvStr - Write an environment variable
# Note: Win9x systems requires reboot
#
# Example:
#  Push "HOMEDIR"           # name
#  Push "C:\New Home Dir\"  # value
#  Call WriteEnvStr

Function WriteEnvStr
	 Exch $1 # $1 has environment variable value
	 Exch
	 Exch $0 # $0 has environment variable name
	 Push $2
	 Call IsNT
	 Pop $2
  	 StrCmp $2 1 WriteEnvStr_NT
    	 # Not on NT
    	 StrCpy $2 $WINDIR 2 # Copy drive of windows (c:)
    	 FileOpen $2 "$2\autoexec.bat" a
    	 FileSeek $2 0 END
    	 FileWrite $2 "$\r$\nSET $0=$1$\r$\n"
    	 FileClose $2
    	 SetRebootFlag true
	 Goto WriteEnvStr_done
WriteEnvStr_NT:
	WriteRegExpandStr ${WriteEnvStr_RegKey} $0 $1
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
WriteEnvStr_done:
	Pop $2
    	Pop $1
    	Pop $0
FunctionEnd

# un.DeleteEnvStr - Remove an environment variable
# Note: Win9x systems requires reboot
#
# Example:
#  Push "HOMEDIR"           # name
#  Call un.DeleteEnvStr

Function un.DeleteEnvStr
  	Exch $0 # $0 now has the name of the variable
  	Push $1
  	Push $2
  	Push $3
  	Push $4
  	Push $5
	Call un.IsNT
  	Pop $1
  	StrCmp $1 1 DeleteEnvStr_NT
    	# Not on NT
    	StrCpy $1 $WINDIR 2
    	FileOpen $1 "$1\autoexec.bat" r
    	GetTempFileName $4
    	FileOpen $2 $4 w
    	StrCpy $0 "SET $0="
    	SetRebootFlag true
DeleteEnvStr_dosLoop:
	FileRead $1 $3
      	StrLen $5 $0
      	StrCpy $5 $3 $5
      	StrCmp $5 $0 DeleteEnvStr_dosLoop
      	StrCmp $5 "" DeleteEnvStr_dosLoopEnd
      	FileWrite $2 $3
      	Goto DeleteEnvStr_dosLoop
DeleteEnvStr_dosLoopEnd:
	FileClose $2
      	FileClose $1
      	StrCpy $1 $WINDIR 2
      	Delete "$1\autoexec.bat"
      	CopyFiles /SILENT $4 "$1\autoexec.bat"
      	Delete $4
      	Goto DeleteEnvStr_done
DeleteEnvStr_NT:
	DeleteRegValue ${WriteEnvStr_RegKey} $0
    	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
DeleteEnvStr_done:
	Pop $5
    	Pop $4
    	Pop $3
    	Pop $2
    	Pop $1
    	Pop $0
FunctionEnd

;----------------------------------------
; based upon a script of "Written by KiCHiK 2003-01-18 05:57:02"
;----------------------------------------
!verbose 3
!include "WinMessages.NSH"
!verbose 4
;====================================================
; get_NT_environment 
;     Returns: the selected environment
;     Output : head of the stack
;====================================================
!macro select_NT_profile UN
Function ${UN}select_NT_profile
   MessageBox MB_YESNO|MB_ICONQUESTION "Change the environment for all users?$\r$\nSaying no here will change the envrironment for the current user only.$\r$\n(Administrator permissions required for all users,$\r$\ndefaults to Yes on silent installations)" /SD IDYES IDNO environment_single 
      DetailPrint "Selected environment for all users"
      Push "all"
      Return
   environment_single:
      DetailPrint "Selected environment for current user only."
      Push "current"
      Return
FunctionEnd
!macroend
!insertmacro select_NT_profile ""
!insertmacro select_NT_profile "un."
;----------------------------------------------------
!define NT_current_env 'HKCU "Environment"'
!define NT_all_env     'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
;====================================================
; IsNT - Returns 1 if the current system is NT, 0
;        otherwise.
;     Output: head of the stack
;====================================================
!macro IsNT UN
Function ${UN}IsNT
  Push $0
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 IsNT_yes
  ; we are not NT.
  Pop $0
  Push 0
  Return
 
  IsNT_yes:
    ; NT!!!
    Pop $0
    Push 1
FunctionEnd
!macroend
!insertmacro IsNT ""
!insertmacro IsNT "un."
;====================================================
; AddToPath - Adds the given dir to the search path.
;        Input - head of the stack
;        Note - Win9x systems requires reboot
;====================================================
Function AddToPath
   Exch $0
   Push $1
   Push $2
  
   Call IsNT
   Pop $1
   StrCmp $1 1 AddToPath_NT
      ; Not on NT
      StrCpy $1 $WINDIR 2
      FileOpen $1 "$1\autoexec.bat" a
      FileSeek $1 0 END
      GetFullPathName /SHORT $0 $0
      FileWrite $1 "$\r$\nSET PATH=%PATH%;$0$\r$\n"
      FileClose $1
      Goto AddToPath_done
 
   AddToPath_NT:
      Push $4
      Call select_NT_profile
      Pop  $4
 
      AddToPath_NT_selection_done:
      StrCmp $4 "current" read_path_NT_current
         ReadRegStr $1 ${NT_all_env} "PATH"
         Goto read_path_NT_resume
      read_path_NT_current:
         ReadRegStr $1 ${NT_current_env} "PATH"
      read_path_NT_resume:
      StrCpy $2 $0
      StrCmp $1 "" AddToPath_NTdoIt
         StrCpy $2 "$1;$0"
      AddToPath_NTdoIt:
         StrCmp $4 "current" write_path_NT_current
            ClearErrors
            WriteRegExpandStr ${NT_all_env} "PATH" $2
            IfErrors 0 write_path_NT_resume
            MessageBox MB_YESNO|MB_ICONQUESTION "The path could not be set for all users$\r$\nShould I try for the current user?" \
               IDNO write_path_NT_failed
            ; change selection
            StrCpy $4 "current"
            Goto AddToPath_NT_selection_done
         write_path_NT_current:
            ClearErrors
            WriteRegExpandStr ${NT_current_env} "PATH" $2
            IfErrors 0 write_path_NT_resume
            MessageBox MB_OK|MB_ICONINFORMATION "The path could not be set for the current user."
            Goto write_path_NT_failed
         write_path_NT_resume:
         SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
         DetailPrint "added path for user ($4), $0"
         write_path_NT_failed:
      
      Pop $4
   AddToPath_done:
   Pop $2
   Pop $1
   Pop $0
FunctionEnd
 
;====================================================
; RemoveFromPath - Remove a given dir from the path
;     Input: head of the stack
;====================================================
Function un.RemoveFromPath
   Exch $0
   Push $1
   Push $2
   Push $3
   Push $4
   
   Call un.IsNT
   Pop $1
   StrCmp $1 1 unRemoveFromPath_NT
      ; Not on NT
      StrCpy $1 $WINDIR 2
      FileOpen $1 "$1\autoexec.bat" r
      GetTempFileName $4
      FileOpen $2 $4 w
      GetFullPathName /SHORT $0 $0
      StrCpy $0 "SET PATH=%PATH%;$0"
      SetRebootFlag true
      Goto unRemoveFromPath_dosLoop
     
      unRemoveFromPath_dosLoop:
         FileRead $1 $3
         StrCmp $3 "$0$\r$\n" unRemoveFromPath_dosLoop
         StrCmp $3 "$0$\n" unRemoveFromPath_dosLoop
         StrCmp $3 "$0" unRemoveFromPath_dosLoop
         StrCmp $3 "" unRemoveFromPath_dosLoopEnd
         FileWrite $2 $3
         Goto unRemoveFromPath_dosLoop
 
      unRemoveFromPath_dosLoopEnd:
         FileClose $2
         FileClose $1
         StrCpy $1 $WINDIR 2
         Delete "$1\autoexec.bat"
         CopyFiles /SILENT $4 "$1\autoexec.bat"
         Delete $4
         Goto unRemoveFromPath_done
 
   unRemoveFromPath_NT:
      StrLen $2 $0
      Call un.select_NT_profile
      Pop  $4
 
      StrCmp $4 "current" un_read_path_NT_current
         ReadRegStr $1 ${NT_all_env} "PATH"
         Goto un_read_path_NT_resume
      un_read_path_NT_current:
         ReadRegStr $1 ${NT_current_env} "PATH"
      un_read_path_NT_resume:
 
      Push $1
      Push $0
      Call un.StrStr ; Find $0 in $1
      Pop $0 ; pos of our dir
      IntCmp $0 -1 unRemoveFromPath_done
         ; else, it is in path
         StrCpy $3 $1 $0 ; $3 now has the part of the path before our dir
         IntOp $2 $2 + $0 ; $2 now contains the pos after our dir in the path (';')
         IntOp $2 $2 + 1 ; $2 now containts the pos after our dir and the semicolon.
         StrLen $0 $1
         StrCpy $1 $1 $0 $2
         StrCpy $3 "$3$1"
 
         StrCmp $4 "current" un_write_path_NT_current
            WriteRegExpandStr ${NT_all_env} "PATH" $3
            Goto un_write_path_NT_resume
         un_write_path_NT_current:
            WriteRegExpandStr ${NT_current_env} "PATH" $3
         un_write_path_NT_resume:
         SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
   unRemoveFromPath_done:
   Pop $4
   Pop $3
   Pop $2
   Pop $1
   Pop $0
FunctionEnd
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall sutff
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 
 
;====================================================
; StrStr - Finds a given string in another given string.
;               Returns -1 if not found and the pos if found.
;          Input: head of the stack - string to find
;                      second in the stack - string to find in
;          Output: head of the stack
;====================================================
Function un.StrStr
  Push $0
  Exch
  Pop $0 ; $0 now have the string to find
  Push $1
  Exch 2
  Pop $1 ; $1 now have the string to find in
  Exch
  Push $2
  Push $3
  Push $4
  Push $5
 
  StrCpy $2 -1
  StrLen $3 $0
  StrLen $4 $1
  IntOp $4 $4 - $3
 
  unStrStr_loop:
    IntOp $2 $2 + 1
    IntCmp $2 $4 0 0 unStrStrReturn_notFound
    StrCpy $5 $1 $3 $2
    StrCmp $5 $0 unStrStr_done unStrStr_loop
 
  unStrStrReturn_notFound:
    StrCpy $2 -1
 
  unStrStr_done:
    Pop $5
    Pop $4
    Pop $3
    Exch $2
    Exch 2
    Pop $0
    Pop $1
FunctionEnd
;====================================================Retrieved from "http://nsis.sourceforge.net/Path_manipulation_with_all_users/current_user_selection_in_run-time"


#######################################################################
# INSTALLER PAGES
#######################################################################
  
!insertmacro MUI_PAGE_WELCOME

!ifdef NONFREE
!insertmacro MUI_PAGE_LICENSE ..\..\readme-csound5-complete.txt
!else
!insertmacro MUI_PAGE_LICENSE ..\..\readme-csound5.txt
!endif

!insertmacro MUI_PAGE_DIRECTORY

# Start Menu Folder Page Configuration
  
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PRODUCT}" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
!insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
 
#######################################################################
# LANGUAGES AND DESCRIPTIONS
#######################################################################

!insertmacro MUI_LANGUAGE "English"

LangString DESC_SecCopyUI ${LANG_ENGLISH} "Copy ${PRODUCT} to the application folder."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecCopyUI} $(DESC_SecCopyUI)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

#######################################################################
# INSTALLER SECTIONS
#######################################################################

InstType "Core"
InstType "Complete"

SectionGroup /e "Csound"
  Section "Csound engine, opcodes, and drivers"
    SectionIn 1 2 RO
	# Store the installation folder.
	WriteRegStr HKCU "Software\${PRODUCT}" "" $INSTDIR
	# Back up any old value of .csd.
	ReadRegStr $1 HKCR ".csd" ""
	StrCmp $1 "" noBackup
	StrCmp $1 "${PRODUCT}File" noBackup
	WriteRegStr HKCR ".csd" "backup_val" $1
noBackup:
	WriteRegStr HKCR ".csd" "" "${PRODUCT}File"
	ReadRegStr $0 HKCR "${PRODUCT}File" ""
	StrCmp $0 "" 0 skipAssoc
	WriteRegStr HKCR "${PRODUCT}File" "" "CSound Unified File"
	WriteRegStr HKCR "${PRODUCT}File\shell" "" "open"
	WriteRegStr HKCR "${PRODUCT}File\DefaultIcon" "" $INSTDIR\bin\${PROGRAM}.exe,0
skipAssoc:
	WriteRegStr HKCR "${PRODUCT}File\shell\open\command" "" '$INSTDIR\bin${PROGRAM}.exe "%1"'
	Push $INSTDIR\bin
	Call AddToPath
     	Push "CSOUNDRC" 
	Push $INSTDIR\.csoundrc
	Call WriteEnvStr
	Push ${OPCODEDIR_ENV}
	Push $INSTDIR\${OPCODEDIR_VAL}
	Call WriteEnvStr
	Push "RAWWAVE_PATH" 
	Push "$INSTDIR\samples"
	Call WriteEnvStr
	ReadEnvStr $1 "PYTHONPATH"
	Push "PYTHONPATH"
	Push "$1;$INSTDIR\bin"
	Call WriteEnvStr
	Push "SFOUTYP"
	Push "WAV"
	Call WriteEnvStr
	WriteUninstaller "$INSTDIR\Uninstall.exe"
    SetOutPath $INSTDIR\examples
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	# Create shortcuts. The format of these lines is:
	# link.lnk target.file [parameters [icon.file [icon_index_number [start_options [keyboard_shortcut [description]]]]]]
	CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Csound.lnk" "cmd" "/K $INSTDIR\bin\csound.exe" "" "" "" "" "Csound"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\License.lnk" "$INSTDIR\readme-csound5-complete.txt" "" "" "" "" "" "Csound README"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Manual.lnk" "$INSTDIR\doc\manual\indexframes.html" "" "" "" "" "" "Csound manual"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "" "" "" "" "Uninstall Csound"
	!insertmacro MUI_STARTMENU_WRITE_END
    SetOutPath $INSTDIR
!ifdef NONFREE
      File ..\..\readme-csound5-complete.txt
!else
      File ..\..\readme-csound5.txt
!endif
      File ..\..\etc\.csoundrc
    SetOutPath $INSTDIR\doc
      File ..\..\ChangeLog
      File ..\..\COPYING
      File ..\..\LICENSE.PortMidi
      File ..\..\LICENSE.FLTK
      File ..\..\LICENSE.PortAudio
      File ..\..\csound-build.pdf
    SetOutPath $INSTDIR\bin
      # Csound itself.
      File ..\..\csound.exe
      File ..\..\cs.exe
!ifdef FLOAT
      File ..\..\csound32.dll.5.2
!else
      File ..\..\csound64.dll.5.2
!endif
      # Third party libraries:
      # libsndfile
      File U:\Mega-Nerd\libsndfile\libsndfile-1.dll
      # FLTK
      File U:\fltk-mingw\src\mgwfltknox-1.3.dll
      File U:\fltk-mingw\src\mgwfltknox_images-1.3.dll
      # PortAudio
      File U:\portaudio\portaudio.dll
      # PortMIDI
      File U:\portmidi\portmidi.dll
      File U:\portmidi\porttime.dll
      # Fluidsynth
      File U:\fluidsynth-1.0.9\src\.libs\libfluidsynth-1.dll
      # Image opcodes
      File U:\zlib-1.2.3.win32\bin\zlib1.dll
      File U:\libpng-1.2.24\.libs\libpng-3.dll
      # OSC
      File U:\liblo-0.26\lo.dll
      # MusicXML
      File C:\utah\opt\libmusicxml-2.00-src\win32\codeblocks\libmusicxml2.dll
      # pthreads
      File U:\pthreads\Pre-built.2\lib\pthreadGC2.dll
      # C runtime library
      File C:\windows\system32\MSVCRT.DLL
    # Opcodes, drivers, and other modules:
    SetOutPath $INSTDIR\${OPCODEDIR_VAL}
      File ..\..\ambicode1.dll
      File ..\..\ampmidid.dll
      File ..\..\babo.dll
      File ..\..\barmodel.dll
      File ..\..\chua.dll
      File ..\..\compress.dll
      File ..\..\cs_date.dll
      File ..\..\cs_pan2.dll
      File ..\..\cs_pvs_ops.dll
      File ..\..\doppler.dll
      File ..\..\eqfil.dll
      File ..\..\fluidOpcodes.dll
      File ..\..\ftest.dll
      File ..\..\gabnew.dll
      File ..\..\grain4.dll
      File ..\..\harmon.dll
      File ..\..\hrtferX.dll
      File ..\..\hrtfnew.dll
      File ..\..\image.dll
      File ..\..\linear_algebra.dll
      File ..\..\loscilx.dll
      File ..\..\minmax.dll
      File ..\..\mixer.dll
      File ..\..\modal4.dll
      File ..\..\mutexops.dll
      File ..\..\osc.dll
      File ..\..\partikkel.dll
      File ..\..\phisem.dll
      File ..\..\physmod.dll
      File ..\..\pitch.dll
      File ..\..\pmidi.dll
      File ..\..\ptrack.dll
      File ..\..\pvoc.dll
      File ..\..\pvsbuffer.dll
      File ..\..\rtpa.dll
      File ..\..\rtwinmm.dll
      File ..\..\scansyn.dll
      File ..\..\scoreline.dll
      File ..\..\sfont.dll
      File ..\..\shape.dll
      File ..\..\stackops.dll
      File ..\..\stdopcod.dll
      File ..\..\stdutil.dll
      File ..\..\stk.dll
      File ..\..\system_call.dll
      File ..\..\ugakbari.dll
      File ..\..\vaops.dll
      File ..\..\vbap.dll
      File ..\..\virtual.dll
      File ..\..\vosim.dll
!ifdef NONFREE
      File ..\..\vst4cs.dll
!endif
      File ..\..\widgets.dll
    # Samples:
    SetOutPath $INSTDIR\samples
      File /r ..\..\samples\*
      File /r ..\..\Opcodes\stk\rawwaves\*.raw
    SetOutPath $INSTDIR\examples
      File ..\..\examples\CsoundAC.csd
      File ..\..\examples\CsoundVST.csd
      File ..\..\examples\trapped.csd
      File ..\..\examples\trapped-high-resolution.csd
      File ..\..\examples\xanadu.csd
      File ..\..\examples\xanadu-high-resolution.csd
      File ..\..\examples\tpscaler.csd
    SetOutPath $INSTDIR\examples\cscore
      File /r /x *.wav /x *.orc /x *.sco /x .#* /x *~ /x *.lindenmayer ..\..\examples\cscore\*.*
    SetOutPath $INSTDIR\examples\opcode_demos
      File /x *.wav /x *.orc /x *.sco ..\..\Opcodes\gab\examples\*.*
      File /x *.wav /x *.orc /x *.sco ..\..\examples\opcode_demos\*.*
  SectionEnd
  Section /o "Utilities"
    SectionIn 2
    SetOutPath $INSTDIR\bin
       File ..\..\atsa.exe
       File ..\..\csb64enc.exe
       File ..\..\cvanal.exe
       File ..\..\dnoise.exe
       File ..\..\envext.exe
       File ..\..\extract.exe
       File ..\..\extractor.exe
       File ..\..\het_export.exe
       File ..\..\het_import.exe
       File ..\..\hetro.exe
       File ..\..\lpanal.exe
       File ..\..\lpc_export.exe
       File ..\..\lpc_import.exe
       File ..\..\makecsd.exe
       File ..\..\mixer.exe
       File ..\..\pv_export.exe
       File ..\..\pv_import.exe
       File ..\..\pvanal.exe
       File ..\..\pvlook.exe
       File ..\..\scale.exe
       File ..\..\scot.exe
       File ..\..\scsort.exe
       File ..\..\sdif2ad.exe
       File ..\..\sndinfo.exe
       File ..\..\srconv.exe
  SectionEnd
  SectionGroup "Documentation"
    Section "Csound Reference Manual"
      SectionIn 1 2
      SetOutPath $INSTDIR\doc\manual
  	File /r ..\..\..\manual\html\*
    SectionEnd
    Section /o "A Csound Tutorial"
      SectionIn 2
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Tutorial.lnk" "$INSTDIR\tutorial\tutorial.pdf" "" "" "" "" "" "A Csound Tutorial"
      SetOutPath $INSTDIR\tutorial
  	File ..\..\..\tutorial\tutorial.pdf
  	File ..\..\..\tutorial\*.csd
  	File ..\..\..\tutorial\*.py
  	File ..\..\..\tutorial\tutorial3.cpr
    SectionEnd
    Section /o "A Csound Algorithmic Composition Tutorial"
      SectionIn 2
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CsoundAcTutorial.lnk" "$INSTDIR\tutorial\Csound_Algorithmic_Composition_Tutorial.pdf" "" "" "" "" "" "A Csound Algorithmic Composition Tutorial"
      SetOutPath $INSTDIR\tutorial
  	File ..\..\..\tutorial\Csound_Algorithmic_Composition_Tutorial.pdf
  	File /r ..\..\..\tutorial\code\*.csd
  	File /r ..\..\..\tutorial\code\*.py
  	File /r ..\..\..\tutorial\code\*.mid
    SectionEnd
  SectionGroupEnd
SectionGroupEnd
SectionGroup "Front ends"
  Section /o "QuteCsound (user-defined widgets)"
    SectionIn 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\qutecsound.lnk" "$INSTDIR\bin\qutecsound.exe" "" "" "" "" "" " QuteCsound"
    SetOutPath $INSTDIR\bin
      File ..\..\csnd.dll
      # QuteCsound
      File C:\utah\opt\Qt\2009.03\qt\bin\QtCore4.dll
      File C:\utah\opt\Qt\2009.03\qt\bin\QtGui4.dll
      File C:\utah\opt\Qt\2009.03\qt\bin\QtXml4.dll
      File C:\utah\opt\Qt\2009.03\qt\bin\mingwm10.dll
!ifdef FLOAT
      File D:\utah\opt\qutecsound\src\bin\qutecsound-f.exe
!else
      File D:\utah\opt\qutecsound\src\bin\qutecsound.exe
!endif
  SectionEnd
!ifdef NONFREE
  Section /o "CsoundVST (requires VST host)"
    SectionIn 2
    SetOutPath $INSTDIR\bin
      File ..\..\csnd.dll
      File ..\..\CsoundVST.dll
    SetOutPath $INSTDIR\include
      File ..\..\frontends\CsoundVST\*.h
      File ..\..\frontends\CsoundVST\*.hpp
      File ..\..\frontends\CsoundVST\*.def
  SectionEnd
!endif
  Section /o "tclcsound (requires TCL/Tk)"
    SectionIn 2
    SetOutPath $INSTDIR\bin
      File ..\..\cswish.exe
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\cswish.lnk" "$INSTDIR\bin\cswish.exe" "" "" "" "" "" "Csound wish (tcl/tk)"
      File ..\..\cstclsh.exe
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\cstclsh.lnk" "$INSTDIR\bin\cstclsh.exe" "" "" "" "" "" "Csound tcl shell"
      File ..\..\tclcsound.dll
    SetOutPath $INSTDIR\examples\tclcsound
      File /x *.wav /x *.orc /x *.sco ..\..\examples\tclcsound\*.*
  SectionEnd
  Section /o "csoundapi~ (requires Pure Data)"
    SectionIn 2
    SetOutPath $INSTDIR\bin
      File ..\..\csoundapi~.dll
    SetOutPath $INSTDIR\examples\csoundapi_tilde
      File /x *.wav /x *.orc /x *.sco ..\..\examples\csoundapi_tilde\*.*
  SectionEnd
SectionGroupEnd
SectionGroup "Csound interfaces"
  SectionGroup "C/C++"
    Section /o "cnsd: C/C++ interface to Csound"
      SectionIn 2
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\API Reference.lnk" "$INSTDIR\doc\api\index.html" "" "" "" "" "" "API reference"
      SetOutPath $INSTDIR\bin
        File ..\..\csnd.dll
      SetOutPath $INSTDIR\include
  	File ..\..\H\*.h
  	File ..\..\H\*.hpp
  	File ..\..\interfaces\*.hpp
!ifdef FLOAT
  	File ..\..\csound32.def
!else
  	File ..\..\csound64.def
!endif
      SetOutPath $INSTDIR\doc\api
        File ..\..\doc\html\*
      SetOutPath $INSTDIR\examples\c
        File /x *.wav /x *.orc /x *.sco /x .#* /x *~ /x *.lindenmayer ..\..\examples\c\*.*
    SectionEnd
    Section /o "CsoundAC: C++ interface to Csound algorithmic composition"
      SectionIn 2
      SetOutPath $INSTDIR\bin
        File ..\..\csnd.dll
        File ..\..\libCsoundAC.a
      SetOutPath $INSTDIR\include
  	File ..\..\H\*.h
  	File ..\..\H\*.hpp
  	File ..\..\interfaces\*.hpp
!ifdef FLOAT
  	File ..\..\csound32.def
!else
  	File ..\..\csound64.def
!endif
	File ..\..\frontends\CsoundAC\*.h
	File ..\..\frontends\CsoundAC\*.hpp
      SetOutPath $INSTDIR\doc\api
        File ..\..\doc\html\*
      SetOutPath $INSTDIR\examples\cplusplus
        File /x *.wav /x *.orc /x *.sco /x .#* /x *~ /x *.lindenmayer ..\..\examples\cplusplus\*.*
    SectionEnd
    Section /o "Plugin opcode SDK"
      SectionIn 2
      SetOutPath $INSTDIR\pluginSDK
        File ..\..\pluginSDK\SConstruct
        File ..\..\pluginSDK\examplePlugin.c
        File ..\..\pluginSDK\custom.py
    SectionEnd
  SectionGroupEnd
  SectionGroup "Lua (luajit included)"
    Section /o "luaCsnd: Lua interface to Csound"
      SectionIn 2
      SetOutPath $INSTDIR\bin
        File U:\Lua5.1\src\lua51.dll
        File U:\Lua5.1\src\luajit.exe
        File ..\..\csnd.dll
        File ..\..\luaCsnd.dll
      SetOutPath $INSTDIR\examples\lua
        File ..\..\examples\lua\lua_example.lua
    SectionEnd
    Section /o "luaCsoundAC: Lua interface to CsoundAC"
      SectionIn 2
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\luajit.lnk" "$INSTDIR\bin\luajit.exe" "" "" "" "" "" "Lua JIT shell"
      SetOutPath $INSTDIR\bin
        File U:\Lua5.1\src\lua51.dll
        File U:\Lua5.1\src\luajit.exe
        File ..\..\csnd.dll
        File ..\..\luaCsnd.dll
        File ..\..\luaCsoundAC.dll
      SetOutPath $INSTDIR\examples\lua
        File ..\..\examples\lua\Lindenmayer.lua
    SectionEnd
  SectionGroupEnd
  SectionGroup "Python (requires Python 2.6)"
    Section /o "Python opcodes"
      SectionIn 2
      SetOutPath $INSTDIR\${OPCODEDIR_VAL}
        File ..\..\py.dll
      SetOutPath $INSTDIR\examples\opcode_demos
        File /x *.wav  ..\..\Opcodes\py\examples\*.*
    SectionEnd
    Section /o "csnd: Python interface to Csound"
      SectionIn 2
      SetOutPath $INSTDIR\examples\python
        File /x *.wav /x *.orc /x *.sco /x .#* /x *~ /x *.lindenmayer ..\..\examples\python\*.*
      SetOutPath $INSTDIR\bin
        File ..\..\csnd.dll
	File ..\..\_csnd.pyd  
        File ..\..\csnd.py
      SetOutPath $INSTDIR\examples\python
    SectionEnd
    Section /o "CsoundAC: Python interface to Csound algorithmic comnposition"
      SectionIn 2
      SetOutPath $INSTDIR\bin
        File ..\..\csnd.dll
	File ..\..\_csnd.pyd  
        File ..\..\csnd.py
	File ..\..\_CsoundAC.pyd  
        File ..\..\CsoundAC.py
    SectionEnd
  SectionGroupEnd 
  SectionGroup "Java (requires Java)"
    Section /o "csnd: Java interface to Csound"
      SectionIn 2
      SetOutPath $INSTDIR\bin
        File ..\..\csnd.dll
        File ..\..\_jcsound.dll
        File ..\..\csnd.jar
      SetOutPath $INSTDIR\examples\java
  	File /x *.wav /x *.orc /x *.sco ..\..\examples\java\*.*
    SectionEnd
  SectionGroupEnd
  Section /o "Lisp interface to Csound (requires Lisp with CFFI)"
    SectionIn 2
    SetOutPath $INSTDIR\bin
       File ..\..\csnd.dll
       File ..\..\interfaces\csound.lisp
       File ..\..\interfaces\filebuilding.lisp
    SetOutPath $INSTDIR\examples\lisp
       File ..\..\interfaces\test.lisp
  SectionEnd
SectionGroupEnd

Section "Uninstall"
  	RMDir /r $INSTDIR
  	!insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
  	Delete "$SMPROGRAMS\$MUI_TEMP\Csound.lnk"
	Delete "$SMPROGRAMS\$MUI_TEMP\License.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\Manual.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\Tutorial.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\API Reference.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
	# Delete empty start menu parent directories.
  	StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
startMenuDeleteLoop:
	RMDir $MUI_TEMP
    	GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    	IfErrors startMenuDeleteLoopDone
    	StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
startMenuDeleteLoopDone:
	Push $INSTDIR
  	Call un.RemoveFromPath
  	Push "CSOUNDRC"
  	Call un.DeleteEnvStr 
	Push ${OPCODEDIR_ENV}
  	Call un.DeleteEnvStr 
!ifdef NONFREE
        Push "RAWWAVE_PATH"
       	Call un.DeleteEnvStr 
!endif
	Push "SFOUTYP"
  	Call un.DeleteEnvStr
  	DeleteRegKey /ifempty HKCU "Software\${PRODUCT}"
SectionEnd
