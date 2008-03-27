#J vim:syntax=python
print '''
C S O U N D 5

SCons build file for Csound 5:
API library, plugin opcodes, utilities, and front ends.

By Michael Gogins <gogins at pipeline dot com>

For custom options, run 'scons -h'.
For default options, run 'scons -H'.
If headers or libraries are not found, edit 'custom.py'.
For Linux, run in the standard shell
    with standard Python and just run 'scons'.
For MinGW, run in the MSys shell
    and use www.python.org WIN32 Python to run scons.
For Microsoft Visual C++, run in the Platform SDK
    command shell, and use www.python.org WIN32 Python to run scons.
'''

import time
import glob
import os
import os.path
import sys
import string
import shutil
import copy

#############################################################################
#
#   UTILITY FUNCTIONS
#############################################################################

pluginLibraries = []
executables = []
headers = Split('''
    H/cfgvar.h H/cscore.h H/csdl.h H/csound.h H/csound.hpp H/csoundCore.h
    H/cwindow.h H/msg_attr.h H/OpcodeBase.hpp H/pstream.h H/pvfileio.h
    H/soundio.h H/sysdep.h H/text.h H/version.h
    interfaces/CsoundFile.hpp interfaces/CppSound.hpp interfaces/filebuilding.h
''')
libs = []
pythonModules = []

def today():
    return time.strftime("%Y-%m-%d", time.localtime())

# Detect OPERATING SYSTEM platform.

def getPlatform():
    if sys.platform[:5] == 'linux':
        return 'linux'
    elif sys.platform[:3] == 'win':
        return 'win32'
    elif sys.platform[:6] == 'darwin':
        return 'darwin'
    else:
        return 'unsupported'

#############################################################################
#
#   DEFINE CONFIGURATION
#
#############################################################################

print "System platform is '" + getPlatform() + "'."

# Create options that can be set from the command line.

commandOptions = Options()
commandOptions.Add('CC')
commandOptions.Add('CXX')
commandOptions.Add('LINK')
commandOptions.Add('LINKFLAGS')
commandOptions.Add('custom',
    'Specify name of custom options file (default is "custom.py")',
    'custom.py')
commandOptions.Add('useDouble',
    'Set to 1 to use double-precision floating point for audio samples.',
    '0')
commandOptions.Add('usePortAudio',
    'Set to 1 to use PortAudio for real-time audio input and output.',
    '1')
commandOptions.Add('usePortMIDI',
    'Build PortMidi plugin for real time MIDI input and output.',
    '1')
commandOptions.Add('useALSA',
    'Set to 1 to use ALSA for real-time audio and MIDI input and output.',
    '1')
commandOptions.Add('useJack',
    'Set to 1 if you compiled PortAudio to use Jack; also builds Jack plugin.',
    '1')
commandOptions.Add('useFLTK',
    'Set to 1 to use FLTK for graphs and widget opcodes.',
    '1')
commandOptions.Add('noFLTKThreads',
    'Set to 1 to disable use of a separate thread for FLTK widgets.',
    '1')
commandOptions.Add('pythonVersion',
    'Set to the Python version to be used.',
    '%d.%d' % (int(sys.hexversion) >> 24, (int(sys.hexversion) >> 16) & 255))
commandOptions.Add('buildCsoundVST',
    'Set to 1 to build CsoundVST (needs CsoundAC, FLTK, boost, Python, SWIG).',
    '0')
commandOptions.Add('buildCsoundAC',
    'Set to 1 to build CsoundAC (needs FLTK, boost, Python, SWIG).',
    '0')
commandOptions.Add('buildCsound5GUI',
    'Build FLTK GUI frontend (requires FLTK 1.1.7 or later).',
    '0')
commandOptions.Add('generateTags',
    'Set to 1 to generate TAGS',
    '0')
commandOptions.Add('generatePdf',
    'Set to 1 to generate PDF documentation',
    '0')
commandOptions.Add('buildLoris',
    'Set to 1 to build the Loris Python extension and opcodes',
    '1')
commandOptions.Add('useOSC',
    'Set to 1 if you want OSC support',
    '0')
if getPlatform() != 'win32':
    commandOptions.Add('useUDP',
        'Set to 0 if you do not want UDP support',
        '1')
else:
    commandOptions.Add('useUDP',
        'Set to 1 if you want UDP support',
        '0')
commandOptions.Add('buildPythonOpcodes',
    'Set to 1 to build Python opcodes',
    '0')
commandOptions.Add('prefix',
    'Base directory for installs. Defaults to /usr/local.',
    '/usr/local')
commandOptions.Add('buildRelease',
    'Set to 1 to build for release (implies noDebug).',
    '0')
commandOptions.Add('noDebug',
    'Build without debugging information.',
    '0')
commandOptions.Add('gcc3opt',
    'Enable gcc 3.3.x or later optimizations for the specified CPU architecture (e.g. pentium3); implies noDebug.',
    '0')
commandOptions.Add('gcc4opt',
    'Enable gcc 4.0 or later optimizations for the specified CPU architecture (e.g. pentium3); implies noDebug.',
    '0')
commandOptions.Add('useLrint',
    'Use lrint() and lrintf() for converting floating point values to integers.',
    '0')
commandOptions.Add('useGprof',
    'Build with profiling information (-pg).',
    '0')
commandOptions.Add('Word64',
    'Build for 64bit computer',
    '0')
commandOptions.Add('Lib64',
    'Build for lib64 rather than lib',
    '0')
if getPlatform() == 'win32':
    commandOptions.Add('dynamicCsoundLibrary',
        'Set to 0 to build static Csound library instead of csound.dll',
        '1')
else:
    commandOptions.Add('dynamicCsoundLibrary',
        'Build dynamic Csound library instead of libcsound.a',
        '0')
commandOptions.Add('buildStkOpcodes',
    "Build opcodes encapsulating Perry Cook's Synthesis Toolkit in C++ instruments and effects",
    '0')
commandOptions.Add('install',
    'Enables the Install targets',
    '0')
commandOptions.Add('buildPDClass',
    "build csoundapi~ PD class (needs m_pd.h in the standard places)",
    '0')
commandOptions.Add('useCoreAudio',
    "Set to 1 to use CoreAudio for real-time audio input and output.",
    '1')
commandOptions.Add('useAltivec',
    "On OSX use the gcc AltiVec optmisation flags",
    '0')
commandOptions.Add('buildDSSI',
    "Build DSSI/LADSPA host opcodes",
    '1')
commandOptions.Add('buildUtilities',
    "Build stand-alone executables for utilities that can also be used with -U",
    '1')
commandOptions.Add('buildTclcsound',
    "Build Tclcsound frontend (cstclsh, cswish and tclcsound dynamic module). Requires Tcl/Tk headers and libs",
    '0')
commandOptions.Add('buildWinsound',
    "Build Winsound frontend. Requires FLTK headers and libs",
    '0')
commandOptions.Add('buildVirtual',
    "Build Virtual MIDI keyboard. Requires FLTK 1.1.7 or later headers and libs",
    '0')
commandOptions.Add('buildInterfaces',
    "Build interface library for Python, JAVA, Lua, C++, and other languages.",
    '0')
commandOptions.Add('buildJavaWrapper',
    'Set to 1 to build Java wrapper for the interface library.',
    '0')
commandOptions.Add('buildOSXGUI',
    'On OSX, set to 1 to build the basic GUI frontend',
    '0')
commandOptions.Add('buildCSEditor',
    'Set to 1 to build the Csound syntax highlighting text editor. Requires FLTK headers and libs',
    '0')
commandOptions.Add('withICL',
    'On Windows, set to 1 to build with the Intel C++ Compiler (also requires Microsoft Visual C++), or set to 0 to build with MinGW',
    '0')
commandOptions.Add('withMSVC',
    'On Windows, set to 1 to build with Microsoft Visual C++, or set to 0 to build with MinGW',
    '0')
commandOptions.Add('buildNewParser',
    'Enable building new parser (requires Flex/Bison)',
    '0')
commandOptions.Add('buildvst4cs',
    'Set to 1 to build vst4cs plugins (requires Steinberg VST headers)',
    '0')
commandOptions.Add('buildSDFT',
    'Set to 0 to avoid building SDFT code',
    '1')
commandOptions.Add('useGettext',
    'Set to 1 to use the GBU internationalisation/localisation scheme)',
    '0')
commandOptions.Add('buildImageOpcodes',
    'Set to 0 to avoid building image opcodes',
    '1')
##### JPff for OLPC #####
commandOptions.Add('buildOLPC',
    'Set to 1 to build OLPC version',
    '0')

# Define the common part of the build environment.
# This section also sets up customized options for third-party libraries, which
# should take priority over default options.

commonEnvironment = Environment(ENV = {'PATH' : os.environ['PATH']})
commandOptions.Update(commonEnvironment)

def compilerIntel():
    if getPlatform() == 'win32' and commonEnvironment['withICL'] == '1':
        return True
    else:
        return False

def compilerMicrosoft():
    if getPlatform() == 'win32' and commonEnvironment['withMSVC'] == '1':
        return True
    else:
        return False

def compilerGNU():
    if not compilerIntel() and not compilerMicrosoft():
        return True
    else:
        return False

optionsFilename = 'custom.py'

if compilerIntel():
	print 'Importing complete environment for the Intel C++ Compiler (ICL)...'
	commonEnvironment = Environment(ENV = os.environ)
	commandOptions.Update(commonEnvironment)
	Tool('icl')(commonEnvironment)
	optionsFilename = 'custom-msvc.py'
elif compilerMicrosoft():
	# To enable the Microsoft tools, 
	# the WHOLE environment must be imported,
	# not just the PATH; so we replace the 
	# enviroment with a more complete one.
	print 'Importing complete environment for Microsoft Visual C++ (MSVC)...'
	commonEnvironment = Environment(ENV = os.environ)
	commandOptions.Update(commonEnvironment)
	optionsFilename = 'custom-msvc.py'
elif getPlatform() == 'win32':
        # Similarly, on Windows, to exclude MSVC tools,
	# we have to force MinGW tools and then re-create
	# the environment from scratch.
	commonEnvironment = Environment(ENV = os.environ, tools = ['mingw', 'swig', 'javac', 'jar'])
	commandOptions.Update(commonEnvironment)
	#Tool('mingw')(commonEnvironment)
	optionsFilename = 'custom-mingw.py'

Help(commandOptions.GenerateHelpText(commonEnvironment))

if commonEnvironment['custom']:
    optionsFilename = commonEnvironment['custom']

print "Using options from '%s.'" % optionsFilename

fileOptions = Options(optionsFilename)
fileOptions.Add('customCPPPATH', 'List of custom CPPPATH variables')
fileOptions.Add('customCCFLAGS')
fileOptions.Add('customCXXFLAGS')
fileOptions.Add('customLIBS')
fileOptions.Add('customLIBPATH')
fileOptions.Add('customSHLINKFLAGS')
fileOptions.Add('customSWIGFLAGS')
fileOptions.Update(commonEnvironment)

customCPPPATH = commonEnvironment['customCPPPATH']
commonEnvironment.Prepend(CPPPATH = customCPPPATH)
customCCFLAGS = commonEnvironment['customCCFLAGS']
commonEnvironment.Prepend(CCFLAGS = customCCFLAGS)
customCXXFLAGS = commonEnvironment['customCXXFLAGS']
commonEnvironment.Prepend(CXXFLAGS = customCXXFLAGS)
customLIBS = commonEnvironment['customLIBS']
commonEnvironment.Prepend(LIBS = customLIBS)
customLIBPATH = commonEnvironment['customLIBPATH']
commonEnvironment.Prepend(LIBPATH = customLIBPATH)
customSHLINKFLAGS = commonEnvironment['customSHLINKFLAGS']
commonEnvironment.Prepend(SHLINKFLAGS = customSHLINKFLAGS)
customSWIGFLAGS = commonEnvironment['customSWIGFLAGS']
commonEnvironment.Prepend(SWIGFLAGS = customSWIGFLAGS)

if commonEnvironment['buildOLPC'] == '1':
    commonEnvironment['pythonVersion'] = '2.5'
    buildOLPC = True;
    commonEnvironment.Prepend(CPPFLAGS = ['-DOLPC'])
    commonEnvironment.Prepend(CPPFLAGS = ['-DENABLE_OPCODEDIR_WARNINGS=0'])
    if getPlatform() != 'linux':
       print "Build platform is not linux"
    # Set other options??
    commonEnvironment['useGettext'] = '1'
    commonEnvironment['useDouble'] = '0'
    commonEnvironment['usePortAudio'] = '0'
    commonEnvironment['useJack'] = '0'
    jackFound = False
    commonEnvironment['buildCsoundAC'] = '0'
    commonEnvironment['buildCsound5GUI'] = '0'
    commonEnvironment['useDouble'] = '0'
    commonEnvironment['usePortMIDI'] = '0'
    commonEnvironment['useALSA'] = '1'
    commonEnvironment['useFLTK'] = '0'
    commonEnvironment['buildCsoundVST'] = '0'
    commonEnvironment['buildCsoundAC'] = '0'
    #'buildCsound5GUI'
    commonEnvironment['buildLoris'] = '0'
    commonEnvironment['buildStkOpcodes'] = '0'
    commonEnvironment['useOSC'] = '1'
    commonEnvironment['prefix'] = '/usr'
    commonEnvironment['buildUtilities'] = '0'
    fltkFound = False
    portaudioFound = False
    oscFound = True
    ##commonEnvironment['gcc4opt'] = '1'
    commonEnvironment['useLrint'] = '1'
    commonEnvironment['Word64'] = '0'
    commonEnvironment['Lib64'] = '0'
    commonEnvironment['buildPDClass'] = '0'
    commonEnvironment['buildDSSI'] = '0'
    commonEnvironment['buildVirtual'] = '1'
    commonEnvironment['buildInterfaces'] = '1'
    commonEnvironment['buildSDFT'] = '0'
    commonEnvironment['buildJavaWrapper'] = '0'
    commonEnvironment['buildNewParser'] = '0'
    commonEnvironment['buildvst4cs'] = '0'
    commonEnvironment['buildImageOpcodes'] = '1'
    commonEnvironment['dynamicCsoundLibrary'] = '1'
else:
    buildOLPC = False

# Define options for different platforms.
if getPlatform() != 'win32':
    print "Build platform is '" + getPlatform() + "'."
else:
    if compilerMicrosoft():
        print "Build platform is Microsoft Visual C++ (MSVC)."
    elif compilerIntel():
        print "Build platform is the Intel C++ Compiler (ICL)."
    elif compilerGNU():
        print "Build platform is MinGW/MSYS"

print "SCons tools on this platform: ", commonEnvironment['TOOLS']

commonEnvironment.Prepend(CPPPATH = ['.', './H'])
if commonEnvironment['useLrint'] != '0':
    commonEnvironment.Prepend(CCFLAGS = ['-DUSE_LRINT'])
if commonEnvironment['useGettext'] != '0':
    print "Using GNU gettext scheme"
    commonEnvironment.Prepend(CCFLAGS = ['-DGNU_GETTEXT'])
    if getPlatform() == "win32":
        commonEnvironment.Append(LIBS=['intl'])
else:
    print "Using Istvan localisation"
    commonEnvironment.Prepend(CCFLAGS = ['-DNOGETTEXT'])
    
