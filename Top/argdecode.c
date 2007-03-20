/*
    argdecode.c:

    Copyright (C) 1998 John ffitch

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
#include "csoundCore.h"         /*                      ARGDECODE.C     */
#include "soundio.h"
#include "new_opts.h"
#include "csmodule.h"
#include <ctype.h>

extern void strset_option(CSOUND *csound, char *s);     /* from str_ops.c */

#define FIND(MSG)   if (*s == '\0')  \
                      if (!(--argc) || (((s = *++argv) != NULL) && *s == '-')) \
                         dieu(csound, MSG);

#define STDINASSIGN_SNDFILE     1
#define STDINASSIGN_LINEIN      2
#define STDINASSIGN_MIDIFILE    4
#define STDINASSIGN_MIDIDEV     8

#define STDOUTASSIGN_SNDFILE    1
#define STDOUTASSIGN_MIDIOUT    2

/* IV - Feb 19 2005 */

static void set_stdin_assign(CSOUND *csound, int type, int state)
{
  if (state)
    csound->stdin_assign_flg |= type;
  else
    csound->stdin_assign_flg &= (~type);
}

static void set_stdout_assign(CSOUND *csound, int type, int state)
{
  if (state)
    csound->stdout_assign_flg |= type;
  else
    csound->stdout_assign_flg &= (~type);
}

/* IV - Feb 19 2005 */
static const char *shortUsageList[] = {
  "--help\tprint long usage options",
  "-U unam\trun utility program unam",
  "-C\tuse Cscore processing of scorefile",
  "-I\tI-time only orch run",
  "-n\tno sound onto disk",
  "-i fnam\tsound input filename",
  "-o fnam\tsound output filename",
  "-b N\tsample frames (or -kprds) per software sound I/O buffer",
  "-B N\tsamples per hardware sound I/O buffer",
  "-A\tcreate an AIFF format output soundfile",
  "-W\tcreate a WAV format output soundfile",
  "-J\tcreate an IRCAM format output soundfile",
  "-h\tno header on output soundfile",
  "-c\t8-bit signed_char sound samples",
#ifdef never
  "-a\talaw sound samples",
#endif
  "-8\t8-bit unsigned_char sound samples",
  "-u\tulaw sound samples",
  "-s\tshort_int sound samples",
  "-l\tlong_int sound samples",
  "-f\tfloat sound samples",
  "-3\t24bit sound samples",
  "-r N\torchestra srate override",
  "-k N\torchestra krate override",
  "-K\tDo not generate PEAK chunks",
  "-v\tverbose orch translation",
  "-m N\ttty message level. Sum of:",
  "\t\t1=note amps, 2=out-of-range msg, 4=warnings",
  "\t\t0/32/64/96=note amp format (raw,dB,colors)",
  "\t\t128=print benchmark information",
  "-d\tsuppress all displays",
  "-g\tsuppress graphics, use ascii displays",
  "-G\tsuppress graphics, use Postscript displays",
  "-x fnam\textract from score.srt using extract file 'fnam'",
  "-t N\tuse uninterpreted beats of the score, initially at tempo N",
  "-t 0\tuse score.srt for sorted score rather than a temporary",
  "-L dnam\tread Line-oriented realtime score events from device 'dnam'",
  "-M dnam\tread MIDI realtime events from device 'dnam'",
  "-F fnam\tread MIDIfile event stream from file 'fnam'",
  /*  "-P N\tMIDI sustain pedal threshold (0 - 128)", */
  "-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)",
  "-H#\tprint heartbeat style 1, 2 or 3 at each soundfile write",
  "-N\tnotify (ring the bell) when score or miditrack is done",
  "-T\tterminate the performance when miditrack is done",
  "-D\tdefer GEN01 soundfile loads until performance time",
  "-Q dnam\tselect MIDI output device",
  "-z\tList opcodes in this version",
  "-Z\tDither output",
#if defined(LINUX)
  "--sched     set real-time priority and lock memory",
  "            (requires -d and real time audio (-iadc/-odac))",
  "--sched=N   set specified scheduling priority, and lock memory",
  "            (requires -d and real time audio (-iadc/-odac))",
#endif
  NULL
};

