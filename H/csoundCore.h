/*
    csoundCore.h:

    Copyright (C) 1991-2005 Barry Vercoe, John ffitch, Istvan Varga

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

#if !defined(__BUILDING_LIBCSOUND) && !defined(CSOUND_CSDL_H)
#  error "Csound plugins and host applications should not include csoundCore.h"
#endif

#ifndef CSOUNDCORE_H
#define CSOUNDCORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sysdep.h"

#include <stdarg.h>
#include <setjmp.h>
#include <sndfile.h>

#include "csound.h"
#include "version.h"

#define OK        (0)
#define NOTOK     (-1)

#define CSFILE_FD_R     1
#define CSFILE_FD_W     2
#define CSFILE_STD      3
#define CSFILE_SND_R    4
#define CSFILE_SND_W    5

#define MAXINSNO  (200)
#define PMAX      (1000)
#define VARGMAX   (1001)

#define ORTXT       h.optext->t
#define INCOUNT     ORTXT.inlist->count
#define OUTCOUNT    ORTXT.outlist->count
#define INOCOUNT    ORTXT.inoffs->count
#define OUTOCOUNT   ORTXT.outoffs->count
#define XINCODE     ORTXT.xincod
#  define XINARG1   (p->XINCODE & 1)
#  define XINARG2   (p->XINCODE & 2)
#  define XINARG3   (p->XINCODE & 4)
#  define XINARG4   (p->XINCODE & 8)
#define XOUTCODE    ORTXT.xoutcod
#define XSTRCODE    ORTXT.xincod_str
#define XOUTSTRCODE ORTXT.xoutcod_str

#define MAXLEN     0x1000000L
#define FMAXLEN    ((MYFLT)(MAXLEN))
#define PHMASK     0x0FFFFFFL
#define PFRAC(x)   ((MYFLT)((x) & ftp->lomask) * ftp->lodiv)
#define MAXPOS     0x7FFFFFFFL

#define BYTREVS(n) ((n>>8  & 0xFF) | (n<<8 & 0xFF00))
#define BYTREVL(n) ((n>>24 & 0xFF) | (n>>8 & 0xFF00L) | \
                    (n<<8 & 0xFF0000L) | (n<<24 & 0xFF000000L))

#define OCTRES     8192
#define CPSOCTL(n) ((MYFLT)(1 << ((int)(n) >> 13)) * cpsocfrc[(int)(n) & 8191])

#define LOBITS     10
#define LOFACT     1024
  /* LOSCAL is 1/LOFACT as MYFLT */
#define LOSCAL     FL(0.0009765625)

#define LOMASK     1023

#define SSTRCOD    3945467
#define SSTRSIZ    200
#define ALLCHNLS   0x7fff
#define DFLT_SR    FL(44100.0)
#define DFLT_KR    FL(4410.0)
#define DFLT_KSMPS 10
#define DFLT_NCHNLS 1
#define MAXCHNLS   256

#define MAXNAME   (256)

#define DFLT_DBFS (FL(32768.0))

#define MAXOCTS         8
#define AIFF_MAXCHAN    8
#define MAXCHAN         16      /* 16 MIDI channels; only one port for now */

#define ONEPT           1.02197486              /* A440 tuning factor */
#define LOG10D20        0.11512925              /* for db to ampfac   */
#define DV32768         FL(0.000030517578125)

#ifndef PI
#define PI      (3.14159265358979323846)
#endif
#define TWOPI   (6.28318530717958647692)
#define PI_F    ((MYFLT) PI)
#define TWOPI_F ((MYFLT) TWOPI)

#define WARNMSG 04
#define IGN(X)  (void) X
#define printf  use_csoundMessage_instead_of_printf

  typedef struct {
    int     odebug;
    int     sfread, sfwrite, sfheader, filetyp;
    int     inbufsamps, outbufsamps;
    int     informat, outformat;
    int     insampsiz, sfsampsize;
    int     displays, graphsoff, postscript, msglevel;
    int     Beatmode, cmdTempo, oMaxLag;
    int     usingcscore, Linein;
    int     RTevents, Midiin, FMidiin;
    int     ringbell, termifend, stdoutfd;
    int     rewrt_hdr, heartbeat, gen01defer;
    int     expr_opt;       /* IV - Jan 27 2005: for --expression-opt */
    long    sr_override, kr_override;
    long    instxtcount, optxtsize;
    long    poolcount, gblfixed, gblacount, gblscount;
    char    *infilename, *outfilename, *playscore;
    char    *Linename, *Midiname, *FMidiname;
    char    *Midioutname;   /* jjk 09252000 - MIDI output device, -Q option */
  } OPARMS;

  typedef struct arglst {
    int     count;
    char    *arg[1];
  } ARGLST;

  typedef struct argoffs {
    int     count;
    int     indx[1];
  } ARGOFFS;

  /* Storage for parsed orchestra code, for each opcode in an INSTRTXT. */
  typedef struct text {
    int     linenum;        /* Line num in orch file (currently buggy!)  */
    int     opnum;          /* Opcode index in opcodlst[] */
    char    *opcod;         /* Pointer to opcode name in global pool */
    ARGLST  *inlist;        /* Input args (pointer to item in name list) */
    ARGLST  *outlist;
    ARGOFFS *inoffs;        /* Input args (index into list of values) */
    ARGOFFS *outoffs;
    int     xincod;         /* Rate switch for multi-rate opcode functions */
    int     xoutcod;        /* output rate switch (IV - Sep 1 2002) */
    int     xincod_str;     /* Type switch for string arguments */
    int     xoutcod_str;
    char    intype;         /* Type of first input argument (g,k,a,w etc) */
    char    pftype;         /* Type of output argument (k,a etc) */
  } TEXT;

  /* This struct is filled out by otran() at orch parse time.
     It is used as a template for instrument events. */
  typedef struct instr {
    struct op * nxtop;              /* Linked list of instr opcodes */
    TEXT    t;                      /* Text of instrument (same in nxtop) */
    int     pmax, vmax, pextrab;    /* Arg count, size of data for all
                                       opcodes in instr */
    int     mdepends;               /* Opcode type (i/k/a) */
    int     lclkcnt, lcldcnt;       /* Storage reqs for this instr */
    int     lclwcnt, lclacnt;
    int     lclpcnt, lclscnt;
    int     lclfixed, optxtcount;
    short   muted;
    long    localen;
    long    opdstot;                /* Total size of opds structs in instr */
    long    *inslist;               /* Only used in parsing (?) */
    MYFLT   *psetdata;              /* Used for pset opcode */
    struct insds * instance;        /* Chain of allocated instances of
                                       this instrument */
    struct insds * lst_instance;    /* last allocated instance */
    struct insds * act_instance;    /* Chain of free (inactive) instances */
                                    /* (pointer to next one is INSDS.nxtact) */
    struct instr * nxtinstxt;       /* Next instrument in orch (num order) */
    int     active;                 /* To count activations for control */
    int     maxalloc;
    MYFLT   cpuload;                /* % load this instrumemnt makes */
    struct opcodinfo *opcode_info;  /* IV - Nov 10 2002 */
    char    *insname;               /* instrument name */
  } INSTRTXT;