if commonEnvironment['gcc3opt'] != '0' or commonEnvironment['gcc4opt'] != '0':
    commonEnvironment.Prepend(CCFLAGS = Split('-fomit-frame-pointer -ffast-math'))
    if commonEnvironment['gcc4opt'] != '0':
        commonEnvironment.Prepend(CCFLAGS = ['-ftree-vectorize'])
        cpuType = commonEnvironment['gcc4opt']
    else:
        cpuType = commonEnvironment['gcc3opt']
    if getPlatform() == 'darwin':
        commonEnvironment.Prepend(CCFLAGS = Split('-O3 -mcpu=%s -mtune=%s' % (cpuType, cpuType)))
    else:
        commonEnvironment.Prepend(CCFLAGS = Split('-O3 -mtune=%s' % (cpuType)))
     
if commonEnvironment['buildRelease'] != '0':
    if compilerMicrosoft():
        commonEnvironment.Append(CCFLAGS = Split('/O2'))
    elif compilerIntel():
        commonEnvironment.Append(CCFLAGS = Split('/O3'))
 
if commonEnvironment['noDebug'] == '0':
    if compilerGNU() :
        commonEnvironment.Append(CCFLAGS = ['-g'])
         
if commonEnvironment['useGprof'] == '1':
    commonEnvironment.Append(CCFLAGS = ['-pg'])
    commonEnvironment.Append(LINKFLAGS = ['-pg'])

if compilerGNU():
    commonEnvironment.Prepend(CXXFLAGS = ['-fexceptions'])

commonEnvironment.Prepend(LIBPATH = ['.', '#.'])

if commonEnvironment['buildRelease'] == '0':
    commonEnvironment.Prepend(CPPFLAGS = ['-DBETA'])

if commonEnvironment['Lib64'] == '1':
    commonEnvironment.Prepend(LIBPATH = ['.', '#.', '/usr/local/lib64'])
else:
    commonEnvironment.Prepend(LIBPATH = ['.', '#.', '/usr/local/lib'])

if commonEnvironment['Word64'] == '1':
    commonEnvironment.Append(CCFLAGS = ['-fPIC'])

if commonEnvironment['useDouble'] == '0':
    print 'CONFIGURATION DECISION: Using single-precision floating point for audio samples.'
else:
    print 'CONFIGURATION DECISION: Using double-precision floating point for audio samples.'
    commonEnvironment.Append(CPPFLAGS = ['-DUSE_DOUBLE'])

if commonEnvironment['buildSDFT'] == '1':
    commonEnvironment.Prepend(CPPFLAGS = ['-DSDFT'])

# Define different build environments for different types of targets.