static const char *longUsageList[] = {
  "--format={wav,aiff,au,raw,ircam,w64,wavex,sd2,flac}",
  "--format={alaw,ulaw,schar,uchar,float,short,long,24bit}",
  "\t\t\tSet output file format",
  "--aiff\t\t\tSet AIFF format",
  "--au\t\t\tSet AU format",
  "--wave\t\t\tSet WAV format",
  "--ircam\t\t\tSet IRCAM format",
  "--noheader\t\tRaw format",
  "--nopeaks\t\tDo not write peak information",
  "",
  "--nodisplays\t\tsuppress all displays",
  "--asciidisplay\t\tsuppress graphics, use ascii displays",
  "--postscriptdisplay\tsuppress graphics, use Postscript displays",
  "",
  "--defer-gen1\t\tdefer GEN01 soundfile loads until performance time",
  "--iobufsamps=N\t\tsample frames (or -kprds) per software sound I/O buffer",
  "--hardwarebufsamps=N\tsamples per hardware sound I/O buffer",
  "--cscore\t\tuse Cscore processing of scorefile",
  "",
  "--midifile=FNAME\tread MIDIfile event stream from file",
  "--midioutfile=FNAME\twrite MIDI output to file FNAME",
  "--midi-device=FNAME\tread MIDI realtime events from device",
  "--terminate-on-midi\tterminate the performance when miditrack is done",
  "",
  "--heartbeat=N\t\tprint a heartbeat style 1, 2 or 3 at each soundfile write",
  "--notify\t\tnotify (ring the bell) when score or miditrack is done",
  "--rewrite\t\tcontinually rewrite header while writing soundfile (WAV/AIFF)",
  "",
  "--input=FNAME\t\tsound input filename",
  "--output=FNAME\t\tsound output filename",
  "--logfile=FNAME\t\tlog output to file",
  "",
  "--nosound\t\tno sound onto disk or device",
  "--tempo=N\t\tuse uninterpreted beats of the score, initially at tempo N",
  "--i-only\t\tI-time only orch run",
  "--control-rate=N\torchestra krate override",
  "--sample-rate=N\t\torchestra srate override",
  "--score-in=FNAME\tread Line-oriented realtime score events from device",
  "--messagelevel=N\ttty message level, sum of:",
  "\t\t\t\t1=note amps, 2=out-of-range msg, 4=warnings,",
  "\t\t\t\t0/32/64/96=note amp format (raw,dB,colors),",
  "\t\t\t\t128=print benchmark information",
  "",
  "--extract-score=FNAME\textract from score.srt using extract file",
  "--keep-sorted-score",
  "--expression-opt\toptimise use of temporary variables in expressions",
  "--env:NAME=VALUE\tset environment variable NAME to VALUE",
  "--env:NAME+=VALUE\tappend VALUE to environment variable NAME",
  "--strsetN=VALUE\t\tset strset table at index N to VALUE",
  "--utility=NAME\t\trun utility program",
  "--verbose\t\tverbose orch translation",
  "--list-opcodes\t\tList opcodes in this version",
  "--list-opcodesN\t\tList opcodes in style N in this version",
  "--dither\t\tDither output",
  "--sched\t\t\tset real-time scheduling priority and lock memory",
  "--sched=N\t\tset priority to N and lock memory",
  "--opcode-lib=NAMES\tDynamic libraries to load",
  "--omacro:XXX=YYY\tSet orchestra macro XXX to value YYY",
  "--smacro:XXX=YYY\tSet score macro XXX to value YYY",
  "--midi-key=N\t\tRoute MIDI note on message",
  "\t\t\tkey number to pfield N as MIDI value [0-127]",
  "--midi-key-cps=N\tRoute MIDI note on message",
  "\t\t\tkey number to pfield N as cycles per second",
  "--midi-key-oct=N\tRoute MIDI note on message",
  "\t\t\tkey number to pfield N as linear octave",
  "--midi-key-pch=N\tRoute MIDI note on message",
  "\t\t\tkey number to pfield N as oct.pch",
  "--midi-velocity=N\tRoute MIDI note on message",
  "\t\t\tvelocity number to pfield N as MIDI value [0-127]",
  "--midi-velocity-amp=N\tRoute MIDI note on message",
  "\t\t\tvelocity number to pfield N as amplitude",
  "--no-default-paths\tturn off relative paths from CSD/ORC/SCO",
  "",
  "--help\t\t\tLong help",
  NULL
};

/* IV - Feb 19 2005 */
void print_short_usage(CSOUND *csound)
{
  char    buf[256];
  int     i;
  i = -1;
  while (shortUsageList[++i] != NULL) {
    sprintf(buf, "%s\n", shortUsageList[i]);
    csound->Message(csound, Str(buf));
  }
  csound->Message(csound,
                  Str("flag defaults: csound -s -otest -b%d -B%d -m%d\n"),
                  IOBUFSAMPS, IODACSAMPS, csound->oparms->msglevel);
}

