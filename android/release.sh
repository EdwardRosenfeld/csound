#!/bin/sh 

export RELEASE_DIR=csound-android-6.01.0

#remove backup files ending with ~
find . -name "*~" -exec rm {} \;

rm -rf CsoundAndroid/bin
rm -rf CsoundAndroid/obj
rm -rf CsoundAndroidExamples/bin
rm -rf CSDPlayer/bin

for plugin in pluginlibs/*
do
  rm -r $plugin/obj
done

#rm -rf pluginlibs/libfluidsynth/obj

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR
cd $RELEASE_DIR

cp ../COPYING .
cp ../CHANGELOG .
cp ../introduction_csound_android.pdf .
cp -R ../CSDPlayer .
cp -R ../CsoundAndroid .
cp -R ../CsoundAndroidExamples .
cp -R ../pluginlibs .
cd ..

rm ${RELEASE_DIR}.zip
zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR} 
