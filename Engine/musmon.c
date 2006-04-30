/*
    musmon.c:

    Copyright (C) 1991,2002 Barry Vercoe, John ffitch,
                            Istvan Varga, rasmus ekman

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

#include "csoundCore.h"         /*                         MUSMON.C     */
#include "cscore.h"
#include "midiops.h"
#include "soundio.h"
#include "namedins.h"
#include "oload.h"
#include <math.h>

#define SEGAMPS 01
#define SORMSG  02

extern  int     MIDIinsert(CSOUND *, int, MCHNBLK*, MEVENT*);
extern  int     insert(CSOUND *, int, EVTBLK*);
extern  void    MidiOpen(CSOUND *);
extern  void    m_chn_init_all(CSOUND *);
extern  void    scsort(CSOUND *, FILE *, FILE *);
extern  void    infoff(CSOUND*, MYFLT), orcompact(CSOUND*);
extern  void    beatexpire(CSOUND *, double), timexpire(CSOUND *, double);
extern  void    sfopenin(CSOUND *), sfopenout(CSOUND*), sfnopenout(CSOUND*);
extern  void    iotranset(CSOUND *), sfclosein(CSOUND*), sfcloseout(CSOUND*);
extern  void    MidiClose(CSOUND *);
extern  void    RTclose(CSOUND *);
extern  char    **csoundGetSearchPathFromEnv(CSOUND *, const char *);

typedef struct evt_cb_func {
    void    (*func)(CSOUND *, void *);
    void    *userData;
    struct evt_cb_func  *nxt;
} EVT_CB_FUNC;

typedef struct {
    long    srngcnt[MAXCHNLS], orngcnt[MAXCHNLS];
    short   srngflg;
    short   sectno;
    int     lplayed;
    int     segamps, sormsg;
    EVENT   **ep, **epend;      /* pointers for stepping through lplay list */
    EVENT   *lsect;
} MUSMON_GLOBALS;

#define ST(x)   (((MUSMON_GLOBALS*) csound->musmonGlobals)->x)

/* IV - Jan 28 2005 */
void print_benchmark_info(CSOUND *csound, const char *s)
{
    double  rt, ct;

    if ((csound->oparms->msglevel & 0x80) == 0 || csound->csRtClock == NULL)
      return;
    rt = csoundGetRealTime(csound->csRtClock);
    ct = csoundGetCPUTime(csound->csRtClock);
    csound->Message(csound,
                    Str("Elapsed time at %s: real: %.3fs, CPU: %.3fs\n"),
                    (char*) s, rt, ct);
}

static void settempo(CSOUND *csound, MYFLT tempo)
{
    if (tempo <= FL(0.0))
      return;
    csound->beatTime = 60.0 / (double) tempo;
    csound->curBeat_inc = (double) tempo / (60.0 * (double) csound->global_ekr);
}

int gettempo(CSOUND *csound, GTEMPO *p)
{
    if (csound->oparms->Beatmode)
      *p->ans = (MYFLT) (60.0 / csound->beatTime);
    else
      *p->ans = FL(60.0);
    return OK;
}

int tempset(CSOUND *csound, TEMPO *p)
{
    MYFLT tempo;

    if ((tempo = *p->istartempo) <= FL(0.0)) {
      return csound->InitError(csound, Str("illegal istartempo value"));
    }
    settempo(csound, tempo);
    p->prvtempo = tempo;
    return OK;
}

int tempo(CSOUND *csound, TEMPO *p)
{
    if (*p->ktempo != p->prvtempo) {
      settempo(csound, *p->ktempo);
      p->prvtempo = *p->ktempo;
    }
    return OK;
}

static void print_maxamp(CSOUND *csound, MYFLT x)
{
    if (!(csound->oparms->msglevel & 0x60)) {   /* 0x00: raw amplitudes */
      if (csound->e0dbfs > FL(3000.0))
        csound->Message(csound, "%9.1f", x);
      else if (csound->e0dbfs < FL(3.0))
        csound->Message(csound, "%9.5f", x);
      else if (csound->e0dbfs > FL(300.0))
        csound->Message(csound, "%9.2f", x);
      else if (csound->e0dbfs > FL(30.0))
        csound->Message(csound, "%9.3f", x);
      else
        csound->Message(csound, "%9.4f", x);
    }
    else {                              /* dB values */
      int   attr = 0;
      MYFLT y = x / csound->e0dbfs;     /* relative level */
      if (y < FL(1.0e-10)) {
        /* less than -200 dB: print zero */
        csound->Message(csound, "      0  ");
        return;
      }
      y = FL(20.0) * (MYFLT) log10((double) y);
      if (csound->oparms->msglevel & 0x40) {
        if (y >= FL(0.0))                               /* >= 0 dB: red */
          attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_RED;
        else if (csound->oparms->msglevel & 0x20) {
          if (y >= FL(-6.0))                            /* -6..0 dB: yellow */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_YELLOW;
          else if (y >= FL(-24.0))                      /* -24..-6 dB: green */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_GREEN;
          else                                          /* -200..-24 dB: blue */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_BLUE;
        }
      }
      csound->MessageS(csound, attr, "%+9.2f", y);
    }
}