static void longusage(CSOUND *p)
{
  const char **sp;
  p->Message(p, Str("Usage:\tcsound [-flags] orchfile scorefile\n"));
  p->Message(p, Str("Legal flags are:\n"));
  p->Message(p, Str("Long format:\n\n"));
  for (sp = &(longUsageList[0]); *sp != NULL; sp++)
    p->Message(p, "%s\n", Str(*sp));
  /* IV - Feb 19 2005 */
  dump_cfg_variables(p);
  p->Message(p, Str("\nShort form:\n"));
  print_short_usage(p);
  p->LongJmp(p, 0);
}

void dieu(CSOUND *csound, char *s, ...)
{
    va_list args;

    csound->Message(csound,Str("Usage:\tcsound [-flags] orchfile scorefile\n"));
    csound->Message(csound,Str("Legal flags are:\n"));
    print_short_usage(csound);
    va_start(args, s);
    csound->ErrMsgV(csound, Str("Csound Command ERROR:\t"), s, args);
    va_end(args);

    csound->LongJmp(csound, 1);
}

void set_output_format(OPARMS *O, char c)
{
  switch (c) {
  case 'a':
    O->outformat = AE_ALAW;    /* a-law soundfile */
    break;

  case 'c':
    O->outformat = AE_CHAR;    /* signed 8-bit soundfile */
    break;

  case '8':
    O->outformat = AE_UNCH;    /* unsigned 8-bit soundfile */
    break;

  case 'f':
    O->outformat = AE_FLOAT;   /* float soundfile */
    break;

  case 's':
    O->outformat = AE_SHORT;   /* short_int soundfile*/
    break;

  case 'l':
    O->outformat = AE_LONG;    /* long_int soundfile */
    break;

  case 'u':
    O->outformat = AE_ULAW;    /* mu-law soundfile */
    break;

  case '3':
    O->outformat = AE_24INT;   /* 24bit packed soundfile*/
    break;

  case 'e':
    O->outformat = AE_FLOAT;   /* float soundfile (for rescaling) */
    break;

  default:
    return; /* do nothing */
  };
}

typedef struct  {
    char    *longformat;
    char    shortformat;
} SAMPLE_FORMAT_ENTRY;

static const SAMPLE_FORMAT_ENTRY sample_format_map[] = {
  { "alaw",   'a' },  { "schar",  'c' },  { "uchar",  '8' },
  { "float",  'f' },  { "long",   'l' },
  { "short",  's' },  { "ulaw",   'u' },  { "24bit",  '3' },
  { NULL, '\0' }
};

typedef struct {
    char    *format;
    int     type;
} SOUNDFILE_TYPE_ENTRY;

static const SOUNDFILE_TYPE_ENTRY file_type_map[] = {
    { "wav",    TYP_WAV   },  { "aiff",   TYP_AIFF  },
    { "au",     TYP_AU    },  { "raw",    TYP_RAW   },
    { "paf",    TYP_PAF   },  { "svx",    TYP_SVX   },
    { "nist",   TYP_NIST  },  { "voc",    TYP_VOC   },
    { "ircam",  TYP_IRCAM },  { "w64",    TYP_W64   },
    { "mat4",   TYP_MAT4  },  { "mat5",   TYP_MAT5  },
    { "pvf",    TYP_PVF   },  { "xi",     TYP_XI    },
    { "htk",    TYP_HTK   },  { "sds",    TYP_SDS   },
    { "avr",    TYP_AVR   },  { "wavex",  TYP_WAVEX },
#if defined(HAVE_LIBSNDFILE) && HAVE_LIBSNDFILE >= 1011
    { "sd2",    TYP_SD2   },
#  if HAVE_LIBSNDFILE >= 1013
    { "flac",   TYP_FLAC  },  { "caf",    TYP_CAF   },
#  endif
#endif
    { NULL , -1 }
};

