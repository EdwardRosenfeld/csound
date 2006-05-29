/*
    ugens3.c:

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

#include "csoundCore.h"         /*                              UGENS3.C    */
#include "ugens3.h"
#include <math.h>
#include "oload.h"

int foscset(CSOUND *csound, FOSC *p)
{
    FUNC    *ftp;

    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
      if (*p->iphs >= 0)
        p->cphs = p->mphs = (long)(*p->iphs * FMAXLEN);
      p->ampcod = (XINARG1) ? 1 : 0;
      p->carcod = (XINARG3) ? 1 : 0;
      p->modcod = (XINARG4) ? 1 : 0;
      return OK;
    }
    return NOTOK;
}

int foscil(CSOUND *csound, FOSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar, *ampp, *modp, cps, amp;
    MYFLT   xcar, xmod, *carp, car, fmod, cfreq, mod, ndx, *ftab;
    long    mphs, cphs, minc, cinc, lobits;
    int     n;

    ar = p->rslt;
    ftp = p->ftp;
    if (ftp == NULL) {
      return csound->PerfError(csound, Str("foscil: not initialised"));
    }
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    mphs = p->mphs;
    cphs = p->cphs;
    ampp = p->xamp;
    cps  = *p->kcps;
    carp = p->xcar;
    modp = p->xmod;
    amp  = *ampp;
    xcar = *carp;
    xmod = *modp;

    if (p->XINCODE) {
      for (n=0;n<csound->ksmps;n++) {
        if (p->ampcod) amp = ampp[n];
        if (p->carcod) xcar = carp[n];
        if (p->modcod) xmod = modp[n];
        car = cps * xcar;
        mod = cps * xmod;
        ndx = *p->kndx * mod;
        minc = (long)(mod * csound->sicvt);
        mphs &= PHMASK;
        fmod = *(ftab + (mphs >>lobits)) * ndx;
        mphs += minc;
        cfreq = car + fmod;
        cinc = (long)(cfreq * csound->sicvt);
        cphs &= PHMASK;
        ar[n] = *(ftab + (cphs >>lobits)) * amp;
        cphs += cinc;
      }
    }
    else {
      MYFLT amp = *ampp;
      cps = *p->kcps;
      car = cps * *carp;
      mod = cps * *modp;
      ndx = *p->kndx * mod;
      minc = (long)(mod * csound->sicvt);
      for (n=0;n<csound->ksmps;n++) {
        mphs &= PHMASK;
        fmod = *(ftab + (mphs >>lobits)) * ndx;
        mphs += minc;
        cfreq = car + fmod;
        cinc = (long)(cfreq * csound->sicvt);
        cphs &= PHMASK;
        ar[n] = *(ftab + (cphs >>lobits)) * amp;
        cphs += cinc;
      }
    }
    p->mphs = mphs;
    p->cphs = cphs;

    return OK;
}

int foscili(CSOUND *csound, FOSC *p)
{
    FUNC   *ftp;
    MYFLT  *ar, *ampp, amp, cps, fract, v1, car, fmod, cfreq, mod;
    MYFLT  *carp, *modp, xcar, xmod, ndx, *ftab;
    long   mphs, cphs, minc, cinc, lobits;
    int    n;

    ar = p->rslt;
    ftp = p->ftp;
    if (ftp == NULL) {        /* RWD fix */
      return csound->PerfError(csound, Str("foscili: not initialised"));
    }
    lobits = ftp->lobits;
    mphs = p->mphs;
    cphs = p->cphs;
    ampp = p->xamp;
    cps  = *p->kcps;
    carp = p->xcar;
    modp = p->xmod;
    amp  = *ampp;
    xcar = *carp;
    xmod = *modp;
    if (p->XINCODE) {
      for (n=0;n<csound->ksmps;n++) {
        if (p->ampcod)  amp = ampp[n];
        if (p->carcod)  xcar = carp[n];
        if (p->modcod)  xmod = modp[n];
        car = cps * xcar;
        mod = cps * xmod;
        ndx = *p->kndx * mod;
        minc = (long)(mod * csound->sicvt);
        mphs &= PHMASK;
        fract = PFRAC(mphs);
        ftab = ftp->ftable + (mphs >>lobits);
        v1 = *ftab++;
        fmod = (v1 + (*ftab - v1) * fract) * ndx;
        mphs += minc;
        cfreq = car + fmod;
        cinc = (long)(cfreq * csound->sicvt);
        cphs &= PHMASK;
        fract = PFRAC(cphs);
        ftab = ftp->ftable + (cphs >>lobits);
        v1 = *ftab++;
        ar[n] = (v1 + (*ftab - v1) * fract) * amp;
        cphs += cinc;
      }
    }
    else {
      cps = *p->kcps;
      car = cps * *carp;
      mod = cps * *modp;
      ndx = *p->kndx * mod;
      minc = (long)(mod * csound->sicvt);
      for (n=0;n<csound->ksmps;n++) {
        mphs &= PHMASK;
        fract = PFRAC(mphs);
        ftab = ftp->ftable + (mphs >>lobits);
        v1 = *ftab++;
        fmod = (v1 + (*ftab - v1) * fract) * ndx;
        mphs += minc;
        cfreq = car + fmod;
        cinc = (long)(cfreq * csound->sicvt);
        cphs &= PHMASK;
        fract = PFRAC(cphs);
        ftab = ftp->ftable + (cphs >>lobits);
        v1 = *ftab++;
        ar[n] = (v1 + (*ftab - v1) * fract) * amp;
        cphs += cinc;
      }
    }
    p->mphs = mphs;
    p->cphs = cphs;

    return OK;
}