int musmon(CSOUND *csound)
{
    OPARMS  *O = csound->oparms;

#ifdef USE_DOUBLE
#ifdef BETA
    csound->Message(csound, Str("Csound version %s beta (double samples) %s\n"),
                            CS_PACKAGE_VERSION, __DATE__);
#else
    csound->Message(csound, Str("Csound version %s (double samples) %s\n"),
                            CS_PACKAGE_VERSION, __DATE__);
#endif
#else
#ifdef BETA
    csound->Message(csound, Str("Csound version %s beta (float samples) %s\n"),
                            CS_PACKAGE_VERSION, __DATE__);
#else
    csound->Message(csound, Str("Csound version %s (float samples) %s\n"),
                            CS_PACKAGE_VERSION, __DATE__);
#endif
#endif

    if (csound->musmonGlobals == NULL)
      csound->musmonGlobals = csound->Calloc(csound, sizeof(MUSMON_GLOBALS));
    /* initialise search path cache */
    csoundGetSearchPathFromEnv(csound, "SNAPDIR");
    csoundGetSearchPathFromEnv(csound, "SFDIR;SSDIR;INCDIR");
    csoundGetSearchPathFromEnv(csound, "SFDIR");
    csoundGetSearchPathFromEnv(csound, "SADIR");
    csoundGetSearchPathFromEnv(csound, "SFDIR;SSDIR");

    m_chn_init_all(csound);     /* allocate MIDI channels */
    dispinit(csound);           /* initialise graphics or character display */
    oload(csound);              /* set globals and run inits */

    /* kperf() will not call csoundYield() more than 250 times per second */
    csound->evt_poll_cnt = 0;
    csound->evt_poll_maxcnt = (int) ((double) csound->ekr / 250.0);
    /* Enable musmon to handle external MIDI input, if it has been enabled. */
    if (O->Midiin || O->FMidiin) {
      O->RTevents = 1;
      MidiOpen(csound);                 /*   alloc bufs & open files    */
    }
    csound->Message(csound, Str("orch now loaded\n"));

    csound->multichan = (csound->nchnls > 1 ? 1 : 0);
    ST(segamps) = O->msglevel & SEGAMPS;
    ST(sormsg) = O->msglevel & SORMSG;

    if (O->Linein)
      RTLineset(csound);                /* if realtime input expected   */

    if (csound->enableHostImplementedAudioIO &&
        csound->hostRequestedBufferSize) {
      int bufsize = (int) csound->hostRequestedBufferSize;
      int ksmps = (int) csound->ksmps;
      bufsize = (bufsize + (ksmps >> 1)) / ksmps;
      bufsize = (bufsize ? bufsize * ksmps : ksmps);
      O->outbufsamps = O->inbufsamps = bufsize;
    }
    else {
      if (!O->oMaxLag)
        O->oMaxLag = IODACSAMPS;
      if (!O->outbufsamps)
        O->outbufsamps = IOBUFSAMPS;
      else if (O->outbufsamps < 0) {    /* if k-aligned iobufs requested  */
        /* set from absolute value */
        O->outbufsamps *= -(csound->ksmps);
        csound->Message(csound, Str("k-period aligned audio buffering\n"));
        if (O->oMaxLag <= O->outbufsamps)
          O->oMaxLag = O->outbufsamps << 1;
      }
      /* else keep the user values */
      /* IV - Feb 04 2005: make sure that buffer sizes for real time audio */
      /* are usable */
      if (check_rtaudio_name(O->infilename, NULL, 0) >= 0 ||
          check_rtaudio_name(O->outfilename, NULL, 1) >= 0) {
        O->oMaxLag = ((O->oMaxLag + O->outbufsamps - 1) / O->outbufsamps)
                     * O->outbufsamps;
        if (O->oMaxLag <= O->outbufsamps && O->outbufsamps > 1)
          O->outbufsamps >>= 1;
      }
      O->inbufsamps = O->outbufsamps;
    }
    csound->Message(csound, Str("audio buffered in %d sample-frame blocks\n"),
                            (int) O->outbufsamps);
    O->inbufsamps *= csound->nchnls;    /* now adjusted for n channels  */
    O->outbufsamps *= csound->nchnls;
    iotranset(csound);          /* point recv & tran to audio formatter */
      /* open audio file or device for input first, and then for output */
    if (!csound->enableHostImplementedAudioIO) {
      if (O->sfread)
        sfopenin(csound);
      if (O->sfwrite && !csound->initonly)
        sfopenout(csound);
      else
        sfnopenout(csound);
    }

    if (!(csound->scfp = fopen(O->playscore, "r"))) {
      if (!O->Linein) {
        csoundDie(csound, Str("cannot reopen %s"), O->playscore);
      }
    }
    if (O->usingcscore) {
      if (ST(lsect) == NULL) {
        ST(lsect) = (EVENT*) mmalloc(csound, sizeof(EVENT));
        ST(lsect)->op = 'l';
      }
      csound->Message(csound, Str("using Cscore processing\n"));
      /* override stdout in */
      if (!(csound->oscfp = fopen("cscore.out", "w")))
        csoundDie(csound, Str("cannot create cscore.out"));
      /* rdscor for cscorefns */
      csoundInitializeCscore(csound, csound->scfp, csound->oscfp);
      /* call cscore, optionally re-enter via lplay() */
      csound->cscoreCallback_(csound);
      fclose(csound->oscfp); csound->oscfp = NULL;
      fclose(csound->scfp); csound->scfp = NULL;
      if (ST(lplayed))
        return 0;

      if (!(csound->scfp = fopen("cscore.out", "r"))) /*  rd from cscore.out */
        csoundDie(csound, Str("cannot reopen cscore.out"));
      if (!(csound->oscfp = fopen("cscore.srt", "w"))) /* writ to cscore.srt */
        csoundDie(csound, Str("cannot reopen cscore.srt"));
      csound->Message(csound, Str("sorting cscore.out ..\n"));
      scsort(csound, csound->scfp, csound->oscfp);  /* call the sorter again */
      fclose(csound->scfp); csound->scfp = NULL;
      fclose(csound->oscfp); csound->oscfp = NULL;
      csound->Message(csound, Str("\t... done\n"));
      if (!(csound->scfp = fopen("cscore.srt", "r"))) /*  rd from cscore.srt */
        csoundDie(csound, Str("cannot reopen cscore.srt"));
      csound->Message(csound, Str("playing from cscore.srt\n"));
      O->usingcscore = 0;
    }

    csound->Message(csound, Str("SECTION %d:\n"), ++ST(sectno));
    /* apply score offset if non-zero */
    if (csound->csoundScoreOffsetSeconds_ > FL(0.0))
      csound->SetScoreOffsetSeconds(csound, csound->csoundScoreOffsetSeconds_);

    /* since we are running in components, we exit here to playevents later */
    return 0;
}