static int decode_long(CSOUND *csound, char *s, int argc, char **argv)
{
    OPARMS  *O = csound->oparms;
    /* Add other long options here */
    if (O->odebug)
      csound->Message(csound, "decode_long %s\n", s);
    if (!(strncmp(s, "omacro:", 7))) {
      NAMES *nn = (NAMES*) mmalloc(csound, sizeof(NAMES));
      nn->mac = s;
      nn->next = csound->omacros;
      csound->omacros = nn;
      return 1;
    }
    else if (!(strncmp(s, "smacro:", 7))) {
      NAMES *nn = (NAMES*) mmalloc(csound, sizeof(NAMES));
      nn->mac = s;
      nn->next = csound->smacros;
      csound->smacros = nn;
      return 1;
    }
    else if (!(strncmp(s, "format=", 7))) {
      const SAMPLE_FORMAT_ENTRY   *sfe;
      const SOUNDFILE_TYPE_ENTRY  *ff;
      s += 7;
      do {
        char  *t;
        t = strchr(s, ':');
        if (t != NULL)
          *(t++) = '\0';
        for (ff = &(file_type_map[0]); ff->format != NULL; ff++) {
          if (strcmp(ff->format, s) == 0) {
            O->filetyp = ff->type;
            goto nxtToken;
          }
        }
        for (sfe = &(sample_format_map[0]); sfe->longformat != NULL; sfe++) {
          if (strcmp(s, sfe->longformat) == 0) {
            set_output_format(O, sfe->shortformat);
            goto nxtToken;
          }
        }
        csoundErrorMsg(csound, Str("unknown output format: '%s'"), s);
        return 0;
 nxtToken:
        s = t;
      } while (s != NULL);
      return 1;
    }
    /* -A */
    else if (!(strcmp (s, "aiff"))) {
      O->filetyp = TYP_AIFF;            /* AIFF output request */
      return 1;
    }
    else if (!(strcmp (s, "au"))) {
      O->filetyp = TYP_AU;              /* AU output request */
      return 1;
    }
    else if (!(strncmp (s, "iobufsamps=", 11))) {
      s += 11;
      if (*s=='\0') dieu(csound, Str("no iobufsamps"));
      /* defaults in musmon.c */
      O->inbufsamps = O->outbufsamps = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "hardwarebufsamps=", 17))) {
      s += 17;
      if (*s=='\0') dieu(csound, Str("no hardware bufsamps"));
      O->inbufsamps = O->outbufsamps = atoi(s);
      return 1;
    }
    else if (!(strcmp (s, "cscore"))) {
      O->usingcscore = 1;               /* use cscore processing  */
      return 1;
    }
    else if (!(strcmp (s, "nodisplays"))) {
      O->displays = 0;                  /* no func displays */
      return 1;
    }
    else if (!(strcmp (s, "displays"))) {
      O->displays = 1;                  /* func displays */
      return 1;
    }
    else if (!(strcmp (s, "defer-gen1"))) {
      O->gen01defer = 1;                /* defer GEN01 sample loads */
      return 1;                         /*   until performance time */
    }
    else if (!(strncmp (s, "midifile=", 9))) {
      s += 9;
      if (*s == '\0') dieu(csound, Str("no midifile name"));
      O->FMidiname = s;                 /* Midifile name */
      if (!strcmp(O->FMidiname, "stdin")) {
        set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 1);
#if defined(WIN32) || defined(mac_classic)
        csoundDie(csound, Str("-F: stdin not supported on this platform"));
#endif
      }
      else
        set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 0);
      O->FMidiin = 1;                   /***************/
      return 1;
    }
    else if (!(strncmp (s, "midioutfile=", 12))) {
      s += 12;
      if (*s == '\0') dieu(csound, Str("no midi output file name"));
      O->FMidioutname = s;
      return 1;
    }
    /* -g */
    else if (!(strcmp (s, "asciidisplay"))) {
      O->graphsoff = 1;                 /* don't use graphics but ASCII */
      return 1;
    }
    /* -G */
    else if (!(strcmp (s, "postscriptdisplay"))) {
      O->postscript = 1;                /* don't use graphics but PostScript */
      return 1;
    }
    /* -h */
    else if (!(strcmp (s, "noheader"))) {
      O->filetyp = TYP_RAW;             /* RAW output request */
      return 1;
    }
    else if (!(strncmp (s, "heartbeat=", 10))) {
      s += 10;
      if (*s == '\0') O->heartbeat = 1;
      else O->heartbeat = atoi(s);
      return 1;
    }
#ifdef EMBEDDED_PYTHON
    else if (strncmp(s, "pyvar=", 6) == 0) {
      s += 6;
      if (python_add_cmdline_definition(s))
        dieu(csound, Str("invalid python variable definition syntax"));
      return 1;
    }
