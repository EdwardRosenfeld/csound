/*
* C S O U N D
*
* An auto-extensible system for making music on computers by means of software alone.
* Copyright (c) 2001 by Michael Gogins. All rights reserved.
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
*
* 29 May 2002 - ma++ merge with CsoundLib.
* 30 May 2002 - mkg add csound "this" pointer argument back into merge.
* 27 Jun 2002 - mkg complete Linux dl code and Makefile
*/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include "csound.h"
#include "csoundCore.h"
#include "prototyp.h"

int fltk_abort = 0;
#ifdef INGALLS
static jmp_buf csoundJump_;
#endif
#define csoundMaxExits 64
static void* csoundExitFuncs_[csoundMaxExits];
static long csoundNumExits_ = -1;

  PUBLIC void *csoundCreate(void *hostdata)
  {
      ENVIRON *csound = &cenviron;
      csound->hostdata_ = hostdata;
      return csound;
  }

  PUBLIC int csoundQueryInterface(const char *name, void **interface, int *version)
  {
      if(strcmp(name, "CSOUND") == 0)
        {
          *interface = csoundCreate(0);
          *version = csoundGetVersion();
          return 0;
        }
      return 1;
  }

  PUBLIC void csoundDestroy(void *csound)
  {
      ((ENVIRON *)csound)->Cleanup(csound);
      ((ENVIRON *)csound)->Reset(csound);
#ifdef some_fine_day
      free(csound);
#endif
  }

  PUBLIC int csoundGetVersion()
  {
      return (int) (atof(PACKAGE_VERSION) * 100);
  }

  int csoundGetAPIVersion(void)
  {
      return APIVERSION * 100 + APISUBVER;
  }

  PUBLIC void *csoundGetHostData(void *csound)
  {
      return ((ENVIRON *)csound)->hostdata_;
  }

  PUBLIC void csoundSetHostData(void *csound, void *hostData)
  {
      ((ENVIRON *)csound)->hostdata_ = hostData;
  }

  /*
   * PERFORMANCE
   */

  extern int frsturnon;
  extern int sensevents(ENVIRON *);
  extern int cleanup(void);
  extern int orcompact(void);

  PUBLIC int csoundPerform(void *csound, int argc, char **argv)
  {
      volatile int returnValue;
      /* setup jmp for return after an exit() */
      if ((returnValue = setjmp(cenviron.exitjmp_)))
        {
          csoundMessage(csound, "Early return from csoundPerform().");
          return returnValue;
        }
      return csoundMain(csound, argc, argv);
  }

  PUBLIC int csoundPerformKsmps(void *csound)
  {
      int done = 0;
      volatile int returnValue;
      /* setup jmp for return after an exit()
       */
      if ((returnValue = setjmp(cenviron.exitjmp_)))
        {
          csoundMessage(csound, "Early return from csoundPerformKsmps().");
          return returnValue;
        }
      done = sensevents(csound);
      if (!done && kcnt)
        {
          /*
            Rather than overriding real-time event handling in kperf,
            turn it off before calling kperf, and back on afterwards.
          */
          int rtEvents = O.RTevents;
          O.RTevents = 0;
          kperf(csound,1);
          kcnt -= 1;
          O.RTevents = rtEvents;
        }
      if(done)
        {
          csoundMessage(csound, "Score finished in csoundPerformKsmps()\n");
        }
      return done;
  }

  PUBLIC int csoundPerformKsmpsAbsolute(void *csound)
  {
      int done = 0;
      int rtEvents = O.RTevents;
      volatile int returnValue;
      /* setup jmp for return after an exit()
       */
      if ((returnValue = setjmp(cenviron.exitjmp_)))
        {
          csoundMessage(csound, "Early return from csoundPerformKsmps().");
          return returnValue;
        }
      done = sensevents(csound);

      /*
      Rather than overriding real-time event handling in kperf,
      turn it off before calling kperf, and back on afterwards.
      */
      O.RTevents = 0;
      kperf(csound,1);
      kcnt -= 1;
      O.RTevents = rtEvents;
      return done;
  }


  /* external host's outbuffer passed in csoundPerformBuffer()
   */
  static char *_rtCurOutBuf = 0;
  static long _rtCurOutBufSize = 0;
  static long _rtCurOutBufCount = 0;
  static char *_rtOutOverBuf = 0;
  static long _rtOutOverBufSize = 0;
  static long _rtOutOverBufCount = 0;
  static char *_rtInputBuf = 0;

  PUBLIC int csoundPerformBuffer(void *csound)
  {
      volatile int returnValue;
      /* Number of samples still needed to create before returning.
       */
      static int sampsNeeded = 0;
      int sampsPerKperf = csoundGetKsmps(csound) * csoundGetNchnls(csound);
      int done = 0;
      /* Setup jmp for return after an exit().
       */
      if ((returnValue = setjmp(cenviron.exitjmp_)))
        {
          csoundMessage(csound, "Early return from csoundPerformBuffer().");
          return returnValue;
        }
      _rtCurOutBufCount = 0;
      sampsNeeded += O.outbufsamps;
      while (!done && sampsNeeded > 0)
        {
          done = sensevents(csound);
          if (done)
            {
              return done;
            }
          if (kcnt)
            {
              int rtEvents = O.RTevents;
              O.RTevents = 0;
              kperf(csound,1);
              kcnt -= 1;
              sampsNeeded -= sampsPerKperf;
              O.RTevents = rtEvents;
            }
        }
      return done;
  }

  PUBLIC void csoundCleanup(void *csound)
  {
      orcompact();
      cleanup();
      /* Call all the funcs registered with atexit(). */
      while (csoundNumExits_ >= 0)
        {
          void (*func)(void) = csoundExitFuncs_[csoundNumExits_];
          func();
          csoundNumExits_--;
        }
  }

  /*
   * ATTRIBUTES
   */

  PUBLIC MYFLT csoundGetSr(void *csound)
  {
      return ((ENVIRON *)csound)->esr_;
  }

  PUBLIC MYFLT csoundGetKr(void *csound)
  {
      return ((ENVIRON *)csound)->ekr_;
  }

  PUBLIC int csoundGetKsmps(void *csound)
  {
      return ((ENVIRON *)csound)->ksmps_;
  }

  PUBLIC int csoundGetNchnls(void *csound)
  {
      return ((ENVIRON *)csound)->nchnls_;
  }

  PUBLIC int csoundGetSampleFormat(void *csound)
  {
      return O.outformat; /* should we assume input is same as output ? */
  }

  PUBLIC int csoundGetSampleSize(void *csound)
  {
      return O.sfsampsize; /* should we assume input is same as output ? */
  }

  PUBLIC long csoundGetInputBufferSize(void *csound)
  {
      return O.inbufsamps;
  }

  PUBLIC long csoundGetOutputBufferSize(void *csound)
  {
      return O.outbufsamps;
  }

  PUBLIC void *csoundGetInputBuffer(void *csound)
  {
      return inbuf;
  }

  PUBLIC void *csoundGetOutputBuffer(void *csound)
  {
      return outbuf;
  }

  PUBLIC MYFLT* csoundGetSpin(void *csound)
  {
      return ((ENVIRON *)csound)->spin_;
  }

  PUBLIC MYFLT* csoundGetSpout(void *csound)
  {
      return ((ENVIRON *)csound)->spout_;
  }

  PUBLIC MYFLT csoundGetScoreTime(void *csound)
  {
      return ((ENVIRON *)csound)->kcounter_ * ((ENVIRON *)csound)->onedkr_;
  }

  PUBLIC MYFLT csoundGetProgress(void *csound)
  {
      return -1;
  }

  PUBLIC MYFLT csoundGetProfile(void *csound)
  {
      return -1;
  }

  PUBLIC MYFLT csoundGetCpuUsage(void *csound)
  {
      return -1;
  }

  /*
   * SCORE HANDLING
   */

  static int csoundIsScorePending_ = 0;

  PUBLIC int csoundIsScorePending(void *csound)
  {
      return csoundIsScorePending_;
  }

  PUBLIC void csoundSetScorePending(void *csound, int pending)
  {
      csoundIsScorePending_ = pending;
  }

  static MYFLT csoundScoreOffsetSeconds_ = (MYFLT) 0.0;

  PUBLIC void csoundSetScoreOffsetSeconds(void *csound, MYFLT offset)
  {
      csoundScoreOffsetSeconds_ = offset;
  }

  PUBLIC MYFLT csoundGetScoreOffsetSeconds(void *csound)
  {
      return csoundScoreOffsetSeconds_;
  }

  PUBLIC void csoundRewindScore(void *csound)
  {
      if(((ENVIRON *)csound)->scfp_)
        {
          fseek(((ENVIRON *)csound)->scfp_, 0, SEEK_SET);
        }
  }

  static void csoundDefaultMessageCallback(void *csound, const char *format, va_list args)
  {
      vfprintf(stdout, format, args);
  }

  static void (*csoundMessageCallback_)(void *csound, const char *format, va_list args) = csoundDefaultMessageCallback;

  PUBLIC void csoundSetMessageCallback(void *csound, void (*csoundMessageCallback)(void *csound, const char *format, va_list args))
  {
      csoundMessageCallback_ = csoundMessageCallback;
  }

  PUBLIC void csoundMessageV(void *csound, const char *format, va_list args)
  {
      csoundMessageCallback_(csound, format, args);
  }

  PUBLIC void csoundMessage(void *csound, const char *format, ...)
  {
      va_list args;
      va_start(args, format);
      csoundMessageCallback_(csound, format, args);
      va_end(args);
  }

  PUBLIC void csoundPrintf(const char *format, ...)
  {
      va_list args;
      va_start(args, format);
      csoundMessageCallback_(0, format, args);
      va_end(args);
  }

  static void (*csoundThrowMessageCallback_)(void *csound, const char *format, va_list args) = csoundDefaultMessageCallback;

  PUBLIC void csoundSetThrowMessageCallback(void *csound, void (*csoundThrowMessageCallback)(void *csound, const char *format, va_list args))
  {
      csoundThrowMessageCallback_ = csoundThrowMessageCallback;
  }

  PUBLIC void csoundThrowMessageV(void *csound, const char *format, va_list args)
  {
      csoundThrowMessageCallback_(csound, format, args);
  }

  PUBLIC void csoundThrowMessage(void *csound, const char *format, ...)
  {
      va_list args;
      va_start(args, format);
      csoundThrowMessageCallback_(csound, format, args);
      va_end(args);
  }

  PUBLIC int csoundGetMessageLevel(void *csound)
  {
      return ((ENVIRON *)csound)->oparms_->msglevel;
  }

  PUBLIC void csoundSetMessageLevel(void *csound, int messageLevel)
  {
      ((ENVIRON *)csound)->oparms_->msglevel = messageLevel;
  }

  PUBLIC void csoundInputMessage(void *csound, const char *message)
  {
      writeLine(message, strlen(message));
  }

  static char inChar_ = 0;

  PUBLIC void csoundKeyPress(void *csound, char c)
  {
      inChar_ = c;
  }

  char getChar()
  {
      return inChar_;
  }

  /*
   * CONTROL AND EVENTS
   */

  static void (*csoundInputValueCallback_)(void *csound, char *channelName, MYFLT *value) = 0;

  PUBLIC void csoundSetInputValueCallback(void *csound, void (*inputValueCalback)(void *csound, char *channelName, MYFLT *value))
  {
      csoundInputValueCallback_ = inputValueCalback;
  }

  void InputValue(char *channelName, MYFLT *value)
  {
      if (csoundInputValueCallback_)
        {
          csoundInputValueCallback_(&cenviron_, channelName, value);
        }
      else
        {
          *value = 0.0;
        }
  }

  static void (*csoundOutputValueCallback_)(void *csound, char *channelName, MYFLT value) = 0;

  PUBLIC void csoundSetOutputValueCallback(void *csound, void (*outputValueCalback)(void *csound, char *channelName, MYFLT value))
  {
      csoundOutputValueCallback_ = outputValueCalback;
  }

  void OutputValue(char *channelName, MYFLT value)
  {
      if (csoundOutputValueCallback_)
        {
          csoundOutputValueCallback_(&cenviron_, channelName, value);
        }
  }

  PUBLIC void csoundScoreEvent(void *csound, char type, MYFLT *pfields, long numFields)
  {
      newevent(csound, type, pfields, numFields);
  }

  /*
   *    REAL-TIME AUDIO
   */

