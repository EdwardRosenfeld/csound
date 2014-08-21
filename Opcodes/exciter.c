/*
    exciter.c:

    Copyright (C) 2014 by John ffitch after Markus Schmidt

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

#include "csdl.h"       /*                    EXCITER.C         */
#include <math.h>

/**********************************************************************
 * EXCITER by Markus Schmidt
 **********************************************************************/

typedef struct {
  OPDS        h;
  MYFLT *aout;
  MYFLT *ain;
  MYFLT *pfreq;
  MYFLT *pceil;
  MYFLT *pceil_active;
  MYFLT *pblend;
  MYFLT *pdrive;  
  // Internals
  MYFLT freq_old, ceil_old, ceil_active_old;
  // biquad data
  double hp1[7], hp2[7], hp3[7], hp4[7];
  double lp1[7],  lp2[7];
  double rs1[7], rs2[7], rs3[7], rs4[7];
  double rdrive, rbdr, kpa, kpb, kna, knb, ap, an, imr, kc, srct, sq, pwrq;
  int over;
  double prev_med, prev_out;
  double blend_old, drive_old;
} EXCITER;

void resample_set_params(CSOUND *csound, EXCITER *p);

static int exciter_init(CSOUND *csound, EXCITER *p)
{
    p->freq_old =  p->ceil_old = p->ceil_active_old = FL(0.0);
    p->hp1[5] = p->hp2[5] = p->hp3[5] = p->hp4[5] = 0.0;
    p->hp1[6] = p->hp2[6] = p->hp3[6] = p->hp4[6] = 0.0;
    p->lp1[5] = p->lp2[5] = 0.0;
    p->lp1[6] = p->lp2[6] = 0.0;
    p->rs1[5] = p->rs2[5] = p->rs3[5] = p->rs4[5] = 0.0;
    p->rs1[6] = p->rs2[6] = p->rs3[6] = p->rs4[6] = 0.0;
    p->rdrive = p->rbdr = p->kpa = p->kpb = p->kna = p->knb = p->ap =
      p->an = p->imr = p->kc = p->srct = p->sq = p->pwrq = p->prev_med =
      p->prev_out = 0.0;
    p->over = csound->GetSr(csound) * 2 > 96000 ? 1 : 2;
    resample_set_params(csound, p);
    return OK;
}

inline double process(double st[7], double in)
{
    double tmp = in - st[5] * st[3] - st[6] * st[4];
    double out = tmp * st[0] + st[5] * st[1] + st[6] * st[2];
    st[6] = st[5];
    st[5] = tmp;
    return out;
}

    /** Highpass filter based on Robert Bristow-Johnson's equations
     * @param fc     resonant frequency
     * @param q      resonance (gain at fc)
     */
inline void set_hp_rbj(CSOUND *csound, double hp[], double fc, double q)
{
    double omega=(double)(2*M_PI*fc/csound->GetSr(csound));
    double sn=sin(omega);
    double cs=cos(omega);
    double alpha=(double)(sn/(2.0*q));

    double inv=(double)(1.0/(1.0+alpha));
    
    hp[2]/*a2*/ = hp[0]/*a0*/ =  (inv*(1.0 + cs)/2.0);
    hp[1]/*a1*/ =  -2.0 * hp[0];
    hp[3]/*b1*/ =  (-2.0*cs*inv);
    hp[4]/*b2*/ =  ((1.0 - alpha)*inv);
    hp[5] = hp[6] = 0.0;
    return;
}

inline void set_lp_rbj(CSOUND *csound,
                       double *lp, double fc, double q, double gain)
{
    double omega=(2.0*M_PI*fc/csound->GetSr(csound));
    double sn=sin(omega);
    double cs=cos(omega);
    double alpha=(sn/(2*q));
    double inv=(1.0/(1.0+alpha));

    lp[2] = lp[0] =  gain*(inv*(1.0 - cs)*0.5);
    lp[1] =  lp[0]+lp[0];
    lp[3] =  (-2.0*cs*inv);
    lp[4] =  ((1.0 - alpha)*inv);
}

void resample_set_params(CSOUND *csound, EXCITER *p)
{
    double srate   = csound->GetSr(csound);
    double ff = 25000.0;
    if (srate>50000) ff = srate*0.5;
    // set all filters
    set_lp_rbj(csound, p->rs1, ff, 0.8, srate * 2);
    memcpy(p->rs2, p->rs1, 5*sizeof(double));
    memcpy(p->rs3, p->rs1, 5*sizeof(double));
    memcpy(p->rs4, p->rs1, 5*sizeof(double));
}

void upsample(EXCITER *p, double *tmp, double sample)
{
    tmp[0] = process(p->rs1,sample);
    tmp[0] = process(p->rs2,sample);
    tmp[1] = process(p->rs1,sample);
    tmp[1] = process(p->rs2,sample);
    return;
}