if getPlatform() == 'linux':
    commonEnvironment.Append(CCFLAGS = "-DLINUX")
    commonEnvironment.Append(CPPFLAGS = '-DHAVE_SOCKETS')
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CPPPATH = '/usr/X11R6/include')
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    commonEnvironment.Append(LINKFLAGS = ['-Wl,-Bdynamic'])
elif getPlatform() == 'darwin':
    commonEnvironment.Append(CCFLAGS = "-DMACOSX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    if commonEnvironment['useAltivec'] == '1':
        print 'CONFIGURATION DECISION using Altivec optmisation'
        commonEnvironment.Append(CCFLAGS = "-faltivec")
elif getPlatform() == 'win32':
    commonEnvironment.Append(CCFLAGS =  '-D_WIN32')
    commonEnvironment.Append(CCFLAGS =  '-DWIN32')
    commonEnvironment.Append(CCFLAGS =  '-DPIPES')
    commonEnvironment.Append(CCFLAGS =  '-DOS_IS_WIN32')
    commonEnvironment.Append(CXXFLAGS = '-D_WIN32')
    commonEnvironment.Append(CXXFLAGS = '-DWIN32')
    commonEnvironment.Append(CXXFLAGS = '-DPIPES')
    commonEnvironment.Append(CXXFLAGS = '-DOS_IS_WIN32')
    if compilerGNU():
        commonEnvironment.Prepend(CCFLAGS = "-Wall")
        commonEnvironment.Append(CPPPATH = '/usr/local/include')
        commonEnvironment.Append(CPPPATH = '/usr/include')
        commonEnvironment.Append(CCFLAGS = '-mthreads')
	commonEnvironment.Append(SHLINKFLAGS = Split('-mwindows -mno-cygwin -Wl,--enable-auto-import -Wl,--enable-runtime-pseudo-reloc'))
    else:
        commonEnvironment.Append(CCFLAGS =  '/DMSVC')
        commonEnvironment.Append(CXXFLAGS = '/EHsc')
        commonEnvironment.Append(CCFLAGS =  '/D_CRT_SECURE_NO_DEPRECATE')
        commonEnvironment.Append(CCFLAGS =  '/MD')
        commonEnvironment.Append(CCFLAGS =  '/W2')
        commonEnvironment.Append(CCFLAGS =  '/wd4251')
        commonEnvironment.Append(CCFLAGS =  '/wd4996')
        commonEnvironment.Append(CCFLAGS =  '/MP')
        commonEnvironment.Append(CCFLAGS =  '/GR')
        commonEnvironment.Append(CCFLAGS =  '/G7')
        commonEnvironment.Prepend(CCFLAGS = Split('''/Zi /D_NDEBUG /DNDEBUG'''))
        commonEnvironment.Prepend(LINKFLAGS = Split('''/INCREMENTAL:NO /OPT:REF /OPT:ICF /DEBUG'''))
        commonEnvironment.Prepend(SHLINKFLAGS = Split('''/INCREMENTAL:NO /OPT:REF /OPT:ICF /DEBUG'''))
        # With Visual C++ it is now necessary to embed the manifest into the target.
        commonEnvironment['LINKCOM'] = [commonEnvironment['LINKCOM'],
          'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;1']
        commonEnvironment['SHLINKCOM'] = [commonEnvironment['SHLINKCOM'],
          'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;2']
    if compilerMicrosoft():
        commonEnvironment.Append(CCFLAGS =  '/arch:sse')
    if compilerIntel():
        print 'Generating code optimized for Intel Core 2 Duo and Pentium 4 that will run on other processors also.'
        commonEnvironment.Append(CCFLAGS = Split('/O3 /QaxTP'))

if getPlatform() == 'linux':
    path1 = '/usr/include/python%s' % commonEnvironment['pythonVersion']
    path2 = '/usr/local/include/python%s' % commonEnvironment['pythonVersion']
    pythonIncludePath = [path1, path2]
    path1 = '/usr/include/tcl8.4'
    path2 = '/usr/include/tk8.4'
    tclIncludePath = [path1, path2]
    pythonLinkFlags = []
    if commonEnvironment['Lib64'] == '1':
        tmp = '/usr/lib64/python%s/config' % commonEnvironment['pythonVersion']
        pythonLibraryPath = ['/usr/local/lib64', '/usr/lib64', tmp]
    else:
        tmp = '/usr/lib/python%s/config' % commonEnvironment['pythonVersion']
        pythonLibraryPath = ['/usr/local/lib', '/usr/lib', tmp]
    pythonLibs = ['python%s' % commonEnvironment['pythonVersion']]
elif getPlatform() == 'darwin':
    vers = (int(sys.hexversion) >> 24, (int(sys.hexversion) >> 16) & 255)
    if vers[1] == 3:
        print "Python version is 2.%s, using Apple Python Framework" % vers[1]
        pyBasePath = '/System/Library/Frameworks/Python.framework'
    else:
        print "Python version is 2.%s, using MacPython Framework" % vers[1]
        pyBasePath = '/Library/Frameworks/Python.Framework'
    pvers = "2.%s" % vers[1]
    if commonEnvironment['pythonVersion'] != pvers:
        commonEnvironment['pythonVersion'] = pvers
        print "WARNING python version used is " + pvers
    pythonIncludePath = ['%s/Headers' % pyBasePath]
    pythonLinkFlags = [ '-F' + pyBasePath, '-framework', 'python']
    path1 = '%s/Versions/Current/lib' % pyBasePath
    path2 = '%s/python%s/config' % (path1, commonEnvironment['pythonVersion'])
    pythonLibraryPath = [path1, path2]
    pythonLibs = []
    tclIncludePath = []
elif getPlatform() == 'win32':
    pythonIncludePath = []
    pythonLinkFlags = []
    pythonLibraryPath = []
    tclIncludePath = []
    pythonLibs = ['python%s' % commonEnvironment['pythonVersion'].replace('.', '')]

# Check for prerequisites.
# We check only for headers; checking for libs may fail
# even if they are present, due to secondary dependencies.

libsndfileTests = [
    'int foobar(void)\n{\nreturn (int) SF_FORMAT_SD2;\n}',
    'int foobar(SF_INSTRUMENT *p)\n{\nreturn (int) p->loop_count;\n}',
    'int foobar(void)\n{\nreturn (int) SFC_GET_SIGNAL_MAX;\n}'
]

def CheckSndFile1011(context):
    context.Message('Checking for libsndfile version 1.0.11 or later... ')
    testProgram = '\n#include <sndfile.h>\n\n' + libsndfileTests[0] + '\n\n'
    result = context.TryCompile(testProgram, '.c')
    context.Result(result)
    return result

def CheckSndFile1013(context):
    context.Message('Checking for libsndfile version 1.0.13 or later... ')
    testProgram = '\n#include <sndfile.h>\n\n' + libsndfileTests[1] + '\n\n'
    result = context.TryCompile(testProgram, '.c')
    context.Result(result)
    return result

def CheckSndFile1016(context):
    context.Message('Checking for libsndfile version 1.0.16 or later... ')
    testProgram = '\n#include <sndfile.h>\n\n' + libsndfileTests[2] + '\n\n'
    result = context.TryCompile(testProgram, '.c')
    context.Result(result)
    return result

def CheckGcc4(context):
    # We need gcc4 if we want -fvisibility=hidden
    context.Message('Checking for gcc version 4 or later... ')
    testProgram = '''int main() {
        #if __GNUC__ >= 4
        /* OK */
           return 0;
        #else /* GCC < 4.0 */
        #error __GNUC__ < 4
        #endif
        }
    '''
    result = context.TryCompile(testProgram, '.c' )
    context.Result(result)
    return result

configure = commonEnvironment.Configure(custom_tests = {
    'CheckSndFile1011' : CheckSndFile1011,
    'CheckSndFile1013' : CheckSndFile1013,
    'CheckSndFile1016' : CheckSndFile1016,
    'CheckGcc4'        : CheckGcc4
})

if not configure.CheckHeader("stdio.h", language = "C"):
    print " *** Failed to compile a simple test program. The compiler is"
    print " *** possibly not set up correctly, or is used with invalid flags."
    print " *** Check config.log to find out more about the error."
    Exit(-1)
if not configure.CheckLibWithHeader("sndfile", "sndfile.h", language = "C"):
	print "The sndfile library is required to build Csound 5."
	Exit(-1)
vstSdkFound = configure.CheckHeader("frontends/CsoundVST/vstsdk2.4/public.sdk/source/vst2.x/audioeffectx.h", language = "C++")
if not buildOLPC:
   portaudioFound = configure.CheckLibWithHeader("portaudio","portaudio.h", language = "C")
   portmidiFound = configure.CheckLibWithHeader("portmidi", "portmidi.h", language = "C")
   fltkFound = configure.CheckLibWithHeader("fltk", "FL/Fl.H", language = "C++")
if fltkFound:
    fltk117Found = configure.CheckHeader("FL/Fl_Spinner.H", language = "C++")
else:
    fltk117Found = 0
boostFound = configure.CheckHeader("boost/any.hpp", language = "C++")
alsaFound = configure.CheckLibWithHeader("asound", "alsa/asoundlib.h", language = "C")
oscFound = configure.CheckLibWithHeader("lo", "lo/lo.h", language = "C")
if not buildOLPC:
    jackFound = configure.CheckLibWithHeader("jack", "jack/jack.h", language = "C")
stkFound = configure.CheckHeader("Opcodes/stk/include/Stk.h", language = "C++")
pdhfound = configure.CheckHeader("m_pd.h", language = "C")
tclhfound = configure.CheckHeader("tcl.h", language = "C")
if not tclhfound:
    for i in tclIncludePath:
        tmp = '%s/tcl.h' % i
        tclhfound = tclhfound or configure.CheckHeader(tmp, language = "C")
zlibhfound = configure.CheckHeader("zlib.h", language = "C")
midiPluginSdkFound = configure.CheckHeader("funknown.h", language = "C++")
luaFound = configure.CheckHeader("lua5.1/lua.h", language = "C")
if not buildOLPC:
   luaFound = False
swigFound = 'swig' in commonEnvironment['TOOLS']
print 'Checking for SWIG... %s' % (['no', 'yes'][int(swigFound)])
pythonFound = configure.CheckHeader("Python.h", language = "C")
if not pythonFound:
    for i in pythonIncludePath:
        tmp = '%s/Python.h' % i
        pythonFound = pythonFound or configure.CheckHeader(tmp, language = "C")
if getPlatform() == 'darwin':
    tmp = "/System/Library/Frameworks/JavaVM.Framework/Headers/jni.h"
    javaFound = configure.CheckHeader(tmp, language = "C++")
else:
    javaFound = configure.CheckHeader("jni.h", language = "C++")
if getPlatform() == 'linux' and not javaFound:
    if commonEnvironment['buildInterfaces'] != '0':
        if commonEnvironment['buildJavaWrapper'] != '0':
            baseDir = '/usr/lib'
            if commonEnvironment['Lib64'] == '1':
                baseDir += '64'
            for i in ['java', 'jvm/java', 'jvm/java-1.5.0']:
                javaIncludePath = '%s/%s/include' % (baseDir, i)
                tmp = '%s/linux/jni_md.h' % javaIncludePath
                if configure.CheckHeader(tmp, language = "C++"):
                    javaFound = 1
                    break
    if javaFound:
        commonEnvironment.Append(CPPPATH = [javaIncludePath])
        commonEnvironment.Append(CPPPATH = [javaIncludePath + '/linux'])

if getPlatform() == 'win32':
    commonEnvironment['ENV']['PATH'] = os.environ['PATH']
    commonEnvironment['SYSTEMROOT'] = os.environ['SYSTEMROOT']

if (commonEnvironment['useFLTK'] == '1' and fltkFound):
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_FLTK'])

if vstSdkFound:
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_VST_SDK'])


# Define macros that configure and config.h used to define.
headerMacroCheck = [
    ['io.h',        '-DHAVE_IO_H'       ],
    ['fcntl.h',     '-DHAVE_FCNTL_H'    ],
    ['unistd.h',    '-DHAVE_UNISTD_H'   ],
    ['stdint.h',    '-DHAVE_STDINT_H'   ],
    ['sys/time.h',  '-DHAVE_SYS_TIME_H' ],
    ['sys/types.h', '-DHAVE_SYS_TYPES_H'],
    ['termios.h',   '-DHAVE_TERMIOS_H'  ]]

for h in headerMacroCheck:
    if configure.CheckHeader(h[0], language = "C"):
        commonEnvironment.Append(CPPFLAGS = [h[1]])

if getPlatform() == 'win32':
    if configure.CheckHeader("winsock.h", language = "C"):
        commonEnvironment.Append(CPPFLAGS = '-DHAVE_SOCKETS')
elif configure.CheckHeader("sys/socket.h", language = "C"):
    commonEnvironment.Append(CPPFLAGS = '-DHAVE_SOCKETS')

if getPlatform() == 'darwin':
    commonEnvironment.Append(CPPFLAGS = '-DHAVE_DIRENT_H')
elif configure.CheckHeader("dirent.h", language = "C"):
    commonEnvironment.Append(CPPFLAGS = '-DHAVE_DIRENT_H')

if configure.CheckSndFile1016():
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_LIBSNDFILE=1016'])
elif configure.CheckSndFile1013():
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_LIBSNDFILE=1013'])
elif configure.CheckSndFile1011():
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_LIBSNDFILE=1011'])
else:
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_LIBSNDFILE=1000'])

# Package contents.

# library version is CS_VERSION.CS_APIVERSION
csoundLibraryVersion = '5.1'
csoundLibraryName = 'csound'
if commonEnvironment['useDouble'] != '0':
    csoundLibraryName += '64'
elif getPlatform() == 'win32':
    csoundLibraryName += '32'
# flags for linking with the Csound library
libCsoundLinkFlags = []
libCsoundLibs = ['sndfile']

buildOSXFramework = 0
if getPlatform() == 'darwin':
    if commonEnvironment['dynamicCsoundLibrary'] != '0':
        buildOSXFramework = 1
        CsoundLib_OSX = 'CsoundLib'
        # comment the next two lines out to disable building a separate
        # framework (CsoundLib64) for double precision
        if commonEnvironment['useDouble'] != '0':
            CsoundLib_OSX += '64'
        OSXFrameworkBaseDir = '%s.Framework' % CsoundLib_OSX
        tmp = OSXFrameworkBaseDir + '/Versions/%s'
        OSXFrameworkCurrentVersion = tmp % csoundLibraryVersion

csoundLibraryEnvironment = commonEnvironment.Copy()

if commonEnvironment['buildNewParser'] != '0':
    if getPlatform() == "win32":
        Tool('lex')(csoundLibraryEnvironment)
        Tool('yacc')(csoundLibraryEnvironment)
    print 'CONFIGURATION DECISION: Building with new parser enabled'
    csoundLibraryEnvironment.Append(YACCFLAGS = ['-d', '--report=itemset', '-p','csound_orc'])
    csoundLibraryEnvironment.Append(LEXFLAGS = ['-d', '-Pcsound_orc'])
    csoundLibraryEnvironment.Append(CPPFLAGS = ['-DENABLE_NEW_PARSER'])
    yaccBuild = csoundLibraryEnvironment.CFile(target = 'Engine/csound_orcparse.c',
                               source = 'Engine/csound_orc.y')
    lexBuild = csoundLibraryEnvironment.CFile(target = 'Engine/csound_orclex.c',
                               source = 'Engine/csound_orc.l')
else:
    print 'CONFIGURATION DECISION: Not building with new parser'

csoundLibraryEnvironment.Append(CPPFLAGS = ['-D__BUILDING_LIBCSOUND'])
if commonEnvironment['buildRelease'] != '0':
    csoundLibraryEnvironment.Append(CPPFLAGS = ['-D_CSOUND_RELEASE_'])
    if getPlatform() == 'linux':
        if commonEnvironment['Lib64'] == '0':
            tmp = '%s/lib/csound/plugins' % commonEnvironment['prefix']
        else:
            tmp = '%s/lib64/csound/plugins' % commonEnvironment['prefix']
        if commonEnvironment['useDouble'] != '0':
            tmp += '64'
        s = '-DCS_DEFAULT_PLUGINDIR=\\"%s\\"' % tmp
        csoundLibraryEnvironment.Append(CPPFLAGS = [s])
    elif buildOSXFramework != 0:
        tmp = '/Library/Frameworks/%s' % OSXFrameworkCurrentVersion
        tmp += '/Resources/Opcodes'
        if commonEnvironment['useDouble'] != '0':
            tmp += '64'
        s = '-DCS_DEFAULT_PLUGINDIR=\\"%s\\"' % tmp
        csoundLibraryEnvironment.Append(CPPFLAGS = [s])
csoundDynamicLibraryEnvironment = csoundLibraryEnvironment.Copy()
csoundDynamicLibraryEnvironment.Append(LIBS = ['sndfile'])
if getPlatform() == 'win32':
    # These are the Windows system call libraries.
    if compilerGNU():
        csoundWindowsLibraries = Split('''
advapi32 
comctl32 
comdlg32 
glu32 
kernel32 
msvcrt
odbc32 
odbccp32 
ole32 
oleaut32 
shell32             
user32 
uuid             
winmm 
winspool 
ws2_32 
wsock32 
advapi32 
comctl32 
comdlg32 
glu32 
kernel32 
odbc32 
odbccp32 
ole32 
oleaut32 
shell32             
user32 
uuid             
winmm 
winspool 
ws2_32 
wsock32
        ''')
    else:
        csoundWindowsLibraries = Split('''
            kernel32 gdi32 wsock32 ole32 uuid winmm user32.lib ws2_32.lib
            comctl32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib
            ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib bufferoverflowu
            kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
            advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib
            odbc32.lib odbccp32.lib bufferoverflowu
        ''')
    csoundDynamicLibraryEnvironment.Append(LIBS = csoundWindowsLibraries)
    if compilerGNU():
        csoundDynamicLibraryEnvironment.Append(SHLINKFLAGS = ['-module'])
elif getPlatform() == 'linux':
    csoundDynamicLibraryEnvironment.Append(LIBS = ['dl', 'm', 'pthread'])
csoundInterfacesEnvironment = csoundDynamicLibraryEnvironment.Copy()

if buildOSXFramework:
    csoundFrameworkEnvironment = csoundDynamicLibraryEnvironment.Copy()
    # create directory structure for the framework
    tmp = [OSXFrameworkBaseDir]
    tmp += ['%s/Versions' % OSXFrameworkBaseDir]
    tmp += [OSXFrameworkCurrentVersion]
    tmp += ['%s/Headers' % OSXFrameworkCurrentVersion]
    tmp += ['%s/Resources' % OSXFrameworkCurrentVersion]
    if commonEnvironment['useDouble'] == '0':
        tmp += ['%s/Resources/Opcodes' % OSXFrameworkCurrentVersion]
    else:
        tmp += ['%s/Resources/Opcodes64' % OSXFrameworkCurrentVersion]
    for i in tmp:
        try:
            os.mkdir(i, 0755)
        except:
            pass
    # set up symbolic links
    tmp = [['/Versions/Current', csoundLibraryVersion]]
    tmp += [['/' + CsoundLib_OSX, 'Versions/Current/' + CsoundLib_OSX]]
    tmp += [['/Headers', 'Versions/Current/Headers']]
    tmp += [['/Resources', 'Versions/Current/Resources']]
    for i in tmp:
        try:
            os.remove('%s/%s' % (OSXFrameworkBaseDir, i[0]))
        except:
            pass
        os.symlink(i[1], '%s/%s' % (OSXFrameworkBaseDir, i[0]))

def MacOSX_InstallHeader(headerName):
    if not buildOSXFramework:
        return
    baseName = headerName[(headerName.rfind('/') + 1):]
    targetName = '%s/Headers/%s' % (OSXFrameworkCurrentVersion, baseName)
    cmd = 'cp -f %s %s' % (headerName, targetName)
    csoundFrameworkEnvironment.Command(targetName, headerName, cmd)

def MacOSX_InstallPlugin(fileName):
    if buildOSXFramework:
        pluginDir = '%s/Resources/Opcodes' % OSXFrameworkCurrentVersion
        if commonEnvironment['useDouble'] != '0':
            pluginDir += '64'
        cmd = 'cp -f %s %s/' % (fileName, pluginDir)
        csoundFrameworkEnvironment.Command('%s/%s' % (pluginDir, fileName),
                                           fileName, cmd)

def makePlugin(env, pluginName, srcs):
    pluginLib = env.SharedLibrary(pluginName, srcs)
    pluginLibraries.append(pluginLib)
    MacOSX_InstallPlugin('lib' + pluginName + '.dylib')
    return pluginLib

if buildOLPC:
  libCsoundSources = Split('''
Engine/auxfd.c
Engine/cfgvar.c
Engine/entry1.c
Engine/envvar.c
Engine/express.c
Engine/extract.c
Engine/fgens.c
Engine/insert.c
Engine/linevent.c
Engine/memalloc.c
Engine/memfiles.c
Engine/musmon.c
Engine/namedins.c
Engine/otran.c
Engine/rdorch.c
Engine/rdscor.c
Engine/scsort.c
Engine/scxtract.c
Engine/sort.c
Engine/sread.c
Engine/swrite.c
Engine/twarp.c
InOut/libsnd.c
InOut/libsnd_u.c
InOut/midifile.c
InOut/midirecv.c
InOut/midisend.c
InOut/winascii.c
InOut/windin.c
InOut/window.c
OOps/aops.c
OOps/bus.c
OOps/cmath.c
OOps/diskin.c
OOps/diskin2.c
OOps/disprep.c
OOps/dumpf.c
OOps/fftlib.c
OOps/goto_ops.c
OOps/midiinterop.c
OOps/midiops.c
OOps/midiout.c
OOps/mxfft.c
OOps/oscils.c
OOps/pstream.c
OOps/pvfileio.c
OOps/pvsanal.c
OOps/random.c
OOps/remote.c
OOps/schedule.c
OOps/sndinfUG.c
OOps/str_ops.c
OOps/ugens1.c
OOps/ugens2.c
OOps/ugens3.c
OOps/ugens4.c
OOps/ugens5.c
OOps/ugens6.c
OOps/ugrw1.c
OOps/ugrw2.c
OOps/vdelay.c
Top/argdecode.c
Top/csmodule.c
Top/csound.c
Top/getstring.c
Top/main.c
Top/new_opts.c
Top/one_file.c
Top/opcode.c
Top/threads.c
Top/utility.c

''')
else:
  libCsoundSources = Split('''
Engine/auxfd.c
Engine/cfgvar.c
Engine/entry1.c
Engine/envvar.c
Engine/express.c
Engine/extract.c
Engine/fgens.c
Engine/insert.c
Engine/linevent.c
Engine/memalloc.c
Engine/memfiles.c
Engine/musmon.c
Engine/namedins.c
Engine/otran.c
Engine/rdorch.c
Engine/rdscor.c
Engine/scsort.c
Engine/scxtract.c
Engine/sort.c
Engine/sread.c
Engine/swrite.c
Engine/twarp.c
InOut/libsnd.c
InOut/libsnd_u.c
InOut/midifile.c
InOut/midirecv.c
InOut/midisend.c
InOut/winascii.c
InOut/windin.c
InOut/window.c
InOut/winEPS.c
OOps/aops.c
OOps/bus.c
OOps/cmath.c
OOps/diskin.c
OOps/diskin2.c
OOps/disprep.c
OOps/dumpf.c
OOps/fftlib.c
OOps/goto_ops.c
OOps/midiinterop.c
OOps/midiops.c
OOps/midiout.c
OOps/mxfft.c
OOps/oscils.c
OOps/pstream.c
OOps/pvfileio.c
OOps/pvsanal.c
OOps/random.c
OOps/remote.c
OOps/schedule.c
OOps/sndinfUG.c
OOps/str_ops.c
OOps/ugens1.c
OOps/ugens2.c
OOps/ugens3.c
OOps/ugens4.c
OOps/ugens5.c
OOps/ugens6.c
OOps/ugrw1.c
OOps/ugrw2.c
OOps/vdelay.c
Top/argdecode.c
Top/cscore_internal.c
Top/cscorfns.c
Top/csmodule.c
Top/csound.c
Top/getstring.c
Top/main.c
Top/new_opts.c
Top/one_file.c
Top/opcode.c
Top/threads.c
Top/utility.c
''')

newParserSources = Split('''
Engine/csound_orclex.c
Engine/csound_orcparse.c
Engine/csound_orc_semantics.c
Engine/csound_orc_expressions.c
Engine/csound_orc_optimize.c
Engine/csound_orc_compile.c
Engine/new_orc_parser.c
Engine/symbtab.c
''')

if commonEnvironment['buildNewParser'] != '0':
    libCsoundSources += newParserSources

csoundLibraryEnvironment.Append(CCFLAGS='-fPIC')
if commonEnvironment['dynamicCsoundLibrary'] == '1':
    print 'CONFIGURATION DECISION: Building dynamic Csound library'
    if getPlatform() == 'linux':
        libName = 'lib' + csoundLibraryName + '.so'
        libName2 = libName + '.' + csoundLibraryVersion
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', libName])
        os.symlink(libName2, libName)
        tmp = csoundDynamicLibraryEnvironment['SHLINKFLAGS']
        tmp += ['-Wl,-soname=%s' % libName2]
        cflags = csoundDynamicLibraryEnvironment['CCFLAGS']
        if configure.CheckGcc4():
            cflags   += ['-fvisibility=hidden']
        csoundLibrary = csoundDynamicLibraryEnvironment.SharedLibrary(
            libName2, libCsoundSources,
            SHLINKFLAGS = tmp, SHLIBPREFIX = '', SHLIBSUFFIX = '',
            CCFLAGS = cflags)
    elif getPlatform() == 'darwin':
        libName = CsoundLib_OSX
        libVersion = csoundLibraryVersion
        csoundFrameworkEnvironment.Append(SHLINKFLAGS = Split('''
            -Xlinker -compatibility_version -Xlinker %s
        ''' % libVersion))
        csoundFrameworkEnvironment.Append(SHLINKFLAGS = Split('''
            -Xlinker -current_version -Xlinker %s
        ''' % libVersion))
        csoundFrameworkEnvironment.Append(SHLINKFLAGS = Split('''
            -install_name /Library/Frameworks/%s/%s
        ''' % (OSXFrameworkCurrentVersion, libName)))
        csoundLibraryFile = csoundFrameworkEnvironment.SharedLibrary(
            libName, libCsoundSources, SHLIBPREFIX = '', SHLIBSUFFIX = '')
        csoundFrameworkEnvironment.Command(
            '%s/%s' % (OSXFrameworkCurrentVersion, libName),
            libName,
            'cp -f %s %s/' % (libName, OSXFrameworkCurrentVersion))
        for i in headers:
            MacOSX_InstallHeader(i)
        csoundLibrary = csoundFrameworkEnvironment.Command(
            'CsoundLib_install',
            libName,
            'rm -r /Library/Frameworks/%s; cp -R %s /Library/Frameworks/' % (OSXFrameworkBaseDir, OSXFrameworkBaseDir))
        libCsoundLinkFlags = ['-F.', '-framework', libName, '-lsndfile']
        libCsoundLibs = []
    elif getPlatform() == 'win32':
        csoundLibrary = csoundDynamicLibraryEnvironment.SharedLibrary(
            csoundLibraryName, libCsoundSources,
            SHLIBSUFFIX = '.dll.%s' % csoundLibraryVersion)
    else:
        csoundLibrary = csoundDynamicLibraryEnvironment.SharedLibrary(
            csoundLibraryName, libCsoundSources)
else:
    print 'CONFIGURATION DECISION: Building static Csound library'
    csoundLibraryEnvironment.Append(CCFLAGS='-fPIC')
    csoundLibrary = csoundLibraryEnvironment.Library(
        csoundLibraryName, libCsoundSources)

libCsoundLibs.append(csoundLibrary)
libs.append(csoundLibrary)

pluginEnvironment = commonEnvironment.Copy()
pluginEnvironment.Append(LIBS = Split('sndfile'))

if getPlatform() == 'darwin':
    pluginEnvironment.Append(LINKFLAGS = Split('''
        -framework CoreMidi -framework CoreFoundation -framework CoreAudio
    '''))
    # pluginEnvironment.Append(LINKFLAGS = ['-dynamiclib'])
    pluginEnvironment['SHLIBSUFFIX'] = '.dylib'
    pluginEnvironment.Prepend(CXXFLAGS = "-fno-rtti")

#############################################################################
#
#   Build csound command line front end
#############################################################################

csoundProgramEnvironment = commonEnvironment.Copy()
csoundProgramEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
csoundProgramEnvironment.Append(LIBS = libCsoundLibs)


#############################################################################
#
#   DEFINE TARGETS AND SOURCES
#
#############################################################################

if getPlatform() == 'win32':
    PYDLL = r'%s\%s' % (os.environ['SystemRoot'], pythonLibs[0])
if getPlatform() == 'win32' and pythonLibs[0] < 'python24' and compilerGNU():
    pythonImportLibrary = csoundInterfacesEnvironment.Command(
        '/usr/local/lib/lib%s.a' % (pythonLibs[0]),
        PYDLL,
        ['pexports %s > %s.def' % (PYDLL, pythonLibs[0]),
         'dlltool --input-def %s.def --dllname %s.dll --output-lib /usr/local/lib/lib%s.a' % (pythonLibs[0], PYDLL, pythonLibs[0])])

def fixCFlagsForSwig(env):
    if '-pedantic' in env['CCFLAGS']:
        env['CCFLAGS'].remove('-pedantic')
    if '-pedantic' in env['CXXFLAGS']:
        env['CXXFLAGS'].remove('-pedantic')
    if compilerGNU():
        # work around non-ANSI type punning in SWIG generated wrapper files
        env['CCFLAGS'].append('-fno-strict-aliasing')

def makePythonModule(env, targetName, srcs):
    if getPlatform() == 'darwin':
        env.Prepend(LINKFLAGS = ['-bundle'])
        pyModule_ = env.Program('_%s.so' % targetName, srcs)
    else:
        if getPlatform() == 'linux':
            pyModule_ = env.SharedLibrary('%s' % targetName, srcs, SHLIBPREFIX="_", SHLIBSUFFIX = '.so')
        else:
            pyModule_ = env.SharedLibrary('_%s' % targetName, srcs, SHLIBSUFFIX = '.pyd')
        if getPlatform() == 'win32' and pythonLibs[0] < 'python24':
            Depends(pyModule_, pythonImportLibrary)
        print "PYTHON MODULE %s..." % targetName
    pythonModules.append(pyModule_)
    pythonModules.append('%s.py' % targetName)
    return pyModule_

if not ((pythonFound or luaFound or javaFound) and swigFound and commonEnvironment['buildInterfaces'] == '1'):
    print 'CONFIGURATION DECISION: Not building Csound interfaces library.'
else:
    print 'CONFIGURATION DECISION: Building Csound interfaces library.'
    print "Python Version: " + commonEnvironment['pythonVersion']
    csoundInterfacesEnvironment.Append(CPPPATH = ['interfaces'])
    csoundInterfacesSources = []
    for i in Split('CppSound CsoundFile Soundfile csPerfThread cs_glue filebuilding'):
        csoundInterfacesSources.append(csoundInterfacesEnvironment.SharedObject('interfaces/%s.cpp' % i))
    if commonEnvironment['dynamicCsoundLibrary'] == '1' or getPlatform() == 'win32':
        csoundInterfacesEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
        csoundInterfacesEnvironment.Prepend(LIBS = libCsoundLibs)
    else:
        for i in libCsoundSources:
            csoundInterfacesSources.append(csoundInterfacesEnvironment.SharedObject(i))
    if getPlatform() == 'win32' and compilerGNU():
        csoundInterfacesEnvironment.Append(SHLINKFLAGS = '-Wl,--add-stdcall-alias')
    elif getPlatform() == 'linux':
        csoundInterfacesEnvironment.Prepend(LIBS = ['util'])
    if compilerGNU():
        csoundInterfacesEnvironment.Prepend(LIBS = ['stdc++'])
    csoundInterfacesEnvironment.Append(SWIGFLAGS = Split('''-c++ -includeall -verbose'''))
    csoundWrapperEnvironment = csoundInterfacesEnvironment.Copy()
    fixCFlagsForSwig(csoundWrapperEnvironment)
    csoundWrapperEnvironment.Append(CPPFLAGS = '-D__BUILDING_CSOUND_INTERFACES')
    for option in csoundWrapperEnvironment['CCFLAGS']:
        if string.find(option, '-D') == 0:
            csoundWrapperEnvironment.Append(SWIGFLAGS = [option])
    for option in csoundWrapperEnvironment['CPPFLAGS']:
        if string.find(option, '-D') == 0:
            csoundWrapperEnvironment.Append(SWIGFLAGS = [option])
    for option in csoundWrapperEnvironment['CPPPATH']:
        option = '-I' + option
        csoundWrapperEnvironment.Append(SWIGFLAGS = [option])
    swigflags = csoundWrapperEnvironment['SWIGFLAGS']
    if not (javaFound and commonEnvironment['buildJavaWrapper'] != '0'):
        print 'CONFIGURATION DECISION: Not building Java wrappers for Csound interfaces library.'
    else:
        print 'CONFIGURATION DECISION: Building Java wrappers for Csound interfaces library.'
        csoundJavaWrapperEnvironment = csoundInterfacesEnvironment.Copy()
        if getPlatform() == 'darwin':
            csoundWrapperEnvironment.Append(CPPPATH =
                ['/System/Library/Frameworks/JavaVM.Framework/Headers'])
        if getPlatform() == 'linux':
            # ugly hack to work around bug that requires running scons twice
            tmp = [csoundWrapperEnvironment['SWIG']]
            for i in swigflags:
                tmp += [i]
            tmp += ['-java', '-package', 'csnd']
            tmp += ['-o', 'interfaces/java_interface_wrap.cc']
            tmp += ['interfaces/java_interface.i']
            if os.spawnvp(os.P_WAIT, tmp[0], tmp) != 0:
                Exit(-1)
            csoundJavaWrapperSources = [csoundWrapperEnvironment.SharedObject(
                'interfaces/java_interface_wrap.cc')]
        else:
            csoundJavaWrapperSources = [csoundWrapperEnvironment.SharedObject(
                'interfaces/java_interface.i',
                SWIGFLAGS = [swigflags, '-java', '-package', 'csnd'])]
        csoundJavaWrapperSources += ['interfaces/pyMsgCb_stub.cpp']
        csoundJavaWrapperSources += csoundInterfacesSources
        if getPlatform() == 'darwin':
            csoundJavaWrapperEnvironment.Prepend(LINKFLAGS = ['-bundle'])
            csoundJavaWrapperEnvironment.Append(LINKFLAGS =
                ['-framework', 'JavaVM', '-Wl'])
            csoundJavaWrapper = csoundJavaWrapperEnvironment.Program(
                'lib_jcsound.jnilib', csoundJavaWrapperSources)
        else:
            csoundJavaWrapper = csoundJavaWrapperEnvironment.SharedLibrary(
                '_jcsound', csoundJavaWrapperSources)
        Depends(csoundJavaWrapper, csoundLibrary)
        libs.append(csoundJavaWrapper)
        jcsnd = csoundJavaWrapperEnvironment.Java(
            target = './interfaces', source = './interfaces',
            JAVACFLAGS = ['-source', '1.4', '-target', '1.4'])
        try:
            os.mkdir('interfaces/csnd', 0755)
        except:
            pass
        jcsndJar = csoundJavaWrapperEnvironment.Jar(
            'csnd.jar', ['interfaces/csnd'], JARCHDIR = 'interfaces')
        Depends(jcsndJar, jcsnd)
        libs.append(jcsndJar)
    csoundInterfacesSources.insert(0,
        csoundInterfacesEnvironment.SharedObject('interfaces/pyMsgCb.cpp'))
    if not luaFound:
        print 'CONFIGURATION DECISION: Not building Csound Lua interface library.'
    else:
        print 'CONFIGURATION DECISION: Building Csound Lua interface library.'
        csoundWrapperEnvironment.Append(CPPPATH=['/usr/include/lua5.1'])
        csoundLuaInterface = csoundWrapperEnvironment.SharedObject(
            'interfaces/lua_interface.i',
            SWIGFLAGS = [swigflags, '-lua', '-outdir', '.'])
        if getPlatform() != 'darwin':
           csoundInterfacesSources.insert(0, csoundLuaInterface)
           if getPlatform() == 'win32':
              csoundInterfacesEnvironment.Prepend(LIBS = ['lua51'])
           else:
              csoundInterfacesEnvironment.Prepend(LIBS = ['lua'])
    if getPlatform() == 'linux':
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', '_csnd.so'])
        # os.symlink('lib_csnd.so', '_csnd.so')
        csoundInterfacesEnvironment.Append(LINKFLAGS = ['-Wl,-rpath-link,.'])
    if getPlatform() == 'darwin':
        if commonEnvironment['dynamicCsoundLibrary'] == '1':
            ilibName = "lib_csnd.dylib"
            ilibVersion = csoundLibraryVersion
            csoundInterfacesEnvironment.Append(SHLINKFLAGS = Split('''-Xlinker -compatibility_version -Xlinker %s''' % ilibVersion))
            csoundInterfacesEnvironment.Append(SHLINKFLAGS = Split('''-Xlinker -current_version -Xlinker %s''' % ilibVersion))
            csoundInterfacesEnvironment.Append(SHLINKFLAGS = Split('''-install_name /Library/Frameworks/CsoundLib.framework/Versions/%s/%s''' % ('5.1', ilibName)))
            csoundInterfaces = csoundInterfacesEnvironment.SharedLibrary('_csnd', csoundInterfacesSources)
        else:
            csoundInterfaces = csoundInterfacesEnvironment.Library('_csnd', csoundInterfacesSources)
    elif getPlatform() == 'linux':
        name = 'libcsnd.so'
        soname = name + '.' + csoundLibraryVersion
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', name])
        os.symlink(soname, name)
        linkflags = csoundInterfacesEnvironment['SHLINKFLAGS']
        soflag = [ '-Wl,-soname=%s' % soname ]
        extraflag = ['-L.']
        csoundInterfaces = csoundInterfacesEnvironment.SharedLibrary(
            soname, csoundInterfacesSources, SHLINKFLAGS = linkflags+soflag+extraflag,
            SHLIBPREFIX = '', SHLIBSUFFIX = '')
    else:
        csoundInterfaces = csoundInterfacesEnvironment.SharedLibrary('csnd', csoundInterfacesSources)
    Depends(csoundInterfaces, csoundLibrary)
    libs.append(csoundInterfaces)
    if pythonFound:
        csoundInterfacesEnvironment.Append(LINKFLAGS = pythonLinkFlags)
        if getPlatform() != 'darwin':
            csoundInterfacesEnvironment.Prepend(LIBPATH = pythonLibraryPath)
            csoundInterfacesEnvironment.Prepend(LIBS = pythonLibs)
        csoundInterfacesEnvironment.Append(CPPPATH = pythonIncludePath)
        csndPythonEnvironment = csoundInterfacesEnvironment.Copy()
        if getPlatform() == 'darwin':
            if commonEnvironment['dynamicCsoundLibrary'] == '1':
                csndPythonEnvironment.Append(LIBS = ['_csnd'])                 
            else:
                csndPythonEnvironment.Append(LIBS = ['csound','_csnd'])
        elif getPlatform() == 'linux':
            csndPythonEnvironment.Append(LIBS = csoundInterfaces)
        else:
            csndPythonEnvironment.Append(LIBS = ['csnd'])
        csoundPythonInterface = csndPythonEnvironment.SharedObject(
            'interfaces/python_interface.i',
            SWIGFLAGS = [swigflags, '-python', '-outdir', '.'])
        csndPythonEnvironment.Clean('.', 'interfaces/python_interface_wrap.h')
        if getPlatform() == 'win32' and pythonLibs[0] < 'python24' and compilerGNU():
            Depends(csoundPythonInterface, pythonImportLibrary)
        csndModule = makePythonModule(csndPythonEnvironment, 'csnd', [csoundPythonInterface])
        pythonModules.append('csnd.py')
        Depends(csoundPythonInterface, csoundInterfaces)

if commonEnvironment['generatePdf'] == '0':
    print 'CONFIGURATION DECISION: Not generating Csound API PDF documentation.'
else:
    print 'CONFIGURATION DECISION: Generating Csound API PDF documentation.'
    refmanTex = commonEnvironment.Command('doc/latex/refman.tex', 'Doxyfile', ['doxygen $SOURCE'])
    Depends(refmanTex, csoundLibrary)
    csoundPdf = commonEnvironment.Command('refman.pdf', 'doc/latex/refman.tex', ['pdflatex --include-directory=doc/latex --interaction=nonstopmode --job-name=CsoundAPI $SOURCE'])
    Depends(csoundPdf, refmanTex)

#############################################################################
#
# Plugin opcodes.
#############################################################################

if buildOLPC:
  makePlugin(pluginEnvironment, 'stdopcod', Split('''
                            Opcodes/bbcut.c         Opcodes/biquad.c
    Opcodes/butter.c        Opcodes/clfilt.c        Opcodes/cross2.c
    Opcodes/dam.c           Opcodes/dcblockr.c      Opcodes/filter.c
    Opcodes/flanger.c       Opcodes/follow.c        Opcodes/fout.c
    Opcodes/freeverb.c      Opcodes/ftconv.c        Opcodes/ftgen.c
    Opcodes/gab/gab.c       Opcodes/gab/vectorial.c Opcodes/grain.c
    Opcodes/locsig.c        Opcodes/lowpassr.c      Opcodes/metro.c
    Opcodes/midiops2.c      Opcodes/midiops3.c      Opcodes/newfils.c
    Opcodes/nlfilt.c        Opcodes/oscbnk.c        Opcodes/pluck.c
    Opcodes/repluck.c       Opcodes/reverbsc.c      Opcodes/seqtime.c
    Opcodes/sndloop.c       Opcodes/sndwarp.c       Opcodes/space.c
                            Opcodes/syncgrain.c     Opcodes/ugens7.c
    Opcodes/ugens9.c        Opcodes/ugensa.c        Opcodes/uggab.c
    Opcodes/ugmoss.c                                Opcodes/ugsc.c
    Opcodes/wave-terrain.c  Opcodes/stdopcod.c
'''))
else:
  makePlugin(pluginEnvironment, 'stdopcod', Split('''
    Opcodes/ambicode.c      Opcodes/bbcut.c         Opcodes/biquad.c
    Opcodes/butter.c        Opcodes/clfilt.c        Opcodes/cross2.c
    Opcodes/dam.c           Opcodes/dcblockr.c      Opcodes/filter.c
    Opcodes/flanger.c       Opcodes/follow.c        Opcodes/fout.c
    Opcodes/freeverb.c      Opcodes/ftconv.c        Opcodes/ftgen.c
    Opcodes/gab/gab.c       Opcodes/gab/vectorial.c Opcodes/grain.c
    Opcodes/locsig.c        Opcodes/lowpassr.c      Opcodes/metro.c
    Opcodes/midiops2.c      Opcodes/midiops3.c      Opcodes/newfils.c
    Opcodes/nlfilt.c        Opcodes/oscbnk.c        Opcodes/pluck.c
    Opcodes/repluck.c       Opcodes/reverbsc.c      Opcodes/seqtime.c
    Opcodes/sndloop.c       Opcodes/sndwarp.c       Opcodes/space.c
    Opcodes/spat3d.c        Opcodes/syncgrain.c     Opcodes/ugens7.c
    Opcodes/ugens9.c        Opcodes/ugensa.c        Opcodes/uggab.c
    Opcodes/ugmoss.c        Opcodes/ugnorman.c      Opcodes/ugsc.c
    Opcodes/wave-terrain.c  Opcodes/stdopcod.c
    '''))

if not buildOLPC and (getPlatform() == 'linux' or getPlatform() == 'darwin'):
    makePlugin(pluginEnvironment, 'control', ['Opcodes/control.c'])
makePlugin(pluginEnvironment, 'eqfil', ['Opcodes/eqfil.c'])
makePlugin(pluginEnvironment, 'pvsbuffer', ['Opcodes/pvsbuffer.c'])
makePlugin(pluginEnvironment, 'scoreline', ['Opcodes/scoreline.c'])
if not buildOLPC:
  makePlugin(pluginEnvironment, 'ftest', ['Opcodes/ftest.c'])
makePlugin(pluginEnvironment, 'mixer', ['Opcodes/mixer.cpp'])
makePlugin(pluginEnvironment, 'modal4',
           ['Opcodes/modal4.c', 'Opcodes/physutil.c'])
makePlugin(pluginEnvironment, 'physmod', Split('''
    Opcodes/physmod.c Opcodes/physutil.c Opcodes/mandolin.c Opcodes/singwave.c
    Opcodes/fm4op.c Opcodes/moog1.c Opcodes/shaker.c Opcodes/bowedbar.c
'''))
makePlugin(pluginEnvironment, 'pitch',
           ['Opcodes/pitch.c', 'Opcodes/pitch0.c', 'Opcodes/spectra.c'])
makePlugin(pluginEnvironment, 'scansyn',
           ['Opcodes/scansyn.c', 'Opcodes/scansynx.c'])
sfontEnvironment = pluginEnvironment.Copy()
if compilerGNU():
    sfontEnvironment.Append(CCFLAGS = ['-fno-strict-aliasing'])
if sys.byteorder == 'big':
    sfontEnvironment.Append(CCFLAGS = ['-DWORDS_BIGENDIAN'])
makePlugin(sfontEnvironment, 'sfont', ['Opcodes/sfont.c'])
if not buildOLPC:
  makePlugin(pluginEnvironment, 'babo', ['Opcodes/babo.c'])
makePlugin(pluginEnvironment, 'barmodel', ['Opcodes/bilbar.c'])
makePlugin(pluginEnvironment, 'compress', ['Opcodes/compress.c'])
makePlugin(pluginEnvironment, 'grain4', ['Opcodes/grain4.c'])
if not buildOLPC:
  makePlugin(pluginEnvironment, 'hrtferX', ['Opcodes/hrtferX.c'])
makePlugin(pluginEnvironment, 'loscilx', ['Opcodes/loscilx.c'])
makePlugin(pluginEnvironment, 'minmax', ['Opcodes/minmax.c'])
makePlugin(pluginEnvironment, 'cs_pan2', ['Opcodes/pan2.c'])
makePlugin(pluginEnvironment, 'phisem', ['Opcodes/phisem.c'])
makePlugin(pluginEnvironment, 'pvoc', Split('''
    Opcodes/dsputil.c Opcodes/pvadd.c Opcodes/pvinterp.c Opcodes/pvocext.c
    Opcodes/pvread.c Opcodes/ugens8.c Opcodes/vpvoc.c Opcodes/pvoc.c
'''))
makePlugin(pluginEnvironment, 'cs_pvs_ops', Split('''
    Opcodes/ifd.c Opcodes/partials.c Opcodes/psynth.c Opcodes/pvsbasic.c
    Opcodes/pvscent.c Opcodes/pvsdemix.c Opcodes/pvs_ops.c Opcodes/pvsband.c
'''))
makePlugin(pluginEnvironment, 'stackops', ['Opcodes/stackops.c'])
if not buildOLPC:
  makePlugin(pluginEnvironment, 'vbap',
           ['Opcodes/vbap.c', 'Opcodes/vbap_eight.c', 'Opcodes/vbap_four.c',
            'Opcodes/vbap_sixteen.c', 'Opcodes/vbap_zak.c'])
makePlugin(pluginEnvironment, 'vaops', ['Opcodes/vaops.c'])
makePlugin(pluginEnvironment, 'ugakbari', ['Opcodes/ugakbari.c'])
makePlugin(pluginEnvironment, 'harmon', ['Opcodes/harmon.c'])
makePlugin(pluginEnvironment, 'ampmidid', ['Opcodes/ampmidid.cpp'])
makePlugin(pluginEnvironment, 'cs_date', ['Opcodes/date.c'])
makePlugin(pluginEnvironment, 'system_call', ['Opcodes/system_call.c'])
makePlugin(pluginEnvironment, 'ptrack', ['Opcodes/pitchtrack.c'])
makePlugin(pluginEnvironment, 'mutexops', ['Opcodes/mutexops.cpp'])
makePlugin(pluginEnvironment, 'partikkel', ['Opcodes/partikkel.c'])
makePlugin(pluginEnvironment, 'shape', ['Opcodes/shape.c'])
if buildOLPC :
   oggEnvironment = pluginEnvironment.Copy()
   makePlugin(oggEnvironment, 'ogg', ['Opcodes/ogg.c'])
   oggEnvironment.Append(LIBS=['vorbisfile'])
makePlugin(pluginEnvironment, 'vosim', ['Opcodes/Vosim.c'])

if commonEnvironment['buildImageOpcodes'] == '1' and configure.CheckLibWithHeader("png", "png.h", language="C") and zlibhfound:
	print 'CONFIGURATION DECISION: Building image opcodes'
	pluginEnvironment.Append(LIBS= Split(''' png z '''))
	makePlugin(pluginEnvironment, 'image', ['Opcodes/imageOpcodes.c'])
else:
    print 'CONFIGURATION DECISION: Not building image opcodes'
 
makePlugin(pluginEnvironment, 'gabnew', Split('''
    Opcodes/gab/tabmorph.c  Opcodes/gab/hvs.c
    Opcodes/gab/sliderTable.c
    Opcodes/gab/newgabopc.c
'''))
makePlugin(pluginEnvironment, 'hrtfnew', 'Opcodes/hrtfopcodes.c')
if (not buildOLPC) and jackFound:
 makePlugin(pluginEnvironment, 'jackTransport', 'Opcodes/jackTransport.c')

#############################################################################
#
# Plugins with External Dependencies
#############################################################################

# FLTK widgets

vstEnvironment = commonEnvironment.Copy()
vstEnvironment.Append(CXXFLAGS = '-DVST_FORCE_DEPRECATED=0')
guiProgramEnvironment = commonEnvironment.Copy()

fltkConfigFlags = 'fltk-config --use-images --cflags --cxxflags'
if getPlatform() != 'darwin':
    fltkConfigFlags += ' --ldflags'
try:
    if vstEnvironment.ParseConfig(fltkConfigFlags):
        print 'Parsed fltk-config.'
    else:
        print 'Could not parse fltk-config.'
except:
    print 'Exception when attempting to parse fltk-config.'
if getPlatform() == 'darwin':
    vstEnvironment.Append(LIBS = Split('''
        fltk fltk_images
    ''')) # fltk_png z fltk_jpeg are not on OSX at the mo
if getPlatform() == 'win32':
    if compilerGNU():
        vstEnvironment.Append(LINKFLAGS = "--subsystem:windows")
        guiProgramEnvironment.Append(LINKFLAGS = "--subsystem:windows")
        vstEnvironment.Append(LIBS = ['stdc++', 'supc++'])
        guiProgramEnvironment.Append(LIBS = ['stdc++', 'supc++'])
    else:
        csoundProgramEnvironment.Append(LINKFLAGS = ["/IMPLIB:dummy.lib"])
    csoundProgramEnvironment.Append(LIBS = csoundWindowsLibraries)
    vstEnvironment.Append(LIBS = csoundWindowsLibraries)
    guiProgramEnvironment.Append(LIBS = csoundWindowsLibraries)
else:
    if getPlatform() == 'linux':
        csoundProgramEnvironment.Append(LIBS = ['dl'])
        vstEnvironment.Append(LIBS = ['dl'])
        guiProgramEnvironment.Append(LIBS = ['dl'])
    csoundProgramEnvironment.Append(LIBS = ['pthread', 'm'])
    vstEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    guiProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    if getPlatform() == 'darwin':
        csoundProgramEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework CoreAudio -framework CoreMidi
        '''))

if buildOLPC or (not (commonEnvironment['useFLTK'] == '1' and fltkFound)):
    print 'CONFIGURATION DECISION: Not building with FLTK graphs and widgets.'
else:
    widgetsEnvironment = pluginEnvironment.Copy()
    if (commonEnvironment['buildvst4cs'] == '1'):
        widgetsEnvironment.Append(CCFLAGS = ['-DCS_VSTHOST'])
    if (commonEnvironment['noFLTKThreads'] == '1'):
        widgetsEnvironment.Append(CCFLAGS = ['-DNO_FLTK_THREADS'])
    if getPlatform() == 'linux':
        ## dont do this  widgetsEnvironment.Append(CCFLAGS = ['-DCS_VSTHOST'])
        widgetsEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags')
        widgetsEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'win32':
        if compilerGNU():
            widgetsEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            widgetsEnvironment.Prepend(
                LINKFLAGS = ['-Wl'])#,'--enable-runtime-pseudo-reloc'])
            widgetsEnvironment.Append(LIBS = Split('fltk_images fltk_png fltk_z fltk_jpeg fltk'))
        else:
            widgetsEnvironment.Append(LIBS = Split('fltkimages fltkpng fltkz fltkjpeg fltk'))
        widgetsEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'darwin':
        widgetsEnvironment.Append(LIBS = ['fltk', 'stdc++', 'pthread', 'm'])
        widgetsEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework CoreAudio -framework CoreMidi
            -framework ApplicationServices
        '''))
    makePlugin(widgetsEnvironment, 'widgets',
               ['InOut/FL_graph.cpp', 'InOut/winFLTK.c', 'InOut/widgets.cpp'])

    if commonEnvironment['buildVirtual'] == '0' or not fltk117Found:
        print "CONFIGURATION DECISION: Not building Virtual Keyboard plugin. (FLTK 1.1.7+ required)"
    else:
        print "CONFIGURATION DECISION: Building Virtual Keyboard plugin."
        widgetsEnvironment.Append(CPPPATH = ['./InOut', './InOut/virtual_keyboard'])
        makePlugin(widgetsEnvironment, 'virtual',
                   ['InOut/virtual_keyboard/FLTKKeyboard.cpp',
                    'InOut/virtual_keyboard/FLTKKeyboardWindow.cpp',
                    'InOut/virtual_keyboard/FLTKKeyboardWidget.cpp',
                    'InOut/virtual_keyboard/virtual_keyboard.cpp',
                    'InOut/virtual_keyboard/Bank.cpp',
                    'InOut/virtual_keyboard/KeyboardMapping.cpp',
                    'InOut/virtual_keyboard/Program.cpp',
                    'InOut/virtual_keyboard/SliderBank.cpp',
                    'InOut/virtual_keyboard/SliderData.cpp'])

