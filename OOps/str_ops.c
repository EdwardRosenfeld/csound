/*
    str_ops.c:

    Copyright (C) 2005 Istvan Varga, Matt J. Ingalls, John ffitch

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

#include "csoundCore.h"
#define CSOUND_STR_OPS_C    1
#include "str_ops.h"
#include <ctype.h>

#define STRSMAX 8

#ifndef HAVE_SNPRINTF
/* add any compiler/system that has snprintf() */
#if defined(HAVE_C99)
#define HAVE_SNPRINTF   1
#endif
#endif

/* strset by John ffitch */

static void str_set(CSOUND *csound, int ndx, const char *s)
{
    if (csound->strsets == NULL) {
      csound->strsmax = STRSMAX;
      csound->strsets = (char **) csound->Calloc(csound, (csound->strsmax + 1)
                                                         * sizeof(char*));
    }
    if (ndx > (int) csound->strsmax) {
      int   i, newmax;
      /* assumes power of two STRSMAX */
      newmax = (ndx | (STRSMAX - 1)) + 1;
      csound->strsets = (char**) csound->ReAlloc(csound, csound->strsets,
                                                 (newmax + 1) * sizeof(char*));
      for (i = (csound->strsmax + 1); i <= newmax; i++)
        csound->strsets[i] = NULL;
      csound->strsmax = newmax;
    }
    if (ndx < 0)    /* -ve index */
      csound->Die(csound, Str("illegal strset index"));

    if (csound->strsets[ndx] != NULL) {
      if (strcmp(s, csound->strsets[ndx]) == 0)
        return;
      if (csound->oparms->msglevel & WARNMSG) {
        csound->Warning(csound, Str("strset index conflict at %d"), ndx);
        csound->Warning(csound, Str("previous value: '%s', replaced with '%s'"),
                                csound->strsets[ndx], s);
      }
      csound->Free(csound, csound->strsets[ndx]);
    }
    csound->strsets[ndx] = (char*) csound->Malloc(csound, strlen(s) + 1);
    strcpy(csound->strsets[ndx], s);
    if ((csound->oparms->msglevel & 7) == 7)
      csound->Message(csound, "Strsets[%d]: '%s'\n", ndx, s);
}

int strset_init(CSOUND *csound, STRSET_OP *p)
{
    str_set(csound, (int) MYFLT2LRND(*p->indx), (char*) p->str);
    return OK;
}

/* for argdecode.c */

void strset_option(CSOUND *csound, char *s)
{
    int indx = 0;

    if (!isdigit(*s))
      csound->Die(csound, Str("--strset: invalid format"));
    do {
      indx = (indx * 10) + (int) (*s++ - '0');
    } while (isdigit(*s));
    if (*s++ != '=')
      csound->Die(csound, Str("--strset: invalid format"));
    str_set(csound, indx, s);
}

int strget_init(CSOUND *csound, STRGET_OP *p)
{
    int   indx;

    ((char*) p->r)[0] = '\0';
    if (*(p->indx) == SSTRCOD) {
      if (csound->currevent->strarg == NULL)
        return OK;
      if ((int) strlen(csound->currevent->strarg) >= csound->strVarMaxLen)
        return csound->InitError(csound, Str("strget: buffer overflow"));
      strcpy((char*) p->r, csound->currevent->strarg);
      return OK;
    }
    indx = (int) ((double) *(p->indx) + (*(p->indx) >= FL(0.0) ? 0.5 : -0.5));
    if (indx < 0 || indx > (int) csound->strsmax ||
        csound->strsets == NULL || csound->strsets[indx] == NULL)
      return OK;
    if ((int) strlen(csound->strsets[indx]) >= csound->strVarMaxLen)
      return csound->InitError(csound, Str("strget: buffer overflow"));
    strcpy((char*) p->r, csound->strsets[indx]);
    return OK;
}

/* strcpy */

int strcpy_opcode_init(CSOUND *csound, STRCPY_OP *p)
{
    char  *newVal = (char*) p->str;

    if (p->r == p->str)
      return OK;
    if ((int) strlen(newVal) >= csound->strVarMaxLen)
      return csound->InitError(csound, Str("strcpy: buffer overflow"));
    strcpy((char*) p->r, newVal);
    return OK;
}

