/*
    otran.c:

    Copyright (C) 1991, 1997, 2003 Barry Vercoe, John ffitch, Istvan Varga

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

#include "csoundCore.h"         /*                      OTRAN.C         */
#include "oload.h"
#include <math.h>
#include "pstream.h"
#include "namedins.h"           /* IV - Oct 31 2002 */
#include <ctype.h>

typedef struct NAME_ {
    char          *namep;
    struct NAME_  *nxt;
    int           type, count;
} NAME;

typedef struct {
    NAME      *gblNames[256], *lclNames[256];   /* for 8 bit hash */
    ARGLST    *nullist;
    ARGOFFS   *nulloffs;
    int       lclkcnt, lcldcnt, lclwcnt, lclfixed;
    int       lclpcnt, lclscnt, lclacnt, lclnxtpcnt;
    int       lclnxtkcnt, lclnxtdcnt, lclnxtwcnt, lclnxtacnt, lclnxtscnt;
    int       gblnxtkcnt, gblnxtpcnt, gblnxtacnt, gblnxtscnt;
    int       gblfixed, gblkcount, gblacount, gblscount;
    int       *nxtargoffp, *argofflim, lclpmax;
    char      **strpool;
    long      poolcount, strpool_cnt, argoffsize;
    int       nconsts;
} OTRAN_GLOBALS;

static  int     gexist(CSOUND *, char *), gbloffndx(CSOUND *, char *);
static  int     lcloffndx(CSOUND *, char *);
static  int     constndx(CSOUND *, char *);
static  int     strconstndx(CSOUND *, const char *);
static  void    insprep(CSOUND *, INSTRTXT *);
static  void    lgbuild(CSOUND *, char *);
static  void    gblnamset(CSOUND *, char *);
static  int     plgndx(CSOUND *, char *);
static  NAME    *lclnamset(CSOUND *, char *);
        int     lgexist(CSOUND *, const char *);
static  void    delete_global_namepool(CSOUND *);
static  void    delete_local_namepool(CSOUND *);

extern  const   unsigned char   strhash_tabl_8[256];    /* namedins.c */

#define txtcpy(a,b) memcpy(a,b,sizeof(TEXT));
#define ST(x)   (((OTRAN_GLOBALS*) ((CSOUND*) csound)->otranGlobals)->x)

#define KTYPE   1
/* #define DTYPE   2 */
#define WTYPE   3
#define ATYPE   4
#define PTYPE   5
#define STYPE   6
/* NOTE: these assume that sizeof(MYFLT) is either 4 or 8 */
#define Dfloats (((int) sizeof(DOWNDAT) + 7) / (int) sizeof(MYFLT))
#define Wfloats (((int) sizeof(SPECDAT) + 7) / (int) sizeof(MYFLT))
#define Pfloats (((int) sizeof(PVSDAT) + 7) / (int) sizeof(MYFLT))

#ifdef FLOAT_COMPARE
#undef FLOAT_COMPARE
#endif
#ifdef USE_DOUBLE
#define FLOAT_COMPARE(x,y)  (fabs((double) (x) / (double) (y) - 1.0) > 1.0e-12)
#else
#define FLOAT_COMPARE(x,y)  (fabs((double) (x) / (double) (y) - 1.0) > 5.0e-7)
#endif

void tranRESET(CSOUND *csound)
{
    void  *p;

    delete_local_namepool(csound);
    delete_global_namepool(csound);
    p = (void*) csound->opcodlst;
    csound->opcodlst = NULL;
    csound->oplstend = NULL;
    if (p != NULL)
      free(p);
    csound->otranGlobals = NULL;
}

/* IV - Oct 12 2002: new function to parse arguments of opcode definitions */

static int parse_opcode_args(CSOUND *csound, OENTRY *opc)
{
    OPCODINFO   *inm = (OPCODINFO*) opc->useropinfo;
    char    *types, *otypes;
    int     i, i_incnt, a_incnt, k_incnt, i_outcnt, a_outcnt, k_outcnt, err;
    short   *a_inlist, *k_inlist, *i_inlist, *a_outlist, *k_outlist, *i_outlist;

    /* count the number of arguments, and check types */
    i = i_incnt = a_incnt = k_incnt = i_outcnt = a_outcnt = k_outcnt = err = 0;
    types = inm->intypes; otypes = opc->intypes;
    if (!strcmp(types, "0")) types++;   /* no input args */
    while (*types) {
      switch (*types) {
      case 'a':
        a_incnt++; *otypes++ = *types;
        break;
      case 'K':
        i_incnt++;              /* also updated at i-time */
      case 'k':
        k_incnt++; *otypes++ = 'k';
        break;
      case 'i':
      case 'o':
      case 'p':
      case 'j':
        i_incnt++; *otypes++ = *types;
        break;
      default:
        synterr(csound, "invalid input type for opcode %s", inm->name); err++;
      }
      i++; types++;
      if (i > OPCODENUMOUTS) {
        synterr(csound, "too many input args for opcode %s", inm->name);
        err++; break;
      }
    }
    *otypes++ = 'o'; *otypes = '\0';    /* optional arg for local ksmps */
    inm->inchns = strlen(opc->intypes) - 1; /* total number of input chnls */
    inm->perf_incnt = a_incnt + k_incnt;
    /* same for outputs */
    i = 0;
    types = inm->outtypes; otypes = opc->outypes;
    if (!strcmp(types, "0")) types++;   /* no output args */
    while (*types) {
      switch (*types) {
      case 'a':
        a_outcnt++; *otypes++ = *types;
        break;
      case 'K':
        i_outcnt++;             /* also updated at i-time */
      case 'k':
        k_outcnt++; *otypes++ = 'k';
        break;
      case 'i':
        i_outcnt++; *otypes++ = *types;
        break;
      default:
        synterr(csound, "invalid output type for opcode %s", inm->name); err++;
      }
      i++; types++;
      if (i >= OPCODENUMOUTS) {
        synterr(csound, "too many output args for opcode %s", inm->name);
        err++; break;
      }
    }
    *otypes = '\0';
    inm->outchns = strlen(opc->outypes);    /* total number of output chnls */
    inm->perf_outcnt = a_outcnt + k_outcnt;
    /* now build index lists for the various types of arguments */
    i = i_incnt + inm->perf_incnt + i_outcnt + inm->perf_outcnt;
    i_inlist = inm->in_ndx_list = (short*) mmalloc(csound,
                                                   sizeof(short) * (i + 6));
    a_inlist = i_inlist + i_incnt + 1;
    k_inlist = a_inlist + a_incnt + 1;
    i = 0; types = inm->intypes;
    while (*types) {
      switch (*types++) {
        case 'a': *a_inlist++ = i; break;
        case 'k': *k_inlist++ = i; break;
        case 'K': *k_inlist++ = i;      /* also updated at i-time */
        case 'i':
        case 'o':
        case 'p':
        case 'j': *i_inlist++ = i;
      }
      i++;
    }
    *i_inlist = *a_inlist = *k_inlist = -1;     /* put delimiters */
    i_outlist = inm->out_ndx_list = k_inlist + 1;
    a_outlist = i_outlist + i_outcnt + 1;
    k_outlist = a_outlist + a_outcnt + 1;
    i = 0; types = inm->outtypes;
    while (*types) {
      switch (*types++) {
        case 'a': *a_outlist++ = i; break;
        case 'k': *k_outlist++ = i; break;
        case 'K': *k_outlist++ = i;     /* also updated at i-time */
        case 'i': *i_outlist++ = i;
      }
      i++;
    }
    *i_outlist = *a_outlist = *k_outlist = -1;  /* put delimiters */
    return err;
}