/* A chain of TEXT structs. Note that this is identical with the first two
   members of struct INSTRTEXT, and is so typecast at various points in code. */
  typedef struct op {
    struct op * nxtop;
    TEXT    t;
  } OPTXT;

  typedef struct fdch {
    struct fdch * nxtchp;
    void    *fd;            /* handle returned by csound->FileOpen() */
  } FDCH;

  typedef struct auxch {
    struct auxch * nxtchp;
    long    size;
    void    *auxp, *endp;   /* was char* */
  } AUXCH;

  typedef struct monblk {
    short   pch;
    struct monblk *prv;
  } MONPCH;

  typedef struct {
    int     notnum[4];
  } DPEXCL;

  typedef struct {
    DPEXCL  dpexcl[8];
    int     exclset[75];    /* for keys 25-99 */
  } DPARM;

  typedef struct dklst {
    struct dklst *nxtlst;
    long    pgmno;
    MYFLT   keylst[1];      /* cnt + keynos */
  } DKLST;

  typedef struct mchnblk {
    short   pgmno;          /* most recently received program change */
    short   insno;          /* instrument number assigned to this channel */
    short   RegParNo;
    short   mono;
    MONPCH  *monobas;
    MONPCH  *monocur;
    struct insds *kinsptr[128]; /* list of active notes (NULL: not active) */
    MYFLT   polyaft[128];   /* polyphonic pressure indexed by note number */
    MYFLT   ctl_val[136];   /* ... with GS vib_rate, stored in c128-c135 */
    short   pgm2ins[128];   /* program change to instr number (<=0: ignore) */
    MYFLT   aftouch;        /* channel pressure (0-127) */
    MYFLT   pchbend;        /* pitch bend (-1 to 1) */
    MYFLT   pbensens;       /* pitch bend sensitivity in semitones */
    MYFLT   dummy_;         /* unused */
    short   ksuscnt;        /* number of held (sustaining) notes */
    short   sustaining;     /* current state of sustain pedal (0: off) */
    int     dpmsb;
    int     dplsb;
    int     datenabl;
    DKLST   *klists;        /* chain of dpgm keylists */
    DPARM   *dparms;        /* drumset params         */
  } MCHNBLK;

  /* This struct holds the info for a concrete instrument event
     instance in performance. */
  typedef struct insds {
    struct opds * nxti;     /* Chain of init-time opcodes */
    struct opds * nxtp;     /* Chain of performance-time opcodes */
    struct insds * nxtinstance; /* Next allocated instance */
    struct insds * prvinstance; /* Previous allocated instance */
    struct insds * nxtact;  /* Next in list of active instruments */
    struct insds * prvact;  /* Previous in list of active instruments */
    struct insds * nxtoff;  /* Next instrument to terminate */
    FDCH    *fdchp;         /* Chain of files used by opcodes in this instr */
    AUXCH   *auxchp;        /* Extra memory used by opcodes in this instr */
    int     xtratim;        /* Extra release time requested with
                               xtratim opcode */
    MCHNBLK *m_chnbp;       /* MIDI note info block if event started
                               from MIDI */
    struct insds * nxtolap; /* ptr to next overlapping MIDI voice */
    short   insno;          /* Instrument number */
    short   m_sust;         /* non-zero for sustaining MIDI note */
    unsigned char m_pitch;  /* MIDI pitch, for simple access */
    unsigned char m_veloc;  /* ...ditto velocity */
    char    relesing;       /* Flag to indicate we are releasing,
                               test with release opcode */
    char    actflg;         /* Set if instr instance is active (perfing) */
    double  offbet;         /* Time to turn off event, in score beats */
    double  offtim;         /* Time to turn off event, in seconds (negative on
                               indef/tie) */
    void    *pylocal;       /* Python namespace for just this instance. */
    CSOUND  *csound;        /* ptr to Csound engine and API for externals */
    void    *opcod_iobufs;  /* user defined opcode I/O buffers */
    void    *opcod_deact, *subins_deact;
    void    *nxtd;          /* opcodes to be run at note deactivation */
    MYFLT   p0;             /* Copy of required p-field values for
                               quick access */
    MYFLT   p1;
    MYFLT   p2;
    MYFLT   p3;
  } INSDS;

  typedef int (*SUBR)(CSOUND *, void *);

  /* This struct holds the info for one opcode in a concrete
     instrument instance in performance. */
  typedef struct opds {
    struct opds * nxti;     /* Next opcode in init-time chain */
    struct opds * nxtp;     /* Next opcode in perf-time chain */
    SUBR    iopadr;         /* Initialization (i-time) function pointer */
    SUBR    opadr;          /* Perf-time (k- or a-rate) function pointer */
    OPTXT   *optext;        /* Orch file template part for this opcode */
    INSDS   *insdshead;     /* Owner instrument instance data structure */
  } OPDS;

  typedef struct oentry {
    char    *opname;
    unsigned short  dsblksiz;
    unsigned short  thread;
    char    *outypes;
    char    *intypes;
    int     (*iopadr)(CSOUND *, void *p);
    int     (*kopadr)(CSOUND *, void *p);
    int     (*aopadr)(CSOUND *, void *p);
    void    *useropinfo;    /* IV - Oct 12 2002: user opcode parameters */
    int     prvnum;         /* IV - Oct 31 2002 */
  } OENTRY;

  typedef struct lblblk {
    OPDS    h;
    OPDS    *prvi;
    OPDS    *prvp;
  } LBLBLK;

  typedef struct {
    MYFLT   *begp, *curp, *endp, feedback[6];
    long    scount;
  } OCTDAT;

  typedef struct {
    long    npts, nocts, nsamps;
    MYFLT   lofrq, hifrq, looct, srate;
    OCTDAT  octdata[MAXOCTS];
    AUXCH   auxch;
  } DOWNDAT;

  typedef struct {
    long    ktimstamp, ktimprd;
    long    npts, nfreqs, dbout;
    DOWNDAT *downsrcp;
    AUXCH   auxch;
  } SPECDAT;

  /* This struct holds the data for one score event. */
  typedef struct event {
    char    *strarg;        /* Original argument list string of event */
    char    opcod;          /* Event type */
    short   pcnt;           /* Number of p-fields */
    MYFLT   p2orig;         /* Event start time */
    MYFLT   p3orig;         /* Length */
    MYFLT   p[PMAX+1];      /* All p-fields for this event */
  } EVTBLK;

  typedef struct {
    MYFLT   natcps;
    MYFLT   gainfac;
    short   loopmode1;
    short   loopmode2;
    long    begin1, end1;
    long    begin2, end2;
    MYFLT   fmaxamps[AIFF_MAXCHAN+1];
  } AIFFDAT;

  typedef struct {
    MYFLT   gen01;
    MYFLT   ifilno;
    MYFLT   iskptim;
    MYFLT   iformat;
    MYFLT   channel;
    MYFLT   sample_rate;
    char    strarg[SSTRSIZ];
  } GEN01ARGS;

  typedef struct {
    long    flen;
    long    lenmask;
    long    lobits;
    long    lomask;
    MYFLT   lodiv;
    MYFLT   cvtbas, cpscvt;
    short   loopmode1;
    short   loopmode2;
    long    begin1, end1;       /* all these in ..  */
    long    begin2, end2;
    long    soundend, flenfrms; /* .. sample frames */
    long    nchanls;
    long    fno;
    GEN01ARGS gen01args;
    MYFLT   ftable[1];
  } FUNC;

  typedef struct {
    CSOUND  *csound;
    long    flen;
    int     fno, guardreq;
    EVTBLK  e;
  } FGDATA;

  typedef struct {
    char    *name;
    int     (*fn)(FGDATA *, FUNC *);
  } NGFENS;

  typedef int (*GEN)(FGDATA *, FUNC *);

  typedef struct MEMFIL {
    char    filename[256];      /* Made larger RWD */
    char    *beginp;
    char    *endp;
    long    length;
    struct MEMFIL *next;
  } MEMFIL;

  typedef union {
    unsigned long dwData;
    unsigned char bData[4];
  } MIDIMESSAGE;

  typedef struct {
    short   type;
    short   chan;
    short   dat1;
    short   dat2;
  } MEVENT;

  typedef struct SNDMEMFILE_ {
    char            *name;          /* file ID (short name)         */
    struct SNDMEMFILE_ *nxt;
    char            *fullName;      /* full path filename           */
    size_t          nFrames;        /* file length in sample frames */
    double          sampleRate;     /* sample rate in Hz            */
    int             nChannels;      /* number of channels           */
    int             sampleFormat;   /* AE_SHORT, AE_FLOAT, etc.     */
    int             fileType;       /* TYP_WAV, TYP_AIFF, etc.      */
    int             loopMode;       /* loop mode:                   */
                                    /*   0: no loop information     */
                                    /*   1: off                     */
                                    /*   2: forward                 */
                                    /*   3: backward                */
                                    /*   4: bidirectional           */
    double          startOffs;      /* playback start offset frames */
    double          loopStart;      /* loop start (sample frames)   */
    double          loopEnd;        /* loop end (sample frames)     */
    double          baseFreq;       /* base frequency (in Hz)       */
    double          scaleFac;       /* amplitude scale factor       */
    float           data[1];        /* interleaved sample data      */
  } SNDMEMFILE;

  typedef struct pvx_memfile_ {
    char        *filename;
    struct pvx_memfile_ *nxt;
    float       *data;
    unsigned long nframes;
    int         format;
    int         fftsize;
    int         overlap;
    int         winsize;
    int         wintype;
    int         chans;
    MYFLT       srate;
  } PVOCEX_MEMFILE;