#endif
    else if (!(strncmp (s, "input=", 6))) {
      s += 6;
      if (*s == '\0') dieu(csound, Str("no infilename"));
      O->infilename = s;                /* soundin name */
      if (strcmp(O->infilename, "stdout") == 0)
        csoundDie(csound, Str("input cannot be stdout"));
      if (strcmp(O->infilename, "stdin") == 0) {
        set_stdin_assign(csound, STDINASSIGN_SNDFILE, 1);
#if defined(WIN32) || defined(mac_classic)
        csoundDie(csound, Str("stdin audio not supported"));
#endif
      }
      else
        set_stdin_assign(csound, STDINASSIGN_SNDFILE, 0);
      O->sfread = 1;
      return 1;
    }
    /*
      -I I-time only orch run
     */
    else if (!(strcmp (s, "i-only"))) {
      csound->initonly = 1;
      return 1;
    }
    /*
      -j Used in localisation
      -J create an IRCAM format output soundfile
     */
    else if (!(strcmp (s, "ircam"))) {
      O->filetyp = TYP_IRCAM;           /* IRCAM output request */
      return 1;
    }
    /*
      -k N orchestra krate override
     */
    else if (!(strncmp(s, "control-rate=", 13))) {
      s += 13;
      if (*s=='\0') dieu(csound, Str("no control rate"));
      O->kr_override = atoi(s);
      return 1;
    }
    /* -K */
    else if (!(strcmp (s, "nopeaks"))) {
      csound->peakchunks = 0;           /* Do not write peak information */
      return 1;
    }
    /*
      -L dnam read Line-oriented realtime score events from device 'dnam'
     */
    else if (!(strncmp (s, "score-in=", 9))) {
      s += 9;
      if (*s=='\0') dieu(csound, Str("no Linein score device_name"));
      O->Linename = s;
      if (!strcmp(O->Linename, "stdin")) {
        set_stdin_assign(csound, STDINASSIGN_LINEIN, 1);
#if defined(mac_classic)
        csoundDie(csound, Str("-L: stdin not supported on this platform"));
#endif
      }
      else
        set_stdin_assign(csound, STDINASSIGN_LINEIN, 0);
      O->Linein = 1;
      return 1;
    }
    /*
      -m N tty message level.
      Sum of: 1=note amps, 2=out-of-range msg, 4=warnings
     */
    else if (!(strncmp (s, "messagelevel=", 13))) {
      s += 13;
      if (*s=='\0') dieu(csound, Str("no message level"));
      O->msglevel = atoi(s);
      return 1;
    }
    /*
      -M dnam read MIDI realtime events from device 'dnam'
     */
    else if (!(strncmp (s, "midi-device=", 12))) {
      s += 12;
      if (*s=='\0') dieu(csound, Str("no midi device_name"));
      O->Midiname = s;
      if (!strcmp(O->Midiname, "stdin")) {
        set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 1);
#if defined(WIN32) || defined(mac_classic)
        csoundDie(csound, Str("-M: stdin not supported on this platform"));
#endif
      }
      else
        set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 0);
      O->Midiin = 1;
      return 1;
    }
    /* -n no sound */
    else if (!(strcmp (s, "nosound"))) {
      O->sfwrite = 0;                   /* nosound        */
      return 1;
    }
    /* -N */
    else if (!(strcmp (s, "notify"))) {
      O->ringbell = 1;                  /* notify on completion */
      return 1;
    }
    else if (!(strncmp (s, "output=", 7))) {
      s += 7;
      if (*s == '\0') dieu(csound, Str("no outfilename"));
      O->outfilename = s;               /* soundout name */
      if (strcmp(O->outfilename, "stdin") == 0)
        dieu(csound, Str("-o cannot be stdin"));
      if (strcmp(O->outfilename, "stdout") == 0) {
        set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 1);
#if defined(WIN32) || defined(mac_classic)
        csoundDie(csound, Str("stdout audio not supported"));
#endif
      }
      else
        set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 0);
      O->sfwrite = 1;
      return 1;
    }
    else if (!(strncmp (s, "logfile=", 8))) {
      s += 8;
      if (*s=='\0') dieu(csound, Str("no log file"));
      return 1;
    }
    /* -r N */
    else if (!(strncmp (s, "sample-rate=", 12))) {
      s += 12;
      O->sr_override = atol(s);
      return 1;
    }
    /* R */
    else if (!(strcmp (s, "rewrite"))) {
      O->rewrt_hdr = 1;
      return 1;
    }
    /* -S  */
    /* tempo=N use uninterpreted beats of the score, initially at tempo N
     */
    else if (!(strncmp (s, "tempo=", 6))) {
      s += 6;
      O->cmdTempo = atoi(s);
      O->Beatmode = 1;                  /* on uninterpreted Beats */
      return 1;
    }
    /* -t0 */
    else if (!(strcmp (s, "keep-sorted-score"))) {
      csound->keep_tmp = 1;
      return 1;
    }
    /* IV - Jan 27 2005: --expression-opt */
    else if (!(strcmp (s, "expression-opt"))) {
      O->expr_opt = 1;
      return 1;
    }
    else if (!(strcmp (s, "no-expression-opt"))) {
      O->expr_opt = 0;
      return 1;
    }
    else if (!(strncmp (s, "env:", 4))) {
      if (csoundParseEnv(csound, s + 4) == CSOUND_SUCCESS)
        return 1;
      else
        return 0;
    }
    else if (!(strncmp (s, "strset", 6))) {
      strset_option(csound, s + 6);
      return 1;
    }
    /* -T terminate the performance when miditrack is done */
    else if (!(strcmp (s, "terminate-on-midi"))) {
      O->termifend = 1;                 /* terminate on midifile end */
      return 1;
    }
    else if (!(strncmp (s, "utility=", 8))) {
      int retval;
      s += 8;
      if (*s=='\0') dieu(csound, Str("no utility name"));
      retval = csoundRunUtility(csound, s, argc, argv);
      csound->LongJmp(csound, retval);
    }
    /* -v */
    else if (!(strcmp (s, "verbose"))) {
      O->odebug = 1;                    /* verbose otran  */
      return 1;
    }
    /* -x fnam extract from score.srt using extract file 'fnam' */
    else if (!(strncmp(s, "extract-score=", 14))) {
      s += 14;
      if (*s=='\0') dieu(csound, Str("no xfilename"));
      csound->xfilename = s;
      return 1;
    }
    else if (!(strcmp(s, "wave"))) {
      O->filetyp = TYP_WAV;             /* WAV output request */
      return 1;
    }
    else if (!(strncmp (s, "list-opcodes", 12))) {
      int full = 0;
      s += 12;

      if (*s != '\0') {
        if (isdigit(*s)) full = *s++ - '0';
      }
      csoundLoadExternals(csound);
      if (csoundInitModules(csound) != 0)
        csound->LongJmp(csound, 1);
      list_opcodes(csound, full);
      csound->LongJmp(csound, 0);
    }
    /* -Z */
    else if (!(strcmp (s, "dither"))) {
      csound->dither_output = 1;
      return 1;
    }
    else if (!(strncmp (s, "midi-key=", 9))) {
      s += 9;
      O->midiKey = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-key-cps=", 13))) {
      s += 13 ;
      O->midiKeyCps = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-key-oct=", 13))) {
      s += 13 ;
      O->midiKeyOct = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-key-pch=", 13))) {
      s += 13 ;
      O->midiKeyPch = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-velocity=", 14))) {
      s += 14;
      O->midiVelocity = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-velocity-amp=", 18))) {
      s += 18;
      O->midiVelocityAmp = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "opcode-lib=", 11))) {
      int   nbytes;
      s += 11;
      nbytes = (int) strlen(s) + 1;
      if (csound->dl_opcodes_oplibs == NULL) {
        /* start new library list */
        csound->dl_opcodes_oplibs = (char*) mmalloc(csound, (size_t) nbytes);
        strcpy(csound->dl_opcodes_oplibs, s);
      }
      else {
        /* append to existing list */
        nbytes += ((int) strlen(csound->dl_opcodes_oplibs) + 1);
        csound->dl_opcodes_oplibs = (char*) mrealloc(csound,
                                                     csound->dl_opcodes_oplibs,
                                                     (size_t) nbytes);
        strcat(csound->dl_opcodes_oplibs, ",");
        strcat(csound->dl_opcodes_oplibs, s);
      }
      return 1;
    } else if (!(strcmp(s, "default-paths"))) {
      O->noDefaultPaths = 0;
      return 1;
    } else if (!(strcmp(s, "no-default-paths"))) {
      O->noDefaultPaths = 1;
      return 1;
    }
    else if (!(strcmp(s, "help"))) {
      longusage(csound);
      csound->LongJmp(csound, 0);
    }