int losset(CSOUND *csound, LOSC *p)
{
    FUNC    *ftp;

    if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) {
      long  maxphs = ((long) ftp->flenfrms << LOBITS) + ((long) LOFACT - 1);
      p->ftp = ftp;
      if (*p->ibas != FL(0.0))
        p->cpscvt = ftp->cvtbas / *p->ibas;
      else if ((p->cpscvt = ftp->cpscvt) == FL(0.0)) {
        p->cpscvt = FL(261.62561); /* Middle C */
        if (csound->oparms->msglevel & WARNMSG)
          csound->Warning(csound, Str("no legal base frequency"));
      }
      if ((p->mod1 = (short) *p->imod1) < 0) {
        if ((p->mod1 = ftp->loopmode1) == 0) {
          if (csound->oparms->msglevel & WARNMSG)
            csound->Warning(csound, Str("locscil: sustain defers to "
                                        "non-looping source"));
        }
        p->beg1 = ftp->begin1 << LOBITS;
        p->end1 = ftp->end1 << LOBITS;
      }
      else if (p->mod1 < 0 || p->mod1 > 3)
        goto lerr2;
      else {
        p->beg1 = (long) (*p->ibeg1 * (MYFLT) LOFACT);
        p->end1 = (long) (*p->iend1 * (MYFLT) LOFACT);
        if (!p->beg1 && !p->end1) {
          /* default to looping the whole sample */
          p->end1 = (p->mod1 ? maxphs : ((long) ftp->flenfrms << LOBITS));
        }
        else if (p->beg1 < 0 || p->end1 > maxphs || p->beg1 >= p->end1)
          goto lerr2;
      }
      if ((p->mod2 = (short) *p->imod2) < 0) {
        p->mod2 = ftp->loopmode2;
        p->beg2 = ftp->begin2 << LOBITS;
        p->end2 = ftp->end2 << LOBITS;
      }
      else {
        p->beg2 = (long) (*p->ibeg2 * (MYFLT) LOFACT);
        p->end2 = (long) (*p->iend2 * (MYFLT) LOFACT);
        if (p->mod2 < 0 || p->mod2 > 3 ||
            p->beg2 < 0 || p->end2 > maxphs || p->beg2 >= p->end2)
          goto lerr3;
      }
      p->beg1 = (p->beg1 >= 0L ? p->beg1 : 0L);
      p->end1 = (p->end1 < maxphs ? p->end1 : maxphs);
      if (p->beg1 >= p->end1) {
        p->mod1 = 0;
        p->beg1 = 0L;
        p->end1 = maxphs;
      }
      p->beg2 = (p->beg2 >= 0L ? p->beg2 : 0L);
      p->end2 = (p->end2 < maxphs ? p->end2 : maxphs);
      if (p->beg2 >= p->end2) {
        p->mod2 = 0;
        p->beg2 = 0L;
      }
      if (!p->mod2 && !p->end2)       /* if no release looping */
        p->end2 = maxphs;             /*   set a reading limit */
      p->lphs = 0;
      p->seg1 = 1;
      if ((p->curmod = p->mod1))
        p->looping = 1;
      else p->looping = 0;
      if (p->OUTOCOUNT == 1) {
        p->stereo = 0;
        if (ftp->nchanls != 1)
          return csound->InitError(csound, Str(
                               "mono loscil cannot read from stereo ftable"));
      }
      else {
        p->stereo = 1;
        if (ftp->nchanls != 2)
          return csound->InitError(csound, Str(
                               "stereo loscil cannot read from mono ftable"));
      }
      return OK;
    }
    return NOTOK;

 lerr2:
    return csound->InitError(csound, Str("illegal sustain loop data"));
 lerr3:
    return csound->InitError(csound, Str("illegal release loop data"));
}