#ifdef RTAUDIO
  extern void playopen_(int nchanls, int dsize, float sr, int scale);

  void (*playopen)(int nchanls, int dsize, float sr, int scale) = playopen_;

  extern void rtplay_(void *outBuf, int nbytes);

  void (*rtplay)(void *outBuf, int nbytes) = rtplay_;

  extern void recopen_(int nchanls, int dsize, float sr, int scale);

  void (*recopen)(int nchanls, int dsize, float sr, int scale) = recopen_;

  extern int rtrecord_(char *inBuf, int nbytes);

  int (*rtrecord)(char *inBuf, int nbytes) = rtrecord_;

  extern void rtclose_(void);

  void (*rtclose)(void) = rtclose_;
#endif

  void playopen_mi(int nchanls, int dsize, float sr, int scale) /* open for audio output */
  {
      _rtCurOutBufSize = O.outbufsamps*dsize;
      _rtCurOutBufCount = 0;
      _rtCurOutBuf = mmalloc(_rtCurOutBufSize);

      /* a special case we need to handle 'overlaps'
       */
      /* FIXME - SYY - 11.15. 2003
       * GLOBAL * should be passed in so that ksmps can be grabbed by that
       */
      if (csoundGetKsmps(&cenviron_) * nchanls > O.outbufsamps)
        {
          _rtOutOverBufSize = (csoundGetKsmps(&cenviron_) * nchanls - O.outbufsamps)*dsize;
          _rtOutOverBuf = mmalloc(_rtOutOverBufSize);
          _rtOutOverBufCount = 0;
        }
      else
        {
          _rtOutOverBufSize = 0;
          _rtOutOverBuf = 0;
          _rtOutOverBufCount = 0;
        }
  }

  void rtplay_mi(char *outBuf, int nbytes)
  {
      int bytes2copy = nbytes;
      /* copy any remaining samps from last buffer
       */
      if (_rtOutOverBufCount)
        {
          memcpy(_rtCurOutBuf, _rtOutOverBuf, _rtOutOverBufCount);
          _rtCurOutBufCount = _rtOutOverBufCount;
          _rtOutOverBufCount = 0;
        }
      /* handle any new 'overlaps'
       */
      if (bytes2copy + _rtCurOutBufCount > _rtCurOutBufSize)
        {
          _rtOutOverBufCount = _rtCurOutBufSize - (bytes2copy + _rtCurOutBufCount);
          bytes2copy = _rtCurOutBufSize - _rtCurOutBufCount;

          memcpy(_rtOutOverBuf, outBuf+bytes2copy, _rtOutOverBufCount);
        }
      /* finally copy the buffer
       */
      memcpy(_rtCurOutBuf+_rtOutOverBufCount, outBuf, bytes2copy);
      _rtCurOutBufCount += bytes2copy;
  }

  void recopen_mi(int nchanls, int dsize, float sr, int scale)
  {
      if (O.inbufsamps*O.sfsampsize != O.outbufsamps*O.insampsiz)
        die("Input buffer must be the same size as Output buffer\n");
      _rtInputBuf = mmalloc(O.inbufsamps*O.insampsiz);
  }

  int rtrecord_mi(char *inBuf, int nbytes)
  {
      memcpy(inBuf, _rtInputBuf, nbytes);
      return nbytes;
  }

  void rtclose_mi(void)
  {
      if (_rtCurOutBuf)
        mfree(_rtCurOutBuf);
      if (_rtOutOverBuf)
        mfree(_rtOutOverBuf);
      if (_rtInputBuf)
        mfree(_rtInputBuf);
  }

  PUBLIC void csoundSetPlayopenCallback(void *csound, void (*playopen__)(int nchanls, int dsize, float sr, int scale))
  {
#ifdef RTAUDIO
      playopen = playopen__;
#endif
  }

  PUBLIC void csoundSetRtplayCallback(void *csound, void (*rtplay__)(void *outBuf, int nbytes))
  {
#ifdef RTAUDIO
      rtplay = rtplay__;
#endif
  }

  PUBLIC void csoundSetRecopenCallback(void *csound, void (*recopen__)(int nchanls, int dsize, float sr, int scale))
  {
#ifdef RTAUDIO
      recopen = recopen__;
#endif
  }

  PUBLIC void csoundSetRtrecordCallback(void *csound, int (*rtrecord__)(char *inBuf, int nbytes))
  {
#ifdef RTAUDIO
      rtrecord = rtrecord__;
#endif
  }

  PUBLIC void csoundSetRtcloseCallback(void *csound, void (*rtclose__)(void))
  {
#ifdef RTAUDIO
      rtclose = rtclose__;
#endif
  }

  int csoundExternalMidiEnabled = 0;
  void (*csoundExternalMidiDeviceOpenCallback)(void *csound) = 0;
  int (*csoundExternalMidiReadCallback)(void *csound, unsigned char *midiData, int size) = 0;
  int (*csoundExternalMidiWriteCallback)(void *csound, unsigned char *midiData) = 0;
  void (*csoundExternalMidiDeviceCloseCallback)(void *csound) = 0;

  PUBLIC int csoundIsExternalMidiEnabled(void *csound)
  {
      return csoundExternalMidiEnabled;
  }

  PUBLIC void csoundSetExternalMidiEnabled(void *csound, int enabled)
  {
      csoundExternalMidiEnabled = enabled;
  }

  PUBLIC void csoundSetExternalMidiDeviceOpenCallback(void *csound, void (*csoundExternalMidiDeviceOpenCallback_)(void *csound))
  {
      csoundExternalMidiDeviceOpenCallback = csoundExternalMidiDeviceOpenCallback_;
  }

  void csoundExternalMidiDeviceOpen(void *csound)
  {
    if (csoundExternalMidiDeviceOpenCallback){
      csoundExternalMidiDeviceOpenCallback(csound);
    }
  }

  PUBLIC void csoundSetExternalMidiReadCallback(void *csound, int (*csoundExternalMidiReadCallback_)(void *csound, unsigned char *midiData, int size))
  {
      csoundExternalMidiReadCallback = csoundExternalMidiReadCallback_;
  }

