/*
* C S O U N D   
*
* External language interfaces for the "C" Csound API.
*
* L I C E N S E
*
* This software is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
%module csnd
%feature("autodoc", "1");
%{
	#include "CppSound.hpp"
%}

%apply int { size_t };


%typemap(freearg) char ** {
  free((char *) $1);
}

// Enable the JNI class to load the required native library.
%pragma(java) jniclasscode=%{
  static {
    try {
   	java.lang.System.loadLibrary("_jcsound");
    } catch (UnsatisfiedLinkError e) {
      	java.lang.System.err.println("_csnd native code library failed to load.\n" + e);
      	java.lang.System.exit(1);
    }
  }
%}

%include "CppSound.hpp"