# REAL TIME AUDIO AND MIDI

if commonEnvironment['useCoreAudio'] == '1' and getPlatform() == 'darwin':
    print "CONFIGURATION DECISION: Building CoreAudio plugin."
    coreaudioEnvironment = pluginEnvironment.Copy()
    coreaudioEnvironment.Append(CCFLAGS = ['-I/system/library/Frameworks/CoreAudio.framework/Headers'])
    makePlugin(coreaudioEnvironment, 'rtcoreaudio', ['InOut/rtcoreaudio.c'])
else:
    print "CONFIGURATION DECISION: Not building CoreAudio plugin."

if not (commonEnvironment['useALSA'] == '1' and alsaFound):
    print "CONFIGURATION DECISION: Not building ALSA plugin."
else:
    print "CONFIGURATION DECISION: Building ALSA plugin."
    alsaEnvironment = pluginEnvironment.Copy()
    alsaEnvironment.Append(LIBS = ['asound', 'pthread'])
    makePlugin(alsaEnvironment, 'rtalsa', ['InOut/rtalsa.c'])

if getPlatform() == 'win32':
    winmmEnvironment = pluginEnvironment.Copy()
    winmmEnvironment.Append(LIBS = ['winmm', 'gdi32', 'kernel32'])
    makePlugin(winmmEnvironment, 'rtwinmm', ['InOut/rtwinmm.c'])