static void deactivate_all_notes(CSOUND *csound)
{
    INSDS *ip = csound->actanchor.nxtact;

    while (ip != NULL) {
      INSDS *nxt = ip->nxtact;
      xturnoff_now(csound, ip);
      ip = nxt;
    }
}

static void delete_pending_rt_events(CSOUND *csound)
{
    EVTNODE *ep = csound->OrcTrigEvts;

    while (ep != NULL) {
      EVTNODE *nxt = ep->nxt;
      if (ep->evt.strarg != NULL) {
        free(ep->evt.strarg);
        ep->evt.strarg = NULL;
      }
      /* push to stack of free event nodes */
      ep->nxt = csound->freeEvtNodes;
      csound->freeEvtNodes = ep;
      ep = nxt;
    }
    csound->OrcTrigEvts = NULL;
}

PUBLIC int csoundCleanup(CSOUND *csound)
{
    void    *p;
    MYFLT   *maxp;
    long    *rngp;
    int     n;

    while (csound->evtFuncChain != NULL) {
      p = (void*) csound->evtFuncChain;
      csound->evtFuncChain = ((EVT_CB_FUNC*) p)->nxt;
      free(p);
    }

    /* check if we have already cleaned up */
    if (!(csound->engineState & CS_STATE_CLN))
      return 0;
    /* will not clean up more than once */
    csound->engineState &= ~(CS_STATE_CLN);

    deactivate_all_notes(csound);
    if (csound->instrtxtp &&
        csound->instrtxtp[0] &&
        csound->instrtxtp[0]->instance &&
        csound->instrtxtp[0]->instance->actflg)
      xturnoff_now(csound, csound->instrtxtp[0]->instance);
    delete_pending_rt_events(csound);
    while (csound->freeEvtNodes != NULL) {
      p = (void*) csound->freeEvtNodes;
      csound->freeEvtNodes = ((EVTNODE*) p)->nxt;
      free(p);
    }
    orcompact(csound);
    if (csound->scfp) {
      fclose(csound->scfp); csound->scfp = NULL;
    }

    /* print stats only if musmon was actually run */
    if (csound->musmonGlobals != NULL) {
      csound->Message(csound, Str("end of score.\t\t   overall amps:"));
      for (n = 0; n < csound->nchnls; n++) {
        if (csound->smaxamp[n] > csound->omaxamp[n])
          csound->omaxamp[n] = csound->smaxamp[n];
        if (csound->maxamp[n] > csound->omaxamp[n])
          csound->omaxamp[n] = csound->maxamp[n];
        ST(orngcnt)[n] += (ST(srngcnt)[n] + csound->rngcnt[n]);
      }
      for (maxp = csound->omaxamp, n = csound->nchnls; n--; )
        print_maxamp(csound, *maxp++);
      if (csound->oparms->outformat != AE_FLOAT) {
        csound->Message(csound, Str("\n\t   overall samples out of range:"));
        for (rngp = ST(orngcnt), n = csound->nchnls; n--; )
          csound->Message(csound, "%9ld", *rngp++);
      }
      csound->Message(csound, Str("\n%d errors in performance\n"),
                              csound->perferrcnt);
      print_benchmark_info(csound, Str("end of performance"));
    }
    /* close line input (-L) */
    RTclose(csound);
    /* close MIDI input */
    MidiClose(csound);
    /* IV - Feb 03 2005: do not need to call rtclose from here, */
    /* as sfclosein/sfcloseout will do that. */
    if (!csound->enableHostImplementedAudioIO) {
      sfclosein(csound);
      sfcloseout(csound);
      if (!csound->oparms->sfwrite)
        csound->Message(csound, Str("no sound written to disk\n"));
    }
    if (csound->oparms->ringbell)
      cs_beep(csound);
    return dispexit(csound);    /* hold or terminate the display output     */
}

void cs_beep(CSOUND *csound)
{
#ifdef mac_classic
    SysBeep(30L);
#else
    csound->Message(csound, Str("%c\tbeep!\n"), '\007');
#endif
}