static inline void loscil_linear_interp_mono(MYFLT *ar,
                                             MYFLT *ftbl, long phs, long flen)
{
    MYFLT   fract, tmp;
    int     x;

    fract = (MYFLT) ((int) phs & LOMASK) * LOSCAL;
    x = (int) (phs >> LOBITS);
    tmp = ftbl[x];
    x = (x < (int) flen ? (x + 1) : (int) flen);
    *ar = tmp + ((ftbl[x] - tmp) * fract);
}

static inline void loscil_linear_interp_stereo(MYFLT *arL, MYFLT *arR,
                                               MYFLT *ftbl, long phs, long flen)
{
    MYFLT   fract, tmpL, tmpR;
    int     x;

    fract = (MYFLT) ((int) phs & LOMASK) * LOSCAL;
    x = (int) (phs >> LOBITS) << 1;
    tmpL = ftbl[x];
    tmpR = ftbl[x + 1];
    x = (x < ((int) flen - 1) ? (x + 2) : ((int) flen - 1));
    *arL = tmpL + ((ftbl[x] - tmpL) * fract);
    *arR = tmpR + ((ftbl[x + 1] - tmpR) * fract);
}

static inline void loscil_cubic_interp_mono(MYFLT *ar,
                                            MYFLT *ftbl, long phs, long flen)
{
    MYFLT   fract, tmp, a0, a1, a2, a3;
    int     x;

    fract = (MYFLT) ((int) phs & LOMASK) * LOSCAL;
    a3 = fract * fract; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
    a2 = fract; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
    a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= fract;
    a0 *= fract; a1 *= fract; a2 *= fract; a3 *= fract; a1 += FL(1.0);
    x = (int) (phs >> LOBITS) - 1;
    tmp = ftbl[(x >= 0 ? x : 0)] * a0;
    tmp += ftbl[++x] * a1;
    x++;
    tmp += ftbl[(x < (int) flen ? x : (int) flen)] * a2;
    x++;
    tmp += ftbl[(x < (int) flen ? x : (int) flen)] * a3;
    *ar = tmp;
}

static CS_NOINLINE void
    loscil_cubic_interp_stereo(MYFLT *arL, MYFLT *arR,
                               MYFLT *ftbl, long phs, long flen)
{
    MYFLT   fract, tmpL, tmpR, a0, a1, a2, a3;
    int     x;

    fract = (MYFLT) ((int) phs & LOMASK) * LOSCAL;
    a3 = fract * fract; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
    a2 = fract; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
    a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= fract;
    a0 *= fract; a1 *= fract; a2 *= fract; a3 *= fract; a1 += FL(1.0);
    x = ((int) (phs >> LOBITS) << 1) - 2;
    tmpL = ftbl[(x >= 0 ? x : 0)] * a0;
    tmpR = ftbl[(x >= 0 ? (x + 1) : 1)] * a0;
    x += 2;
    tmpL += ftbl[x] * a1;
    tmpR += ftbl[x + 1] * a1;
    x = (x < ((int) flen - 1) ? (x + 2) : ((int) flen - 1));
    tmpL += ftbl[x] * a2;
    tmpR += ftbl[x + 1] * a2;
    x = (x < ((int) flen - 1) ? (x + 2) : ((int) flen - 1));
    tmpL += ftbl[x] * a3;
    tmpR += ftbl[x + 1] * a3;
    *arL = tmpL;
    *arR = tmpR;
}