static int pnum(char *s)        /* check a char string for pnum format  */
                                /*   and return the pnum ( >= 0 )       */
{                               /* else return -1                       */
    int n;

    if (*s == 'p' || *s == 'P')
      if (sscanf(++s, "%d", &n))
        return(n);
    return(-1);
}

void otran(CSOUND *csound)
{
    OPARMS      *O = csound->oparms;
    TEXT        *tp;
    int         init = 1;
    INSTRTXT    *ip = NULL;
    INSTRTXT    *prvinstxt = &(csound->instxtanchor);
    OPTXT       *bp, *prvbp = NULL;
    ARGLST      *alp;
    char        *s;
    long        pmax = -1, nn;
    long        n, opdstot=0, count, sumcount, instxtcount, optxtcount;

    if (csound->otranGlobals == NULL) {
      csound->otranGlobals = csound->Calloc(csound, sizeof(OTRAN_GLOBALS));
    }
    csound->instrtxtp = (INSTRTXT **) mcalloc(csound, (1 + csound->maxinsno)
                                                      * sizeof(INSTRTXT*));
    csound->opcodeInfo = NULL;          /* IV - Oct 20 2002 */
    opcode_list_create(csound);         /* IV - Oct 31 2002 */

    strconstndx(csound, "\"\"");

    gblnamset(csound, "sr");    /* enter global reserved words */
    gblnamset(csound, "kr");
    gblnamset(csound, "ksmps");
    gblnamset(csound, "nchnls");
    gblnamset(csound, "0dbfs"); /* no commandline override for that! */
    gblnamset(csound, "$sr");   /* incl command-line overrides */
    gblnamset(csound, "$kr");
    gblnamset(csound, "$ksmps");

    rdorchfile(csound);         /* go read orch file    */
    if (csound->pool == NULL) {
      csound->pool = (MYFLT *) mmalloc(csound, NCONSTS * sizeof(MYFLT));
      csound->pool[0] = FL(0.0);
      ST(poolcount) = 1;
      ST(nconsts) = NCONSTS;
    }
    while ((tp = getoptxt(csound, &init)) != NULL) {
        /* then for each opcode: */
        unsigned int threads=0;
        int opnum = tp->opnum;
        switch (opnum) {
        case INSTR:
        case OPCODE:            /* IV - Sep 8 2002 */
            ip = (INSTRTXT *) mcalloc(csound, (long)sizeof(INSTRTXT));
            prvinstxt = prvinstxt->nxtinstxt = ip;
            txtcpy((char *)&ip->t,(char *)tp);
            prvbp = (OPTXT *) ip;               /* begin an optxt chain */
            alp = ip->t.inlist;
/* <---- IV - Oct 16 2002: rewritten this code ---- */
            if (opnum == INSTR) {
              int err = 0, cnt, i;
              if (!alp->count) {  /* IV - Sep 8 2002: check for missing name */
                synterr(csound, Str("missing instrument number or name"));
                continue;
              }
              /* IV - Oct 16 2002: allow both numbers and names for instr */
              for (cnt = 0; cnt < alp->count; cnt++) {
                char *c = alp->arg[cnt];
                if (strlen(c) <= 0) {
                  synterr(csound, Str("missing instrument number or name"));
                  err++; continue;
                }
                if (isdigit(*c)) {      /* numbered instrument */
                  if (!sscanf(c, "%ld", &n) || n < 0) {
                    synterr(csound, Str("illegal instr number"));
                    err++; continue;
                  }
                  if (n > csound->maxinsno) {
                    int old_maxinsno = csound->maxinsno;
                    /* expand */
                    while (n>csound->maxinsno) csound->maxinsno += MAXINSNO;
                    csound->instrtxtp = (INSTRTXT**)
                      mrealloc(csound, csound->instrtxtp, (1 + csound->maxinsno)
                                                          * sizeof(INSTRTXT*));
                    /* Array expected to be nulled so.... */
                    for (i = old_maxinsno + 1; i <= csound->maxinsno; i++)
                      csound->instrtxtp[i] = NULL;
                  }
                  if (csound->instrtxtp[n] != NULL) {
                    synterr(csound, Str("instr %ld redefined"), (long) n);
                    err++; continue;
                  }
                  csound->instrtxtp[n] = ip;
                }
                else {                  /* named instrument */
                  long  insno_priority = -1L;
                  if (*c == '+') {
                    insno_priority--; c++;
                  }
                  /* IV - Oct 31 2002: some error checking */
                  if (!check_instr_name(c)) {
                    synterr(csound, Str("invalid name for instrument"));
                    err++; continue;
                  }
                  /* IV - Oct 31 2002: store the name */
                  if (!named_instr_alloc(csound, c, ip, insno_priority)) {
                    synterr(csound, "instr %s redefined", c);
                    err++; continue;
                  }
                  ip->insname = c;  /* IV - Nov 10 2002: also in INSTRTXT */
                  n = -2;
                }
              }
              if (err) continue;
              if (n) putop(csound, &ip->t);     /* print, except i0 */
            }
            else {                  /* opcode definition with string name */
              OENTRY    tmpEntry, *opc, *newopc;
              long      newopnum;
              OPCODINFO *inm;
              char      *name = alp->arg[0];

              /* some error checking */
              if (!alp->count || (strlen(name) <= 0)) {
                  synterr(csound, Str("No opcode name"));
                  continue;
                }
              /* IV - Oct 31 2002 */
              if (!check_instr_name(name)) {
                synterr(csound, Str("invalid name for opcode"));
                continue;
              }
              if (ip->t.inlist->count != 3) {
                synterr(csound, Str("opcode declaration error "
                                    "(usage: opcode name, outtypes, intypes) "
                                    "-- opcode %s"), name);
                continue;
              }

              /* IV - Oct 31 2002: check if opcode is already defined */
              newopnum = find_opcode(csound, name);
              if (newopnum) {
                /* IV - Oct 31 2002: redefine old opcode if possible */
                if (newopnum < SETEND || !strcmp(name, "subinstr")) {
                  synterr(csound, Str("cannot redefine %s"), name);
                  continue;
                }
                csound->Message(csound,
                                Str("WARNING: redefined opcode: %s\n"), name);
              }
              /* IV - Oct 31 2002 */
              /* store the name in a linked list (note: must use mcalloc) */
              inm = (OPCODINFO *) mcalloc(csound, sizeof(OPCODINFO));
              inm->name = name;
              inm->intypes = alp->arg[2];
              inm->outtypes = alp->arg[1];
              inm->ip = ip;
              inm->prv = csound->opcodeInfo;
              csound->opcodeInfo = inm;
              /* IV - Oct 31 2002: */
              /* create a fake opcode so we can call it as such */
              opc = csound->opcodlst + find_opcode(csound, ".userOpcode");
              memcpy(&tmpEntry, opc, sizeof(OENTRY));
              csound->AppendOpcodes(csound, &tmpEntry, 1);
              newopc = (OENTRY*) csound->oplstend - 1;
              newopnum = (long) ((OENTRY*) newopc - (OENTRY*) csound->opcodlst);
              newopc->opname = name;
              newopc->useropinfo = (void*) inm; /* ptr to opcode parameters */
              opcode_list_add_entry(csound, newopnum, 1);
              ip->insname = name; ip->opcode_info = inm; /* IV - Nov 10 2002 */
              /* check in/out types and copy to the opcode's */
              newopc->outypes = mmalloc(csound, strlen(alp->arg[1]) + 1);
                /* IV - Sep 8 2002: opcodes have an optional arg for ksmps */
              newopc->intypes = mmalloc(csound, strlen(alp->arg[2]) + 2);
              if (parse_opcode_args(csound, newopc)) continue;
              n = -2;
/* ---- IV - Oct 16 2002: end of new code ----> */
              putop(csound, &ip->t);
            }
            delete_local_namepool(csound);          /* clear lcl namlist */
            ST(lclnxtkcnt) = ST(lclnxtdcnt) = 0;    /*   for rebuilding  */
            ST(lclnxtwcnt) = ST(lclnxtacnt) = 0;
            ST(lclnxtpcnt) = ST(lclnxtscnt) = 0;
            opdstot = 0;
            threads = 0;
            pmax = 3L;                              /* set minimum pflds */
            break;
        case ENDIN:
        case ENDOP:             /* IV - Sep 8 2002 */
            bp = (OPTXT *) mcalloc(csound, (long)sizeof(OPTXT));
            txtcpy((char *)&bp->t, (char *)tp);
            prvbp->nxtop = bp;
            bp->nxtop = NULL;   /* terminate the optxt chain */
            if (O->odebug) {
              putop(csound, &bp->t);
              csound->Message(csound, "pmax %ld, kcnt %d, dcnt %d, "
                                      "wcnt %d, acnt %d, pcnt %d, scnt %d\n",
                                      pmax, ST(lclnxtkcnt), ST(lclnxtdcnt),
                                      ST(lclnxtwcnt), ST(lclnxtacnt),
                                      ST(lclnxtpcnt), ST(lclnxtscnt));
            }
            ip->pmax = (int)pmax;
            ip->pextrab = ((n = pmax-3L) > 0 ? (int) n * sizeof(MYFLT) : 0);
            ip->pextrab = ((int) ip->pextrab + 7) & (~7);
            ip->mdepends = threads >> 4;
            ip->lclkcnt = ST(lclnxtkcnt);
            /* align to 8 bytes for "spectral" types */
            if ((int) sizeof(MYFLT) < 8 &&
                (ST(lclnxtdcnt) + ST(lclnxtwcnt) + ST(lclnxtpcnt)) > 0)
              ip->lclkcnt = (ip->lclkcnt + 1) & (~1);
            ip->lcldcnt = ST(lclnxtdcnt);
            ip->lclwcnt = ST(lclnxtwcnt);
            ip->lclacnt = ST(lclnxtacnt);
            ip->lclpcnt = ST(lclnxtpcnt);
            ip->lclscnt = ST(lclnxtscnt);
            ip->lclfixed = ST(lclnxtkcnt) + ST(lclnxtdcnt) * Dfloats
                                          + ST(lclnxtwcnt) * Wfloats
                                          + ST(lclnxtpcnt) * Pfloats;
            ip->opdstot = opdstot;              /* store total opds reqd */
            ip->muted = 1;      /* Allow to play */
            n = -1;             /* No longer in an instrument */
            break;
        default:
            bp = (OPTXT *) mcalloc(csound, (long)sizeof(OPTXT));
            txtcpy((char *)&bp->t,(char *)tp);
            prvbp = prvbp->nxtop = bp;  /* link into optxt chain */
            threads |= csound->opcodlst[opnum].thread;
            opdstot += csound->opcodlst[opnum].dsblksiz;  /* sum opds's */
            if (O->odebug) putop(csound, &bp->t);
            for (alp=bp->t.inlist, nn=alp->count; nn>0; ) {
              s = alp->arg[--nn];
              if ((n = pnum(s)) >= 0)
                { if (n > pmax)  pmax = n; }
              else lgbuild(csound, s);
            }
            for (alp=bp->t.outlist, nn=alp->count; nn>0; ) {
              s = alp->arg[--nn];
              if (*s == '"') {
                synterr(csound, Str("string constant used as output"));
                continue;
              }
              if ((n = pnum(s)) >= 0) {
                if (n > pmax)  pmax = n;
              }
              else lgbuild(csound, s);
              if (!nn && bp->t.opcod[1] == '.'        /* rsvd glbal = n ? */
                  && strcmp(bp->t.opcod,"=.r")==0) {  /*  (assume const)  */
                MYFLT constval = csound->pool[constndx(csound,
                                                       bp->t.inlist->arg[0])];
                if (strcmp(s,"sr") == 0)
                  csound->tran_sr = constval;         /* modify otran defaults*/
                else if (strcmp(s,"kr") == 0)
                  csound->tran_kr = constval;
                else if (strcmp(s,"ksmps") == 0)
                  csound->tran_ksmps = constval;
                else if (strcmp(s,"nchnls") == 0)
                  csound->tran_nchnls = (int)constval;
                /* we have set this as reserved in rdorch.c */
                else if (strcmp(s,"0dbfs") == 0)
                  csound->tran_0dbfs = constval;
              }
            }
            n = 0;              /* Mark as unfinished */
            break;
        }
    }
    if (n != -1)
      synterr(csound, Str("Missing endin"));
    /* now add the instruments with names, assigning them fake instr numbers */
    named_instr_assign_numbers(csound);         /* IV - Oct 31 2002 */
    if (csound->opcodeInfo) {
      int num = csound->maxinsno;       /* store after any other instruments */
      OPCODINFO *inm = csound->opcodeInfo;
      /* IV - Oct 31 2002: now add user defined opcodes */
      while (inm) {
        /* we may need to expand the instrument array */
        if (++num > csound->maxopcno) {
          int i;
          i = (csound->maxopcno > 0 ? csound->maxopcno : csound->maxinsno);
          csound->maxopcno = i + MAXINSNO;
          csound->instrtxtp = (INSTRTXT**)
            mrealloc(csound, csound->instrtxtp, (1 + csound->maxopcno)
                                                * sizeof(INSTRTXT*));
          /* Array expected to be nulled so.... */
          while (++i <= csound->maxopcno) csound->instrtxtp[i] = NULL;
        }
        inm->instno = num;
        csound->instrtxtp[num] = inm->ip;
        inm = inm->prv;
      }
    }
    /* Deal with defaults and consistency */
    if (csound->tran_ksmps == FL(-1.0)) {
      if (csound->tran_sr == FL(-1.0)) csound->tran_sr = DFLT_SR;
      if (csound->tran_kr == FL(-1.0)) csound->tran_kr = DFLT_KR;
      csound->tran_ksmps = (MYFLT) ((int) (csound->tran_sr / csound->tran_kr
                                           + FL(0.5)));
    }
    else if (csound->tran_kr == FL(-1.0)) {
      if (csound->tran_sr == FL(-1.0)) csound->tran_sr = DFLT_SR;
      csound->tran_kr = csound->tran_sr / csound->tran_ksmps;
    }
    else if (csound->tran_sr == FL(-1.0)) {
      csound->tran_sr = csound->tran_kr * csound->tran_ksmps;
    }
    /* That deals with missing values, however we do need ksmps to be integer */
    {
      CSOUND    *p = (CSOUND*) csound;
      char      err_msg[128];
      sprintf(err_msg, "sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:",
                       p->tran_sr, p->tran_kr, p->tran_ksmps);
      if (p->tran_sr <= FL(0.0))
        synterr(p, Str("%s invalid sample rate"), err_msg);
      if (p->tran_kr <= FL(0.0))
        synterr(p, Str("%s invalid control rate"), err_msg);
      else if (p->tran_ksmps < FL(0.75) ||
               FLOAT_COMPARE(p->tran_ksmps, MYFLT2LRND(p->tran_ksmps)))
        synterr(p, Str("%s invalid ksmps value"), err_msg);
      else if (FLOAT_COMPARE(p->tran_sr, (double) p->tran_kr * p->tran_ksmps))
        synterr(p, Str("%s inconsistent sr, kr, ksmps"), err_msg);
    }

    ip = csound->instxtanchor.nxtinstxt;
    bp = (OPTXT *) ip;
    while (bp != (OPTXT *) NULL && (bp = bp->nxtop) != NULL) {
      /* chk instr 0 for illegal perfs */
      int thread, opnum = bp->t.opnum;
      if (opnum == ENDIN) break;
      if (opnum == LABEL) continue;
      if ((thread = csound->opcodlst[opnum].thread) & 06 ||
          (!thread && bp->t.pftype != 'b'))
        synterr(csound, Str("perf-pass statements illegal in header blk"));
    }
    if (csound->synterrcnt) {
      print_opcodedir_warning(csound);
      csound->Die(csound, Str("%d syntax errors in orchestra.  "
                              "compilation invalid"), csound->synterrcnt);
    }
    if (O->odebug) {
      long  n;
      MYFLT *p;
      csound->Message(csound, "poolcount = %ld, strpool_cnt = %ld\n",
                              ST(poolcount), ST(strpool_cnt));
      csound->Message(csound, "pool:");
      for (n = ST(poolcount), p = csound->pool; n--; p++)
        csound->Message(csound, "\t%g", *p);
      csound->Message(csound, "\n");
      csound->Message(csound, "strpool:");
      for (n = 0L; n < ST(strpool_cnt); n++)
        csound->Message(csound, "\t%s", ST(strpool)[n]);
      csound->Message(csound, "\n");
    }
    ST(gblfixed) = ST(gblnxtkcnt) + ST(gblnxtpcnt) * (int) Pfloats;
    ST(gblkcount) = ST(gblnxtkcnt);
    /* align to 8 bytes for "spectral" types */
    if ((int) sizeof(MYFLT) < 8 && ST(gblnxtpcnt))
      ST(gblkcount) = (ST(gblkcount) + 1) & (~1);
    ST(gblacount) = ST(gblnxtacnt);
    ST(gblscount) = ST(gblnxtscnt);

    ip = &(csound->instxtanchor);
    for (sumcount = 0; (ip = ip->nxtinstxt) != NULL; ) {/* for each instxt */
      OPTXT *optxt = (OPTXT *) ip;
      int optxtcount = 0;
      while ((optxt = optxt->nxtop) != NULL) {      /* for each op in instr  */
        TEXT *ttp = &optxt->t;
        optxtcount += 1;
        if (ttp->opnum == ENDIN                     /*    (until ENDIN)      */
            || ttp->opnum == ENDOP) break;  /* (IV - Oct 26 2002: or ENDOP) */
        if ((count = ttp->inlist->count)!=0)
          sumcount += count +1;                     /* count the non-nullist */
        if ((count = ttp->outlist->count)!=0)       /* slots in all arglists */
          sumcount += (count + 1);
      }
      ip->optxtcount = optxtcount;                  /* optxts in this instxt */
    }
    ST(argoffsize) = (sumcount + 1) * sizeof(int);  /* alloc all plus 1 null */
    /* as argoff ints */
    csound->argoffspace = (int*) mmalloc(csound, ST(argoffsize));
    ST(nxtargoffp) = csound->argoffspace;
    ST(nulloffs) = (ARGOFFS *) csound->argoffspace; /* setup the null argoff */
    *ST(nxtargoffp)++ = 0;
    ST(argofflim) = ST(nxtargoffp) + sumcount;
    ip = &(csound->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL)        /* add all other entries */
      insprep(csound, ip);                      /*   as combined offsets */
    if (O->odebug) {
      int *p = csound->argoffspace;
      csound->Message(csound, "argoff array:\n");
      do {
        csound->Message(csound, "\t%d", *p++);
      } while (p < ST(argofflim));
      csound->Message(csound, "\n");
    }
    if (ST(nxtargoffp) != ST(argofflim))
      csoundDie(csound, Str("inconsistent argoff sumcount"));

    ip = &(csound->instxtanchor);               /* set the OPARMS values */
    instxtcount = optxtcount = 0;
    while ((ip = ip->nxtinstxt) != NULL) {
      instxtcount += 1;
      optxtcount += ip->optxtcount;
    }
    csound->instxtcount = instxtcount;
    csound->optxtsize = instxtcount * sizeof(INSTRTXT)
                        + optxtcount * sizeof(OPTXT);
    csound->poolcount = ST(poolcount);
    csound->gblfixed = ST(gblnxtkcnt) + ST(gblnxtpcnt) * (int) Pfloats;
    csound->gblacount = ST(gblnxtacnt);
    csound->gblscount = ST(gblnxtscnt);
    /* clean up */
    delete_local_namepool(csound);
    delete_global_namepool(csound);
}