int lplay(CSOUND *csound, EVLIST *a)    /* cscore re-entry into musmon */
{
    if (csound->musmonGlobals == NULL)
      csound->musmonGlobals = csound->Calloc(csound, sizeof(MUSMON_GLOBALS));
    ST(lplayed) = 1;
    if (!ST(sectno))
      csound->Message(csound, Str("SECTION %d:\n"), ++ST(sectno));
    ST(ep) = &a->e[1];                  /* from 1st evlist member */
    ST(epend) = ST(ep) + a->nevents;    /*   to last              */
    while (csoundPerform(csound) == 0)  /* play list members      */
      ;
    return OK;
}

/* make list to turn on instrs for indef */
/* perf called from i0 for execution in playevents */

int turnon(CSOUND *csound, TURNON *p)
{
    EVTBLK  evt;
    int     isNamedInstr;

    evt.strarg = NULL;
    evt.opcod = 'i';
    evt.pcnt = 3;
    isNamedInstr = (int) csound->GetInputArgSMask(p);
    if (isNamedInstr) {
      long  insno = csound->strarg2insno(csound, p->insno, isNamedInstr);
      if (insno <= 0L)
        return NOTOK;
      evt.p[1] = (MYFLT) insno;
    }
    else
      evt.p[1] = *p->insno;
    evt.p[2] = *p->itime;
    evt.p[3] = FL(-1.0);

    return insert_score_event(csound, &evt, csound->curTime);
}

/* Print current amplitude values, and update section amps. */

static void print_amp_values(CSOUND *csound, int score_evt)
{
    CSOUND        *p = csound;
    MYFLT         *maxp, *smaxp;
    unsigned long *maxps, *smaxps;
    long          *rngp, *srngp;
    int           n;

    if (ST(segamps) || (p->rngflg && ST(sormsg))) {
      if (score_evt)
        p->Message(p, "B%7.3f ..%7.3f T%7.3f TT%7.3f M:",
                      p->prvbt - p->beatOffs,  p->curbt - p->beatOffs,
                      p->curp2 - p->timeOffs,  p->curp2);
      else
        p->Message(p, "  rtevent:\t   T%7.3f TT%7.3f M:",
                      p->curp2 - p->timeOffs,  p->curp2);
      for (n = p->nchnls, maxp = p->maxamp; n--; )
        print_maxamp(p, *maxp++);               /* IV - Jul 9 2002 */
      p->Message(p, "\n");
      if (p->rngflg) {
        p->Message(p, Str("\t number of samples out of range:"));
        for (n = p->nchnls, rngp = p->rngcnt; n--; )
          p->Message(p, "%9ld", *rngp++);
        p->Message(p, "\n");
      }
    }
    if (p->rngflg) {
      p->rngflg = 0;
      ST(srngflg)++;
    }
    for (n = p->nchnls,
         maxp = p->maxamp - 1, smaxp = p->smaxamp - 1,
         maxps = p->maxpos - 1, smaxps = p->smaxpos - 1,
         rngp = p->rngcnt, srngp = ST(srngcnt); n--; ) {
      ++maxps; ++smaxps;
      if (*++maxp > *++smaxp) {
        *smaxp = *maxp;
        *smaxps = *maxps;
      }
      *maxp = FL(0.0);
      *maxps = 0;
      *srngp++ += *rngp;
      *rngp++ = 0;
    }
}

/* Update overall amplitudes from section values, */
/* and optionally print message (1: section end, 2: lplay end). */

static void section_amps(CSOUND *csound, int enable_msgs)
{
    CSOUND        *p = csound;
    MYFLT         *maxp, *smaxp;
    unsigned long *maxps, *smaxps;
    long          *rngp, *srngp;
    int           n;

    if (enable_msgs) {
      if (enable_msgs == 1)
        p->Message(p, Str("end of section %d\t sect peak amps:"), ST(sectno));
      else if (enable_msgs == 2)
        p->Message(p, Str("end of lplay event list\t      peak amps:"));
      for (n = p->nchnls, maxp = p->smaxamp; n--; )
        print_maxamp(p, *maxp++);               /* IV - Jul 9 2002 */
      p->Message(p, "\n");
      if (ST(srngflg)) {
        p->Message(p, Str("\t number of samples out of range:"));
        for (n = p->nchnls, srngp = ST(srngcnt); n--; )
          p->Message(p, "%9ld", *srngp++);
        p->Message(p, "\n");
      }
    }
    ST(srngflg) = 0;
    for (n = p->nchnls,
         smaxp = p->smaxamp - 1, maxp = p->omaxamp - 1,
         smaxps = p->smaxpos - 1, maxps = p->omaxpos - 1,
         srngp = ST(srngcnt), rngp = ST(orngcnt); n--; ) {
      ++maxps; ++smaxps;
      if (*++smaxp > *++maxp) {
        *maxp = *smaxp;                 /* keep ovrl maxamps */
        *maxps = *smaxps;               /* And where */
      }
      *smaxp = FL(0.0);
      *smaxps = 0;
      *rngp++ += *srngp;                /*   and orng counts */
      *srngp++ = 0;
    }
}

static CS_NOINLINE void printScoreError(CSOUND *p, int rtEvt,
                                        const char *fmt, ...)
{
    va_list args;

    if (rtEvt)
      p->Message(p, "\t\t   T%7.3f", p->curp2 - p->timeOffs);
    else
      p->Message(p, "\t  B%7.3f", p->curbt - p->beatOffs);
    va_start(args, fmt);
    p->ErrMsgV(p, NULL, fmt, args);
    va_end(args);
    p->perferrcnt++;
}