if not (commonEnvironment['usePortAudio'] == '1' and portaudioFound):
    print "CONFIGURATION DECISION: Not building PortAudio module."
else:
    print "CONFIGURATION DECISION: Building PortAudio module."
    portaudioEnvironment = pluginEnvironment.Copy()
    if getPlatform() == 'win32':
       portaudioEnvironment.Append(LIBS = ['portaudio'])
    else:
       portaudioEnvironment.Append(LIBS = ['portaudio'])
    if (getPlatform() == 'linux'):
        if (commonEnvironment['useJack']=='1' and jackFound):
            print "Adding Jack library for PortAudio"
            portaudioEnvironment.Append(LIBS = ['jack'])
        portaudioEnvironment.Append(LIBS = ['asound', 'pthread'])
    elif getPlatform() == 'win32':
        portaudioEnvironment.Append(LIBS = ['winmm', 'dsound'])
        portaudioEnvironment.Append(LIBS = csoundWindowsLibraries)
    makePlugin(portaudioEnvironment, 'rtpa', ['InOut/rtpa.c'])

if not (commonEnvironment['useJack'] == '1' and jackFound):
    print "CONFIGURATION DECISION: Not building JACK plugin."
else:
    print "CONFIGURATION DECISION: Building JACK plugin."
    jackEnvironment = pluginEnvironment.Copy()
    if getPlatform() == 'linux':
        jackEnvironment.Append(LIBS = ['jack', 'asound', 'pthread'])
    elif getPlatform() == 'win32':
        jackEnvironment.Append(LIBS = ['jackdmp', 'pthread'])
    else:
        jackEnvironment.Append(LIBS = ['jack', 'pthread'])
    makePlugin(jackEnvironment, 'rtjack', ['InOut/rtjack.c'])

if commonEnvironment['usePortMIDI'] == '1' and portmidiFound:
    print 'CONFIGURATION DECISION: Building with PortMIDI.'
    portMidiEnvironment = pluginEnvironment.Copy()
    portMidiEnvironment.Append(LIBS = ['portmidi'])
    if getPlatform() != 'darwin':
        portMidiEnvironment.Append(LIBS = ['porttime'])
    if getPlatform() == 'win32':
        portMidiEnvironment.Append(LIBS = csoundWindowsLibraries)
    if getPlatform() == 'linux' and alsaFound:
        portMidiEnvironment.Append(LIBS = ['asound'])
    makePlugin(portMidiEnvironment, 'pmidi', ['InOut/pmidi.c'])
else:
    print 'CONFIGURATION DECISION: Not building with PortMIDI.'


# OSC opcodes

if not (commonEnvironment['useOSC'] == '1' and oscFound):
    print "CONFIGURATION DECISION: Not building OSC plugin."
else:
    print "CONFIGURATION DECISION: Building OSC plugin."
    oscEnvironment = pluginEnvironment.Copy()
    oscEnvironment.Append(LIBS = ['lo', 'pthread'])
    if getPlatform() == 'win32':
        oscEnvironment.Append(LIBS = csoundWindowsLibraries)
        if compilerGNU():
           oscEnvironment.Append(SHLINKFLAGS = ['-Wl,--enable-stdcall-fixup'])
    makePlugin(oscEnvironment, 'osc', ['Opcodes/OSC.c'])