#ifdef PORTMIDI
  int csoundExternalMidiRead(void *csound, void *mbuf, int size)
  {
    if (csoundExternalMidiReadCallback) {
      return csoundExternalMidiReadCallback(csound, mbuf, size);
    }
    return -1;
  }
#else
  int csoundExternalMidiRead(void *csound, unsigned char *mbuf, int size)
  {
    if (csoundExternalMidiReadCallback) {
      return csoundExternalMidiReadCallback(csound, mbuf, size);
    }
    return -1;
  }
#endif

  PUBLIC void csoundSetExternalMidiWriteCallback(void *csound, int (*csoundExternalMidiWriteCallback_)(void *csound, unsigned char *midiData))
  {
      csoundExternalMidiWriteCallback = csoundExternalMidiWriteCallback_;
  }

  int csoundExternalMidiWrite(unsigned char *midiData)
  {
    if(csoundExternalMidiWriteCallback){
      return csoundExternalMidiWriteCallback(&cenviron_, midiData);
    }
    return -1;
  }

  PUBLIC void csoundSetExternalMidiDeviceCloseCallback(void *csound, void (*csoundExternalMidiDeviceCloseCallback_)(void *csound))
  {
      csoundExternalMidiDeviceCloseCallback = csoundExternalMidiDeviceCloseCallback_;
  }

  void csoundExternalMidiDeviceClose(void *csound)
  {
    if(csoundExternalMidiDeviceCloseCallback){
      csoundExternalMidiDeviceCloseCallback(csound);
    }
  }

  /*
   *    FUNCTION TABLE DISPLAY.
   */

  static int isGraphable_ = 1;

  PUBLIC void csoundSetIsGraphable(void *csound, int isGraphable)
  {
      isGraphable_ = isGraphable;
  }

  int Graphable()
  {
      return isGraphable_;
  }

  static void defaultCsoundMakeGraph(void *csound, WINDAT *windat, char *name)
  {
#if defined(USE_FLTK)
      extern void MakeGraph_(WINDAT *,char*);
      MakeGraph_(windat, name);
#else
      extern void MakeAscii(WINDAT *,char*);
      MakeAscii(windat, name);
#endif
  }

  static void (*csoundMakeGraphCallback_)(void *csound,  WINDAT *windat, char *name) = defaultCsoundMakeGraph;

  PUBLIC void csoundSetMakeGraphCallback(void *csound, void (*makeGraphCallback)(void *csound, WINDAT *windat, char *name))
  {
      csoundMakeGraphCallback_ = makeGraphCallback;
  }

  void MakeGraph(WINDAT *windat, char *name)
  {
      csoundMakeGraphCallback_(&cenviron, windat, name);
  }

  static void defaultCsoundDrawGraph(void *csound, WINDAT *windat)
  {
#if defined(USE_FLTK)
      extern void DrawGraph_(WINDAT *);
      DrawGraph_(windat);
#else
      extern void MakeAscii(WINDAT *, char*);
      MakeAscii(windat, "");
#endif
  }

  static void (*csoundDrawGraphCallback_)(void *csound,  WINDAT *windat) = defaultCsoundDrawGraph;

  PUBLIC void csoundSetDrawGraphCallback(void *csound, void (*drawGraphCallback)(void *csound, WINDAT *windat))
  {
      csoundDrawGraphCallback_ = drawGraphCallback;
  }

  void DrawGraph(WINDAT *windat)
  {
      csoundDrawGraphCallback_(&cenviron, windat);
  }

  static void defaultCsoundKillGraph(void *csound, WINDAT *windat)
  {
      extern void KillAscii(WINDAT *wdptr);
      KillAscii(windat);
  }

  static void (*csoundKillGraphCallback_)(void *csound,  WINDAT *windat) = defaultCsoundKillGraph;

  PUBLIC void csoundSetKillGraphCallback(void *csound, void (*killGraphCallback)(void *csound, WINDAT *windat))
  {
      csoundKillGraphCallback_ = killGraphCallback;
  }

  void KillGraph(WINDAT *windat)
  {
      csoundKillGraphCallback_(&cenviron, windat);
  }

  static int defaultCsoundExitGraph(void *csound)
  {
      return CSOUND_SUCCESS;
  }

  static int (*csoundExitGraphCallback_)(void *csound) = defaultCsoundExitGraph;

  PUBLIC void csoundSetExitGraphCallback(void *csound, int (*exitGraphCallback)(void *csound))
  {
      csoundExitGraphCallback_ = exitGraphCallback;
  }

  int ExitGraph()
  {
      return csoundExitGraphCallback_(0);
  }

  void MakeXYin(XYINDAT *xyindat, MYFLT x, MYFLT y)
  {
      printf("xyin not supported. use invalue opcode instead.\n");
  }

  void ReadXYin(XYINDAT *xyindat)
  {
      printf("xyin not supported. use invlaue opcodes instead.\n");
  }

  /*
   * OPCODES
   */

  PUBLIC opcodelist *csoundNewOpcodeList()
  {
      /* create_opcodlst();
         return new_opcode_list(); */

      return NULL;
  }

  PUBLIC void csoundDisposeOpcodeList(opcodelist *list)
  {
      /* dispose_opcode_list(list); */
  }

  PUBLIC int csoundAppendOpcode(void *csound,
                                char *opname,
                                int dsblksiz,
                                int thread,
                                char *outypes,
                                char *intypes,
                                int (*iopadr)(void *),
                                int (*kopadr)(void *),
                                int (*aopadr)(void *),
                                int (*dopadr)(void *))
  {
      int oldSize = (int)((char *)((ENVIRON *)csound)->oplstend_ -
                          (char *)((ENVIRON *)csound)->opcodlst_);
      int newSize = oldSize + sizeof(OENTRY);
      int oldCount = oldSize / sizeof(OENTRY);
      int newCount = oldCount + 1;
      OENTRY *oldOpcodlst = ((ENVIRON *)csound)->opcodlst_;
      ((ENVIRON *)csound)->opcodlst_ = (OENTRY *) mrealloc(((ENVIRON *)csound)->opcodlst_, newSize);
      if(!((ENVIRON *)csound)->opcodlst_)
        {
          ((ENVIRON *)csound)->opcodlst_ = oldOpcodlst;
          err_printf("Failed to allocate new opcode entry.");
          return 0;
        }
      else
        {
          OENTRY *oentry = ((ENVIRON *)csound)->opcodlst_ + oldCount;
          ((ENVIRON *)csound)->oplstend_ = ((ENVIRON *)csound)->opcodlst_ + newCount;
          oentry->opname = opname;
          oentry->dsblksiz = dsblksiz;
          oentry->thread = thread;
          oentry->outypes = outypes;
          oentry->intypes = intypes;
          oentry->iopadr = (SUBR) iopadr;
          oentry->kopadr = (SUBR) kopadr;
          oentry->aopadr = (SUBR) aopadr;
          oentry->dopadr = (SUBR) dopadr;
          printf("Appended opcodlst[%d]: opcode = %-20s intypes = %-20s outypes = %-20s\n",
                 oldCount,
                 oentry->opname,
                 oentry->intypes,
                 oentry->outypes);
          return 1;
        }
  }

  int csoundOpcodeCompare(const void *v1, const void *v2)
  {
      return strcmp(((OENTRY*)v1)->opname, ((OENTRY*)v2)->opname);
  }

  void csoundOpcodeDeinitialize(void *csound, INSDS *ip_)
  {
      INSDS *ip = ip_;
      OPDS *pds;
      while((ip = (INSDS *)ip->nxti))
        {
          pds = (OPDS *)ip;
          while((pds = pds->nxti))
            {
              if(pds->dopadr)
                {
                  (*pds->dopadr)(csound,pds);
                }
            }
        }
      ip = ip_;
      while((ip = (INSDS *)ip->nxtp))
        {
          pds = (OPDS *)ip;
          while((pds = pds->nxtp))
            {
              if(pds->dopadr)
                {
                  (*pds->dopadr)(csound,pds);
                }
            }
        }
  }

  /*
   * MISC FUNCTIONS
   */