static int process_score_event(CSOUND *p, EVTBLK *evt, int rtEvt)
{
    EVTBLK  *saved_currevent;
    int     insno, n;

    saved_currevent = p->currevent;
    p->currevent = evt;
    switch (evt->opcod) {                       /* scorevt or Linevt:     */
    case 'e':           /* quit realtime */
    case 'l':
    case 's':
      while (p->frstoff != NULL) {
        INSDS *nxt = p->frstoff->nxtoff;
        xturnoff_now(p, p->frstoff);
        p->frstoff = nxt;
      }
      p->currevent = saved_currevent;
      return (evt->opcod == 'l' ? 3 : (evt->opcod == 's' ? 1 : 2));
    case 'q':
      if (evt->p[1] == SSTRCOD && evt->strarg) {    /* IV - Oct 31 2002 */
        if ((insno = (int) named_instr_find(p, evt->strarg)) < 1) {
          printScoreError(p, rtEvt, Str(" - note deleted. instr %s undefined"),
                                    evt->strarg);
          break;
        }
        p->Message(p, Str("Setting instrument %s %s\n"),
                      evt->strarg, (evt->p[3] == 0 ? Str("off") : Str("on")));
        p->instrtxtp[insno]->muted = (short) evt->p[3];
      }
      else {                                        /* IV - Oct 31 2002 */
        insno = abs((int) evt->p[1]);
        if ((unsigned int) (insno - 1) >= (unsigned int) p->maxinsno ||
            p->instrtxtp[insno] == NULL) {
          printScoreError(p, rtEvt,
                          Str(" - note deleted. instr %d(%d) undefined"),
                          insno, p->maxinsno);
          break;
        }
        p->Message(p, Str("Setting instrument %d %s\n"),
                      insno, (evt->p[3] == 0 ? Str("off") : (Str("on"))));
        p->instrtxtp[insno]->muted = (short) evt->p[3];
      }
      break;
    case 'i':
      if (evt->p[1] == SSTRCOD && evt->strarg) {    /* IV - Oct 31 2002 */
        if ((insno = (int) named_instr_find(p, evt->strarg)) < 1) {
          printScoreError(p, rtEvt, Str(" - note deleted. instr %s undefined"),
                                    evt->strarg);
          break;
        }
        evt->p[1] = (MYFLT) insno;
        if (p->oparms->Beatmode && !rtEvt && evt->p3orig > FL(0.0))
          evt->p[3] = evt->p3orig * (MYFLT) p->beatTime;
        if ((n = insert(p, insno, evt))) {      /* else alloc, init, activate */
          printScoreError(p, rtEvt,
                          Str(" - note deleted.  i%d (%s) had %d init errors"),
                          insno, evt->strarg, n);
        }
      }
      else {                                        /* IV - Oct 31 2002 */
        insno = abs((int) evt->p[1]);
        if ((unsigned int) (insno - 1) >= (unsigned int) p->maxinsno ||
            p->instrtxtp[insno] == NULL) {
          printScoreError(p, rtEvt,
                          Str(" - note deleted. instr %d(%d) undefined"),
                          insno, p->maxinsno);
        }
        else if (evt->p[1] < FL(0.0))           /* if p1 neg,             */
          infoff(p, -evt->p[1]);                /*  turnoff any infin cpy */
        else {
          if (p->oparms->Beatmode && !rtEvt && evt->p3orig > FL(0.0))
            evt->p[3] = evt->p3orig * (MYFLT) p->beatTime;
          if ((n = insert(p, insno, evt))) {    /* else alloc, init, activate */
            printScoreError(p, rtEvt,
                            Str(" - note deleted.  i%d had %d init errors"),
                            insno, n);
          }
        }
      }
      break;
    case 'f':
      {
        FUNC  *dummyftp;
        p->hfgens(p, &dummyftp, evt, 0);
      }
      break;
    case 'a':
      {
        int kCnt;
        kCnt = (int) ((double) p->global_ekr * (double) evt->p[3] + 0.5);
        if (kCnt > p->advanceCnt) {
          p->advanceCnt = kCnt;
          p->Message(p, Str("time advanced %5.3f beats by score request\n"),
                        evt->p3orig);
        }
      }
      break;
    }
    p->currevent = saved_currevent;
    return 0;
}

static int process_rt_event(CSOUND *p, int sensType)
{
    EVTBLK  *evt;
    int     retval, insno, n;

    retval = 0;
    if (p->curp2 < p->curTime) {
      p->curp2 = p->curTime;
      print_amp_values(p, 0);
    }
    if (sensType == 4) {                  /* Realtime orc event   */
      EVTNODE *e = p->OrcTrigEvts;
      /* Events are sorted on insertion, so just check the first */
      evt = &(e->evt);
      /* pop from the list */
      p->OrcTrigEvts = e->nxt;
      retval = process_score_event(p, evt, 1);
      if (evt->strarg != NULL) {
        free(evt->strarg);
        evt->strarg = NULL;
      }
      /* push back to free alloc stack so it can be reused later */
      e->nxt = p->freeEvtNodes;
      p->freeEvtNodes = e;
    }
    else if (sensType == 2) {                           /* Midievent:    */
      MEVENT *mep;
      MCHNBLK *chn;
      /* realtime or Midifile  */
      mep = p->midiGlobals->Midevtblk;
      chn = p->m_chnbp[mep->chan];
      insno = chn->insno;
      if (mep->type == NOTEON_TYPE && mep->dat2) {      /* midi note ON: */
        if ((n = MIDIinsert(p, insno, chn, mep))) {     /* alloc,init,activ */
          p->Message(p, Str("\t\t   T%7.3f - note deleted. "), p->curp2);
          p->Message(p, Str("instr %d had %d init errors\n"), insno, n);
          p->perferrcnt++;
        }
      }
      else {                            /* else midi note OFF:    */
        INSDS *ip = chn->kinsptr[mep->dat1];
        if (ip == NULL) {               /*  if already off, done  */
          p->Mxtroffs++;
          return retval;
        }
        if (chn->sustaining) {          /*  if sustain pedal on   */
          while (ip != NULL && ip->m_sust)
            ip = ip->nxtolap;
          if (ip != NULL) {
            ip->m_sust = 1;             /*    let the note ring   */
            chn->ksuscnt++;
          }
          else
            p->Mxtroffs++;
        }
        else                            /*  else some kind of off */
          xturnoff(p, ip);
      }
    }
    return retval;
}

