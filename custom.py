'''
Modify this file, by platform, to handle nonstandard options for third-party
dependencies. If you do modify this file, you should make it read-only
(or otherwise protect it) so that CVS will not overwrite it.

Order is important: place local paths ahead of system paths.
'''
import sys

customCPPPATH = []
customCCFLAGS = []
customCXXFLAGS = []
customLIBS = []
customLIBPATH = []
customSHLINKFLAGS = []
customSWIGFLAGS = []

if sys.platform[:5] == 'linux':
    platform = 'linux'
elif sys.platform == 'darwin':
    platform = 'darwin'
    customCPPPATH.append('/usr/include/malloc')
    customCPPPATH.append('/opt/local/include/boost-1_32/')
    customCPPPATH.append('/usr/include/python2.3')
    customCXXFLAGS.append('-fabi-version=0')
elif sys.platform[:3] == 'win':
    # For the basic build you need MinGW, MSys, and libsndfile.
    # Add them here:
    customLIBPATH.append('c:/utah/usr/mingw/lib')
    customCPPPATH.append('c:/utah/usr/msys/1.0/local/include')
    customLIBPATH.append('c:/utah/usr/msys/1.0/local/lib')
    # If libsndfile is not in a standard location add it here:
    #customCPPPATH.append('C:/utah/usr/libsndfile-1.0.11/src')
    #customLIBPATH.append('C:/utah/usr/libsndfile-1.0.11')
    ################################################################
    # If you want real-time audio you need PortAudio.
    # If it is not in a standard location add it here
    # (of course you must build it first):
    # customCPPPATH.append('C:/projects/portaudio/pa_common')
    # customLIBPATH.append('C:/projects/portaudio/lib')
    ################################################################
    # If you want FLTK widgets or if you want to build CsoundVST,
    # you need FLTK. If it is not in a standard location,
    # add it here (of course you must build it first):
    # customCPPPATH.append('C:/utah/opt/fltk-1.1.6')
    # customLIBPATH.append('C:/utah/opt/fltk-1.1.6/lib')
    ################################################################
    # If you want to build CsoundVST you need Python and
    # a MinGW import library for Python. Add them here:
    customCPPPATH.append('c:/utah/opt/Python23/include')
    customCPPPATH.append('c:/utah/usr/mingw/include')
    ################################################################
    # If you want to build Lua interfaces you need Lua.
    # Add it here:
    customCPPPATH.append('c:/utah/opt/lua50/include')
    customLIBPATH.append('c:/utah/opt/lua50/lib/mingw3')
    ################################################################
    # If you want to build CsoundVST you need boost.
    # If it is not in a standard lcoation add it here
    # (you do NOT need to build it first):
    customCPPPATH.append('c:/utah/opt/boost/')
    ################################################################
    # If you want to build the FluidSynth opcodes
    # you need FluidSynth. If it is not a standard location,
    # add it here (you do NOT need to build it first):
    #customLIBPATH.append('c:/utah/opt/fluidsynth-1.0.3-win32')
    #customCPPPATH.append('c:/utah/opt/fluidsynth-1.0.3-win32/include')
    ################################################################
    # If you want to build the Java wrapper for CsoundVST
    # you need Java. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append('c:/utah/opt/jdk1.5.0/include')
    customCPPPATH.append('c:/utah/opt/jdk1.5.0/include/win32')
    ################################################################
    # If you want to build the PD external csoundapi~
    # you need PD. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append('c:/utah/opt/pd-0.38-4-devel-2/src')
    customLIBPATH.append('c:/utah/opt/pd-0.38-4-devel-2/bin')
    platform = 'mingw'
else:
    platform = 'unsupported platform'