/* prep an instr template for efficient allocs  */
/* repl arg refs by offset ndx to lcl/gbl space */

static void insprep(CSOUND *csound, INSTRTXT *tp)
{
    OPARMS      *O = csound->oparms;
    OPTXT       *optxt;
    OENTRY      *ep;
    int         n, opnum, inreqd;
    char        **argp;
    char        **labels, **lblsp;
    LBLARG      *larg, *largp;
    ARGLST      *outlist, *inlist;
    ARGOFFS     *outoffs, *inoffs;
    int         indx, *ndxp;

    labels = (char **)mmalloc(csound, (csound->nlabels) * sizeof(char *));
    lblsp = labels;
    larg = (LBLARG *)mmalloc(csound, (csound->ngotos) * sizeof(LBLARG));
    largp = larg;
    ST(lclkcnt) = tp->lclkcnt;
    ST(lcldcnt) = tp->lcldcnt;
    ST(lclwcnt) = tp->lclwcnt;
    ST(lclfixed) = tp->lclfixed;
    ST(lclpcnt) = tp->lclpcnt;
    ST(lclscnt) = tp->lclscnt;
    ST(lclacnt) = tp->lclacnt;
    delete_local_namepool(csound);              /* clear lcl namlist */
    ST(lclnxtkcnt) = ST(lclnxtdcnt) = 0;        /*   for rebuilding  */
    ST(lclnxtwcnt) = ST(lclnxtacnt) = 0;
    ST(lclnxtpcnt) = ST(lclnxtscnt) = 0;
    ST(lclpmax) = tp->pmax;                     /* set pmax for plgndx */
    ndxp = ST(nxtargoffp);
    optxt = (OPTXT *)tp;
    while ((optxt = optxt->nxtop) != NULL) {    /* for each op in instr */
      TEXT *ttp = &optxt->t;
      if ((opnum = ttp->opnum) == ENDIN         /*  (until ENDIN)  */
          || opnum == ENDOP)            /* (IV - Oct 31 2002: or ENDOP) */
        break;
      if (opnum == LABEL) {
        if (lblsp - labels >= csound->nlabels) {
          int oldn = lblsp - labels;
          csound->nlabels += NLABELS;
          if (lblsp - labels >= csound->nlabels)
            csound->nlabels = lblsp - labels + 2;
          if (csound->oparms->msglevel)
            csound->Message(csound,
                            Str("LABELS list is full...extending to %d\n"),
                            csound->nlabels);
          labels =
            (char**)mrealloc(csound, labels, csound->nlabels*sizeof(char*));
          lblsp = &labels[oldn];
        }
        *lblsp++ = ttp->opcod;
        continue;
      }
      ep = &(csound->opcodlst[opnum]);
      if (O->odebug) csound->Message(csound, "%s argndxs:", ep->opname);
      if ((outlist = ttp->outlist) == ST(nullist) || !outlist->count)
        ttp->outoffs = ST(nulloffs);
      else {
        ttp->outoffs = outoffs = (ARGOFFS *) ndxp;
        outoffs->count = n = outlist->count;
        argp = outlist->arg;                    /* get outarg indices */
        ndxp = outoffs->indx;
        while (n--) {
          *ndxp++ = indx = plgndx(csound, *argp++);
          if (O->odebug) csound->Message(csound, "\t%d",indx);
        }
      }
      if ((inlist = ttp->inlist) == ST(nullist) || !inlist->count)
        ttp->inoffs = ST(nulloffs);
      else {
        ttp->inoffs = inoffs = (ARGOFFS *) ndxp;
        inoffs->count = inlist->count;
        inreqd = strlen(ep->intypes);
        argp = inlist->arg;                     /* get inarg indices */
        ndxp = inoffs->indx;
        for (n=0; n < inlist->count; n++, argp++, ndxp++) {
          if (n < inreqd && ep->intypes[n] == 'l') {
            if (largp - larg >= csound->ngotos) {
              int oldn = csound->ngotos;
              csound->ngotos += NGOTOS;
              if (csound->oparms->msglevel)
                csound->Message(csound,
                                Str("GOTOS list is full..extending to %d\n"),
                                csound->ngotos);
              if (largp - larg >= csound->ngotos)
                csound->ngotos = largp - larg + 1;
              larg = (LBLARG *)
                mrealloc(csound, larg, csound->ngotos * sizeof(LBLARG));
              largp = &larg[oldn];
            }
            if (O->odebug)
              csound->Message(csound, "\t***lbl");  /* if arg is label,  */
            largp->lbltxt = *argp;
            largp->ndxp = ndxp;                     /*  defer till later */
            largp++;
          }
          else {
            char *s = *argp;
            indx = plgndx(csound, s);
            if (O->odebug) csound->Message(csound, "\t%d",indx);
            *ndxp = indx;
          }
        }
      }
      if (O->odebug) csound->Message(csound, "\n");
    }
 nxt:
    while (--largp >= larg) {                   /* resolve the lbl refs */
      char *s = largp->lbltxt;
      char **lp;
      for (lp = labels; lp < lblsp; lp++)
        if (strcmp(s, *lp) == 0) {
          *largp->ndxp = lp - labels + LABELOFS;
          goto nxt;
        }
      csoundDie(csound, Str("target label '%s' not found"), s);
    }
    ST(nxtargoffp) = ndxp;
    mfree(csound, labels);
    mfree(csound, larg);
}