#define RNDINT(x) ((int) ((double) (x) + ((double) (x) < 0.0 ? -0.5 : 0.5)))

extern  int     sensMidi(CSOUND *);

/* sense events for one k-period            */
/* return value is one of the following:    */
/*   0: continue performance                */
/*   1: terminate (e.g. end of MIDI file)   */
/*   2: normal end of score                 */

int sensevents(CSOUND *csound)
{
    CSOUND *p = csound;
    EVTBLK  *e;
    OPARMS  *O = csound->oparms;
    int     retval, sensType;

    if (p->MTrkend && O->termifend) {   /* end of MIDI file:  */
      deactivate_all_notes(p);
      p->Message(p, Str("terminating.\n"));
      return 1;                         /* abort with perf incomplete */
    }
    /* if turnoffs pending, remove any expired instrs */
    if (p->frstoff != NULL) {
      double  tval;
      /* the following comparisons must match those in schedofftim() */
      if (O->Beatmode) {
        tval = p->curBeat + (0.51 * p->curBeat_inc);
        if (p->frstoff->offbet <= tval) beatexpire(p, tval);
      }
      else {
        tval = p->curTime + (0.51 * p->curTime_inc);
        if (p->frstoff->offtim <= tval) timexpire(p, tval);
      }
    }
    e = &(p->evt);
    if (--(p->cyclesRemaining) <= 0) {  /* if done performing score segment: */
      if (!p->cyclesRemaining) {
        p->prvbt = p->curbt;            /* update beats and times */
        p->curbt = p->nxtbt;
        p->curp2 = p->nxtim;
        print_amp_values(p, 1);         /* print amplitudes for this segment */
      }
      else                              /* this should only happen at */
        p->cyclesRemaining = 0;         /* beginning of performance   */
    }

 retest:
    while (p->cyclesRemaining <= 0) {   /* read each score event:     */
      if (e->opcod != '\0') {
        /* if there is a pending score event, handle it now */
        switch (e->opcod) {
          case 'e':                     /* end of score, */
          case 'l':                     /* lplay list,   */
          case 's':                     /* or section:   */
            if (p->frstoff != NULL) {         /* if still have notes with   */
              p->nxtim = p->frstoff->offtim;  /*  finite length, wait until */
              p->nxtbt = p->frstoff->offbet;  /*  all are turned off        */
              break;
            }
            /* end of: 1: section, 2: score, 3: lplay list */
            retval = (e->opcod == 'l' ? 3 : (e->opcod == 's' ? 1 : 2));
            goto scode;
          default:                            /* q, i, f, a:              */
            process_score_event(p, e, 0);     /*   handle event now       */
            e->opcod = '\0';                  /*   and get next one       */
            continue;
        }
      }
      else {
        /* else read next score event */
        if (O->usingcscore) {           /*    get next lplay event      */
          /* FIXME: this may be non-portable */
          if (ST(ep) < ST(epend))                       /* nxt event    */
            memcpy((void*) e, (void*) &((*ST(ep)++)->strarg), sizeof(EVTBLK));
          else                                          /* else lcode   */
            memcpy((void*) e, (void*) &(ST(lsect)->strarg), sizeof(EVTBLK));
        }
        else if (!(rdscor(p, e)))       /*   or rd nxt evt from scorfil */
          e->opcod = 'e';
        p->currevent = e;
        switch (e->opcod) {
        case 'w':
          if (!O->Beatmode)                   /* Not beatmode: read 'w' */
            settempo(p, e->p2orig);           /*   to init the tempo    */
          continue;                           /*   for this section     */
        case 'q':
        case 'i':
        case 'f':
        case 'a':
          p->nxtim = (double) e->p[2] + p->timeOffs;
          p->nxtbt = (double) e->p2orig + p->beatOffs;
          break;
        case 'e':
        case 'l':
        case 's':
          continue;
        default:
          p->Message(p, Str("error in score.  illegal opcode %c (ASCII %d)\n"),
                        e->opcod, e->opcod);
          p->perferrcnt++;
          continue;
        }
      }
      /* calculate the number of k-periods remaining until next event */
      if (O->Beatmode)
        p->cyclesRemaining = RNDINT((p->nxtbt - p->curBeat) / p->curBeat_inc);
      else
        p->cyclesRemaining = RNDINT((p->nxtim - p->curTime) / p->curTime_inc);
    }

    /* handle any real time events now: */
    /* FIXME: the initialisation pass of real time */
    /*   events is not sorted by instrument number */
    /*   (although it never was sorted anyway...)  */
    if (O->RTevents) {
      /* run all registered callback functions */
      if (p->evtFuncChain != NULL && !p->advanceCnt) {
        EVT_CB_FUNC *fp = (EVT_CB_FUNC*) p->evtFuncChain;
        do {
          fp->func(p, fp->userData);
          fp = fp->nxt;
        } while (fp != NULL);
      }
      /* check for pending real time events */
      while (p->OrcTrigEvts != NULL &&
             p->OrcTrigEvts->start_kcnt <= (unsigned long) p->global_kcounter) {
        if ((retval = process_rt_event(p, 4)) != 0)
          goto scode;
      }
      /* MIDI note messages */
      if (O->Midiin || O->FMidiin) {
        while ((sensType = sensMidi(p)) != 0) {
          if ((retval = process_rt_event(p, sensType)) != 0)
            goto scode;
        }
      }
    }

    /* no score event at this time, return to continue performance */
    return 0;

 scode:
    /* end of section (retval == 1), score (retval == 2), */
    /* or lplay list (retval == 3) */
    e->opcod = '\0';
    if (retval == 3) {
      section_amps(p, 2);
      return 1;
    }
    /* for s, or e after s */
    if (retval == 1 || (retval == 2 && ST(sectno) > 1)) {
      delete_pending_rt_events(p);
      if (O->Beatmode)
        p->curbt = p->curBeat;
      p->curp2 = p->nxtim = p->timeOffs = p->curTime;
      p->prvbt = p->nxtbt = p->beatOffs = p->curbt;
      section_amps(p, 1);
    }
    else
      section_amps(p, 0);
    if (retval == 1) {                        /* if s code,        */
      orcompact(p);                           /*   rtn inactiv spc */
      if (p->actanchor.nxtact == NULL)        /*   if no indef ins */
        rlsmemfiles(p);                       /*    purge memfiles */
      p->Message(p, Str("SECTION %d:\n"), ++ST(sectno));
      goto retest;                            /*   & back for more */
    }
    return 2;                   /* done with entire score */
}