int loscil(CSOUND *csound, LOSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar1, *ar2, *ftbl, *xamp;
    long    phs, inc, beg, end;
    int     nsmps = csound->ksmps, aamp;

    ftp = p->ftp;
    ftbl = ftp->ftable;
    if ((inc = (long)(*p->kcps * p->cpscvt)) < 0)
      inc = -inc;
    xamp = p->xamp;
    aamp = (p->XINCODE) ? 1 : 0;
    if (p->seg1) {                      /* if still segment 1  */
      beg = p->beg1;
      end = p->end1;
      if (p->h.insdshead->relesing)     /*    sense note_off   */
        p->looping = 0;
    }
    else {
      beg = p->beg2;
      end = p->end2;
    }
    phs = p->lphs;
    ar1 = p->ar1;
    if (p->stereo) {
      ar2 = p->ar2;
      goto phsck2;
    }
 phschk:
    if (phs >= end && p->curmod != 3)
      goto put0;
    switch (p->curmod) {
    case 0:
      do {                                      /* NO LOOPING  */
        loscil_linear_interp_mono(ar1, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end)
          goto nxtseg;
      } while (--nsmps);
      break;
    case 1:
      do {                                      /* NORMAL LOOPING */
        loscil_linear_interp_mono(ar1, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg;
          phs -= end - beg;
        }
      } while (--nsmps);
      break;
    case 2:
    case2:
      do {                                      /* BIDIR FORW, EVEN */
        loscil_linear_interp_mono(ar1, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (--nsmps) goto case3;
          else break;
        }
      } while (--nsmps);
      break;
    case 3:
    case3:
      do {                                      /* BIDIR BACK, EVEN */
        loscil_linear_interp_mono(ar1, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs -= inc) < beg) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (--nsmps) goto case2;
          else break;
        }
      } while (--nsmps);
      break;

    nxtseg:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (--nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phschk;
        }
        break;
      }
      if (--nsmps) goto phsout;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout:
    p->lphs = phs;
 put0:
    do {
      *ar1++ = FL(0.0);
    } while (--nsmps);
    return OK;

 phsck2:
    if (phs >= end && p->curmod != 3)
      goto put0s;                               /* for STEREO:  */
    switch (p->curmod) {
    case 0:
      do {                                      /* NO LOOPING  */
        loscil_linear_interp_stereo(ar1, ar2, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        *ar2++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end)
          goto nxtseg2;
      } while (--nsmps);
      break;
    case 1:
      do {                                      /* NORMAL LOOPING */
        loscil_linear_interp_stereo(ar1, ar2, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        *ar2++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg2;
          phs -= end - beg;
        }
      } while (--nsmps);
      break;
    case 2:
    case2s:
      do {                                      /* BIDIR FORW, EVEN */
        loscil_linear_interp_stereo(ar1, ar2, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        *ar2++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg2;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (--nsmps) goto case3s;
          else break;
        }
      } while (--nsmps);
      break;
    case 3:
    case3s:
      do {                                      /* BIDIR BACK, EVEN */
        loscil_linear_interp_stereo(ar1, ar2, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        *ar2++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs -= inc) < beg) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (--nsmps) goto case2s;
          else break;
        }
      } while (--nsmps);
      break;

    nxtseg2:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (--nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phsck2;
        }
        break;
      }
      if (--nsmps) goto phsout2;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout2:
    p->lphs = phs;
 put0s:
    do {
      *ar1++ = FL(0.0);
      *ar2++ = FL(0.0);
    } while (--nsmps);

    return OK;
}