# UDP opcodes

if commonEnvironment['useUDP'] == '0':
    print "CONFIGURATION DECISION: Not building UDP plugins."
else:
    print "CONFIGURATION DECISION: Building UDP plugins."
    udpEnvironment = pluginEnvironment.Copy()
    udpEnvironment.Append(LIBS = ['pthread'])
    makePlugin(udpEnvironment, 'udprecv', ['Opcodes/sockrecv.c'])
    makePlugin(udpEnvironment, 'udpsend', ['Opcodes/socksend.c'])

# end udp opcodes

# OGG opcodes
if buildOLPC:
    print "CONFIGURATION DECISION: Building OGG plugins."
    oggEnvironment = pluginEnvironment.Copy()
    oggEnvironment.Append(LINKFLAGS = ['-lvorbisfile', '-lvorbis', '-logg'])
    makePlugin(oggEnvironment, 'oggplay', ['Opcodes/oggplay.c'])
# end ogg opcodes

# FLUIDSYNTH OPCODES

if not buildOLPC:
  if not configure.CheckHeader("fluidsynth.h", language = "C"):
    print "CONFIGURATION DECISION: Not building fluid opcodes."
  else:
	print "CONFIGURATION DECISION: Building fluid opcodes."
	fluidEnvironment = pluginEnvironment.Copy()
	if getPlatform() == 'win32':
	   	if compilerGNU():
		   fluidEnvironment.Append(LIBS = ['fluidsynth'])
		else:
		   fluidEnvironment.Append(LIBS = ['fluidsynth'])
		fluidEnvironment.Append(CPPFLAGS = ['-DFLUIDSYNTH_NOT_A_DLL'])
		fluidEnvironment.Append(LIBS = ['winmm', 'dsound'])
		fluidEnvironment.Append(LIBS = csoundWindowsLibraries)
	elif getPlatform() == 'linux' or getPlatform() == 'darwin':
	     	fluidEnvironment.Append(LIBS = ['fluidsynth'])
		fluidEnvironment.Append(LIBS = ['pthread'])
	makePlugin(fluidEnvironment, 'fluidOpcodes',
			   ['Opcodes/fluidOpcodes/fluidOpcodes.c'])

# VST HOST OPCODES
if (commonEnvironment['buildvst4cs'] != '1'):
    print "CONFIGURATION DECISION: Not building vst4cs opcodes."
else:
    print "CONFIGURATION DECISION: Building vst4cs opcodes."
    if (getPlatform() == 'win32'or getPlatform() == 'linux') and fltkFound:
        vst4Environment = vstEnvironment.Copy()
        vst4Environment.Append(CPPFLAGS = ['-DCS_VSTHOST'])
        vst4Environment.Append(CPPPATH = ['frontends/CsoundVST'])
        if compilerGNU():
            vst4Environment.Append(LIBS = Split('fltk_images fltk_png fltk_z fltk_jpeg fltk'))
            vst4Environment.Append(LIBS = ['stdc++'])
        else:
            vst4Environment.Append(LIBS = Split('fltkimages fltkpng fltkz fltkjpeg fltk'))
        if getPlatform() == 'win32':
            vst4Environment.Append(LIBS = csoundWindowsLibraries)
        makePlugin(vst4Environment, 'vst4cs', Split('''
            Opcodes/vst4cs/src/vst4cs.cpp
            Opcodes/vst4cs/src/fxbank.cpp
            Opcodes/vst4cs/src/vsthost.cpp
        '''))
    elif getPlatform() == 'darwin' and fltkFound:
        vst4Environment = vstEnvironment.Copy()
        vst4Environment.Append(LIBS = ['fltk'])
        vst4Environment.Append(LIBS = ['stdc++'])
        vst4Environment.Append(LINKFLAGS=['-framework', 'carbon', '-framework',
                                          'ApplicationServices'])
        vst4Environment.Append(CPPPATH = ['frontends/CsoundVST'])
        vst4Environment.Append(CPPFLAGS = ['-DCS_VSTHOST'])
        makePlugin(vst4Environment, 'vst4cs', Split('''
            Opcodes/vst4cs/src/vst4cs.cpp Opcodes/vst4cs/src/fxbank.cpp
            Opcodes/vst4cs/src/vsthost.cpp
        '''))

# DSSI HOST OPCODES

if (commonEnvironment['buildDSSI'] == '1' and (getPlatform() == 'linux' or getPlatform() == 'darwin') and
configure.CheckHeader("ladspa.h", language = "C")):
    print "CONFIGURATION DECISION: Building DSSI plugin host opcodes."
    dssiEnvironment = pluginEnvironment.Copy()
    dssiEnvironment.Append(LIBS = ['dl'])
    makePlugin(dssiEnvironment, 'dssi4cs',
               ['Opcodes/dssi4cs/src/load.c', 'Opcodes/dssi4cs/src/dssi4cs.c'])
else:
    print "CONFIGURATION DECISION: Not building DSSI plugin host opcodes."

# Loris opcodes

if not (commonEnvironment['buildLoris'] == '1' and configure.CheckHeader("Opcodes/Loris/src/loris.h") and configure.CheckHeader("fftw3.h")):
    print "CONFIGURATION DECISION: Not building Loris Python extension and Csound opcodes."
else:
    print "CONFIGURATION DECISION: Building Loris Python extension and Csound opcodes."
    # For Loris, we build only the loris Python extension module and
    # the Csound opcodes (modified for Csound 5).
    # It is assumed that you have copied all contents of the Loris
    # distribution into the csound5/Opcodes/Loris directory, e.g.
    # csound5/Opcodes/Loris/src/*, etc.
    lorisEnvironment = pluginEnvironment.Copy()
    lorisEnvironment.Append(CCFLAGS = '-DHAVE_FFTW3_H')
    if commonEnvironment['buildRelease'] == '0':
        lorisEnvironment.Append(CCFLAGS = '-DDEBUG_LORISGENS')
    if getPlatform() == 'win32':
        lorisEnvironment.Append(CCFLAGS = '-D_MSC_VER')
    if compilerGNU():
        lorisEnvironment.Prepend(LIBS = ['stdc++'])
        lorisEnvironment.Append(CCFLAGS = Split('''
            -Wno-comment -Wno-unknown-pragmas -Wno-sign-compare
        '''))
    lorisEnvironment.Append(CPPPATH = Split('''
        Opcodes/Loris Opcodes/Loris/src ./
    '''))
    lorisSources = glob.glob('Opcodes/Loris/src/*.[Cc]')
    if 'Opcodes/Loris/src/lorisgens.C' in lorisSources:
        lorisSources.remove('Opcodes/Loris/src/lorisgens.C')
    lorisLibrarySources = []
    for i in lorisSources:
        lorisLibrarySources += lorisEnvironment.SharedObject(i)
    lorisLibrary = lorisEnvironment.StaticLibrary(
        'lorisbase', lorisLibrarySources)
    lorisEnvironment.Prepend(LIBS = ['lorisbase', 'fftw3'])
    # The following file has been patched for Csound 5
    # and you should update it from Csound 5 CVS.
    lorisOpcodes = makePlugin(lorisEnvironment, 'loris',
                              ['Opcodes/Loris/lorisgens5.C'])
    Depends(lorisOpcodes, lorisLibrary)
    lorisPythonEnvironment = lorisEnvironment.Copy()
    fixCFlagsForSwig(lorisPythonEnvironment)
    lorisPythonEnvironment.Append(CPPPATH = pythonIncludePath)
    lorisPythonEnvironment.Append(LINKFLAGS = pythonLinkFlags)
    lorisPythonEnvironment.Append(LIBPATH = pythonLibraryPath)
    if getPlatform() != 'darwin':
        lorisPythonEnvironment.Prepend(LIBS = pythonLibs)
    lorisPythonEnvironment.Append(SWIGPATH = ['./'])
    lorisPythonEnvironment.Prepend(SWIGFLAGS = Split('''
        -module loris -c++ -includeall -verbose -outdir . -python
        -DHAVE_FFTW3_H -I./Opcodes/Loris/src -I.
    '''))
    lorisPythonWrapper = lorisPythonEnvironment.SharedObject(
        'Opcodes/Loris/scripting/loris.i')
    lorisPythonEnvironment['SHLIBPREFIX'] = ''
    lorisPythonModule = makePythonModule(lorisPythonEnvironment,
                                         'loris', lorisPythonWrapper)
    Depends(lorisPythonModule, lorisLibrary)

# STK opcodes

if not (commonEnvironment['buildStkOpcodes'] == '1' and stkFound):
    print 'CONFIGURATION DECISION: Not building STK opcodes.'
else:
    print 'CONFIGURATION DECISION: Building STK opcodes.'
    # For the STK opcodes, the STK distribution include, src, and rawwaves
    # directories should be copied thusly:
    #   csound5/Opcodes/stk/include
    #   csound5/Opcodes/stk/src
    #   csound5/Opcodes/stk/rawwaves
    # Then, the following sources (and any other future I/O or OS dependent
    # sources) should be ignored:
    removeSources = Split('''
        Opcodes/stk/src/InetWvIn.cpp    Opcodes/stk/src/InetWvOut.cpp
        Opcodes/stk/src/Mutex.cpp       Opcodes/stk/src/RtAudio.cpp
        Opcodes/stk/src/RtMidi.cpp
        Opcodes/stk/src/RtWvIn.cpp      Opcodes/stk/src/RtWvOut.cpp
        Opcodes/stk/src/Socket.cpp      Opcodes/stk/src/TcpClient.cpp
        Opcodes/stk/src/TcpServer.cpp   Opcodes/stk/src/Thread.cpp
        Opcodes/stk/src/UdpSocket.cpp
    ''')
    stkEnvironment = pluginEnvironment.Copy()
    if getPlatform() == 'win32':
        stkEnvironment.Append(CCFLAGS = '-D__OS_WINDOWS__')
    elif getPlatform() == 'linux':
        stkEnvironment.Append(CCFLAGS = '-D__OS_LINUX__')
    elif getPlatform() == 'darwin':
        stkEnvironment.Append(CCFLAGS = '-D__OS_MACOSX__')
    if sys.byteorder == 'big':
        stkEnvironment.Append(CCFLAGS = '-D__BIG_ENDIAN__')
    else:
        stkEnvironment.Append(CCFLAGS = '-D__LITTLE_ENDIAN__')
    stkEnvironment.Prepend(CPPPATH = Split('''
        Opcodes/stk/include Opcodes/stk/src ./ ./../include
    '''))
    stkSources_ = glob.glob('Opcodes/stk/src/*.cpp')
    stkSources = []
    for source in stkSources_:
        stkSources.append(source.replace('\\', '/'))
    for removeMe in removeSources:
        stkSources.remove(removeMe)
    stkLibrarySources = []
    for i in stkSources:
        stkLibrarySources += stkEnvironment.SharedObject(i)
    stkLibrary = stkEnvironment.StaticLibrary('stk_base', stkLibrarySources)
    stkEnvironment.Prepend(LIBS = ['stk_base'])
    if compilerGNU():
        stkEnvironment.Append(LIBS = ['stdc++'])
    if getPlatform() == 'win32':
        stkEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'linux' or getPlatform() == 'darwin':
        stkEnvironment.Append(LIBS = ['pthread'])
    # This is the one that actually defines the opcodes.
    # They are straight wrappers, as simple as possible.
    stk = makePlugin(stkEnvironment, 'stk', ['Opcodes/stk/stkOpcodes.cpp'])
    Depends(stk, stkLibrary)

# Python opcodes

if not (pythonFound and commonEnvironment['buildPythonOpcodes'] != '0'):
    print "CONFIGURATION DECISION: Not building Python opcodes."
else:
    print "CONFIGURATION DECISION: Building Python opcodes."
    pyEnvironment = pluginEnvironment.Copy()
    if getPlatform() != 'darwin':
       pyEnvironment.Append(CPPPATH = pythonIncludePath)
       pyEnvironment.Append(LIBPATH = pythonLibraryPath)
    else:
       pyEnvironment.Append(CPPPATH = "/System/Library/Frameworks/Python.framework/Headers")

    pyEnvironment.Append(LINKFLAGS = pythonLinkFlags)
    pyEnvironment.Append(LIBS = pythonLibs)
    if getPlatform() == 'linux':
        pyEnvironment.Append(LIBS = ['util', 'dl', 'm'])
    elif getPlatform() == 'darwin':
        pyEnvironment.Append(LIBS = ['dl', 'm'])
    elif getPlatform() == 'win32':
        pyEnvironment['ENV']['PATH'] = os.environ['PATH']
        pyEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
    pythonOpcodes = makePlugin(pyEnvironment, 'py',
                               ['Opcodes/py/pythonopcodes.c'])
    if getPlatform() == 'win32' and pythonLibs[0] < 'python24':
        Depends(pythonOpcodes, pythonImportLibrary)

#############################################################################
#
# Utility programs.
if buildOLPC:
  stdutilSources = Split('''
                        util/cvanal.c       util/dnoise.c
    util/envext.c
                        util/hetro.c        util/lpanal.c
                                            util/mixer.c
    util/pvanal.c       util/pvlook.c       util/scale.c
    util/sndinfo.c      util/srconv.c
    util/std_util.c
  ''')
else :
  stdutilSources = Split('''
    util/atsa.c         util/cvanal.c       util/dnoise.c
    util/envext.c       util/xtrct.c        util/het_export.c
    util/het_import.c   util/hetro.c        util/lpanal.c
    util/lpc_export.c   util/lpc_import.c   util/mixer.c
    util/pvanal.c       util/pvlook.c       util/scale.c
    util/sndinfo.c      util/srconv.c       util/pv_export.c
    util/pv_import.c
    util/std_util.c
  ''')

stdutilSources += pluginEnvironment.SharedObject('util/sdif', 'SDIF/sdif.c')
makePlugin(pluginEnvironment, 'stdutil', stdutilSources)

if not buildOLPC:
  if (commonEnvironment['buildUtilities'] != '0'):
    utils = [
        ['atsa',        'util/atsa_main.c'    ],
        ['cvanal',      'util/cvl_main.c'     ],
        ['dnoise',      'util/dnoise_main.c'  ],
        ['envext',      'util/env_main.c'     ],
        ['extractor',   'util/xtrc_main.c'    ],
        ['het_export',  'util/hetx_main.c'    ],
        ['het_import',  'util/heti_main.c'    ],
        ['hetro',       'util/het_main.c'     ],
        ['lpanal',      'util/lpc_main.c'     ],
        ['lpc_export',  'util/lpcx_main.c'    ],
        ['lpc_import',  'util/lpci_main.c'    ],
        ['mixer',       'util/mixer_main.c'   ],
        ['pvanal',      'util/pvc_main.c'     ],
        ['pvlook',      'util/pvl_main.c'     ],
        ['pv_export',   'util/pvx_main.c'     ],
        ['pv_import',   'util/pvi_main.c'     ],
        ['scale',       'util/scale_main.c'   ],
        ['sndinfo',     'util/sndinfo_main.c' ],
        ['srconv',      'util/srconv_main.c'  ]]
    for i in utils:
      executables.append(csoundProgramEnvironment.Program(i[0], i[1]))