static void lgbuild(CSOUND *csound, char *s)
{                               /* build pool of floating const values  */
    char c;                     /* build lcl/gbl list of ds names, offsets */
                                /*   (no need to save the returned values) */
    if (((c = *s) >= '0' && c <= '9') || c == '.' || c == '-' || c == '+')
      constndx(csound, s);
    else if (c == '"')
      strconstndx(csound, s);
    else if (!(lgexist(csound, s))) {
      if (c == 'g' || (c == '#' && s[1] == 'g'))
        gblnamset(csound, s);
      else
        lclnamset(csound, s);
    }
}

static int plgndx(CSOUND *csound, char *s)
{                               /* get storage ndx of const, pnum, lcl or gbl */
    char        c;              /* argument const/gbl indexes are positiv+1, */
    int         n, indx;        /* pnum/lcl negativ-1 called only after      */
                                /* poolcount & lclpmax are finalised */
    c = *s;
    /* must trap 0dbfs as name starts with a digit! */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0))
      indx = constndx(csound, s) + 1;
    else if (c == '"')
      indx = strconstndx(csound, s) + STR_OFS + 1;
    else if ((n = pnum(s)) >= 0)
      indx = -n;
    else if (c == 'g' || (c == '#' && *(s+1) == 'g') || gexist(csound, s))
      indx = (int) (ST(poolcount) + 1 + gbloffndx(csound, s));
    else
      indx = -(ST(lclpmax) + 1 + lcloffndx(csound, s));