int strcpy_opcode_perf(CSOUND *csound, STRCPY_OP *p)
{
    char  *newVal = (char*) p->str;

    if (p->r == p->str)
      return OK;
    if ((int) strlen(newVal) >= csound->strVarMaxLen)
      return csound->PerfError(csound, Str("strcpy: buffer overflow"));
    strcpy((char*) p->r, newVal);
    return OK;
}

/* strcat */

int strcat_opcode_init(CSOUND *csound, STRCAT_OP *p)
{
    char  *newVal1 = (char*) p->str1;
    char  *newVal2 = (char*) p->str2;

    if ((int) (strlen(newVal1) + strlen(newVal2)) >= csound->strVarMaxLen)
      return csound->InitError(csound, Str("strcat: buffer overflow"));
    if (p->r != p->str2) {
      if (p->r != p->str1)
        strcpy((char*) p->r, newVal1);
      strcat((char*) p->r, newVal2);
      return OK;
    }
    if (newVal1[0] == '\0')
      return OK;
    memmove(newVal2 + strlen(newVal1), newVal2, strlen(newVal2) + 1);
    if (p->r != p->str1)
      memcpy(newVal2, newVal1, strlen(newVal1));
    return OK;
}

int strcat_opcode_perf(CSOUND *csound, STRCAT_OP *p)
{
    char  *newVal1 = (char*) p->str1;
    char  *newVal2 = (char*) p->str2;

    if ((int) (strlen(newVal1) + strlen(newVal2)) >= csound->strVarMaxLen)
      return csound->PerfError(csound, Str("strcat: buffer overflow"));
    if (p->r != p->str2) {
      if (p->r != p->str1)
        strcpy((char*) p->r, newVal1);
      strcat((char*) p->r, newVal2);
      return OK;
    }
    if (newVal1[0] == '\0')
      return OK;
    memmove(newVal2 + strlen(newVal1), newVal2, strlen(newVal2) + 1);
    if (p->r != p->str1)
      memcpy(newVal2, newVal1, strlen(newVal1));
    return OK;
}

/* strcmp */

int strcmp_opcode(CSOUND *csound, STRCAT_OP *p)
{
    int i;

    *(p->r) = FL(0.0);
    if (p->str1 == p->str2)
      return OK;
    i = strcmp((char*) p->str1, (char*) p->str2);
    if (i < 0)
      *(p->r) = FL(-1.0);
    else if (i > 0)
      *(p->r) = FL(1.0);
    return OK;
}

/* perform a sprintf-style format -- based on code by Matt J. Ingalls */

static CS_NOINLINE int
    sprintf_opcode(CSOUND *csound,
                   void *p,          /* opcode data structure pointer       */
                   char *dst,        /* pointer to space for output string  */
                   const char *fmt,  /* format string                       */
                   MYFLT **kvals,    /* array of argument pointers          */
                   int numVals,      /* number of arguments                 */
                   int strCode,      /* bit mask for string arguments       */
                   int maxLen,       /* available space in output buffer    */
                   int (*err_func)(CSOUND *csound, const char *msg, ...))
{
    int     len = 0;
    char    strseg[2048], *outstring = dst, *opname = csound->GetOpcodeName(p);
    MYFLT   *pp = NULL;
    int     i = 0, j = 0, n;
    const char  *segwaiting = NULL;
    int     maxChars;

    if ((int) ((OPDS*) p)->optext->t.xincod != 0)
      return err_func(csound, Str("%s: a-rate argument not allowed"), opname);
    if ((int) ((OPDS*) p)->optext->t.inoffs->count > 31)
      csound->Die(csound, Str("%s: too many arguments"), opname);

    while (1) {
      if (i >= 2047) {
        return err_func(csound, Str("%s: format string too long"), opname);
      }
      if (*fmt != '%' && *fmt != '\0') {
        strseg[i++] = *fmt++;
        continue;
      }
      if (fmt[0] == '%' && fmt[1] == '%') {
        strseg[i++] = *fmt++;
        strseg[i++] = *fmt++;
        continue;
      }
      /* if already a segment waiting, then lets print it */
      if (segwaiting != NULL) {
        maxChars = maxLen - len;
        strseg[i] = '\0';
        if (numVals <= 0) {
          return err_func(csound, Str("%s: insufficient arguments for format"),
                                  opname);
        }
        numVals--;
        if ((*segwaiting == 's' && !(strCode & 1)) ||
            (*segwaiting != 's' && (strCode & 1))) {
          return err_func(csound,
                          Str("%s: argument type inconsistent with format"),
                          opname);
        }
        strCode >>= 1;
        pp = kvals[j++];
        switch (*segwaiting) {
        case 'd':
        case 'i':
        case 'o':
        case 'x':
        case 'X':
        case 'u':
        case 'c':
#ifdef HAVE_SNPRINTF
          n = snprintf(outstring, maxChars, strseg, (int) MYFLT2LRND(*pp));
#else
          n = sprintf(outstring, strseg, (int) MYFLT2LRND(*pp));
#endif
          break;
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
#ifdef HAVE_SNPRINTF
          n = snprintf(outstring, maxChars, strseg, (double) *pp);
#else
          n = sprintf(outstring, strseg, (double) *pp);
#endif
          break;
        case 's':
          if ((char*) pp == dst) {
            return err_func(csound, Str("%s: output argument may not be "
                                        "the same as any of the input args"),
                                    opname);
          }
#ifdef HAVE_SNPRINTF
          n = snprintf(outstring, maxChars, strseg, (char*) pp);
#else
          n = sprintf(outstring, strseg, (char*) pp);
#endif
          break;
        default:
          return err_func(csound, Str("%s: invalid format string"), opname);
        }
        if (n < 0 || n >= maxChars) {
#ifdef HAVE_SNPRINTF
          /* safely detected excess string length */
          return err_func(csound, Str("%s: buffer overflow"), opname);
#else
          /* wrote past end of buffer - hope that did not already crash ! */
          csound->Die(csound, Str("%s: buffer overflow"), opname);
#endif
        }
        outstring += n;
        len += n;
        i = 0;
      }
      if (*fmt == '\0')
        break;
      /* copy the '%' */
      strseg[i++] = *fmt++;
      /* find the format code */
      segwaiting = fmt;
      while (!isalpha(*segwaiting) && *segwaiting != '\0')
        segwaiting++;
    }
    if (numVals > 0) {
      return err_func(csound, Str("%s: too many arguments for format"), opname);
    }
    return 0;
}