int loscil3(CSOUND *csound, LOSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar1, *ar2, *ftbl, *xamp;
    long    phs, inc, beg, end;
    int     nsmps = csound->ksmps, aamp;

    ftp = p->ftp;
    ftbl = ftp->ftable;
    if ((inc = (long)(*p->kcps * p->cpscvt)) < 0)
      inc = -inc;
    xamp = p->xamp;
    aamp = (p->XINCODE) ? 1 : 0;
    if (p->seg1) {                      /* if still segment 1  */
      beg = p->beg1;
      end = p->end1;
      if (p->h.insdshead->relesing)   /*    sense note_off   */
        p->looping = 0;
    }
    else {
      beg = p->beg2;
      end = p->end2;
    }
    phs = p->lphs;
    ar1 = p->ar1;
    if (p->stereo) {
      ar2 = p->ar2;
      goto phsck2;
    }
 phschk:
    if (phs >= end && p->curmod != 3)
      goto put0;
    switch (p->curmod) {
    case 0:
      do {                                      /* NO LOOPING  */
        loscil_cubic_interp_mono(ar1, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end)
          goto nxtseg;
      } while (--nsmps);
      break;
    case 1:
      do {                                      /* NORMAL LOOPING */
        loscil_cubic_interp_mono(ar1, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg;
          phs -= end - beg;
        }
      } while (--nsmps);
      break;
    case 2:
    case2:
      do {                                      /* BIDIR FORW, EVEN */
        loscil_cubic_interp_mono(ar1, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (--nsmps) goto case3;
          else break;
        }
      } while (--nsmps);
      break;
    case 3:
    case3:
      do {                                      /* BIDIR BACK, EVEN */
        loscil_cubic_interp_mono(ar1, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs -= inc) < beg) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (--nsmps) goto case2;
          else break;
        }
      } while (--nsmps);
      break;

    nxtseg:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (--nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phschk;
        }
        break;
      }
      if (--nsmps) goto phsout;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout:
    p->lphs = phs;
 put0:
    do {
      *ar1++ = FL(0.0);
    } while (--nsmps);
    return OK;

 phsck2:
    if (phs >= end && p->curmod != 3)
      goto put0s;                               /* for STEREO:  */
    switch (p->curmod) {
    case 0:
      do {                                      /* NO LOOPING  */
        loscil_cubic_interp_stereo(ar1, ar2, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        *ar2++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end)
          goto nxtseg2;
      } while (--nsmps);
      break;
    case 1:
      do {                                      /* NORMAL LOOPING */
        loscil_cubic_interp_stereo(ar1, ar2, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        *ar2++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg2;
          phs -= end - beg;
        }
      } while (--nsmps);
      break;
    case 2:
    case2s:
      do {                                      /* BIDIR FORW, EVEN */
        loscil_cubic_interp_stereo(ar1, ar2, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        *ar2++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg2;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (--nsmps) goto case3s;
          else break;
        }
      } while (--nsmps);
      break;
    case 3:
    case3s:
      do {                                      /* BIDIR BACK, EVEN */
        loscil_cubic_interp_stereo(ar1, ar2, ftbl, phs, ftp->flen);
        *ar1++ *= *xamp;
        *ar2++ *= *xamp;
        if (aamp)  xamp++;
        if ((phs -= inc) < beg) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (--nsmps) goto case2s;
          else break;
        }
      } while (--nsmps);
      break;

    nxtseg2:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (--nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phsck2;
        }
        break;
      }
      if (--nsmps) goto phsout2;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout2:
    p->lphs = phs;
 put0s:
    do {
      *ar1++ = FL(0.0);
      *ar2++ = FL(0.0);
    } while (--nsmps);

    return OK;
}

#define ISINSIZ 32768L
#define ADMASK  32767L

int adset(CSOUND *csound, ADSYN *p)
{
    long    n;
    char    filnam[MAXNAME];
    MEMFIL  *mfp;
    short   *adp, *endata, val;
    PTLPTR  *ptlap, *ptlfp, *ptlim;
    int     size;

    if (csound->isintab == NULL) {  /* if no sin table yet, make one */
      short *ip;
      csound->isintab = ip = (short*) mmalloc(csound, ISINSIZ * sizeof(short));
      for (n = 0; n < ISINSIZ; n++)
        *ip++ = (short) (sin(TWOPI * n / ISINSIZ) * 32767.0);
    }
    csound->strarg2name(csound, filnam, p->ifilcod, "adsyn.", p->XSTRCODE);
    if ((mfp = p->mfp) == NULL || strcmp(mfp->filename,filnam) != 0) {
      if ((mfp = ldmemfile(csound, filnam)) == NULL) {  /*   readfile if reqd */
        csound->InitError(csound, Str("ADSYN cannot load %s"), filnam);
        return NOTOK;
      }
      p->mfp = mfp;                         /*   & record         */
    }

    adp = (short *) mfp->beginp;            /* align on file data */
    endata = (short *) mfp->endp;
    size = 1+(*adp == -1 ? MAXPTLS : *adp++); /* Old no header -> MAXPIL */
    if (p->aux.auxp==NULL || p->aux.size < (long)sizeof(PTLPTR)*size)
      csound->AuxAlloc(csound, sizeof(PTLPTR)*size, &p->aux);

    ptlap = ptlfp = (PTLPTR*)p->aux.auxp;   /* find base ptl blk */
    ptlim = ptlap + size;
    do {
      if ((val = *adp++) < 0) {             /* then for each brkpt set,   */
        switch (val) {
        case -1:
          ptlap->nxtp = ptlap + 1;       /* chain the ptl blks */
          if ((ptlap = ptlap->nxtp) >= ptlim) goto adsful;
          ptlap->ap = (DUPLE *) adp;     /*  record start amp  */
          ptlap->amp = ptlap->ap->val;
          break;
        case -2:
          if ((ptlfp += 1) >= ptlim) goto adsful;
          ptlfp->fp = (DUPLE *) adp;     /*  record start frq  */
          ptlfp->frq = ptlfp->fp->val;
          ptlfp->phs = 0;                /*  and clr the phase */
          break;
        default:
          csound->InitError(csound, Str("illegal code %d encountered"), val);
          return NOTOK;
        }
      }
    } while (adp < endata);
    if (ptlap != ptlfp) {
      csound->InitError(csound, Str("%d amp tracks, %d freq tracks"),
                                (int) (ptlap - (PTLPTR*)p->aux.auxp) - 1,
                                (int) (ptlfp - (PTLPTR*)p->aux.auxp) - 1);
      return NOTOK;
    }
    ptlap->nxtp = NULL;   /* terminate the chain */
    p->mksecs = 0;

    return OK;

 adsful:
    return csound->InitError(csound, Str("partial count exceeds MAXPTLS"));
}