#ifdef __BUILDING_LIBCSOUND

#define INSTR   1
#define ENDIN   2
#define OPCODE  3
#define ENDOP   4
#define LABEL   5
#define SETBEG  6
#define PSET    6
#define SETEND  7

#define TOKMAX  50L     /* Should be 50 but bust */

/* IV - Oct 24 2002: max number of input/output args for user defined opcodes */
#define OPCODENUMOUTS   24

#define MBUFSIZ         (4096)
#define MIDIINBUFMAX    (1024)
#define MIDIINBUFMSK    (MIDIINBUFMAX-1)

  /* MIDI globals */

  typedef struct midiglobals {
    MEVENT  *Midevtblk;
    int     sexp;
    int     MIDIoutDONE;
    int     MIDIINbufIndex;
    MIDIMESSAGE MIDIINbuffer2[MIDIINBUFMAX];
    int     (*MidiInOpenCallback)(CSOUND *, void **, const char *);
    int     (*MidiReadCallback)(CSOUND *, void *, unsigned char *, int);
    int     (*MidiInCloseCallback)(CSOUND *, void *);
    int     (*MidiOutOpenCallback)(CSOUND *, void **, const char *);
    int     (*MidiWriteCallback)(CSOUND *, void *, const unsigned char *, int);
    int     (*MidiOutCloseCallback)(CSOUND *, void *);
    const char *(*MidiErrorStringCallback)(int);
    void    *midiInUserData;
    void    *midiOutUserData;
    void    *midiFileData;
    int     rawControllerMode;
    char    muteTrackList[256];
    unsigned char mbuf[MBUFSIZ];
    unsigned char *bufp, *endatp;
    short   datreq, datcnt;
  } MGLOBAL;

  typedef struct eventnode {
    struct eventnode  *nxt;
    unsigned long     start_kcnt;
    EVTBLK            evt;
  } EVTNODE;

  typedef struct {
    OPDS    h;
    MYFLT   *ktempo, *istartempo;
    MYFLT   prvtempo;
  } TEMPO;

  typedef struct opcodinfo {              /* IV - Oct 24 2002 */
    long    instno;
    char    *name, *intypes, *outtypes;
    short   inchns, outchns, perf_incnt, perf_outcnt;
    short   *in_ndx_list, *out_ndx_list;
    INSTRTXT *ip;
    struct opcodinfo *prv;
  } OPCODINFO;

  typedef struct polish {
    char    opcod[12];
    int     incount;
    char    *arg[4];     /* Was [4][12] */
  } POLISH;

  typedef struct token {
    char    *str;
    short   prec;
  } TOKEN;

  typedef struct names {
    char    *mac;
    struct names *next;
  } NAMES;