int sprintf_opcode_init(CSOUND *csound, SPRINTF_OP *p)
{
    if (sprintf_opcode(csound, p, (char*) p->r, (char*) p->sfmt, &(p->args[0]),
                               (int) p->INOCOUNT - 1, ((int) p->XSTRCODE >> 1),
                               csound->strVarMaxLen, csound->InitError) != 0) {
      ((char*) p->r)[0] = '\0';
      return NOTOK;
    }
    return OK;
}

int sprintf_opcode_perf(CSOUND *csound, SPRINTF_OP *p)
{
    if (sprintf_opcode(csound, p, (char*) p->r, (char*) p->sfmt, &(p->args[0]),
                               (int) p->INOCOUNT - 1, ((int) p->XSTRCODE >> 1),
                               csound->strVarMaxLen, csound->PerfError) != 0) {
      ((char*) p->r)[0] = '\0';
      return NOTOK;
    }
    return OK;
}

static CS_NOINLINE int
    printf_opcode_(CSOUND *csound, PRINTF_OP *p,
                                    int (*err_func)(CSOUND*, const char*, ...))
{
    char  buf[3072];
    int   err;
    err = sprintf_opcode(csound,
                         p, buf, (char*) p->sfmt, &(p->args[0]),
                         (int) p->INOCOUNT - 2, ((int) p->XSTRCODE >> 2),
                         3072, err_func);
    if (err == OK)
      csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", buf);
    return err;
}

int printf_opcode_init(CSOUND *csound, PRINTF_OP *p)
{
    if (*p->ktrig > FL(0.0))
      return (printf_opcode_(csound, p, csound->InitError));
    return OK;
}

int printf_opcode_set(CSOUND *csound, PRINTF_OP *p)
{
    p->prv_ktrig = FL(0.0);
    return OK;
}

int printf_opcode_perf(CSOUND *csound, PRINTF_OP *p)
{
    if (*p->ktrig == p->prv_ktrig)
      return OK;
    p->prv_ktrig = *p->ktrig;
    if (p->prv_ktrig > FL(0.0))
      return (printf_opcode_(csound, p, csound->PerfError));
    return OK;
}

int puts_opcode_init(CSOUND *csound, PUTS_OP *p)
{
    p->noNewLine = (*p->no_newline == FL(0.0) ? 0 : 1);
    if (*p->ktrig > FL(0.0)) {
      if (!p->noNewLine)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*) p->str);
      else
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", (char*) p->str);
    }
    p->prv_ktrig = *p->ktrig;
    return OK;
}

