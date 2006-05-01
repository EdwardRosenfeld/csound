#!/bin/sh

if [ "$#" == "2" ] ; then
  if [ "$1" == "remove" ] ; then
    if ( test -e "$2" ) ; then
      echo -e "Removing '$2'"
      rm -Rf "$2" ;
    fi ;
    exit 0 ;
  fi ;
  exit -1 ;
elif [ "$#" != "0" ] ; then
  exit 0 ;
fi

"$0" remove ".sconf_temp"
"$0" remove "config.log"

find . -type f -iname ".scons*" -exec "$0" remove '{}' \;
find . -type f -iname "*.xmg"   -exec "$0" remove '{}' \;
find . -type f -iname "*.o"     -exec "$0" remove '{}' \;
find . -type f -iname "*.os"    -exec "$0" remove '{}' \;
find . -type f -iname "*.so"    -exec "$0" remove '{}' \;
find . -type f -iname "*.a"     -exec "$0" remove '{}' \;
find . -type f -iname "*.wav"   -exec "$0" remove '{}' \;
find . -type f -iname "*.aif*"  -exec "$0" remove '{}' \;
find . -type f -iname "*.sf"    -exec "$0" remove '{}' \;
find . -type f -iname "*.pcm"   -exec "$0" remove '{}' \;
find . -type f -iname "*.exe"   -exec "$0" remove '{}' \;
find . -type f -iname "*.dll"   -exec "$0" remove '{}' \;
find . -type f -iname "*.obj"   -exec "$0" remove '{}' \;
find . -type f -iname "*.lib"   -exec "$0" remove '{}' \;
find "./interfaces" -type f -iname "*.java" -exec "$0" remove '{}' \;
find "./interfaces" -type f -iname "*.class" -exec "$0" remove '{}' \;

find . -type f -exec file '{}' \; | grep -G -e ':.*\<ELF\>' | cut -d ':' -f 1 | xargs -n 1 "$0" remove
find . -type f -exec file '{}' \; | grep -G -e ':.*\<80386\>' | cut -d ':' -f 1 | xargs -n 1 "$0" remove
find . -type f -exec file '{}' \; | grep -G -e ':.*\<PE\>' | cut -d ':' -f 1 | xargs -n 1 "$0" remove

"$0" remove "./frontends/CsoundVST/CsoundVST_wrap.cc"
"$0" remove "./frontends/CsoundVST/CsoundVST_wrap.h"
"$0" remove "./CsoundVST.py"
"$0" remove "./CsoundVST.pyc"
"$0" remove "./CsoundVST.pyo"
"$0" remove "./interfaces/python_interface_wrap.cc"
"$0" remove "./interfaces/python_interface_wrap.h"
"$0" remove "./interfaces/java_interface_wrap.cc"
"$0" remove "./interfaces/java_interface_wrap.h"
"$0" remove "./interfaces/lua_interface_wrap.cc"
"$0" remove "./csnd.py"
"$0" remove "./csnd.pyc"
"$0" remove "./csnd.pyo"
"$0" remove "./interfaces/csnd.py"
"$0" remove "./interfaces/csnd.pyc"
"$0" remove "./interfaces/csnd.pyo"
"$0" remove "./csnd.jar"
"$0" remove "./interfaces/csnd"
"$0" remove "./Opcodes/Loris/scripting/loris_wrap.cc"
"$0" remove "./loris.py"
"$0" remove "./loris.pyc"
"$0" remove "./loris.pyo"
"$0" remove "./frontends/winsound/winsound.cxx"
"$0" remove "./frontends/winsound/winsound.h"
"$0" remove "./frontends/CsoundVST/ScoreGeneratorVST_wrap.cc"
"$0" remove "./frontends/CsoundVST/ScoreGeneratorVST_wrap.h"
"$0" remove "./scoregen.py"
"$0" remove "./frontends/fltk_gui/CsoundAboutWindow_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundAboutWindow_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundGlobalSettingsPanel_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundGlobalSettingsPanel_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundGUIConsole_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundGUIConsole_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundGUIMain_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundGUIMain_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundPerformanceSettingsPanel_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundPerformanceSettingsPanel_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundUtilitiesWindow_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundUtilitiesWindow_FLTK.hpp"

rm -f "./libcsound.so"
rm -f "./libcsound64.so"
rm -f "./_csnd.so"
rm -f "./_CsoundVST.so"