if not buildOLPC:
  executables.append(csoundProgramEnvironment.Program('scsort',
    ['util1/sortex/smain.c']))
  executables.append(csoundProgramEnvironment.Program('extract',
    ['util1/sortex/xmain.c']))
  if compilerGNU():
    executables.append(commonEnvironment.Program('cs',
      ['util1/csd_util/cs.c']))
    executables.append(commonEnvironment.Program('csb64enc',
      ['util1/csd_util/base64.c', 'util1/csd_util/csb64enc.c']))
    executables.append(commonEnvironment.Program('makecsd',
      ['util1/csd_util/base64.c', 'util1/csd_util/makecsd.c']))
    executables.append(commonEnvironment.Program('scot',
      ['util1/scot/scot_main.c', 'util1/scot/scot.c']))
#executables.append(csoundProgramEnvironment.Program('cscore',
#    ['util1/cscore/cscore_main.c']))
  executables.append(commonEnvironment.Program('sdif2ad',
    ['SDIF/sdif2adsyn.c', 'SDIF/sdif.c', 'SDIF/sdif-mem.c']))

for i in executables:
   Depends(i, csoundLibrary)

#############################################################################
#
# Front ends.
#############################################################################

def addOSXResourceFork(env, baseName, dirName):
    if getPlatform() == 'darwin':
        if dirName != '':
            fileName = dirName + '/' + baseName
        else:
            fileName = baseName
        env.Command(('%s/resources' % fileName).replace('/', '_'), fileName,
                    "/Developer/Tools/Rez -i APPL -o $SOURCE cs5.r")

csoundProgramSources = ['frontends/csound/csound_main.c']
if getPlatform() == 'linux':
    csoundProgramSources = ['frontends/csound/sched.c'] + csoundProgramSources
csoundProgram = csoundProgramEnvironment.Program('csound', csoundProgramSources)
executables.append(csoundProgram)
Depends(csoundProgram, csoundLibrary)

def fluidTarget(env, dirName, baseName, objFiles):
    flFile = dirName + '/' + baseName + '.fl'
    cppFile = dirName + '/' + baseName + '.cpp'
    hppFile = dirName + '/' + baseName + '.hpp'
    env.Command([cppFile, hppFile], flFile,
                'fluid -c -o %s -h %s %s' % (cppFile, hppFile, flFile))
    for i in objFiles:
        Depends(i, cppFile)
    return cppFile

# Build Csound5gui (FLTK frontend)

if not (commonEnvironment['buildCsound5GUI'] != '0' and fltk117Found):
    print 'CONFIGURATION DECISION: Not building FLTK CSOUND5GUI frontend.'
else:
    print 'CONFIGURATION DECISION: Building FLTK GUI CSOUND5GUI frontend.'
    csound5GUIEnvironment = csoundProgramEnvironment.Copy()
    csound5GUIEnvironment.Append(CPPPATH = ['./interfaces'])
    if jackFound:
        csound5GUIEnvironment.Append(LIBS = ['jack'])
        csound5GUIEnvironment.Prepend(CPPFLAGS = ['-DHAVE_JACK'])
    if getPlatform() == 'linux':
        csound5GUIEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags')
        csound5GUIEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'win32':
        if compilerGNU():
            csound5GUIEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            csound5GUIEnvironment.Prepend(LINKFLAGS = Split('''
                -mwindows -Wl,--enable-runtime-pseudo-reloc
            '''))
            csound5GUIEnvironment.Append(LIBS = Split('fltk_images fltk_png fltk_z fltk_jpeg fltk'))
        else:
            csound5GUIEnvironment.Append(LIBS = Split('fltkimages fltkpng fltkz fltkjpeg fltk'))
        csound5GUIEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'darwin':
        csound5GUIEnvironment.Prepend(CXXFLAGS = "-fno-rtti")
        csound5GUIEnvironment.Append(LIBS = Split('''
            fltk stdc++ pthread m
        '''))
        csound5GUIEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework ApplicationServices
        '''))
    csound5GUISources = Split('''
        frontends/fltk_gui/ConfigFile.cpp
        frontends/fltk_gui/CsoundCopyrightInfo.cpp
        frontends/fltk_gui/CsoundGlobalSettings.cpp
        frontends/fltk_gui/CsoundGUIConsole.cpp
        frontends/fltk_gui/CsoundGUIMain.cpp
        frontends/fltk_gui/CsoundPerformance.cpp
        frontends/fltk_gui/CsoundPerformanceSettings.cpp
        frontends/fltk_gui/CsoundUtility.cpp
        frontends/fltk_gui/CsoundEditor.cpp
        frontends/fltk_gui/Fl_Native_File_Chooser.cxx
        frontends/fltk_gui/main.cpp
    ''')
    csound5GUIFluidSources = Split('''
        CsoundAboutWindow_FLTK
        CsoundGlobalSettingsPanel_FLTK
        CsoundGUIConsole_FLTK
        CsoundGUIMain_FLTK
        CsoundPerformanceSettingsPanel_FLTK
        CsoundUtilitiesWindow_FLTK
    ''')
    csound5GUIObjectFiles = []
    csound5GUIFluidObjectFiles = []
    for i in csound5GUISources:
        csound5GUIObjectFiles += csound5GUIEnvironment.Object(i)
    csound5GUIObjectFiles += csound5GUIEnvironment.Object(
        'frontends/fltk_gui/csPerfThread', 'interfaces/csPerfThread.cpp')
    for i in csound5GUIFluidSources:
        csound5GUIFluidObjectFiles += csound5GUIEnvironment.Object(
            fluidTarget(csound5GUIEnvironment, 'frontends/fltk_gui', i,
                        csound5GUIObjectFiles))
    csound5GUIObjectFiles += csound5GUIFluidObjectFiles
    csound5GUI = csound5GUIEnvironment.Program('csound5gui',
                                               csound5GUIObjectFiles)
    Depends(csound5GUI, csoundLibrary)
    executables.append(csound5GUI)
    if getPlatform() == 'darwin':
        appDir = 'frontends/fltk_gui/Csound5GUI.app/Contents/MacOS'
        addOSXResourceFork(csound5GUIEnvironment, 'csound5gui', '')
        csound5GUIEnvironment.Command(
            '%s/csound5gui' % appDir, 'csound5gui', "cp $SOURCE %s/" % appDir)
        addOSXResourceFork(csound5GUIEnvironment, 'csound5gui', appDir)

# Build Cseditor

if not ((commonEnvironment['buildCSEditor'] == '1') and fltkFound):
    print 'CONFIGURATION DECISION: Not building Csound Text Editor.'
else:
    csEditorEnvironment = commonEnvironment.Copy()
    if getPlatform() == 'linux':
        csEditorEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags')
        csEditorEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
        csEditor = csEditorEnvironment.Program( 'cseditor', 'frontends/cseditor/cseditor.cxx')
        executables.append(csEditor)
    elif getPlatform() == 'win32':
        if compilerGNU():
            csEditorEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            csEditorEnvironment.Prepend(LINKFLAGS = Split('''-mwindows -Wl,--enable-runtime-pseudo-reloc'''))
            csEditorEnvironment.Append(LIBS = Split('fltk_images fltk_png fltk_z fltk_jpeg fltk'))
        else:
            csEditorEnvironment.Append(LIBS = Split('fltkimages fltkpng fltkz fltkjpeg fltk'))
        csEditorEnvironment.Append(LIBS = csoundWindowsLibraries)
        csEditor = csEditorEnvironment.Program('cseditor', ['frontends/cseditor/cseditor.cxx'])
        executables.append(csEditor)
    elif getPlatform() == 'darwin':
        csEditorEnvironment.Prepend(CXXFLAGS = "-fno-rtti")
        csEditorEnvironment.Append(LIBS = Split('''
            fltk stdc++ pthread m
        '''))
        csEditorEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework ApplicationServices
        '''))
        csEditor = csEditorEnvironment.Command('cseditor', 'frontends/cseditor/cseditor.cxx', "fltk-config --compile frontends/cseditor/cseditor.cxx")
        executables.append(csEditor)

# Build CsoundAC

if not ((commonEnvironment['buildCsoundAC'] == '1') and fltkFound and boostFound and fltkFound):
    print 'CONFIGURATION DECISION: Not building CsoundAC extension module for Csound with algorithmic composition.'
else:
    print 'CONFIGURATION DECISION: Building CsoundAC extension module for Csound with algorithmic composition.'
    acEnvironment = vstEnvironment.Copy()
    headers += glob.glob('frontends/CsoundAC/*.hpp')
    acEnvironment.Prepend(CPPPATH = ['frontends/CsoundAC', 'interfaces'])
    acEnvironment.Append(CPPPATH = pythonIncludePath)
    acEnvironment.Append(LINKFLAGS = pythonLinkFlags)
    acEnvironment.Append(LIBPATH = pythonLibraryPath)
    if getPlatform() != 'darwin':
        acEnvironment.Prepend(LIBS = pythonLibs)
    acEnvironment.Prepend(LIBS = csndModule)
    acEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    acEnvironment.Prepend(LIBS = libCsoundLibs)
    acEnvironment.Append(SWIGFLAGS = Split('-c++ -includeall -verbose -outdir .'))
    if getPlatform() == 'linux':
        acEnvironment.Append(LIBS = ['util', 'dl', 'm'])
        acEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        acEnvironment.Append(LINKFLAGS = ['-Wl,-rpath-link,.'])
        guiProgramEnvironment.Prepend(LINKFLAGS = ['-Wl,-rpath-link,.'])
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', '_CsoundAC.so'])
        os.symlink('lib_CsoundAC.so', '_CsoundAC.so')
    elif getPlatform() == 'darwin':
        acEnvironment.Append(LIBS = ['dl', 'm'])
        acEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        acEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
        acEnvironment['SHLIBSUFFIX'] = '.dylib'
    elif getPlatform() == 'win32':
        if  compilerGNU():
            acEnvironment.Prepend(LIBS = Split('fltk fltk_images fltk_png fltk_jpeg fltk_z'))
        else:
            acEnvironment.Prepend(LIBS = Split('fltk fltkimages fltkpng fltkjpeg fltkz'))
    for option in acEnvironment['CCFLAGS']:
        if string.find(option, '-D') == 0:
            acEnvironment.Append(SWIGFLAGS = [option])
    for option in acEnvironment['CPPFLAGS']:
        if string.find(option, '-D') == 0:
            acEnvironment.Append(SWIGFLAGS = [option])
    for option in acEnvironment['CPPPATH']:
        option = '-I' + option
        acEnvironment.Append(SWIGFLAGS = [option])
    print 'PATH =', commonEnvironment['ENV']['PATH']
    csoundAcSources = Split('''
    frontends/CsoundAC/Cell.cpp
    frontends/CsoundAC/Composition.cpp
    frontends/CsoundAC/Conversions.cpp
    frontends/CsoundAC/Counterpoint.cpp
    frontends/CsoundAC/CounterpointNode.cpp
    frontends/CsoundAC/Event.cpp
    frontends/CsoundAC/Hocket.cpp
    frontends/CsoundAC/ImageToScore.cpp
    frontends/CsoundAC/Lindenmayer.cpp
    frontends/CsoundAC/MCRM.cpp
    frontends/CsoundAC/Midifile.cpp
    frontends/CsoundAC/MusicModel.cpp
    frontends/CsoundAC/Node.cpp
    frontends/CsoundAC/Random.cpp
    frontends/CsoundAC/Rescale.cpp
    frontends/CsoundAC/Score.cpp
    frontends/CsoundAC/ScoreNode.cpp
    frontends/CsoundAC/Sequence.cpp
    frontends/CsoundAC/Shell.cpp
    frontends/CsoundAC/Soundfile.cpp
    frontends/CsoundAC/StrangeAttractor.cpp
    frontends/CsoundAC/System.cpp
    frontends/CsoundAC/Voicelead.cpp
    frontends/CsoundAC/VoiceleadingNode.cpp
    ''')
    swigflags = acEnvironment['SWIGFLAGS']
    acWrapperEnvironment = acEnvironment.Copy()
    fixCFlagsForSwig(acWrapperEnvironment)
    if commonEnvironment['dynamicCsoundLibrary'] == '1':
        csoundac = acEnvironment.Library('CsoundAC', csoundAcSources)
    else:
        csoundac = acEnvironment.Library('CsoundAC', csoundAcSources)
    libs.append(csoundac)
    Depends(csoundac, csoundInterfaces)
    Depends(csoundac, csoundLibrary)
    csoundAcPythonWrapper = acWrapperEnvironment.SharedObject(
        'frontends/CsoundAC/CsoundAC.i', SWIGFLAGS = [swigflags, '-python'])
    acWrapperEnvironment.Clean('.', 'frontends/CsoundAC/CsoundAC_wrap.h')
    if getPlatform() == 'darwin':
        acPythonEnvironment = acEnvironment.Copy()
        acPythonEnvironment.Prepend(LIBS = ['CsoundAC'])
        csoundAcPythonModule = makePythonModule(acPythonEnvironment, 'CsoundAC',
                                                csoundAcPythonWrapper)
    else:
        acPythonEnvironment = acEnvironment.Copy()
        acPythonEnvironment.Append(LINKFLAGS = pythonLinkFlags)
        acPythonEnvironment.Prepend(LIBPATH = pythonLibraryPath)
        acPythonEnvironment.Prepend(LIBS = pythonLibs)
        acPythonEnvironment.Append(CPPPATH = pythonIncludePath)
        acPythonEnvironment.Prepend(LIBS = ['CsoundAC'])
        csoundAcPythonModule = makePythonModule(acPythonEnvironment, 'CsoundAC',
                                                [csoundAcPythonWrapper])
        if getPlatform() == 'win32' and pythonLibs[0] < 'python24' and compilerGNU():
            Depends(csoundvstPythonModule, pythonImportLibrary)
        pythonModules.append('CsoundAC.py')
    if commonEnvironment['useDouble'] != '0' :
        counterpoint = acEnvironment.Program(
            'counterpoint', ['frontends/CsoundAC/CounterpointMain.cpp'],
            LIBS = Split('CsoundAC csnd csound64'))
    else:
        counterpoint = acEnvironment.Program(
            'counterpoint', ['frontends/CsoundAC/CounterpointMain.cpp'],
            LIBS = Split('CsoundAC csnd csound32'))

# Build CsoundVST

if not ((commonEnvironment['buildCsoundVST'] == '1') and boostFound and fltkFound):
    print 'CONFIGURATION DECISION: Not building CsoundVST plugin and standalone.'