int puts_opcode_perf(CSOUND *csound, PUTS_OP *p)
{
    if (*p->ktrig != p->prv_ktrig && *p->ktrig > FL(0.0)) {
      p->prv_ktrig = *p->ktrig;
      if (!p->noNewLine)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*) p->str);
      else
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", (char*) p->str);
    }
    return OK;
}

static int strtod_opcode(CSOUND *csound, STRSET_OP *p,
                         int (*err_func)(CSOUND*, const char*, ...))
{
    char    *s = NULL, *tmp;
    double  x;

    if (p->XSTRCODE)
      s = (char*) p->str;
    else {
      if (*p->str == SSTRCOD)
        s = csound->currevent->strarg;
      else {
        int ndx = (int) MYFLT2LRND(*p->str);
        if (ndx >= 0 && ndx <= (int) csound->strsmax && csound->strsets != NULL)
          s = csound->strsets[ndx];
      }
      if (s == NULL)
        return err_func(csound, Str("strtod: empty string"));
    }
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0')
      return err_func(csound, Str("strtod: empty string"));
    x = strtod(s, &tmp);
    if (*tmp != '\0')
      return err_func(csound, Str("strtod: invalid format"));
    *p->indx = (MYFLT) x;
    return OK;
}

int strtod_opcode_init(CSOUND *csound, STRSET_OP *p)
{
    return strtod_opcode(csound, p, csound->InitError);
}

int strtod_opcode_perf(CSOUND *csound, STRSET_OP *p)
{
    return strtod_opcode(csound, p, csound->PerfError);
}

static int strtol_opcode(CSOUND *csound, STRSET_OP *p,
                         int (*err_func)(CSOUND*, const char*, ...))
{
    char  *s = NULL;
    int   sgn = 0, radix = 10;
    long  x = 0L;

    if (p->XSTRCODE)
      s = (char*) p->str;
    else {
      if (*p->str == SSTRCOD)
        s = csound->currevent->strarg;
      else {
        int ndx = (int) MYFLT2LRND(*p->str);
        if (ndx >= 0 && ndx <= (int) csound->strsmax && csound->strsets != NULL)
          s = csound->strsets[ndx];
      }
      if (s == NULL)
        return err_func(csound, Str("strtol: empty string"));
    }
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0')
      return err_func(csound, Str("strtol: empty string"));
    if (*s == '+') s++;
    else if (*s == '-') sgn++, s++;
    if (*s == '0') {
      if (s[1] == 'x' || s[1] == 'X')
        radix = 16, s += 2;
      else if (s[1] != '\0')
        radix = 8, s++;
      else {
        *p->indx = FL(0.0);
        return OK;
      }
    }
    if (*s == '\0')
      return err_func(csound, Str("strtol: invalid format"));
    switch (radix) {
      case 8:
        while (*s >= '0' && *s <= '7') x = (x * 8L) + (long) (*s++ - '0');
        break;
      case 10:
        while (*s >= '0' && *s <= '9') x = (x * 10L) + (long) (*s++ - '0');
        break;
      default:
        while (1) {
          if (*s >= '0' && *s <= '9')
            x = (x * 16L) + (long) (*s++ - '0');
          else if (*s >= 'A' && *s <= 'F')
            x = (x * 16L) + (long) (*s++ - 'A') + 10L;
          else if (*s >= 'a' && *s <= 'f')
            x = (x * 16L) + (long) (*s++ - 'a') + 10L;
          else
            break;
        }
    }
    if (*s != '\0')
      return err_func(csound, Str("strtol: invalid format"));
    if (sgn) x = -x;
    *p->indx = (MYFLT) x;
    return OK;
}

int strtol_opcode_init(CSOUND *csound, STRSET_OP *p)
{
    return strtol_opcode(csound, p, csound->InitError);
}

int strtol_opcode_perf(CSOUND *csound, STRSET_OP *p)
{
    return strtol_opcode(csound, p, csound->PerfError);
}

extern int create_new_channel(CSOUND *, MYFLT **p, const char *name, int type);

/* perf time stub for printing "not initialised" error message */

int notinit_opcode_stub(CSOUND *csound, void *p)
{
    return csound->PerfError(csound, Str("%s: not initialised"),
                                     csound->GetOpcodeName(p));
}

/* print error message on failed channel query */