/*    csound->Message(csound, " [%s -> %d (%x)]\n", s, indx, indx); */
    return(indx);
}

static int strconstndx(CSOUND *csound, const char *s)
{                                   /* get storage ndx of string const value */
    int     i, cnt;                 /* builds value pool on 1st occurrence   */

    /* check syntax */
    cnt = (int) strlen(s);
    if (cnt < 2 || *s != '"' || s[cnt - 1] != '"') {
      synterr(csound, Str("string syntax '%s'"), s);
      return 0;
    }
    /* check if a copy of the string is already stored */
    for (i = 0; i < ST(strpool_cnt); i++) {
      if (strcmp(s, ST(strpool)[i]) == 0)
        return i;
    }
    /* not found, store new string */
    cnt = ST(strpool_cnt)++;
    if (!(cnt & 0x7F)) {
      /* extend list */
      if (!cnt) ST(strpool) = csound->Malloc(csound, 0x80 * sizeof(MYFLT*));
      else      ST(strpool) = csound->ReAlloc(csound, ST(strpool),
                                              (cnt + 0x80) * sizeof(MYFLT*));
    }
    ST(strpool)[cnt] = (char*) csound->Malloc(csound, strlen(s) + 1);
    strcpy(ST(strpool)[cnt], s);
    /* and return index */
    return cnt;
}

static int constndx(CSOUND *csound, char *s)
{                                   /* get storage ndx of float const value */
    MYFLT   newval;                 /* builds value pool on 1st occurrence  */
    long    n;                      /* final poolcount used in plgndx above */
    MYFLT   *fp;                    /* pool may be moved w. ndx still valid */
    char    *str = s;

#ifdef USE_DOUBLE
    if (sscanf(s,"%lf",&newval) != 1) goto flerror;
#else
    if (sscanf(s,"%f",&newval) != 1) goto flerror;
#endif
    /* It is tempting to assume that if this loop is removed then we
     * would not share constants.  However this breaks something else
     * as this function is used to retrieve constants as well....
     * I (JPff) have not understood this yet.
     */
    for (fp = csound->pool, n = ST(poolcount); n--; fp++) {
                                                    /* now search constpool */
      if (newval == *fp)                            /* if val is there      */
        return (fp - csound->pool);                 /*    return w. index   */
    }
    if (++ST(poolcount) > ST(nconsts)) {
      int indx = fp - csound->pool;
      ST(nconsts) += NCONSTS;
      if (csound->oparms->msglevel)
        csound->Message(csound, Str("extending Floating pool to %d\n"),
                                ST(nconsts));
      csound->pool = (MYFLT*) mrealloc(csound, csound->pool,
                                               ST(nconsts) * sizeof(MYFLT));
      fp = csound->pool + indx;
    }
    *fp = newval;                                   /* else enter newval    */
/*  csound->Message(csound, "Constant %d: %f\n", fp - csound->pool, newval); */
    return (fp - csound->pool);                     /*   and return new ndx */

 flerror:
    synterr(csound, Str("numeric syntax '%s'"), str);
    return(0);
}

