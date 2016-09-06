#!/bin/sh

if [ "$1" = "clean" ]
then
    echo "Cleaning up shared libraries..."
    find . -name "*.so" -delete
fi

cd ${CSOUND_HOME}/android/pluginlibs/luajit-2.0
make HOST_CC="gcc -m32" CROSS=$NDKP TARGET_FLAGS="$NDKF $NDKARCH" TARGET_SYS=linux $1

cd ${CSOUND_HOME}/android/pluginlibs/LuaCsound
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/stk-csound
mkdir -p ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
cp -rf ../stk/rawwaves/*.raw ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/libstdutil
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/doppler
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/fluidsynth-android
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/libfluidsynth
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/liblo-android
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/libOSC
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/libscansyn
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/signalflowgraph
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/CsoundAndroid
./build.sh $1
cd ..

rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd6
cp -r CsoundAndroid/src/csnd6  CsoundForAndroid/CsoundAndroid/src/main/java/

rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs

cd ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication

./install_libs.sh