static inline unsigned long time2kcnt(CSOUND *csound, double tval)
{
    if (tval > 0.0) {
      tval *= (double) csound->global_ekr;
#ifdef HAVE_C99
      return (unsigned long) llrint(tval);
#else
      return (unsigned long) (tval + 0.5);
#endif
    }
    return 0UL;
}

/* Schedule new score event to be played. 'time_ofs' is the amount of */
/* time in seconds to add to evt->p[2] to get the actual start time   */
/* of the event (measured from the beginning of performance, and not  */
/* section) in seconds.                                               */
/* Required parameters in 'evt':                                      */
/*   char   *strarg   string argument of event (NULL if none)         */
/*   char   opcod     event opcode (a, e, f, i, l, q, s)              */
/*   short  pcnt      number of p-fields (>=3 for q, i, a; >=4 for f) */
/*   MYFLT  p[]       array of p-fields, p[1]..p[pcnt] should be set  */
/*  p2orig and p3orig are calculated from p[2] and p[3].              */
/* The contents of 'evt', including the string argument, need not be  */
/* preserved after calling this function, as a copy of the event is   */
/* made.                                                              */
/* Return value is zero on success.                                   */

int insert_score_event(CSOUND *csound, EVTBLK *evt, double time_ofs)
{
    double        start_time;
    EVTNODE       *e, *prv;
    CSOUND        *st = csound;
    MYFLT         *p;
    unsigned long start_kcnt;
    int           i, retval;

    retval = -1;
    /* make a copy of the event... */
    if (csound->freeEvtNodes != NULL) {             /* pop alloc from stack */
      e = csound->freeEvtNodes;                     /*   if available       */
      csound->freeEvtNodes = e->nxt;
    }
    else {
      e = (EVTNODE*) calloc((size_t) 1, sizeof(EVTNODE)); /* or alloc new one */
      if (e == NULL)
        return CSOUND_MEMORY;
    }
    if (evt->strarg != NULL) {  /* copy string argument if present */
      e->evt.strarg = (char*) malloc((size_t) strlen(evt->strarg) + (size_t) 1);
      if (e->evt.strarg == NULL) {
        free(e);
        return CSOUND_MEMORY;
      }
      strcpy(e->evt.strarg, evt->strarg);
    }
    e->evt.opcod = evt->opcod;
    e->evt.pcnt = evt->pcnt;
    p = &(e->evt.p[0]);
    i = 0;
    while (++i <= evt->pcnt)    /* copy p-field list */
      p[i] = evt->p[i];
    /* ...and use the copy from now on */
    evt = &(e->evt);

    /* check for required p-fields */
    switch (evt->opcod) {
      case 'f':
        if (evt->pcnt < 4)
          goto pfld_err;
      case 'i':
      case 'q':
      case 'a':
        if (evt->pcnt < 3)
          goto pfld_err;
        /* calculate actual start time in seconds and k-periods */
        start_time = (double) p[2] + time_ofs;
        start_kcnt = time2kcnt(csound, start_time);
        /* correct p2 value for section offset */
        p[2] = (MYFLT) (start_time - st->timeOffs);
        if (p[2] < FL(0.0))
          p[2] = FL(0.0);
        /* start beat: this is possibly wrong */
        evt->p2orig = (MYFLT) (((start_time - st->curTime) / st->beatTime)
                               + (st->curBeat - st->beatOffs));
        if (evt->p2orig < FL(0.0))
          evt->p2orig = FL(0.0);
        evt->p3orig = p[3];
        break;
      default:
        start_kcnt = 0UL;   /* compiler only */
    }

    switch (evt->opcod) {
      case 'i':                         /* note event */
        /* calculate the length in beats */
        if (evt->p3orig > FL(0.0))
          evt->p3orig = (MYFLT) ((double) evt->p3orig / st->beatTime);
      case 'q':                         /* mute instrument */
        /* check for a valid instrument number or name */
        if (evt->strarg != NULL && p[1] == SSTRCOD)
          i = (int) named_instr_find(csound, evt->strarg);
        else
          i = (int) fabs((double) p[1]);
        if ((unsigned int) (i - 1) >= (unsigned int) csound->maxinsno ||
            csound->instrtxtp[i] == NULL) {
          csoundMessage(csound, Str("insert_score_event(): invalid instrument "
                                    "number or name\n"));
          goto err_return;
        }
        break;
      case 'a':                         /* advance score time */
        /* calculate the length in beats */
        evt->p3orig = (MYFLT) ((double) evt->p3orig / st->beatTime);
      case 'f':                         /* function table */
        break;
      case 'e':                         /* end of score, */
      case 'l':                         /*   lplay list, */
      case 's':                         /*   section:    */
        start_time = time_ofs;
        if (evt->pcnt >= 2)
          start_time += (double) p[2];
        evt->pcnt = 0;
        start_kcnt = time2kcnt(csound, start_time);
        break;
      default:
        csoundMessage(csound, Str("insert_score_event(): unknown opcode: %c\n"),
                              evt->opcod);
        goto err_return;
    }
    /* queue new event */
    e->start_kcnt = start_kcnt;
    prv = csound->OrcTrigEvts;
    /* if list is empty, or at beginning of list: */
    if (prv == NULL || start_kcnt < prv->start_kcnt) {
      e->nxt = prv;
      csound->OrcTrigEvts = e;
    }
    else {                                      /* otherwise sort by time */
      while (prv->nxt != NULL && start_kcnt >= prv->nxt->start_kcnt)
        prv = prv->nxt;
      e->nxt = prv->nxt;
      prv->nxt = e;
    }
    /* Make sure sensevents() looks for RT events */
    csound->oparms->RTevents = 1;
    return 0;

 pfld_err:
    csoundMessage(csound, Str("insert_score_event(): insufficient p-fields\n"));
 err_return:
    /* clean up */
    if (e->evt.strarg != NULL)
      free(e->evt.strarg);
    e->evt.strarg = NULL;
    e->nxt = csound->freeEvtNodes;
    csound->freeEvtNodes = e;
    return retval;
}