#ifdef ENABLE_NEW_PARSER
    else if (!(strcmp(s, "new-parser"))) {
      O->newParser = 1;             /* Use New Parser */
      return 1;
    }
#endif

    csoundErrorMsg(csound, Str("unknown long option: '--%s'"), s);
    return 0;
}

int argdecode(CSOUND *csound, int argc, char **argv_)
{
  OPARMS  *O = csound->oparms;
  char    *s, **argv;
  int     n;
  char    c;

  /* make a copy of the option list */
  {
    char  *p1, *p2;
    int   nbytes, i;
    /* calculate the number of bytes to allocate */
    /* N.B. the argc value passed to argdecode is decremented by one */
    nbytes = (argc + 1) * (int) sizeof(char*);
    for (i = 0; i <= argc; i++)
      nbytes += ((int) strlen(argv_[i]) + 1);
    p1 = (char*) mmalloc(csound, nbytes);   /* will be freed by memRESET() */
    p2 = (char*) p1 + ((int) sizeof(char*) * (argc + 1));
    argv = (char**) p1;
    for (i = 0; i <= argc; i++) {
      argv[i] = p2;
      strcpy(p2, argv_[i]);
      p2 = (char*) p2 + ((int) strlen(argv_[i]) + 1);
    }
  }
  csound->keep_tmp = 0;

  do {
    s = *++argv;
    if (*s++ == '-') {                  /* read all flags:  */
      while ((c = *s++) != '\0') {
        switch (c) {
        case 'U':
          FIND(Str("no utility name"));
          {
            int retval = csoundRunUtility(csound, s, argc, argv);
            csound->LongJmp(csound, retval);
          }
          break;
        case 'C':
          O->usingcscore = 1;           /* use cscore processing  */
          break;
        case 'I':
          csound->initonly = 1;         /* I-only implies */
        case 'n':
          O->sfwrite = 0;               /* nosound        */
          break;
        case 'i':
          FIND(Str("no infilename"));
          O->infilename = s;            /* soundin name */
          s += (int) strlen(s);
          if (strcmp(O->infilename, "stdout") == 0)
            csoundDie(csound, Str("input cannot be stdout"));
          if (strcmp(O->infilename, "stdin") == 0) {
            set_stdin_assign(csound, STDINASSIGN_SNDFILE, 1);
#if defined(WIN32) || defined(mac_classic)
            csoundDie(csound, Str("stdin audio not supported"));
#endif
          }
          else
            set_stdin_assign(csound, STDINASSIGN_SNDFILE, 0);
          O->sfread = 1;
          break;
        case 'o':
          FIND(Str("no outfilename"));
          O->outfilename = s;           /* soundout name */
          s += (int) strlen(s);
          if (strcmp(O->outfilename, "stdin") == 0)
            dieu(csound, Str("-o cannot be stdin"));
          if (strcmp(O->outfilename, "stdout") == 0) {
            set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 1);
#if defined(WIN32) || defined(mac_classic)
            csoundDie(csound, Str("stdout audio not supported"));
#endif
          }
          else
            set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 0);
          O->sfwrite = 1;
          break;
        case 'b':
          FIND(Str("no iobufsamps"));
          sscanf(s, "%d%n", &(O->outbufsamps), &n);
          /* defaults in musmon.c */
          O->inbufsamps = O->outbufsamps;
          s += n;
          break;
        case 'B':
          FIND(Str("no hardware bufsamps"));
          sscanf(s, "%d%n", &(O->oMaxLag), &n);
          /* defaults in rtaudio.c */
          s += n;
          break;
        case 'A':
          O->filetyp = TYP_AIFF;        /* AIFF output request*/
          break;
        case 'J':
          O->filetyp = TYP_IRCAM;       /* IRCAM output request */
          break;
        case 'W':
          O->filetyp = TYP_WAV;         /* WAV output request */
          break;
        case 'h':
          O->filetyp = TYP_RAW;         /* RAW output request */
          break;
        case 'c':
        case 'a':
        case 'u':
        case '8':
        case 's':
        case '3':
        case 'l':
        case 'f':
          set_output_format(O, c);
          break;
        case 'r':
          FIND(Str("no sample rate"));
          sscanf(s, "%f", &(O->sr_override));
          while (*++s);
          break;
        case 'j':
          FIND("");
          while (*++s);
          break;
        case 'k':
          FIND(Str("no control rate"));
          sscanf(s, "%f",  &(O->kr_override));
          while (*++s);
          break;
        case 'v':
          O->odebug = 1;                /* verbose otran  */
          break;
        case 'm':
          FIND(Str("no message level"));
#ifdef useoctal
          if (*s=='0') sscanf(s, "%o%n", &(O->msglevel), &n);
          else
#endif
            sscanf(s, "%d%n", &(O->msglevel), &n);
          s += n;
          break;
        case 'd':
          O->displays = 0;              /* no func displays */
          break;
        case 'g':
          O->graphsoff = 1;             /* don't use graphics */
          break;
        case 'G':
          O->postscript = 1;            /* Postscript graphics*/
          break;
        case 'x':
          FIND(Str("no xfilename"));
          csound->xfilename = s;        /* extractfile name */
          while (*++s);
          break;
        case 't':
          FIND(Str("no tempo value"));
          {
            int val;
            sscanf(s, "%d%n", &val, &n); /* use this tempo .. */
            s += n;
            if (val < 0) dieu(csound, Str("illegal tempo"));
            else if (val == 0) {
              csound->keep_tmp = 1;
              break;
            }
            else O->cmdTempo = val;
            O->Beatmode = 1;            /* on uninterpreted Beats */
          }
          break;
        case 'L':
          FIND(Str("no Linein score device_name"));
          O->Linename = s;              /* Linein device name */
          s += (int) strlen(s);
          if (!strcmp(O->Linename, "stdin")) {
            set_stdin_assign(csound, STDINASSIGN_LINEIN, 1);
#if defined(mac_classic)
            csoundDie(csound, Str("-L: stdin not supported on this platform"));
#endif
          }
          else
            set_stdin_assign(csound, STDINASSIGN_LINEIN, 0);
          O->Linein = 1;
          break;
        case 'M':
          FIND(Str("no midi device_name"));
          O->Midiname = s;              /* Midi device name */
          s += (int) strlen(s);
          if (!strcmp(O->Midiname, "stdin")) {
            set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 1);
#if defined(WIN32) || defined(mac_classic)
            csoundDie(csound, Str("-M: stdin not supported on this platform"));
#endif
          }
          else
            set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 0);
          O->Midiin = 1;
          break;
        case 'F':
          FIND(Str("no midifile name"));
          O->FMidiname = s;             /* Midifile name */
          s += (int) strlen(s);
          if (!strcmp(O->FMidiname, "stdin")) {
            set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 1);
#if defined(WIN32) || defined(mac_classic)
            csoundDie(csound, Str("-F: stdin not supported on this platform"));
#endif
          }
          else
            set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 0);
          O->FMidiin = 1;               /*****************/
          break;
        case 'Q':
          FIND(Str("no MIDI output device"));
          O->Midioutname = s;
          s += (int) strlen(s);
          break;
        case 'R':
          O->rewrt_hdr = 1;
          break;
        case 'H':
          if (isdigit(*s)) {
            sscanf(s, "%d%n", &(O->heartbeat), &n);
            s += n;
          }
          else O->heartbeat = 1;
          break;
        case 'N':
          O->ringbell = 1;              /* notify on completion */
          break;
        case 'T':
          O->termifend = 1;             /* terminate on midifile end */
          break;
        case 'D':
          O->gen01defer = 1;            /* defer GEN01 sample loads
                                           until performance time */
          break;
        case 'K':
          csound->peakchunks = 0;       /* Do not write peak information */
          break;
        case 'z':
          {
            int full = 0;
            if (*s != '\0') {
              if (isdigit(*s)) full = *s++ - '0';
            }
            csoundLoadExternals(csound);
            if (csoundInitModules(csound) != 0)
              csound->LongJmp(csound, 1);
            list_opcodes(csound, full);
          }
          csound->LongJmp(csound, 0);
          break;
        case 'Z':
          csound->dither_output = 1;
          break;
        case '@':
          FIND(Str("No indirection file"));
          {
            FILE *ind;
            void *fd;
            fd = csound->FileOpen(csound, &ind, CSFILE_STD, s, "rb", NULL);
            if (fd == NULL) {
              dieu(csound, Str("Cannot open indirection file %s\n"), s);
            }
            else {
              readOptions(csound, ind);
              csound->FileClose(csound, fd);
            }
            while (*s++); s--;
          }
          break;
        case 'O':
          FIND(Str("no log file"));
          while (*s++); s--;
          break;
        case '-':
