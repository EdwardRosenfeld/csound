/*
    sysdep.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUND_SYSDEP_H
#define CSOUND_SYSDEP_H

/* check for the presence of a modern compiler (for use of certain features) */

#ifdef HAVE_GCC3
#  undef HAVE_GCC3
#endif
#ifdef HAVE_C99
#  undef HAVE_C99
#endif
#if (defined(__GNUC__) && (__GNUC__ >= 3))
#  define HAVE_C99 1
#  if defined(__BUILDING_LIBCSOUND) || defined(CSOUND_CSDL_H)
#    ifndef _ISOC99_SOURCE
#      define _ISOC99_SOURCE  1
#    endif
#    ifndef _ISOC9X_SOURCE
#      define _ISOC9X_SOURCE  1
#    endif
#  endif
#  if !(defined(__MACH__) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 2))
#    define HAVE_GCC3 1
#  endif
#elif (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#  define HAVE_C99 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#if defined(HAVE_FCNTL_H) || defined(__unix) || defined(__unix__)
#include <fcntl.h>
#endif
#if defined(HAVE_UNISTD_H) || defined(__unix) || defined(__unix__)
#include <unistd.h>
#endif

/* Experiment with doubles or floats */

#ifndef __MYFLT_DEF
#  define __MYFLT_DEF
#  ifndef USE_DOUBLE
#    define MYFLT float
#  else
#    define MYFLT double
#  endif
#endif

#if defined(__BUILDING_LIBCSOUND) || defined(CSOUND_CSDL_H)

#define FL(x) ((MYFLT) (x))

/* find out operating system if not specified on the command line */

#if defined(_WIN32) || defined(__WIN32__)
#  ifndef WIN32
#    define WIN32 1
#  endif
#elif (defined(linux) || defined(__linux)) && !defined(LINUX)
#  define LINUX 1
#endif

#if defined(WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
#  ifndef MSVC
#    define MSVC 1
#  endif
#elif defined(MSVC)
#  undef MSVC
#endif

/* inline keyword: always available in C++, C99, and GCC 3.x and above */
/* add any other compiler that supports 'inline' */

#if !(defined(__cplusplus) || defined(inline))
#  if defined(HAVE_C99) || defined(HAVE_GCC3)
#    if defined(__GNUC__) && defined(__STRICT_ANSI__)
#      define inline __inline__
#    endif
#  elif defined(MSVC)
#    define inline  __inline
#  else
#    define inline
#  endif
#endif

#if defined(macintosh)
#  define mac_classic   /* All Mac Compiles Before OSX, including Carbon */
   /* define mills_macintosh in your prefix file
      to compile the Mills "Perf" version */
#  ifndef  USE_GUSI2
#    include <stat.h>
#  endif
#  define  O_NDELAY (0)
#  define  DIRSEP ':'
#elif defined(SYMANTEC)
#  include <unix.h>     /* for open() etc protos on mac */
#  define  DIRSEP ':'
   extern  off_t lseek(int, off_t, int);
#else
#  define DIRSEP '/'
#  ifdef  LATTICE
#    ifdef HAVE_SYS_TYPES_H
#      include <sys/types.h>
#    endif
#  else
#    ifdef __WATCOMC__
#      if !defined(O_NDELAY)
#        define  O_NDELAY (0)
#      endif
#      include <io.h>
#    else
#      ifdef WIN32
#        undef  DIRSEP
#        define DIRSEP '\\'
#        if !defined(O_NDELAY)
#          define  O_NDELAY (0)
#        endif
#        include <io.h>
#      else
#        ifdef DOSGCC
#          if !defined(O_NDELAY)
#            define  O_NDELAY (0)
#          endif
#        endif
#        ifdef HAVE_SYS_TYPES_H
#          include <sys/types.h>
#        endif
#      endif
/*  RWD for WIN32 on VC++ */
#      ifndef MSVC
#        include <sys/file.h>
#      endif
#    endif
#  endif
#  include <sys/stat.h>
#endif

#endif  /* __BUILDING_LIBCSOUND || CSOUND_CSDL_H */

/* standard integer types */

#ifdef USE_GUSI2
/* When compiling with GUSI on MacOS 9 (for Python),  */
/* all of the other integer types are already defined */
typedef int64_t             int_least64_t;
typedef uint64_t            uint_least64_t;
#else
#if defined(HAVE_STDINT_H) || defined(HAVE_C99)
#  include <stdint.h>
#else
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
#    if defined(__GNUC__) || !defined(WIN32)
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
typedef long long           int_least64_t;
typedef unsigned long long  uint_least64_t;
#    else
typedef __int64             int64_t;
typedef unsigned __int64    uint64_t;
typedef __int64             int_least64_t;
typedef unsigned __int64    uint_least64_t;
#    endif
typedef long                intptr_t;
typedef unsigned long       uintptr_t;
#endif
#endif /* USE_GUSI2 */