#include "sort.h"
#include "text.h"
#include "prototyp.h"
#include "cwindow.h"
#include "envvar.h"

#endif  /* __BUILDING_LIBCSOUND */

  struct CSOUND_ {
    /* Csound API function pointers (320 total) */
    int (*GetVersion)(void);
    int (*GetAPIVersion)(void);
    void *(*GetHostData)(CSOUND *);
    void (*SetHostData)(CSOUND *, void *hostData);
    CSOUND *(*Create)(void *hostData);
    int (*Compile)(CSOUND *, int argc, char **argv);
    int (*Perform)(CSOUND *, int argc, char **argv);
    int (*PerformKsmps)(CSOUND *);
    int (*PerformBuffer)(CSOUND *);
    int (*Cleanup)(CSOUND *);
    void (*Reset)(CSOUND *);
    void (*Destroy)(CSOUND *);
    MYFLT (*GetSr)(CSOUND *);
    MYFLT (*GetKr)(CSOUND *);
    int (*GetKsmps)(CSOUND *);
    int (*GetNchnls)(CSOUND *);
    int (*GetSampleFormat)(CSOUND *);
    int (*GetSampleSize)(CSOUND *);
    long (*GetInputBufferSize)(CSOUND *);
    long (*GetOutputBufferSize)(CSOUND *);
    MYFLT *(*GetInputBuffer)(CSOUND *);
    MYFLT *(*GetOutputBuffer)(CSOUND *);
    MYFLT *(*GetSpin)(CSOUND *);
    MYFLT *(*GetSpout)(CSOUND *);
    MYFLT (*GetScoreTime)(CSOUND *);
    void (*SetMakeXYinCallback)(CSOUND *,
                                void (*)(CSOUND *, XYINDAT *, MYFLT, MYFLT));
    void (*SetReadXYinCallback)(CSOUND *, void (*)(CSOUND *, XYINDAT *));
    void (*SetKillXYinCallback)(CSOUND *, void (*)(CSOUND *, XYINDAT *));
    int (*IsScorePending)(CSOUND *);
    void (*SetScorePending)(CSOUND *, int pending);
    MYFLT (*GetScoreOffsetSeconds)(CSOUND *);
    void (*SetScoreOffsetSeconds)(CSOUND *, MYFLT offset);
    void (*RewindScore)(CSOUND *);
    CS_PRINTF2 void (*Message)(CSOUND *, const char *fmt, ...);
    CS_PRINTF3 void (*MessageS)(CSOUND *, int attr, const char *fmt, ...);
    void (*MessageV)(CSOUND *, int attr, const char *format, va_list args);
    void (*ThrowMessage)(CSOUND *, const char *format, ...);
    void (*ThrowMessageV)(CSOUND *, const char *format, va_list args);
    void (*SetMessageCallback)(CSOUND *,
                void (*csoundMessageCallback)(CSOUND *,
                                              int attr, const char *format,
                                              va_list valist));
    void (*SetThrowMessageCallback)(CSOUND *,
                void (*throwMessageCallback)(CSOUND *, const char *format,
                                                       va_list valist));
    int (*GetMessageLevel)(CSOUND *);
    void (*SetMessageLevel)(CSOUND *, int messageLevel);
    void (*InputMessage)(CSOUND *, const char *message__);
    void (*KeyPress)(CSOUND *, char c__);
    void (*SetInputValueCallback)(CSOUND *,
                void (*inputValueCalback)(CSOUND *, const char *channelName,
                                                    MYFLT *value));
    void (*SetOutputValueCallback)(CSOUND *,
                void (*outputValueCalback)(CSOUND *, const char *channelName,
                                                     MYFLT value));
    int (*ScoreEvent)(CSOUND *,
                      char type, const MYFLT *pFields, long numFields);
    void (*SetExternalMidiInOpenCallback)(CSOUND *,
                int (*func)(CSOUND *, void **, const char *));
    void (*SetExternalMidiReadCallback)(CSOUND *,
                int (*func)(CSOUND *, void *, unsigned char *, int));
    void (*SetExternalMidiInCloseCallback)(CSOUND *,
                int (*func)(CSOUND *, void *));
    void (*SetExternalMidiOutOpenCallback)(CSOUND *,
                int (*func)(CSOUND *, void **, const char *));
    void (*SetExternalMidiWriteCallback)(CSOUND *,
                int (*func)(CSOUND *, void *, const unsigned char *, int));
    void (*SetExternalMidiOutCloseCallback)(CSOUND *,
                int (*func)(CSOUND *, void *));
    void (*SetExternalMidiErrorStringCallback)(CSOUND *,
                const char *(*func)(int));
    int (*SetIsGraphable)(CSOUND *, int isGraphable);
    void (*SetMakeGraphCallback)(CSOUND *,
                void (*makeGraphCallback)(CSOUND *, WINDAT *p,
                                                    const char *name));
    void (*SetDrawGraphCallback)(CSOUND *,
                void (*drawGraphCallback)(CSOUND *, WINDAT *p));
    void (*SetKillGraphCallback)(CSOUND *,
                void (*killGraphCallback)(CSOUND *, WINDAT *p));
    void (*SetExitGraphCallback)(CSOUND *, int (*exitGraphCallback)(CSOUND *));
    int (*NewOpcodeList)(CSOUND *, opcodeListEntry **);
    void (*DisposeOpcodeList)(CSOUND *, opcodeListEntry *);
    int (*AppendOpcode)(CSOUND *, const char *opname, int dsblksiz,
                        int thread, const char *outypes, const char *intypes,
                        int (*iopadr)(CSOUND *, void *),
                        int (*kopadr)(CSOUND *, void *),
                        int (*aopadr)(CSOUND *, void *));
    int (*AppendOpcodes)(CSOUND *, const OENTRY *opcodeList, int n);
    int (*OpenLibrary)(void **library, const char *libraryPath);
    int (*CloseLibrary)(void *library);
    void *(*GetLibrarySymbol)(void *library, const char *procedureName);
    int (*CheckEvents)(CSOUND *);
    void (*SetYieldCallback)(CSOUND *, int (*yieldCallback)(CSOUND *));
    const char *(*GetEnv)(CSOUND *, const char *name);
    char *(*FindInputFile)(CSOUND *, const char *filename, const char *envList);
    char *(*FindOutputFile)(CSOUND *,
                            const char *filename, const char *envList);
    void (*SetPlayopenCallback)(CSOUND *,
                int (*playopen__)(CSOUND *, const csRtAudioParams *parm));
    void (*SetRtplayCallback)(CSOUND *,
                void (*rtplay__)(CSOUND *, const MYFLT *outBuf, int nbytes));
    void (*SetRecopenCallback)(CSOUND *,
                int (*recopen__)(CSOUND *, const csRtAudioParams *parm));
    void (*SetRtrecordCallback)(CSOUND *,
                int (*rtrecord__)(CSOUND *, MYFLT *inBuf, int nbytes));
    void (*SetRtcloseCallback)(CSOUND *, void (*rtclose__)(CSOUND *));
    void (*AuxAlloc)(CSOUND *, long nbytes, AUXCH *auxchp);
    void *(*Malloc)(CSOUND *, size_t nbytes);
    void *(*Calloc)(CSOUND *, size_t nbytes);
    void *(*ReAlloc)(CSOUND *, void *oldp, size_t nbytes);
    void (*Free)(CSOUND *, void *ptr);
    /* Internal functions that are needed */
    void (*dispset)(CSOUND *, WINDAT *, MYFLT *, long, char *, int, char *);
    void (*display)(CSOUND *, WINDAT *);
    int (*dispexit)(CSOUND *);
    MYFLT (*intpow)(MYFLT, long);
    MEMFIL *(*ldmemfile)(CSOUND *, const char *);
    long (*strarg2insno)(CSOUND *, void *p, int is_string);
    char *(*strarg2name)(CSOUND *, char *, void *, const char *, int);
    int (*hfgens)(CSOUND *, FUNC **, const EVTBLK *, int);
    int (*insert_score_event)(CSOUND *, EVTBLK *, double);
    int (*FTAlloc)(CSOUND *, int tableNum, int len);
    int (*FTDelete)(CSOUND *, int tableNum);
    FUNC *(*FTFind)(CSOUND *, MYFLT *argp);
    FUNC *(*FTFindP)(CSOUND *, MYFLT *argp);
    FUNC *(*FTnp2Find)(CSOUND *, MYFLT *argp);
    MYFLT *(*GetTable)(CSOUND *, int tableNum, int *tableLength);
    SNDMEMFILE *(*LoadSoundFile)(CSOUND *, const char *, SF_INFO *);
    char *(*getstrformat)(int format);
    int (*sfsampsize)(int format);
    char *(*type2string)(int type);
    void *(*SAsndgetset)(CSOUND *,
                         char *, void *, MYFLT *, MYFLT *, MYFLT *, int);
    void *(*sndgetset)(CSOUND *, void *);
    int (*getsndin)(CSOUND *, void *, MYFLT *, int, void *);
    void (*rewriteheader)(SNDFILE *ofd);
    int (*Rand31)(int *seedVal);
    void (*FDRecord)(CSOUND *, FDCH *fdchp);
    void (*FDClose)(CSOUND *, FDCH *fdchp);
    void (*SetDebug)(CSOUND *, int d);
    int (*GetDebug)(CSOUND *);
    int (*TableLength)(CSOUND *, int table);
    MYFLT (*TableGet)(CSOUND *, int table, int index);
    void (*TableSet)(CSOUND *, int table, int index, MYFLT value);
    void *(*CreateThread)(uintptr_t (*threadRoutine)(void *), void *userdata);
    uintptr_t (*JoinThread)(void *thread);
    void *(*CreateThreadLock)(void);
    void (*DestroyThreadLock)(void *lock);
    int (*WaitThreadLock)(void *lock, size_t milliseconds);
    void (*NotifyThreadLock)(void *lock);
    void (*WaitThreadLockNoTimeout)(void *lock);
    void (*Sleep)(size_t milliseconds);
    void (*InitTimerStruct)(RTCLOCK *);
    double (*GetRealTime)(RTCLOCK *);
    double (*GetCPUTime)(RTCLOCK *);
    uint32_t (*GetRandomSeedFromTime)(void);
    void (*SeedRandMT)(CsoundRandMTState *p,
                       const uint32_t *initKey, uint32_t keyLength);
    uint32_t (*RandMT)(CsoundRandMTState *p);
    int (*PerformKsmpsAbsolute)(CSOUND *);
    char *(*LocalizeString)(const char *);
    int (*CreateGlobalVariable)(CSOUND *, const char *name, size_t nbytes);
    void *(*QueryGlobalVariable)(CSOUND *, const char *name);
    void *(*QueryGlobalVariableNoCheck)(CSOUND *, const char *name);
    int (*DestroyGlobalVariable)(CSOUND *, const char *name);
    int (*CreateConfigurationVariable)(CSOUND *, const char *name,
                                       void *p, int type, int flags,
                                       void *min, void *max,
                                       const char *shortDesc,
                                       const char *longDesc);
    int (*SetConfigurationVariable)(CSOUND *, const char *name, void *value);
    int (*ParseConfigurationVariable)(CSOUND *,
                                      const char *name, const char *value);
    csCfgVariable_t *(*QueryConfigurationVariable)(CSOUND *, const char *name);
    csCfgVariable_t **(*ListConfigurationVariables)(CSOUND *);
    int (*DeleteConfigurationVariable)(CSOUND *, const char *name);
    const char *(*CfgErrorCodeToString)(int errcode);
    int (*GetSizeOfMYFLT)(void);
    void **(*GetRtRecordUserData)(CSOUND *);
    void **(*GetRtPlayUserData)(CSOUND *);
    MYFLT (*GetInverseComplexFFTScale)(CSOUND *, int FFTsize);
    MYFLT (*GetInverseRealFFTScale)(CSOUND *, int FFTsize);
    void (*ComplexFFT)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*InverseComplexFFT)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*RealFFT)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*InverseRealFFT)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*RealFFTMult)(CSOUND *, MYFLT *outbuf, MYFLT *buf1, MYFLT *buf2,
                                  int FFTsize, MYFLT scaleFac);
    void (*RealFFTnp2)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*InverseRealFFTnp2)(CSOUND *, MYFLT *buf, int FFTsize);
    int (*AddUtility)(CSOUND *, const char *name,
                      int (*UtilFunc)(CSOUND *, int, char **));
    int (*RunUtility)(CSOUND *, const char *name, int argc, char **argv);
    char **(*ListUtilities)(CSOUND *);
    int (*SetUtilityDescription)(CSOUND *, const char *utilName,
                                           const char *utilDesc);
    const char *(*GetUtilityDescription)(CSOUND *, const char *utilName);
    int (*RegisterSenseEventCallback)(CSOUND *, void (*func)(CSOUND *, void *),
                                                void *userData);
    int (*RegisterDeinitCallback)(CSOUND *, void *p,
                                            int (*func)(CSOUND *, void *));
    int (*RegisterResetCallback)(CSOUND *, void *userData,
                                           int (*func)(CSOUND *, void *));
    void *(*CreateFileHandle)(CSOUND *, void *, int, const char *);
    void *(*FileOpen)(CSOUND *,
                      void *, int, const char *, void *, const char *);
    char *(*GetFileName)(void *);
    int (*FileClose)(CSOUND *, void *);
    /* PVOC-EX system */
    int (*PVOC_CreateFile)(CSOUND *, const char *,
                           unsigned long, unsigned long, unsigned long,
                           unsigned long, long, int, int,
                           float, float *, unsigned long);
    int (*PVOC_OpenFile)(CSOUND *, const char *, void *, void *);
    int (*PVOC_CloseFile)(CSOUND *, int);
    int (*PVOC_PutFrames)(CSOUND *, int, const float *, long);
    int (*PVOC_GetFrames)(CSOUND *, int, float *, unsigned long);
    int (*PVOC_FrameCount)(CSOUND *, int);
    int (*PVOC_Rewind)(CSOUND *, int, int);
    const char *(*PVOC_ErrorString)(CSOUND *);
    int (*PVOCEX_LoadFile)(CSOUND *, const char *, PVOCEX_MEMFILE *);
    char *(*GetOpcodeName)(void *p);
    int (*GetInputArgCnt)(void *p);
    unsigned long (*GetInputArgAMask)(void *p);
    unsigned long (*GetInputArgSMask)(void *p);
    char *(*GetInputArgName)(void *p, int n);
    int (*GetOutputArgCnt)(void *p);
    unsigned long (*GetOutputArgAMask)(void *p);
    unsigned long (*GetOutputArgSMask)(void *p);
    char *(*GetOutputArgName)(void *p, int n);
    int (*SetReleaseLength)(void *p, int n);
    MYFLT (*SetReleaseLengthSeconds)(void *p, MYFLT n);
    int (*GetMidiChannelNumber)(void *p);
    MCHNBLK *(*GetMidiChannel)(void *p);
    int (*GetMidiNoteNumber)(void *p);
    int (*GetMidiVelocity)(void *p);
    int (*GetReleaseFlag)(void *p);
    double (*GetOffTime)(void *p);
    MYFLT *(*GetPFields)(void *p);
    int (*GetInstrumentNumber)(void *p);
    CS_NORETURN CS_PRINTF2 void (*Die)(CSOUND *, const char *msg, ...);
    CS_PRINTF2 int (*InitError)(CSOUND *, const char *msg, ...);
    CS_PRINTF2 int (*PerfError)(CSOUND *, const char *msg, ...);
    CS_PRINTF2 void (*Warning)(CSOUND *, const char *msg, ...);
    CS_PRINTF2 void (*DebugMsg)(CSOUND *, const char *msg, ...);
    CS_NORETURN void (*LongJmp)(CSOUND *, int);
    CS_PRINTF2 void (*ErrorMsg)(CSOUND *, const char *fmt, ...);
    void (*ErrMsgV)(CSOUND *, const char *hdr, const char *fmt, va_list);
    int (*GetChannelPtr)(CSOUND *, MYFLT **p, const char *name, int type);
    int (*ListChannels)(CSOUND *, char ***names, int **types);
    int (*SetControlChannelParams)(CSOUND *, const char *name,
                                   int type, MYFLT dflt, MYFLT min, MYFLT max);
    int (*GetControlChannelParams)(CSOUND *, const char *name,
                                   MYFLT *dflt, MYFLT *min, MYFLT *max);
    SUBR dummyfn_1;
    SUBR dummyfn_2[116];
    /* ----------------------- public data fields ----------------------- */
    OPDS          *ids, *pds;           /* used by init and perf loops */
    int           ksmps, global_ksmps, nchnls, spoutactive;
    long          kcounter, global_kcounter;
    int           reinitflag;
    int           tieflag;
    MYFLT         esr, onedsr, sicvt;
    MYFLT         tpidsr, pidsr, mpidsr, mtpdsr;
    MYFLT         onedksmps;
    MYFLT         ekr, global_ekr;
    MYFLT         onedkr;
    MYFLT         kicvt;
    MYFLT         e0dbfs, dbfs_to_float;
    double        timeOffs, beatOffs;   /* start time of current section    */
    double        curTime, curTime_inc; /* cur. time in secs, inc. per kprd */
    double        curBeat, curBeat_inc; /* cur. time in beats, inc per kprd */
    double        beatTime;             /* beat time = 60 / tempo           */
    unsigned int  rtin_dev, rtout_dev;
    char          *rtin_devs, *rtout_devs;
    int           nchanik, nchania, nchanok, nchanoa;
    MYFLT         *chanik, *chania, *chanok, *chanoa;
    MYFLT         *zkstart;
    MYFLT         *zastart;
    long          zklast;
    long          zalast;
    MYFLT         *spin;
    MYFLT         *spout;
    int           nspin;
    int           nspout;
    OPARMS        *oparms;
    EVTBLK        *currevent;
    INSDS         *curip;
    void          *hostdata;
    void          *rtRecord_userdata;
    void          *rtPlay_userdata;
    char          *orchname, *scorename;
    int           holdrand;
    int           strVarMaxLen;     /* maximum length of string variables + 1 */
    int           maxinsno;
    int           strsmax;
    char          **strsets;
    INSTRTXT      **instrtxtp;
    MCHNBLK       *m_chnbp[64];     /* reserve space for up to 4 MIDI devices */
    RTCLOCK       *csRtClock;
    CsoundRandMTState *csRandState;
    int           randSeed1;
    int           randSeed2;
    /* ------- private data (not to be used by hosts or externals) ------- */