#define ADSYN_MAXLONG FL(2147483647.0)

int adsyn(CSOUND *csound, ADSYN *p)
{
    PTLPTR  *curp, *prvp;
    DUPLE   *ap, *fp;
    short   curtim, diff, ktogo;
    long    phs, sinc, *sp, amp;
    int     nsmps;
    MYFLT   *ar;
    MYFLT   ampscale, frqscale;
    long    timkincr, nxtim;

    if (csound->isintab == NULL) {      /* RWD fix */
      return csound->PerfError(csound, Str("adsyn: not initialised"));
    }
    /* IV - Jul 11 2002 */
    ampscale = *p->kamod * csound->e0dbfs;      /* since 15-bit sine table */
    frqscale = *p->kfmod * ISINSIZ * csound->onedsr;
    /* 1024 * msecs of analysis */
    timkincr = (long)(*p->ksmod*FL(1024000.0)*csound->onedkr);
    sp = (long *) p->rslt;                      /* use out array for sums */
    nsmps = csound->ksmps;
    do {
      *sp++ = 0L;                               /* cleared first to zero */
    } while (--nsmps);
    curtim = (short)(p->mksecs >> 10);          /* cvt mksecs to msecs */
    curp = (PTLPTR*)p->aux.auxp;                /* now for each partial:    */
    while ((prvp = curp) && (curp = curp->nxtp) != NULL ) {
      ap = curp->ap;
      fp = curp->fp;
      while (curtim >= (ap+1)->tim)       /* timealign ap, fp */
        curp->ap = ap += 1;
      while (curtim >= (fp+1)->tim)
        curp->fp = fp += 1;
      if ((amp = curp->amp)) {            /* for non-zero amp   */
        sinc = (long)(curp->frq * frqscale);
        phs = curp->phs;
        sp = (long *) p->rslt;
        nsmps = csound->ksmps;            /*   addin a sinusoid */
        do {
          *sp++ += csound->isintab[phs] * amp;
          phs += sinc;
          phs &= ADMASK;
        } while (--nsmps);
        curp->phs = phs;
      }
      if ((nxtim = (ap+1)->tim) == 32767) {   /* if last amp this partial */
        prvp->nxtp = curp->nxtp;            /*   remov from activ chain */
        curp = prvp;
      }
      else {                                 /* else interp towds nxt amp */
        if ((diff = (short)((ap+1)->val - amp))) {
          ktogo = (short)(((nxtim<<10) - p->mksecs + timkincr - 1) / timkincr);
          curp->amp += diff / ktogo;
        }
        if ((nxtim = (fp+1)->tim) != 32767            /*      & nxt frq */
            && (diff = (fp+1)->val - curp->frq)) {
          ktogo = (short)(((nxtim<<10) - p->mksecs + timkincr - 1) / timkincr);
          curp->frq += diff / ktogo;
        }
      }
    }
    p->mksecs += timkincr;                  /* advance the time */
    ar = p->rslt;
    sp = (long *) ar;
    nsmps = csound->ksmps;
    do {
      /* a quick-hack fix: should change adsyn to use floats table and
         buffers and should replace hetro format anyway.... */
      *ar++ = (MYFLT) ((*sp++ * ampscale) / ADSYN_MAXLONG);
    } while (--nsmps);
    return OK;
}