/* function attributes */

#if defined(HAVE_GCC3) && !defined(SWIG)
/* deprecated function, variable, or type that is to be removed eventually */
#  define CS_DEPRECATED __attribute__ ((__deprecated__))
/* a function that should not be inlined */
#  define CS_NOINLINE   __attribute__ ((__noinline__))
/* a function that never returns (e.g. csoundDie()) */
#  define CS_NORETURN   __attribute__ ((__noreturn__))
/* printf-style function with first argument as format string */
#  define CS_PRINTF1    __attribute__ ((__format__ (__printf__, 1, 2)))
/* printf-style function with second argument as format string */
#  define CS_PRINTF2    __attribute__ ((__format__ (__printf__, 2, 3)))
/* printf-style function with third argument as format string */
#  define CS_PRINTF3    __attribute__ ((__format__ (__printf__, 3, 4)))
/* a function with no side effects or dependencies on volatile data */
#  define CS_PURE       __attribute__ ((__pure__))
#else
#  define CS_DEPRECATED
#  define CS_NOINLINE
#  define CS_NORETURN
#  define CS_PRINTF1
#  define CS_PRINTF2
#  define CS_PRINTF3
#  define CS_PURE
#endif

#if defined(__BUILDING_LIBCSOUND) || defined(CSOUND_CSDL_H)

/* macros for converting floats to integers */
/* MYFLT2LONG: converts with unspecified rounding */
/* MYFLT2LRND: rounds to nearest integer */

#ifdef USE_LRINT
#  ifndef USE_DOUBLE
#    define MYFLT2LONG(x) ((long) lrintf((float) (x)))
#    define MYFLT2LRND(x) ((long) lrintf((float) (x)))
#  else
#    define MYFLT2LONG(x) ((long) lrint((double) (x)))
#    define MYFLT2LRND(x) ((long) lrint((double) (x)))
#  endif
#elif defined(MSVC)
#  ifndef USE_DOUBLE
static inline long MYFLT2LRND(float fval)
{
    int result;
    _asm {
      fld   fval
      fistp result
      mov   eax, result
    }
    return result;
}
#  else
static inline long MYFLT2LRND(double fval)
{
    int result;
    _asm {
      fld   fval
      fistp result
      mov   eax, result
    }
    return result;
}
#  endif
#  define MYFLT2LONG(x) MYFLT2LRND(x)
#else
#  ifndef USE_DOUBLE
#    define MYFLT2LONG(x) ((long) (x))
#    if defined(HAVE_GCC3) && defined(__i386__) && !defined(__ICC)
#      define MYFLT2LRND(x) ((long) lrintf((float) (x)))
#    else
static inline long MYFLT2LRND(float fval)
{
    return ((long) (fval + (fval < 0.0f ? -0.5f : 0.5f)));
}
#    endif
#  else
#    define MYFLT2LONG(x) ((long) (x))
#    if defined(HAVE_GCC3) && defined(__i386__)
#      define MYFLT2LRND(x) ((long) lrint((double) (x)))
#    else
static inline long MYFLT2LRND(double fval)
{
    return ((long) (fval + (fval < 0.0 ? -0.5 : 0.5)));
}
#    endif
#  endif
#endif

/* inline functions and macros for clamping denormals to zero */

#if defined(__i386__) || defined(MSVC)
static inline float csoundUndenormalizeFloat(float x)
{
    volatile float  tmp = 1.0e-30f;
    return ((x + 1.0e-30f) - tmp);
}

static inline double csoundUndenormalizeDouble(double x)
{
    volatile double tmp = 1.0e-200;
    return ((x + 1.0e-200) - tmp);
}
#else
#  define csoundUndenormalizeFloat(x)   x
#  define csoundUndenormalizeDouble(x)  x
#endif

#ifndef USE_DOUBLE
#  define csoundUndenormalizeMYFLT      csoundUndenormalizeFloat
#else
#  define csoundUndenormalizeMYFLT      csoundUndenormalizeDouble
#endif

#endif  /* __BUILDING_LIBCSOUND || CSOUND_CSDL_H */

#endif  /* CSOUND_SYSDEP_H */