double downsample(EXCITER *p, double *sample)
{
    sample[0] = process(p->rs1, sample[0]);
    sample[0] = process(p->rs2, sample[0]);
    sample[1] = process(p->rs3, sample[1]);
    sample[1] = process(p->rs4, sample[1]);
    return sample[0];
}

static inline double M(float x)
{
    return (fabs(x) > 0.00000001f) ? x : 0.0f;
}

static inline double D(float x)
{
    x = fabs(x);
    return (x > 0.00000001f) ? sqrtf(x) : 0.0f;
}

inline double distort(EXCITER *p, double in)
{
    double samples[2];
    int i;
    upsample(p, samples, in);
    for (i = 0; i < p->over; i++) {
      double proc = samples[i];
      double med;
      if (proc >= 0.0) {
        med = (D(p->ap + proc * (p->kpa - proc)) + p->kpb) * p->pwrq;
      } else {
        med = (D(p->an - proc * (p->kna + proc)) + p->knb) * p->pwrq * (-1.0);
      }
      proc = p->srct * (med - p->prev_med + p->prev_out);
      p->prev_med = M(med);
      p->prev_out = M(proc);
      samples[i] = proc;
    }
    return downsample(p, samples);
}

inline void set_distort(CSOUND *csound, EXCITER *p)
{
    // set distortion coeffs
    if ((p->drive_old != *p->pdrive) || (p->blend_old != *p->pblend)) {
        p->rdrive = 12.0 / *p->pdrive;
        p->rbdr = p->rdrive / (10.5 - *p->pblend) * 780.0 / 33.0;
        p->kpa = D(2.0 * (p->rdrive*p->rdrive) - 1.0) + 1.0;
        p->kpb = (2.0 - p->kpa) / 2.0;
        p->ap = ((p->rdrive*p->rdrive) - p->kpa + 1.0) / 2.0;
        p->kc = p->kpa / D(2.0 * D(2.0 * (p->rdrive*p->rdrive) - 1.0) -
                           2.0 * p->rdrive*p-> rdrive);

        p->srct = (0.1f * csound->GetSr(csound)) /
          (0.1f * csound->GetSr(csound) + 1.0);
        p->sq = p->kc*p->kc + 1.0;
        p->knb = -1.0 * p->rbdr / D(p->sq);
        p->kna = 2.0 * p->kc * p->rbdr / D(p->sq);
        p->an = p->rbdr*p->rbdr / p->sq;
        p->imr = 2.0 * p->knb + D(2.0 * p->kna + 4.0 * p->an - 1.0);
        p->pwrq = 2.0 / (p->imr + 1.0);

        p->drive_old = *p->pdrive;
        p->blend_old = *p->pblend;
    }
}


inline void params_changed(CSOUND *csound, EXCITER *p)
{
    // set the params of all filters
    if (*p->pfreq != p->freq_old) {
      set_hp_rbj(csound, p->hp1, *p->pfreq, 0.707);
      memcpy(p->hp2, p->hp1, 5*sizeof(double));
      memcpy(p->hp3, p->hp1, 5*sizeof(double));
      memcpy(p->hp4, p->hp1, 5*sizeof(double));
      p->freq_old = *p->pfreq;
    }
    // set the params of all filters
    if (*p->pceil != p->ceil_old || *p->pceil_active != p->ceil_active_old ) {
      set_lp_rbj(csound, p->lp1, *p->pceil, 0.707, 1.0);
      memcpy(p->lp2, p->lp1, 5*sizeof(double));
      p->ceil_old = *p->pceil;
      p->ceil_active_old = *p->pceil_active;
    }
    // set distortion
    set_distort(csound, p);
}

int exciter_perf(CSOUND *csound, EXCITER *p)
//uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
 
     if (UNLIKELY(offset)) memset(p->aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    params_changed(csound, p);
   // process
    for (n = offset; n<nsmps; n++) {
      // cycle through samples
      double out, in;;
      in = (double)p->ain[n];
      // all pre filters in chain
      out = process(p->hp2, process(p->hp1, in));
      out = distort(p, out);      // saturate
      // all post filters in chain
      out = process(p->hp4, process(p->hp3, in)); 
                
      if (p->ceil_active_old > FL(0.5)) {
        // all H/P post filters in chain (surely LP - JPff)
        out = process(p->lp1, process(p->lp2,out));
      }
      p->aout[n] = out;
    } // cycle through samples
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY excite_localops[] = {
  { "exciter", S(EXCITER),   0, 5, "a", "akkkkk",
                             (SUBR)exciter_init, NULL, (SUBR)exciter_perf },
};

LINKAGE_BUILTIN(excite_localops)
