export IFMKBASE=../csound5_install/csoundlib/package_contents/Library/Frameworks
export APPSBASE=../csound5_install/csoundapps/package_contents/usr/local/bin
export FMKBASE=./CsoundLib.framework

cp _csnd.so ../csound5_install/csoundlib/package_contents/System/Library/Frameworks/Python.framework/Versions/2.3/lib/Python2.3/
cp csnd.py ../csound5_install/csoundlib/package_contents/System/Library/Frameworks/Python.framework/Versions/2.3/lib/Python2.3/
cp lib_csnd.dylib $FMKBASE/Versions/5.1/
cp tclcsound.dylib $FMKBASE/Versions/5.1/Resources/TclTk/
cp lib_jcsound.jnilib $FMKBASE/Versions/5.1/Resources/Java/
cp csnd.jar $FMKBASE/Versions/5.1/Resources/Java/
cp csoundapi~.pd_darwin $FMKBASE/Versions/5.1/Resources/PD
cp interfaces/csound5.lisp $FMKBASE/Versions/5.1/Resources/Lisp
cp -R $FMKBASE $IFMKBASE
          
cp csound  $APPSBASE
cp dnoise   $APPSBASE
cp het_export $APPSBASE     
cp lpanal  $APPSBASE        
cp scale  $APPSBASE         
cp tabdes $APPSBASE
cp cs   $APPSBASE           
cp cstclsh $APPSBASE        
cp envext $APPSBASE          
cp het_import $APPSBASE     
cp lpc_export $APPSBASE     
cp mixer  $APPSBASE         
cp scsort $APPSBASE
cp cswish  $APPSBASE        
cp extract $APPSBASE         
cp hetro $APPSBASE          
cp lpc_import $APPSBASE     
cp pvanal  $APPSBASE        
cp sndinfo $APPSBASE
cp csb64enc $APPSBASE        
cp cvanal  $APPSBASE        
cp extractor $APPSBASE       
cp linseg    $APPSBASE      
cp makecsd   $APPSBASE      
cp pvlook   $APPSBASE       
cp srconv $APPSBASE

cp cswish frontends/tclcsound/cswish.app/Contents/MacOS/.
cp -R frontends/tclcsound/cswish.app "../csound5_install/csoundapps/package_contents/Applications/Csound 5 Wish.app"
cp -R "frontends/OSX/build/Csound 5.app" ../csound5_install/csoundapps/package_contents/Applications
cp -R frontends/Winsound/Winsound.app "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/."
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/Winsound.app/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/Winsound.app/Contents/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/Winsound.app/Contents/Resources/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/Winsound.app/Contents/MacOS/CVS"
cp -R frontends/fltk_gui/csound5GUI.app "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/."
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/csound5GUI.app/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/csound5GUI.app/Contents/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/csound5GUI.app/Contents/Resources/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/csound5GUI.app/Contents/MacOS/CVS"