void putop(CSOUND *csound, TEXT *tp)
{
    int n, nn;

    if ((n = tp->outlist->count)!=0) {
      nn = 0;
      while (n--) csound->Message(csound,"%s\t", tp->outlist->arg[nn++]);
    }
    else csound->Message(csound,"\t");
    csound->Message(csound,"%s\t", tp->opcod);
    if ((n = tp->inlist->count)!=0) {
      nn = 0;
      while (n--) csound->Message(csound,"%s\t",tp->inlist->arg[nn++]);
    }
    csound->Message(csound,"\n");
}

static inline unsigned char name_hash(const char *s)
{
    unsigned char *c = (unsigned char*) &(s[0]);
    unsigned char h = (unsigned char) 0;
    for ( ; *c != (unsigned char) 0; c++)
      h = strhash_tabl_8[*c ^ h];
    return h;
}

static inline int sCmp(const char *x, const char *y)
{
    int tmp = 0;
    while (x[tmp] == y[tmp] && x[tmp] != (char) 0)
      tmp++;
    return (x[tmp] != y[tmp]);
}

/* tests whether variable name exists   */
/*      in gbl namelist                 */

static int gexist(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(s);
    NAME          *p;

    for (p = ST(gblNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
    return (p == NULL ? 0 : 1);
}

/* returns non-zero if 's' is defined in the global or local pool of names */

int lgexist(CSOUND *csound, const char *s)
{
    unsigned char h = name_hash(s);
    NAME          *p;

    for (p = ST(gblNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (p != NULL)
      return 1;
    for (p = ST(lclNames)[h]; p != NULL && sCmp(p->namep, s); p = p->nxt);
    return (p == NULL ? 0 : 1);
}

/* builds namelist & type counts for gbl names */

static void gblnamset(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(s);
    NAME          *p = ST(gblNames)[h];
                                                /* search gbl namelist: */
    for ( ; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (p != NULL)                              /* if name is there     */
      return;                                   /*    return            */
    p = (NAME*) malloc(sizeof(NAME));
    if (p == NULL)
      csound->Die(csound, Str("gblnamset(): memory allocation failure"));
    p->namep = s;                               /* else record newname  */
    p->nxt = ST(gblNames)[h];
    ST(gblNames)[h] = p;
    if (*s == '#')  s++;
    if (*s == 'g')  s++;
    switch ((int) *s) {                         /*   and its type-count */
      case 'a': p->type = ATYPE; p->count = ST(gblnxtacnt)++; break;
      case 'S': p->type = STYPE; p->count = ST(gblnxtscnt)++; break;
      case 'f': p->type = PTYPE; p->count = ST(gblnxtpcnt)++; break;
      default:  p->type = KTYPE; p->count = ST(gblnxtkcnt)++;
    }
}

/* builds namelist & type counts for lcl names  */
/*  called by otran for each instr for lcl cnts */
/*  lists then redone by insprep via lcloffndx  */

static NAME *lclnamset(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(s);
    NAME          *p = ST(lclNames)[h];
                                                /* search lcl namelist: */
    for ( ; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (p != NULL)                              /* if name is there     */
      return p;                                 /*    return ptr        */
    p = (NAME*) malloc(sizeof(NAME));
    if (p == NULL)
      csound->Die(csound, Str("lclnamset(): memory allocation failure"));
    p->namep = s;                               /* else record newname  */
    p->nxt = ST(lclNames)[h];
    ST(lclNames)[h] = p;
    if (*s == '#')  s++;
    switch (*s) {                               /*   and its type-count */
/*       case 'd': p->type = DTYPE; p->count = ST(lclnxtdcnt)++; break; */
      case 'w': p->type = WTYPE; p->count = ST(lclnxtwcnt)++; break;
      case 'a': p->type = ATYPE; p->count = ST(lclnxtacnt)++; break;
      case 'f': p->type = PTYPE; p->count = ST(lclnxtpcnt)++; break;
      case 'S': p->type = STYPE; p->count = ST(lclnxtscnt)++; break;
      default:  p->type = KTYPE; p->count = ST(lclnxtkcnt)++; break;
    }
    return p;
}

/* get named offset index into gbl dspace     */
/* called only after otran and gblfixed valid */

static int gbloffndx(CSOUND *csound, char *s)
{
    unsigned char h = name_hash(s);
    NAME          *p = ST(gblNames)[h];

    for ( ; p != NULL && sCmp(p->namep, s); p = p->nxt);
    if (p == NULL)
      csoundDie(csound, Str("unexpected global name"));
    switch (p->type) {
      case ATYPE: return (ST(gblfixed) + p->count);
      case STYPE: return (ST(gblfixed) + ST(gblacount) + p->count);
      case PTYPE: return (ST(gblkcount) + p->count * (int) Pfloats);
    }
    return p->count;
}

/* get named offset index into instr lcl dspace   */
/* called by insprep aftr lclcnts, lclfixed valid */

static int lcloffndx(CSOUND *csound, char *s)
{
    NAME    *np = lclnamset(csound, s);         /* rebuild the table    */
    switch (np->type) {                         /* use cnts to calc ndx */
      case KTYPE: return np->count;
/*       case DTYPE: return (ST(lclkcnt) + np->count * Dfloats); */
      case WTYPE: return (ST(lclkcnt) + ST(lcldcnt) * Dfloats
                                      + np->count * Wfloats);
      case ATYPE: return (ST(lclfixed) + np->count);
      case PTYPE: return (ST(lclkcnt) + ST(lcldcnt) * Dfloats
                                      + ST(lclwcnt) * Wfloats
                                      + np->count * (int) Pfloats);
      case STYPE: return (ST(lclfixed) + ST(lclacnt) + np->count);
      default:    csoundDie(csound, Str("unknown nametype"));
    }
    return 0;
}

static void delete_global_namepool(CSOUND *csound)
{
    int i;

    if (csound->otranGlobals == NULL)
      return;
    for (i = 0; i < 256; i++) {
      while (ST(gblNames)[i] != NULL) {
        NAME  *nxt = ST(gblNames)[i]->nxt;
        free(ST(gblNames)[i]);
        ST(gblNames)[i] = nxt;
      }
    }
}

static void delete_local_namepool(CSOUND *csound)
{
    int i;

    if (csound->otranGlobals == NULL)
      return;
    for (i = 0; i < 256; i++) {
      while (ST(lclNames)[i] != NULL) {
        NAME  *nxt = ST(lclNames)[i]->nxt;
        free(ST(lclNames)[i]);
        ST(lclNames)[i] = nxt;
      }
    }
}

 /* ------------------------------------------------------------------------ */

/* get size of string in MYFLT units */

static int strlen_to_samples(const char *s)
{
    int n = (int) strlen(s);
    n = (n + (int) sizeof(MYFLT)) / (int) sizeof(MYFLT);
    return n;
}

/* convert string constant */

static void unquote_string(char *dst, const char *src)
{
    int i, j, n = (int) strlen(src) - 1;
    for (i = 1, j = 0; i < n; i++) {
      if (src[i] != '\\')
        dst[j++] = src[i];
      else {
        switch (src[++i]) {
        case 'a':   dst[j++] = '\a';  break;
        case 'b':   dst[j++] = '\b';  break;
        case 'f':   dst[j++] = '\f';  break;
        case 'n':   dst[j++] = '\n';  break;
        case 'r':   dst[j++] = '\r';  break;
        case 't':   dst[j++] = '\t';  break;
        case 'v':   dst[j++] = '\v';  break;
        case '"':   dst[j++] = '"';   break;
        case '\\':  dst[j++] = '\\';  break;
        default:
          if (src[i] >= '0' && src[i] <= '7') {
            int k = 0, l = (int) src[i] - '0';
            while (++k < 3 && src[i + 1] >= '0' && src[i + 1] <= '7')
              l = (l << 3) | ((int) src[++i] - '0');
            dst[j++] = (char) l;
          }
          else {
            dst[j++] = '\\'; i--;
          }
        }
      }
    }
    dst[j] = '\0';
}

static int create_strconst_ndx_list(CSOUND *csound, int **lst, int offs)
{
    int     *ndx_lst;
    char    **strpool;
    int     strpool_cnt, ndx, i;

    strpool_cnt = ST(strpool_cnt);
    strpool = ST(strpool);
    /* strpool_cnt always >= 1 because of empty string at index 0 */
    ndx_lst = (int*) csound->Malloc(csound, strpool_cnt * sizeof(int));
    for (i = 0, ndx = offs; i < strpool_cnt; i++) {
      ndx_lst[i] = ndx;
      ndx += strlen_to_samples(strpool[i]);
    }
    *lst = ndx_lst;
    /* return with total size in MYFLT units */
    return (ndx - offs);
}

static void convert_strconst_pool(CSOUND *csound, MYFLT *dst)
{
    char    **strpool, *s;
    int     strpool_cnt, ndx, i;

    strpool_cnt = ST(strpool_cnt);
    strpool = ST(strpool);
    if (strpool == NULL)
      return;
    for (ndx = i = 0; i < strpool_cnt; i++) {
      s = (char*) ((MYFLT*) dst + (int) ndx);
      unquote_string(s, strpool[i]);
      ndx += strlen_to_samples(strpool[i]);
    }
    /* original pool is no longer needed */
    ST(strpool) = NULL;
    ST(strpool_cnt) = 0;
    for (i = 0; i < strpool_cnt; i++)
      csound->Free(csound, strpool[i]);
    csound->Free(csound, strpool);
}

void oload(CSOUND *p)
{
    long    n, combinedsize, insno, *lp;
    long    gblabeg, gblsbeg, gblscbeg, lclabeg, lclsbeg;
    MYFLT   *combinedspc, *gblspace, *fp1;
    INSTRTXT *ip;
    OPTXT   *optxt;
    OPARMS  *O = p->oparms;
    int     *strConstIndexList;
    MYFLT   ensmps;

    p->esr = p->tran_sr; p->ekr = p->tran_kr;
    p->ksmps = (int) ((ensmps = p->tran_ksmps) + FL(0.5));
    ip = p->instxtanchor.nxtinstxt;        /* for instr 0 optxts:  */
    optxt = (OPTXT *) ip;
    while ((optxt = optxt->nxtop) !=  NULL) {
      TEXT  *ttp = &optxt->t;
      ARGOFFS *inoffp, *outoffp;
      int opnum = ttp->opnum;
      if (opnum == ENDIN) break;
      if (opnum == LABEL) continue;
      outoffp = ttp->outoffs;           /* use unexpanded ndxes */
      inoffp = ttp->inoffs;             /* to find sr.. assigns */
      if (outoffp->count == 1 && inoffp->count == 1) {
        int rindex = (int) outoffp->indx[0] - (int) p->poolcount;
        if (rindex > 0 && rindex <= 5) {
          MYFLT conval = p->pool[inoffp->indx[0] - 1];
          switch (rindex) {
            case 1:  p->esr = conval;   break;  /* & use those values */
            case 2:  p->ekr = conval;   break;  /*  to set params now */
            case 3:  p->ksmps = (int) ((ensmps = conval) + FL(0.5)); break;
            case 4:  p->nchnls = (int) (conval + FL(0.5));  break;
            default: p->e0dbfs = conval; break;
          }
        }
      }
    }
    /* why I want oload() to return an error value.... */
    if (p->e0dbfs <= FL(0.0))
      p->Die(p, Str("bad value for 0dbfs: must be positive."));
    if (O->odebug)
      p->Message(p, "esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d "
                    "0dbfs = %.1f\n",
                    p->esr, p->ekr, p->ksmps, p->nchnls, p->e0dbfs);
    if (O->sr_override) {        /* if command-line overrides, apply now */
      p->esr = (MYFLT) O->sr_override;
      p->ekr = (MYFLT) O->kr_override;
      p->ksmps = (int) ((ensmps = ((MYFLT) O->sr_override
                                   / (MYFLT) O->kr_override)) + FL(0.5));
      p->Message(p, Str("sample rate overrides: "
                        "esr = %7.1f, ekr = %7.1f, ksmps = %d\n"),
                    p->esr, p->ekr, p->ksmps);
    }
    /* number of MYFLT locations to allocate for a string variable */
    p->strVarSamples = (p->strVarMaxLen + (int) sizeof(MYFLT) - 1)
                       / (int) sizeof(MYFLT);
    p->strVarMaxLen = p->strVarSamples * (int) sizeof(MYFLT);
    /* calculate total size of global pool */
    combinedsize = p->poolcount                 /* floating point constants */
                   + p->gblfixed                /* k-rate / spectral        */
                   + p->gblacount * p->ksmps            /* a-rate variables */
                   + p->gblscount * p->strVarSamples;   /* string variables */
    gblscbeg = combinedsize + 1;                /* string constants         */
    combinedsize += create_strconst_ndx_list(p, &strConstIndexList, gblscbeg);

    combinedspc = (MYFLT*) mcalloc(p, combinedsize * sizeof(MYFLT));
    /* copy pool into combined space */
    memcpy(combinedspc, p->pool, p->poolcount * sizeof(MYFLT));
    mfree(p, (void*) p->pool);
    p->pool = combinedspc;
    gblspace = p->pool + p->poolcount;
    gblspace[0] = p->esr;           /*   & enter        */
    gblspace[1] = p->ekr;           /*   rsvd word      */
    gblspace[2] = (MYFLT) p->ksmps; /*   curr vals      */
    gblspace[3] = (MYFLT) p->nchnls;
    gblspace[4] = p->e0dbfs;
    p->gbloffbas = p->pool - 1;
    /* string constants: unquote, convert escape sequences, and copy to pool */
    convert_strconst_pool(p, (MYFLT*) p->gbloffbas + (long) gblscbeg);

    gblabeg = p->poolcount + p->gblfixed + 1;
    gblsbeg = gblabeg + p->gblacount;
    ip = &(p->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL) {      /* EXPAND NDX for A & S Cells */
      optxt = (OPTXT *) ip;                     /*   (and set localen)        */
      lclabeg = (long) (ip->pmax + ip->lclfixed + 1);
      lclsbeg = (long) (lclabeg + ip->lclacnt);
      if (O->odebug) p->Message(p, "lclabeg %ld, lclsbeg %ld\n",
                                   lclabeg, lclsbeg);
      ip->localen = ((long) ip->lclfixed
                     + (long) ip->lclacnt * (long) p->ksmps
                     + (long) ip->lclscnt * (long) p->strVarSamples)
                    * (long) sizeof(MYFLT);
      /* align to 64 bits */
      ip->localen = (ip->localen + 7L) & (~7L);
      for (insno=0, n=0; insno <= p->maxinsno; insno++)
        if (p->instrtxtp[insno] == ip)  n++;            /* count insnos  */
      lp = ip->inslist = (long *) mmalloc(p, (long)(n+1) * sizeof(long));
      for (insno=0; insno <= p->maxinsno; insno++)
        if (p->instrtxtp[insno] == ip)  *lp++ = insno;  /* creat inslist */
      *lp = -1;                                         /*   & terminate */
      insno = *ip->inslist;                             /* get the first */
      while ((optxt = optxt->nxtop) !=  NULL) {
        TEXT    *ttp = &optxt->t;
        ARGOFFS *aoffp;
        long    indx;
        long    posndx;
        int     *ndxp;
        int     opnum = ttp->opnum;
        if (opnum == ENDIN || opnum == ENDOP) break;    /* IV - Sep 8 2002 */
        if (opnum == LABEL) continue;
        aoffp = ttp->outoffs;           /* ------- OUTARGS -------- */
        n = aoffp->count;
        for (ndxp = aoffp->indx; n--; ndxp++) {
          indx = *ndxp;
          if (indx > 0) {               /* positive index: global   */
            if (indx >= STR_OFS)        /* string constant          */
              p->Die(p, Str("internal error: string constant outarg"));
            if (indx > gblsbeg)         /* global string variable   */
              indx = gblsbeg + (indx - gblsbeg) * p->strVarSamples;
            else if (indx > gblabeg)    /* global a-rate variable   */
              indx = gblabeg + (indx - gblabeg) * p->ksmps;
            else if (indx <= 3 && O->sr_override &&
                     ip == p->instxtanchor.nxtinstxt)   /* for instr 0 */
              indx += 3;        /* deflect any old sr,kr,ksmps targets */
          }
          else {                        /* negative index: local    */
            posndx = -indx;
            if (indx < LABELIM)         /* label                    */
              continue;
            if (posndx > lclsbeg)       /* local string variable    */
              indx = -(lclsbeg + (posndx - lclsbeg) * p->strVarSamples);
            else if (posndx > lclabeg)  /* local a-rate variable    */
              indx = -(lclabeg + (posndx - lclabeg) * p->ksmps);
          }
          *ndxp = (int) indx;
        }
        aoffp = ttp->inoffs;            /* inargs:                  */
        if (opnum >= SETEND) goto realops;
        switch (opnum) {                /*      do oload SETs NOW   */
        case PSET:
          p->Message(p, "PSET: isno=%ld, pmax=%d\n", insno, ip->pmax);
          if ((n = aoffp->count) != ip->pmax) {
            p->Warning(p, Str("i%d pset args != pmax"), (int) insno);
            if (n < ip->pmax) n = ip->pmax; /* cf pset, pmax    */
          }                                 /* alloc the larger */
          ip->psetdata = (MYFLT *) mcalloc(p, n * sizeof(MYFLT));
          for (n = aoffp->count, fp1 = ip->psetdata, ndxp = aoffp->indx;
               n--; ) {
            *fp1++ = p->gbloffbas[*ndxp++];
            p->Message(p, "..%f..", *(fp1-1));
          }
          p->Message(p, "\n");
          break;
        }
        continue;       /* no runtime role for the above SET types */

      realops:
        n = aoffp->count;               /* -------- INARGS -------- */
        for (ndxp = aoffp->indx; n--; ndxp++) {
          indx = *ndxp;
          if (indx > 0) {               /* positive index: global   */
            if (indx >= STR_OFS)        /* string constant          */
              indx = (long) strConstIndexList[indx - (long) (STR_OFS + 1)];
            else if (indx > gblsbeg)    /* global string variable   */
              indx = gblsbeg + (indx - gblsbeg) * p->strVarSamples;
            else if (indx > gblabeg)    /* global a-rate variable   */
              indx = gblabeg + (indx - gblabeg) * p->ksmps;
          }
          else {                        /* negative index: local    */
            posndx = -indx;
            if (indx < LABELIM)         /* label                    */
              continue;
            if (posndx > lclsbeg)       /* local string variable    */
              indx = -(lclsbeg + (posndx - lclsbeg) * p->strVarSamples);
            else if (posndx > lclabeg)  /* local a-rate variable    */
              indx = -(lclabeg + (posndx - lclabeg) * p->ksmps);
          }
          *ndxp = (int) indx;
        }
      }
    }
    p->Free(p, strConstIndexList);

    p->tpidsr = TWOPI_F / p->esr;               /* now set internal  */
    p->mtpdsr = -(p->tpidsr);                   /*    consts         */
    p->pidsr = PI_F / p->esr;
    p->mpidsr = -(p->pidsr);
    p->onedksmps = FL(1.0) / (MYFLT) p->ksmps;
    p->sicvt = FMAXLEN / p->esr;
    p->kicvt = FMAXLEN / p->ekr;
    p->onedsr = FL(1.0) / p->esr;
    p->onedkr = FL(1.0) / p->ekr;
    /* IV - Sep 8 2002: save global variables that depend on ksmps */
    p->global_ksmps     = p->ksmps;
    p->global_ekr       = p->ekr;
    p->global_kcounter  = p->kcounter;
    reverbinit(p);
    dbfs_init(p, p->e0dbfs);
    p->nspout = p->ksmps * p->nchnls;  /* alloc spin & spout */
    p->nspin = p->nspout;
    p->spin  = (MYFLT *) mcalloc(p, p->nspin * sizeof(MYFLT));
    p->spout = (MYFLT *) mcalloc(p, p->nspout * sizeof(MYFLT));
    /* chk consistency one more time (FIXME: needed ?) */
    {
      char  s[256];
      sprintf(s, Str("sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:"),
                 p->esr, p->ekr, ensmps);
      if (p->ksmps < 1 || FLOAT_COMPARE(ensmps, p->ksmps))
        csoundDie(p, Str("%s invalid ksmps value"), s);
      if (p->esr <= FL(0.0))
        csoundDie(p, Str("%s invalid sample rate"), s);
      if (p->ekr <= FL(0.0))
        csoundDie(p, Str("%s invalid control rate"), s);
      if (FLOAT_COMPARE(p->esr, (double) p->ekr * ensmps))
        csoundDie(p, Str("%s inconsistent sr, kr, ksmps"), s);
    }
    /* initialise sensevents state */
    p->prvbt = p->curbt = p->nxtbt = 0.0;
    p->curp2 = p->nxtim = p->timeOffs = p->beatOffs = 0.0;
    p->curTime = p->curBeat = 0.0;
    p->curTime_inc = 1.0 / (double) p->ekr;
    if (O->Beatmode && O->cmdTempo > 0) {
      /* if performing from beats, set the initial tempo */
      p->curBeat_inc = (double) O->cmdTempo / (60.0 * (double) p->ekr);
      p->beatTime = 60.0 / (double) O->cmdTempo;
    }
    else {
      p->curBeat_inc = 1.0 / (double) p->ekr;
      p->beatTime = 1.0;
    }
    p->cyclesRemaining = 0;
    memset(&(p->evt), 0, sizeof(EVTBLK));

    /* run instr 0 inits */
    if (init0(p) != 0)
      csoundDie(p, Str("header init errors"));
}