#if defined(LINUX)
          if (!(strcmp (s, "sched"))) {             /* ignore --sched */
            while (*(++s));
            break;
          }
          if (!(strncmp(s, "sched=", 6))) {
            while (*(++s));
            break;
          }
#endif
          if (!decode_long(csound, s, argc, argv))
            csound->LongJmp(csound, 1);
          while (*(++s));
          break;
        case '+':                                   /* IV - Feb 01 2005 */
          if (parse_option_as_cfgvar(csound, (char*) s - 2) != 0)
            csound->LongJmp(csound, 1);
          while (*(++s));
          break;
        default:
          dieu(csound, Str("unknown flag -%c"), c);
        }
      }
    }
    else {
      /* 0: normal, 1: ignore, 2: fail */
      if (csound->orcname_mode == 2) {
        csound->Die(csound, Str("error: orchestra and score name not "
                                "allowed in .csoundrc"));
      }
      else if (csound->orcname_mode == 0) {
        if (csound->orchname == NULL)
          csound->orchname = --s;
        else if (csound->scorename == NULL)
          csound->scorename = --s;
        else {
          csound->Message(csound,"argc=%d Additional string \"%s\"\n",argc,--s);
          dieu(csound, Str("too many arguments"));
        }
      }
    }
  } while (--argc);
  return 1;
}