#ifdef __BUILDING_LIBCSOUND
    /* callback function pointers */
    SUBR          first_callback_;
    void          (*InputValueCallback_)(CSOUND *,
                                         const char *channelName, MYFLT *value);
    void          (*OutputValueCallback_)(CSOUND *,
                                          const char *channelName, MYFLT value);
    void          (*csoundMessageCallback_)(CSOUND *, int attr,
                                            const char *format, va_list args);
    void          (*csoundThrowMessageCallback_)(CSOUND *, const char *format,
                                                           va_list args);
    void          (*csoundMakeGraphCallback_)(CSOUND *, WINDAT *windat,
                                                        const char *name);
    void          (*csoundDrawGraphCallback_)(CSOUND *, WINDAT *windat);
    void          (*csoundKillGraphCallback_)(CSOUND *, WINDAT *windat);
    int           (*csoundExitGraphCallback_)(CSOUND *);
    int           (*csoundYieldCallback_)(CSOUND *);
    void          (*csoundMakeXYinCallback_)(CSOUND *, XYINDAT *, MYFLT, MYFLT);
    void          (*csoundReadXYinCallback_)(CSOUND *, XYINDAT *);
    void          (*csoundKillXYinCallback_)(CSOUND *, XYINDAT *);
    void          (*cscoreCallback_)(CSOUND *);
    SUBR          last_callback_;
    /* these are not saved on RESET */
    int           (*playopen_callback)(CSOUND *, const csRtAudioParams *parm);
    void          (*rtplay_callback)(CSOUND *, const MYFLT *outBuf, int nbytes);
    int           (*recopen_callback)(CSOUND *, const csRtAudioParams *parm);
    int           (*rtrecord_callback)(CSOUND *, MYFLT *inBuf, int nbytes);
    void          (*rtclose_callback)(CSOUND *);
    /* end of callbacks */
    MYFLT         cpu_power_busy;
    char          *xfilename;
    /* oload.h */
    short         nlabels;
    short         ngotos;
    int           peakchunks;
    int           keep_tmp;
    int           dither_output;
    OENTRY        *opcodlst;
    void          *opcode_list;
    OENTRY        *oplstend;
    int           maxopcno;
    long          nrecs;
    FILE*         Linepipe;
    int           Linefd;
    MYFLT         *ls_table;
    FILE*         scfp;
    FILE*         oscfp;
    MYFLT         maxamp[MAXCHNLS];
    MYFLT         smaxamp[MAXCHNLS];
    MYFLT         omaxamp[MAXCHNLS];
    unsigned long maxpos[MAXCHNLS], smaxpos[MAXCHNLS], omaxpos[MAXCHNLS];
    FILE*         scorein;
    FILE*         scoreout;
    MYFLT         *pool;
    int           *argoffspace;
    INSDS         *frstoff;
    jmp_buf       exitjmp;
    SRTBLK        *frstbp;
    int           sectcnt;
    int           inerrcnt, synterrcnt, perferrcnt;
    INSTRTXT      instxtanchor;
    INSDS         actanchor;
    long          rngcnt[MAXCHNLS];
    short         rngflg, multichan;
    void          *evtFuncChain;
    EVTNODE       *OrcTrigEvts;             /* List of events to be started */
    EVTNODE       *freeEvtNodes;
    int           csoundIsScorePending_;
    int           advanceCnt;
    int           initonly;
    int           evt_poll_cnt;
    int           evt_poll_maxcnt;
    int           Mforcdecs, Mxtroffs, MTrkend;
    MYFLT         tran_sr, tran_kr, tran_ksmps;
    MYFLT         tran_0dbfs;
    int           tran_nchnls;
    OPCODINFO     *opcodeInfo;
    void          *instrumentNames;
    void          *strsav_str;
    void          *strsav_space;
    FUNC**        flist;
    int           maxfnum;
    GEN           *gensub;
    int           genmax;
    int           ftldno;
    void          **namedGlobals;
    int           namedGlobalsCurrLimit;
    int           namedGlobalsMaxLimit;
    void          **cfgVariableDB;
    double        prvbt, curbt, nxtbt;
    double        curp2, nxtim;
    int           cyclesRemaining;
    EVTBLK        evt;
    void          *memalloc_db;
    MGLOBAL       *midiGlobals;
    void          *envVarDB;
    MEMFIL        *memfiles;
    PVOCEX_MEMFILE *pvx_memfiles;
    int           FFT_max_size;
    void          *FFT_table_1;
    void          *FFT_table_2;
    /* statics from twarp.c should be TSEG* */
    void          *tseg, *tpsave, *tplim;
    /* Statics from express.c */
    long          polmax;
    long          toklen;
    char          *tokenstring;
    POLISH        *polish;
    TOKEN         *token;
    TOKEN         *tokend;
    TOKEN         *tokens;
    TOKEN         **tokenlist;
    int           toklength;
    int           acount, kcount, icount, Bcount, bcount;
    char          *stringend;
    TOKEN         **revp, **pushp, **argp, **endlist;
    char          *assign_outarg;
    int           argcnt_offs, opcode_is_assign, assign_type;
    int           strVarSamples;    /* number of MYFLT locations for string */
    MYFLT         *gbloffbas;       /* was static in oload.c */
    void          *otranGlobals;
    void          *rdorchGlobals;
    void          *sreadGlobals;
    void          *extractGlobals;
    void          *oneFileGlobals;
    void          *lineventGlobals;
    void          *musmonGlobals;
    void          *libsndGlobals;
    void          (*spinrecv)(CSOUND *);
    void          (*spoutran)(CSOUND *);
    int           (*audrecv)(CSOUND *, MYFLT *, int);
    void          (*audtran)(CSOUND *, const MYFLT *, int);
    int           warped;               /* rdscor.c */
    int           sstrlen;
    char          *sstrbuf;
    int           enableMsgAttr;        /* csound.c */
    int           sampsNeeded;
    MYFLT         csoundScoreOffsetSeconds_;
    int           inChar_;
    int           isGraphable_;
    int           delayr_stack_depth;   /* ugens6.c */
    void          *first_delayr;
    void          *last_delayr;
    long          revlpsiz[6];
    long          revlpsum;
    double        rndfrac;              /* aops.c */
    MYFLT         *logbase2;
    NAMES         *omacros, *smacros;
    void          *namedgen;            /* fgens.c */
    void          *open_files;          /* fileopen.c */
    void          *searchPathCache;
    void          *sndmemfiles;
    void          *reset_list;
    void          *pvFileTable;         /* pvfileio.c */
    int           pvNumFiles;
    int           pvErrorCode;
    void          *pvbufreadaddr;       /* pvinterp.c */
    void          *tbladr;              /* vpvoc.c */
    int           enableHostImplementedAudioIO;
    int           hostRequestedBufferSize;
    /* engineState is sum of:
     *   1: csoundPreCompile was called
     *   2: csoundCompile was called
     *   4: csoundRunUtility was called
     *   8: csoundCleanup needs to be called
     *  16: csoundLongJmp was called
     */
    int           engineState;
    int           stdin_assign_flg;
    int           stdout_assign_flg;
    int           orcname_mode;         /* 0: normal, 1: ignore, 2: fail */
    void          *csmodule_db;
    char          *dl_opcodes_oplibs;
    char          *SF_csd_licence;
    char          *SF_id_title;
    char          *SF_id_copyright;
    char          *SF_id_software;
    char          *SF_id_artist;
    char          *SF_id_comment;
    char          *SF_id_date;
    void          *utility_db;
    short         *isintab;             /* ugens3.c */
    void          *lprdaddr;            /* ugens5.c */
    int           currentLPCSlot;
    int           max_lpc_slot;
    void          *chn_db;
    int           opcodedirWasOK;
    int           disable_csd_options;
    CsoundRandMTState randState_;
#endif  /* __BUILDING_LIBCSOUND */
  };

/*
 * Move the C++ guards to enclose the entire file,
 * in order to enable C++ to #include this file.
 */

#ifdef __cplusplus
}
#endif

#endif  /* CSOUNDCORE_H */