/* called by csoundRewindScore() to reset performance to time zero */

void musmon_rewind_score(CSOUND *csound)
{
    /* deactivate all currently playing notes */
    deactivate_all_notes(csound);
    /* flush any pending real time events */
    delete_pending_rt_events(csound);

    if (csound->global_kcounter != 0L) {
      /* reset score time */
      csound->global_kcounter = csound->kcounter = 0L;
      csound->nxtbt = csound->curbt = csound->prvbt = 0.0;
      csound->nxtim = csound->curp2 = 0.0;
      csound->beatOffs = csound->timeOffs = 0.0;
      csound->curBeat = csound->curTime = 0.0;
      csound->cyclesRemaining = 0;
      csound->evt.strarg = NULL;
      csound->evt.opcod = '\0';
      /* reset tempo */
      if (csound->oparms->Beatmode)
        settempo(csound, (MYFLT) csound->oparms->cmdTempo);
      else
        settempo(csound, FL(60.0));
      /* rewind score file */
      if (csound->scfp != NULL)
        fseek(csound->scfp, 0L, SEEK_SET);
      /* update section/overall amplitudes, reset to section 1 */
      section_amps(csound, 1);
      ST(sectno) = 1;
      csound->Message(csound, Str("SECTION %d:\n"), ST(sectno));
    }

    /* apply score offset if non-zero */
    csound->advanceCnt = 0;
    if (csound->csoundScoreOffsetSeconds_ > FL(0.0))
      csound->SetScoreOffsetSeconds(csound, csound->csoundScoreOffsetSeconds_);
}

/**
 * Register a function to be called once in every control period
 * by sensevents(). Any number of functions may be registered,
 * and will be called in the order of registration.
 * The callback function takes two arguments: the Csound instance
 * pointer, and the userData pointer as passed to this function.
 * Returns zero on success.
 */
PUBLIC int csoundRegisterSenseEventCallback(CSOUND *csound,
                                            void (*func)(CSOUND *, void *),
                                            void *userData)
{
    EVT_CB_FUNC *fp = (EVT_CB_FUNC*) csound->evtFuncChain;

    if (fp == NULL) {
      fp = (EVT_CB_FUNC*) calloc((size_t) 1, sizeof(EVT_CB_FUNC));
      csound->evtFuncChain = (void*) fp;
    }
    else {
      while (fp->nxt != NULL)
        fp = fp->nxt;
      fp->nxt = (EVT_CB_FUNC*) calloc((size_t) 1, sizeof(EVT_CB_FUNC));
      fp = fp->nxt;
    }
    if (fp == NULL)
      return CSOUND_MEMORY;
    fp->func = func;
    fp->userData = userData;
    fp->nxt = NULL;
    csound->oparms->RTevents = 1;

    return 0;
}