#if !defined(USE_FLTK)
  int POLL_EVENTS(void)
  {
      return 1;
  }
#else
  extern int POLL_EVENTS(void);
#endif

  int defaultCsoundYield(void *csound)
  {
      return POLL_EVENTS();
  }

  static int (*csoundYieldCallback_)(void *csound) = defaultCsoundYield;

  void csoundSetYieldCallback(void *csound, int (*yieldCallback)(void *csound))
  {
      csoundYieldCallback_ = yieldCallback;
  }

  int csoundYield(void *csound)
  {
      return csoundYieldCallback_(csound);
  }

  const static int MAX_ENVIRONS = 10;

  typedef struct Environs
  {
    char *environmentVariableName;
    char *path;
  } Environs;

  static Environs *csoundEnv_ = 0;

  static int csoundNumEnvs_ = 0;

  PUBLIC void csoundSetEnv(void *csound, const char *environmentVariableName, const char *path)
  {
      int i = 0;
      if (!environmentVariableName || !path)
        return;

      if (csoundEnv_ == NULL)
        {
          csoundEnv_ = (Environs *) mcalloc(MAX_ENVIRONS * sizeof(Environs));
          if (!csoundEnv_)
            {
              return;
            }
        }
      for (i = 0; i < csoundNumEnvs_; i++)
        {
          if (strcmp(csoundEnv_[i].environmentVariableName, environmentVariableName) == 0)
            {
              mrealloc(csoundEnv_[i].path, strlen(path)+1);
              strcpy(csoundEnv_[i].path, path);
              return;
            }
        }
      if (csoundNumEnvs_ >= MAX_ENVIRONS)
        {
          /* warning("Exceeded maximum number of environment paths"); */
          csoundMessage(csound, "Exceeded maximum number of environment paths");
          return;
        }

      csoundNumEnvs_++;
      csoundEnv_[csoundNumEnvs_].environmentVariableName =  mmalloc(strlen(environmentVariableName)+1);
      strcpy(csoundEnv_[csoundNumEnvs_].environmentVariableName, environmentVariableName);
      csoundEnv_[csoundNumEnvs_].path = mmalloc(strlen(path) + 1);
      strcpy(csoundEnv_[csoundNumEnvs_].path, path);
  }

  char *csoundGetEnv(const char *environmentVariableName)
  {
      int i;
      for (i = 0; i < csoundNumEnvs_; i++)
        {
          if (strcmp(csoundEnv_[i].environmentVariableName, environmentVariableName) == 0)
            {
              return (csoundEnv_[i].path);
            }
        }
      return 0;
  }

  PUBLIC void csoundReset(void *csound)
  {
      mainRESET(csound);
      csoundIsScorePending_ = 1;
      csoundScoreOffsetSeconds_ = (MYFLT) 0.0;
  }

  PUBLIC int csoundGetDebug(void *csound_)
  {
    ENVIRON *csound = (ENVIRON *)csound;
    return csound->oparms_->odebug;
  }

  PUBLIC void csoundSetDebug(void *csound_, int debug)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    csound->oparms_->odebug = debug;
  }

  PUBLIC int csoundTableLength(void *csound_, int table)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    MYFLT table_ = table;
    FUNC *ftp = (FUNC *)csound->ftfind_(csound,&table_);
    if(ftp) {
        return ftp->flen;
    } else {
        return -1;
    }
  }

  MYFLT csoundTableGet(void *csound_, int table, int index)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    MYFLT table_ = table;
    FUNC *ftp = (FUNC *)csound->ftfind_(csound,&table_);
    return ftp->ftable[index];
  }

  PUBLIC void csoundTableSet(void *csound_, int table, int index, MYFLT value)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    MYFLT table_ = table;
    FUNC *ftp = (FUNC *)csound->ftfind_(csound,&table_);
    ftp->ftable[index] = value;
  }

#ifdef INGALLS
  /*
   * Returns a non-zero to the host,
   * which may call csoundCleanup().
   */
  void exit(int status)
  {
      longjmp(csoundJump_, status +1);
  }

  int atexit(void (*func)(void))
  {
      if (++csoundNumExits_ < csoundMaxExits)
        {
          csoundExitFuncs_[csoundNumExits_] = func;
          return 0;
        }
      else
        {
          return -1;
        }
  }
#endif

#ifdef __cplusplus
};
#endif