else:
    print 'CONFIGURATION DECISION: Building CsoundVST plugin and standalone.'
    headers += glob.glob('frontends/CsoundVST/*.h')
    headers += glob.glob('frontends/CsoundVST/*.hpp')
    vstEnvironment.Prepend(CPPPATH = ['interfaces', 'frontends/CsoundVST'])
    guiProgramEnvironment.Append(CPPPATH = ['frontends/CsoundVST', 'interfaces'])
    vstEnvironment.Prepend(LIBS = ['csnd'])
    vstEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    vstEnvironment.Append(LIBS = libCsoundLibs)
    if getPlatform() == 'linux':
        vstEnvironment.Append(LIBS = ['util', 'dl', 'm'])
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(LINKFLAGS = ['-Wl,-rpath-link,.'])
        guiProgramEnvironment.Prepend(LINKFLAGS = ['-Wl,-rpath-link,.'])
    elif getPlatform() == 'darwin':
        vstEnvironment.Append(LIBS = ['dl', 'm'])
        # vstEnvironment.Append(CXXFLAGS = ['-fabi-version=0']) # if gcc3.2-3
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
        vstEnvironment['SHLIBSUFFIX'] = '.dylib'
    elif getPlatform() == 'win32':
        if compilerGNU():
            vstEnvironment['ENV']['PATH'] = os.environ['PATH']
            vstEnvironment.Append(SHLINKFLAGS = Split('-Wl,--add-stdcall-alias --no-export-all-symbols'))
            vstEnvironment.Append(CCFLAGS = ['-DNDEBUG'])
            guiProgramEnvironment.Prepend(LINKFLAGS = Split('''
                                   -mwindows -Wl,--enable-runtime-pseudo-reloc
                                   '''))
            vstEnvironment.Prepend(LINKFLAGS = ['-Wl,--enable-runtime-pseudo-reloc'])
            guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')
            vstEnvironment.Append(LIBS = Split('fltk fltk_images fltk_png fltk_jpeg fltk_z'))
        else:
            vstEnvironment.Append(LIBS = Split('csound64 csnd fltk fltkimages fltkpng fltkjpeg fltkz'))
    print 'PATH =', commonEnvironment['ENV']['PATH']
    csoundVstSources = Split('''
    frontends/CsoundVST/vstsdk2.4/public.sdk/source/vst2.x/audioeffect.cpp
    frontends/CsoundVST/vstsdk2.4/public.sdk/source/vst2.x/audioeffectx.cpp
    frontends/CsoundVST/vstsdk2.4/public.sdk/source/vst2.x/vstplugmain.cpp
    frontends/CsoundVST/CsoundVST.cpp
    frontends/CsoundVST/CsoundVstFltk.cpp
    frontends/CsoundVST/CsoundVSTMain.cpp
    frontends/CsoundVST/CsoundVstUi.cpp
    ''')
    if getPlatform() == 'win32':
        vstEnvironment.Append(LIBS = csoundWindowsLibraries)
        if compilerGNU():
           vstEnvironment.Append(SHLINKFLAGS = ['-module'])
           vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        csoundVstSources.append('frontends/CsoundVST/_CsoundVST.def')
    csoundvst = vstEnvironment.SharedLibrary('CsoundVST', csoundVstSources)
    libs.append(csoundvst)
    Depends(csoundvst, csoundInterfaces)
    Depends(csoundvst, csoundLibrary)
    guiProgramEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    if commonEnvironment['useDouble'] != '0':
      csoundvstGui = guiProgramEnvironment.Program(
        'CsoundVSTShell', ['frontends/CsoundVST/csoundvst_main.cpp'],
        LIBS = Split('csound64 csnd CsoundVST'))
    else:
      csoundvstGui = guiProgramEnvironment.Program(
        'CsoundVSTShell', ['frontends/CsoundVST/csoundvst_main.cpp'],
        LIBS = Split('csound32 csnd CsoundVST'))
    executables.append(csoundvstGui)
    Depends(csoundvstGui, csoundvst)

# Build csoundapi~ (pd class)

if commonEnvironment['buildPDClass'] == '1' and pdhfound:
    print "CONFIGURATION DECISION: Building PD csoundapi~ class"
    pdClassEnvironment = commonEnvironment.Copy()
    pdClassEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    pdClassEnvironment.Append(LIBS = libCsoundLibs)
    if getPlatform() == 'darwin':
        pdClassEnvironment.Append(LINKFLAGS = Split('''
            -bundle -flat_namespace -undefined suppress
            -framework Carbon -framework ApplicationServices
        '''))
        pdClass = pdClassEnvironment.Program(
            'csoundapi~.pd_darwin',
            'frontends/csoundapi_tilde/csoundapi_tilde.c')
    elif getPlatform() == 'linux':
        pdClass = pdClassEnvironment.SharedLibrary(
            'csoundapi~.pd_linux',
            'frontends/csoundapi_tilde/csoundapi_tilde.c',
            SHLIBPREFIX = '', SHLIBSUFFIX = '')
    elif getPlatform() == 'win32':
        pdClassEnvironment.Append(LIBS = ['pd'])
        pdClassEnvironment.Append(LIBS = csoundWindowsLibraries)
        pdClassEnvironment.Append(SHLINKFLAGS = ['-module'])
        pdClassEnvironment['ENV']['PATH'] = os.environ['PATH']
        pdClass = pdClassEnvironment.SharedLibrary(
            'csoundapi~', 'frontends/csoundapi_tilde/csoundapi_tilde.c')
    Depends(pdClass, csoundLibrary)
    libs.append(pdClass)

# Build tclcsound

if commonEnvironment['buildTclcsound'] == '1' and tclhfound:
    print "CONFIGURATION DECISION: Building Tclcsound frontend"
    csTclEnvironment = commonEnvironment.Copy()
    csTclEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    csTclEnvironment.Append(LIBS = libCsoundLibs)
    if getPlatform() == 'darwin':
        csTclEnvironment.Append(CCFLAGS = Split('''
            -I/Library/Frameworks/Tcl.Framework/Headers
            -I/Library/Frameworks/Tk.Framework/Headers
            -I/System/Library/Frameworks/Tcl.Framework/Headers
            -I/System/Library/Frameworks/Tk.Framework/Headers
        '''))
        csTclEnvironment.Append(LINKFLAGS = Split('''
            -framework tk -framework tcl
        '''))
    elif getPlatform() == 'linux':
        csTclEnvironment.Append(CPPPATH = tclIncludePath)
        csTclEnvironment.Append(LIBS = ['tcl8.4', 'tk8.4', 'dl', 'pthread'])
    elif getPlatform() == 'win32':
        csTclEnvironment.Append(LIBS = ['tcl84', 'tk84'])
        csTclEnvironment.Append(LIBS = csoundWindowsLibraries)
        csTclEnvironment.Append(SHLINKFLAGS = ['-module'])
    csTclCmdObj = csTclEnvironment.SharedObject(
        'frontends/tclcsound/commands.c')
    csTcl = csTclEnvironment.Program(
        'cstclsh', ['frontends/tclcsound/main_tclsh.c', csTclCmdObj])
    csTk = csTclEnvironment.Program(
        'cswish', ['frontends/tclcsound/main_wish.c', csTclCmdObj])
    Tclcsoundlib = csTclEnvironment.SharedLibrary(
        'tclcsound', ['frontends/tclcsound/tclcsound.c', csTclCmdObj],
        SHLIBPREFIX = '')
    if getPlatform() == 'darwin':
        csTclEnvironment.Command('cswish_resources', 'cswish',
                                 "/Developer/Tools/Rez -i APPL -o cswish frontends/tclcsound/cswish.r")
        if commonEnvironment['dynamicCsoundLibrary'] == '1':
           csTclEnvironment.Command('tclcsound_install', 'tclcsound.dylib',
                                    'mkdir /Library/Frameworks/CsoundLib.framework/Resources/TclTk; cp -R tclcsound.dylib /Library/Frameworks/CsoundLib.framework/Resources/TclTk/')
    Depends(csTcl, csoundLibrary)
    Depends(csTk, csoundLibrary)
    Depends(Tclcsoundlib, csoundLibrary)
    executables.append(csTcl)
    executables.append(csTk)
    libs.append(Tclcsoundlib)
    try:
            os.mkdir('tclcsound', 0755)
    except:
            pass
#    if getPlatform() == 'darwin':
#      csTclEnvironment.Command('tclcsound/pkgIndex.tcl', 'tclcsound.dylib','cp tclcsound.dylib tclcsound; tclsh pkgbuild.tcl')
#    elif getPlatform() == 'linux':
#      csTclEnvironment.Command('tclcsound/pkgIndex.tcl', 'tclcsound.so','cp tclcsound.so tclcsound; tclsh pkgbuild.tcl')
#    elif  getPlatform() == 'win32':
#      csTclEnvironment.Command('tclcsound/tclcsound.dll', 'tclcsound.dll','cp tclcsound.dll tclcsound')
#      csTclEnvironment.Command('tclcsound/pkgIndex.tcl', 'tclcsound/tclcsound.dll','tclsh84 pkgbuild.tcl')

else:
    print "CONFIGURATION DECISION: Not building Tclcsound"

# Build Winsound FLTK frontend

if commonEnvironment['buildWinsound'] == '1' and fltkFound:
    print "CONFIGURATION DECISION: Building Winsound frontend"
    # should these be installed ?
    # headers += glob.glob('frontends/winsound/*.h')
    csWinEnvironment = commonEnvironment.Copy()
    csWinEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    csWinEnvironment.Append(LIBS = libCsoundLibs)
    # not used
    # if (commonEnvironment['noFLTKThreads'] == '1'):
    #     csWinEnvironment.Append(CCFLAGS = ['-DNO_FLTK_THREADS'])
    if getPlatform() == 'linux':
        csWinEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags')
        csWinEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'win32':
        if compilerGNU():
            csWinEnvironment.Append(LIBS = Split('fltk_images fltk_png fltk_z fltk_jpeg fltk'))
            csWinEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            csWinEnvironment.Prepend(LINKFLAGS = Split('''
                -mwindows -Wl,--enable-runtime-pseudo-reloc
            '''))
        else:
            csWinEnvironment.Append(LIBS = Split('fltkimages fltkpng fltkz fltkjpeg fltk'))
        csWinEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'darwin':
        csWinEnvironment.Append(CXXFLAGS = ['-fno-rtti'])
        csWinEnvironment.Append(LIBS = ['fltk', 'stdc++', 'pthread', 'm'])
        csWinEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework CoreAudio -framework CoreMidi
            -framework ApplicationServices
        '''))
        appDir = 'frontends/winsound/Winsound.app/Contents/MacOS'
        addOSXResourceFork(csWinEnvironment, 'winsound', '')
        csWinEnvironment.Command(
            '%s/winsound' % appDir, 'winsound', "cp $SOURCE %s/" % appDir)
        addOSXResourceFork(csWinEnvironment, 'winsound', appDir)
    winsoundFL = 'frontends/winsound/winsound.fl'
    winsoundSrc = 'frontends/winsound/winsound.cxx'
    winsoundHdr = 'frontends/winsound/winsound.h'
    csWinEnvironment.Command(
        [winsoundSrc, winsoundHdr], winsoundFL,
        'fluid -c -o %s -h %s %s' % (winsoundSrc, winsoundHdr, winsoundFL))
    winsoundMain = csWinEnvironment.Object('frontends/winsound/main.cxx')
    Depends(winsoundMain, winsoundSrc)
    winsound5 = csWinEnvironment.Program(
        'winsound', [winsoundMain, winsoundSrc])
    Depends(winsound5, csoundLibrary)
    executables.append(winsound5)
else:
    print "CONFIGURATION DECISION: Not building Winsound"

if (getPlatform() == 'darwin' and commonEnvironment['buildOSXGUI'] == '1'):
    print "CONFIGURATION DECISION: building OSX GUI frontend"
    csOSXGUIEnvironment = commonEnvironment.Copy()
    OSXGUI = csOSXGUIEnvironment.Command(
        '''frontends/OSX/build/Csound 5.app/Contents/MacOS/Csound 5''',
        'frontends/OSX/main.c',
        "cd frontends/OSX; xcodebuild -buildstyle Deployment")
    Depends(OSXGUI, csoundLibrary)
else:
    print "CONFIGURATION DECISION: Not building OSX GUI frontend"

# build csLADSPA
print "CONFIGURATION DEFAULT:  Building csLadspa."
csLadspaEnv = commonEnvironment.Copy()
csLadspaEnv.Append(LIBS=libCsoundLibs)
csLadspaEnv.Append(CCFLAGS='-I./frontends/csladspa')
if getPlatform() == "darwin":
 if commonEnvironment['dynamicCsoundLibrary'] != '0':
  csLadspaEnv.Append(LINKFLAGS=Split('''-bundle -undefined suppress -flat_namespace -framework CsoundLib'''))
 else:
  csLadspaEnv.Append(LINKFLAGS="-bundle")
 csladspa = csLadspaEnv.Program('csladspa.so', 'frontends/csladspa/csladspa.cpp' )
else:
 csladspa = csLadspaEnv.SharedLibrary('frontends/csladspa/csladspa.cpp')
Depends(csladspa, csoundLibrary)
libs.append(csladspa)

if (commonEnvironment['generateTags']=='0') or (getPlatform() != 'darwin' and getPlatform() != 'linux'):
    print "CONFIGURATION DECISION: Not calling TAGS"
else:
    print "CONFIGURATION DECISION: Calling TAGS"
    allSources = string.join(glob.glob('*/*.h*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*.c*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*.hpp'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.c*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.h'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.hpp'))
    tags = commonEnvironment.Command('TAGS', Split(allSources), 'etags $SOURCES')
    Depends(tags, csoundLibrary)

def gettextTarget(env, baseName, target):
    gtFile = 'po/' + baseName + '.po'
    ttFile = 'po/' + target + '/LC_MESSAGES/csound5.mo'
    env.Command(ttFile, gtFile,
                'msgfmt -o %s %s' % (ttFile, gtFile))
    return ttFile

if commonEnvironment['useGettext'] == '1':
    csound5GettextEnvironment = csoundProgramEnvironment.Copy()
    gettextTarget(csound5GettextEnvironment, 'french', 'fr')
    gettextTarget(csound5GettextEnvironment, 'american', 'en_US')
    gettextTarget(csound5GettextEnvironment, 'csound', 'en_GB')
    gettextTarget(csound5GettextEnvironment, 'es_CO', 'es_CO')
    ##  The following are incomplete
    gettextTarget(csound5GettextEnvironment, 'german', 'de')
    ## italian is not in the repository (VL 28-01-08), will not build
    #gettextTarget(csound5GettextEnvironment, 'italian', 'it')
    gettextTarget(csound5GettextEnvironment, 'romanian', 'ro')

# INSTALL OPTIONS

PREFIX = commonEnvironment['prefix']

BIN_DIR = PREFIX + "/bin"
INCLUDE_DIR = PREFIX + "/include/csound"

if (commonEnvironment['Lib64'] == '1'):
    LIB_DIR = PREFIX + "/lib64"
    PYTHON_DIR = '%s/lib64' % sys.prefix
else:
    LIB_DIR = PREFIX + "/lib"
    PYTHON_DIR = '%s/lib' % sys.prefix
PYTHON_DIR += '/python%s/site-packages' % commonEnvironment['pythonVersion']

for i in sys.path:
    if i[:sys.prefix.__len__()] == sys.prefix and i[-13:] == 'site-packages':
        PYTHON_DIR = i

if commonEnvironment['useDouble'] == '0':
    PLUGIN_DIR = LIB_DIR + "/csound/plugins"
else:
    PLUGIN_DIR = LIB_DIR + "/csound/plugins64"

if commonEnvironment['install'] == '1':
    installExecutables = Alias('install-executables',
        Install(BIN_DIR, executables))
    installOpcodes = Alias('install-opcodes',
        Install(PLUGIN_DIR, pluginLibraries))
    installHeaders = Alias('install-headers',
        Install(INCLUDE_DIR, headers))
    installLibs = Alias('install-libs',
        Install(LIB_DIR, libs))
    installPythonModules = Alias('install-py',
        Install(PYTHON_DIR, pythonModules))
    Alias('install', [installExecutables, installOpcodes, installLibs, installHeaders, installPythonModules])

if getPlatform() == 'darwin' and commonEnvironment['useFLTK'] == '1':
    print "CONFIGURATION DECISION: Adding resource fork for csound"
    addOSXResourceFork(commonEnvironment, 'csound', '')


###Code to create pkconfig files
#env = Environment(tools=['default', 'scanreplace'], toolpath=['tools'])
#env['prefix'] = '/usr/local'
#env.ScanReplace('csound5.pc.in')