static CS_NOINLINE int print_chn_err(void *p, int err)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *msg;

    if (((OPDS*) p)->opadr != (SUBR) NULL)
      ((OPDS*) p)->opadr = (SUBR) notinit_opcode_stub;
    if (err == CSOUND_MEMORY)
      msg = "memory allocation failure";
    else if (err < 0)
      msg = "invalid channel name";
    else
      msg = "channel already exists with incompatible type";
    return csound->InitError(csound, Str(msg));
}

/* receive control value from bus at performance time */

static int chnget_opcode_perf_k(CSOUND *csound, CHNGET *p)
{
    *(p->arg) = *(p->fp);
    return OK;
}

/* receive audio data from bus at performance time */

static int chnget_opcode_perf_a(CSOUND *csound, CHNGET *p)
{
    int   i = 0;

    do {
      p->arg[i] = p->fp[i];
    } while (++i < csound->ksmps);
    return OK;
}

/* receive control value from bus at init time */

int chnget_opcode_init_i(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (err)
      return print_chn_err(p, err);
    *(p->arg) = *(p->fp);
    return OK;
}

/* init routine for chnget opcode (control data) */

int chnget_opcode_init_k(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (!err) {
      p->h.opadr = (SUBR) chnget_opcode_perf_k;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnget opcode (audio data) */

int chnget_opcode_init_a(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (!err) {
      p->h.opadr = (SUBR) chnget_opcode_perf_a;
      return OK;
    }
    return print_chn_err(p, err);
}

/* receive string value from bus at init time */

int chnget_opcode_init_S(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (err)
      return print_chn_err(p, err);
    strcpy((char*) p->arg, (char*) p->fp);
    return OK;
}

/* send control value to bus at performance time */

static int chnset_opcode_perf_k(CSOUND *csound, CHNGET *p)
{
    *(p->fp) = *(p->arg);
    return OK;
}

/* send audio data to bus at performance time */

static int chnset_opcode_perf_a(CSOUND *csound, CHNGET *p)
{
    int   i = 0;

    do {
      p->fp[i] = p->arg[i];
    } while (++i < csound->ksmps);
    return OK;
}

/* send audio data to bus at performance time, mixing to previous output */

static int chnmix_opcode_perf(CSOUND *csound, CHNGET *p)
{
    int   i = 0;

    do {
      p->fp[i] += p->arg[i];
    } while (++i < csound->ksmps);
    return OK;
}

/* clear an audio channel to zero at performance time */

static int chnclear_opcode_perf(CSOUND *csound, CHN_OPCODE *p)
{
    int   i = 0;

    do {
      /* NOTE: p->imode is a pointer to the channel data here */
      p->imode[i] = FL(0.0);
    } while (++i < csound->ksmps);
    return OK;
}

/* send control value to bus at init time */

int chnset_opcode_init_i(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (err)
      return print_chn_err(p, err);
    *(p->fp) = *(p->arg);
    return OK;
}

/* init routine for chnset opcode (control data) */

int chnset_opcode_init_k(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (!err) {
      p->h.opadr = (SUBR) chnset_opcode_perf_k;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnset opcode (audio data) */

int chnset_opcode_init_a(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (!err) {
      p->h.opadr = (SUBR) chnset_opcode_perf_a;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnmix opcode */

int chnmix_opcode_init(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (!err) {
      p->h.opadr = (SUBR) chnmix_opcode_perf;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnclear opcode */

int chnclear_opcode_init(CSOUND *csound, CHN_OPCODE *p)
{
    int   err;

    /* NOTE: p->imode is a pointer to the channel data here */
    err = csoundGetChannelPtr(csound, &(p->imode), (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (!err) {
      p->h.opadr = (SUBR) chnclear_opcode_perf;
      return OK;
    }
    return print_chn_err(p, err);
}

/* send string to bus at init time */

int chnset_opcode_init_S(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (err)
      return print_chn_err(p, err);
    if ((int) strlen((char*) p->arg) >= csound->strVarMaxLen) {
      /* can only happen with constants */
      return csound->InitError(csound, Str("string is too long"));
    }
    strcpy((char*) p->fp, (char*) p->arg);
    return OK;
}

/* declare control channel, optionally with special parameters */

int chn_k_opcode_init(CSOUND *csound, CHN_OPCODE_K *p)
{
    MYFLT *dummy;
    int   type, mode, err;

    mode = (int) MYFLT2LRND(*(p->imode));
    if (mode < 1 || mode > 3)
      return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_CONTROL_CHANNEL;
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, type);
    if (err)
      return print_chn_err(p, err);
    type = (int) MYFLT2LRND(*(p->itype));
    err = csoundSetControlChannelParams(csound, (char*) p->iname, type,
                                        *(p->idflt), *(p->imin), *(p->imax));
    if (!err)
      return OK;
    if (err == CSOUND_MEMORY)
      return print_chn_err(p, err);
    return csound->InitError(csound, Str("invalid channel parameters"));
}

/* declare audio channel */

int chn_a_opcode_init(CSOUND *csound, CHN_OPCODE *p)
{
    MYFLT *dummy;
    int   type, mode, err;

    mode = (int) MYFLT2LRND(*(p->imode));
    if (mode < 1 || mode > 3)
      return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_AUDIO_CHANNEL;
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, type);
    if (err)
      return print_chn_err(p, err);
    return OK;
}

/* declare string channel */

int chn_S_opcode_init(CSOUND *csound, CHN_OPCODE *p)
{
    MYFLT *dummy;
    int   type, mode, err;

    mode = (int) MYFLT2LRND(*(p->imode));
    if (mode < 1 || mode > 3)
      return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_STRING_CHANNEL;
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, type);
    if (err)
      return print_chn_err(p, err);
    return OK;
}

/* export new channel from global orchestra variable */

int chnexport_opcode_init(CSOUND *csound, CHNEXPORT_OPCODE *p)
{
    MYFLT       *dummy;
    const char  *argName;
    int         type = CSOUND_CONTROL_CHANNEL, mode, err;

    /* must have an output argument of type 'gi', 'gk', 'ga', or 'gS' */
    if (csound->GetOutputArgCnt(p) != 1)
      goto arg_err;
    argName = csound->GetOutputArgName(p, 0);
    if (argName == NULL)
      goto arg_err;
    if (argName[0] != 'g')
      goto arg_err;
    switch ((int) argName[1]) {
    case 'i':
    case 'k':
      break;
    case 'a':
      type = CSOUND_AUDIO_CHANNEL;
      break;
    case 'S':
      type = CSOUND_STRING_CHANNEL;
      break;
    default:
      goto arg_err;
    }
    /* mode (input and/or output) */
    mode = (int) MYFLT2LRND(*(p->imode));
    if (mode < 1 || mode > 3)
      return csound->InitError(csound, Str("invalid mode parameter"));
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;
    /* check if the channel already exists (it should not) */
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, 0);
    if (err >= 0)
      return csound->InitError(csound, Str("channel already exists"));
    /* now create new channel, using output variable for data storage */
    dummy = p->arg;
    err = create_new_channel(csound, &dummy, (char*) p->iname, type);
    if (err)
      return print_chn_err(p, err);
    /* if control channel, set additional parameters */
    if ((type & CSOUND_CHANNEL_TYPE_MASK) != CSOUND_CONTROL_CHANNEL)
      return OK;
    type = (int) MYFLT2LRND(*(p->itype));
    err = csoundSetControlChannelParams(csound, (char*) p->iname, type,
                                        *(p->idflt), *(p->imin), *(p->imax));
    if (!err)
      return OK;
    if (err == CSOUND_MEMORY)
      return print_chn_err(p, err);
    return csound->InitError(csound, Str("invalid channel parameters"));

 arg_err:
    return csound->InitError(csound, Str("invalid export variable"));
}

/* returns all parameters of a channel */

int chnparams_opcode_init(CSOUND *csound, CHNPARAMS_OPCODE *p)
{
    MYFLT *dummy;
    int   err;

    /* all values default to zero... */
    *(p->itype) = FL(0.0);
    *(p->imode) = FL(0.0);
    *(p->ictltype) = FL(0.0);
    *(p->idflt) = FL(0.0);
    *(p->imin) = FL(0.0);
    *(p->imax) = FL(0.0);
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, 0);
    /* ...if channel does not exist */
    if (err <= 0)
      return OK;
    /* type (control/audio/string) */
    *(p->itype) = (MYFLT) (err & 15);
    /* mode (input and/or output) */
    *(p->imode) = (MYFLT) ((err & 48) >> 4);
    /* check for control channel parameters */
    if ((err & 15) == CSOUND_CONTROL_CHANNEL) {
      err = csoundGetControlChannelParams(csound, (char*) p->iname,
                                          p->idflt, p->imin, p->imax);
      if (err > 0)
        *(p->ictltype) = (MYFLT) err;
    }
    return OK;
}

