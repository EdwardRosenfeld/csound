/*
    ambicode.c:

    Copyright (C) 2005 Samuel Groner,
    Institute for Computer Music and Sound Technology, www.icst.net

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

#include "csdl.h"
#include <math.h>

typedef struct {
    OPDS    h;                                      /* required header */
    MYFLT   *mw, *mx, *my, *mz, *mr, *ms, *mt, *mu, *mv, *mk,
            *ml, *mm, *mn, *mo, *mp, *mq;           /* addr outarg */
    MYFLT   *asig, *kalpha, *kin[VARGMAX];          /* addr inargs */
    /* private dataspace */
    double  w, x, y, z, r, s, t, u, v, k, l, m, n, o, p, q;
} AMBIC;

typedef struct {
    OPDS    h;                                      /* required header */
    MYFLT   *m0, *m1, *m2, *m3, *m4, *m5, *m6, *m7; /* addr outarg */
    MYFLT   *isetup, *aw, *ax, *ay, *a[VARGMAX];    /* addr inargs */
    /* private dataspace */
    double  w[8], x[8], y[8], z[8], r[8], s[8], t[8], u[8],
            v[8], k[8], l[8], m[8], n[8], o[8], p[8], q[8];
} AMBID;

static int iambicode(CSOUND *csound, AMBIC *p)
{
    /* check correct number of input and output arguments */
    switch (p->OUTOCOUNT) {
      case 4:
        {
          /* 2nd order */
          if (p->INOCOUNT != 5) {
            return csound->InitError(csound, "Wrong number of input arguments! "
                                             "5 needed!");
          }
          break;
        }

      case 9:
        {
          /* 3rd order */
          if (p->INOCOUNT != 6) {
            return csound->InitError(csound, "Wrong number of input arguments! "
                                             "6 needed!");
          }
          break;
        }

      case 16:
        {
          /* 4th order */
          if (p->INOCOUNT != 7) {
            return csound->InitError(csound, "Wrong number of input arguments! "
                                             "7 needed!");
          }
          break;
        }

      default:
        {
          return csound->InitError(csound, "Wrong number of output arguments! "
                                           "4, 9 or 16 needed!");
        }
    }
    return OK;
}

static void ambicode_set_coefficients(AMBIC *p)
{
    /* convert degrees to radian */
    MYFLT kalpha_rad = (*p->kalpha) / FL(57.295779513082320876798154814105);
    MYFLT kbeta_rad = (*p->kin[0]) / FL(57.295779513082320876798154814105);

    /* calculate ambisonic coefficients (Furse-Malham-set) */

    /* 0th order */
    p->w = FL(1.0) / sqrt(FL(2.0));

    /* 1st order */
    p->x = cos(kalpha_rad) * cos(kbeta_rad);
    p->y = sin(kalpha_rad) * cos(kbeta_rad);
    p->z = sin(kbeta_rad);

    /* 2nd order */
    p->r = FL(0.5) * (FL(3.0) * p->z * p->z - FL(1.0));
    p->s = FL(2.0) * p->x * p->z;
    p->t = FL(2.0) * p->y * p->z;
    p->u = p->x * p->x - p->y * p->y;
    p->v = FL(2.0) * p->x * p->y;

    /* 3rd order */
    p->k = FL(0.5) * p->z * (FL(5.0) * p->z * p->z - FL(3.0));
    p->l = FL(8.0) / FL(11.0) * p->y * (FL(5.0) * p->z * p->z - FL(1.0));
    p->m = FL(8.0) / FL(11.0) * p->x * (FL(5.0) * p->z * p->z - FL(1.0));
    p->n = FL(2.0) * p->x * p->y * p->z;
    p->o = p->z * (p->x * p->x - p->y * p->y);
    p->p = FL(3.0) * p->y * (FL(3.0) * p->x * p->x - p->y * p->y);
    p->q = FL(3.0) * p->x * (p->x * p->x - FL(3.0) * p->y * p->y);
}

static int aambicode(CSOUND *csound, AMBIC *p)
{
    int nn = csound->ksmps;  /* array size from orchestra */

    /* update coefficients */
    ambicode_set_coefficients(p);

    /* init input array pointer */
    MYFLT *inptp = p->asig;

    /* init output array pointer 0th order */
    MYFLT *rsltp_w = p->mw;

    /* init output array pointers 1th order */
    MYFLT *rsltp_x = p->mx;
    MYFLT *rsltp_y = p->my;
    MYFLT *rsltp_z = p->mz;

    /* init output array pointers 2nd order */
    MYFLT *rsltp_r = p->mr;
    MYFLT *rsltp_s = p->ms;
    MYFLT *rsltp_t = p->mt;
    MYFLT *rsltp_u = p->mu;
    MYFLT *rsltp_v = p->mv;

    /* init output array pointers 3rd order */
    MYFLT *rsltp_k = p->mk;
    MYFLT *rsltp_l = p->ml;
    MYFLT *rsltp_m = p->mm;
    MYFLT *rsltp_n = p->mn;
    MYFLT *rsltp_o = p->mo;
    MYFLT *rsltp_p = p->mp;
    MYFLT *rsltp_q = p->mq;

    if (p->OUTOCOUNT == 4 && p->INOCOUNT == 5) {
      /* 1st order */

      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p->w * *p->kin[1];

        /* 1st order */
        *rsltp_x++ = *inptp * p->x * *p->kin[2];
        *rsltp_y++ = *inptp * p->y * *p->kin[2];
        *rsltp_z++ = *inptp * p->z * *p->kin[2];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }
    else if (p->OUTOCOUNT == 9 && p->INOCOUNT == 6) {
      /* 2nd order */

      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p->w * *p->kin[1];

        /* 1st order */
        *rsltp_x++ = *inptp * p->x * *p->kin[2];
        *rsltp_y++ = *inptp * p->y * *p->kin[2];
        *rsltp_z++ = *inptp * p->z * *p->kin[2];

        /* 2nd order */
        *rsltp_r++ = *inptp * p->r * *p->kin[3];
        *rsltp_s++ = *inptp * p->s * *p->kin[3];
        *rsltp_t++ = *inptp * p->t * *p->kin[3];
        *rsltp_u++ = *inptp * p->u * *p->kin[3];
        *rsltp_v++ = *inptp * p->v * *p->kin[3];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }
    else if (p->OUTOCOUNT == 16 && p->INOCOUNT == 7) {
      /* 3rd order */

      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p->w * *p->kin[1];

        /* 1st order */
        *rsltp_x++ = *inptp * p->x * *p->kin[2];
        *rsltp_y++ = *inptp * p->y * *p->kin[2];
        *rsltp_z++ = *inptp * p->z * *p->kin[2];

        /* 2nd order */
        *rsltp_r++ = *inptp * p->r * *p->kin[3];
        *rsltp_s++ = *inptp * p->s * *p->kin[3];
        *rsltp_t++ = *inptp * p->t * *p->kin[3];
        *rsltp_u++ = *inptp * p->u * *p->kin[3];
        *rsltp_v++ = *inptp * p->v * *p->kin[3];

        /* 3rd order */
        *rsltp_k++ = *inptp * p->k * *p->kin[4];
        *rsltp_l++ = *inptp * p->l * *p->kin[4];
        *rsltp_m++ = *inptp * p->m * *p->kin[4];
        *rsltp_n++ = *inptp * p->n * *p->kin[4];
        *rsltp_o++ = *inptp * p->o * *p->kin[4];
        *rsltp_p++ = *inptp * p->p * *p->kin[4];
        *rsltp_q++ = *inptp * p->q * *p->kin[4];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }
    return OK;
}

static void ambideco_set_coefficients(AMBID *p, MYFLT alpha, MYFLT beta,
                                      int index)
{
    /* convert degrees to radian */
    MYFLT alpha_rad = alpha / FL(57.295779513082320876798154814105);
    MYFLT beta_rad = beta / FL(57.295779513082320876798154814105);

    /* calculate ambisonic coefficients (Furse-Malham-set) */

    /* 0th order */
    p->w[index] = FL(1.0) / sqrt(FL(2.0));

    /* 1st order */
    p->x[index] = cos(alpha_rad) * cos(beta_rad);
    p->y[index] = sin(alpha_rad) * cos(beta_rad);
    p->z[index] = sin(beta_rad);

    /* 2nd order */
    p->r[index] = FL(0.5) * (FL(3.0) * p->z[index] * p->z[index] - FL(1.0));
    p->s[index] = FL(2.0) * p->x[index] * p->z[index];
    p->t[index] = FL(2.0) * p->y[index] * p->z[index];
    p->u[index] = p->x[index] * p->x[index] - p->y[index] * p->y[index];
    p->v[index] = FL(2.0) * p->x[index] * p->y[index];

    /* 3rd order */
    p->k[index] = FL(0.5) * p->z[index] *
      (FL(5.0) * p->z[index] * p->z[index] - FL(3.0));
    p->l[index] = FL(8.0) / FL(11.0) * p->y[index] *
      (FL(5.0) * p->z[index] * p->z[index] - FL(1.0));
    p->m[index] = FL(8.0) / FL(11.0) * p->x[index] *
      (FL(5.0) * p->z[index] * p->z[index] - FL(1.0));
    p->n[index] = FL(2.0) * p->x[index] * p->y[index] * p->z[index];
    p->o[index] = p->z[index] *
      (p->x[index] * p->x[index] - p->y[index] * p->y[index]);
    p->p[index] = FL(3.0) * p->y[index] *
      (FL(3.0) * p->x[index] * p->x[index] - p->y[index] * p->y[index]);
    p->q[index] = FL(3.0) * p->x[index] *
      (p->x[index] * p->x[index] - FL(3.0) * p->y[index] * p->y[index]);
}

static int iambideco(CSOUND *csound, AMBID *p)
{
    /* check correct number of input arguments */
    if ((p->INOCOUNT != 5) && (p->INOCOUNT != 10) && (p->INOCOUNT != 17)) {
      return csound->InitError(csound, "Wrong number of input arguments!");
    }

    switch ((int)*p->isetup) {
      case 1:
        {
          if (p->OUTOCOUNT != 2) {
            return csound->InitError(csound, "Wrong number of output cells! "
                                             "There must be 2 output cells.");
          }
          else {
            ambideco_set_coefficients(p, FL(330.0), FL(0.0), 0);    /* left */
            ambideco_set_coefficients(p, FL(30.0), FL(0.0), 1);     /* right */
          }
          break;
        }

      case 2:
        {
          if (p->OUTOCOUNT != 4) {
            return csound->InitError(csound, "Wrong number of output cells! "
                                             "There must be 4 output cells.");
          }
          else {
            ambideco_set_coefficients(p, FL(45.0), FL(0.0), 0);
            ambideco_set_coefficients(p, FL(135.0), FL(0.0), 1);
            ambideco_set_coefficients(p, FL(225.0), FL(0.0), 2);
            ambideco_set_coefficients(p, FL(315.0), FL(0.0), 3);
          }
          break;
        }

      case 3: {
        if (p->OUTOCOUNT != 5) {
          return csound->InitError(csound, "Wrong number of output cells! "
                                           "There must be 5 output cells.");
        }
        else {
          ambideco_set_coefficients(p, FL(330.0), FL(0.0), 0);  /* left */
          ambideco_set_coefficients(p, FL(30.0), FL(0.0), 1);   /* right */
          ambideco_set_coefficients(p, FL(0.0), FL(0.0), 2);    /* center */
          ambideco_set_coefficients(p, FL(250.0), FL(0.0), 3);  /* surround L */
          ambideco_set_coefficients(p, FL(110.0), FL(0.0), 4);  /* surround R */
        }
        break;
      }

      case 4:
        {
          if (p->OUTOCOUNT != 8) {
            return csound->InitError(csound, "Wrong number of output cells! "
                                             "There must be 8 output cells.");
          }
          else {
            ambideco_set_coefficients(p, FL(22.5), FL(0.0), 0);
            ambideco_set_coefficients(p, FL(67.5), FL(0.0), 1);
            ambideco_set_coefficients(p, FL(112.5), FL(0.0), 2);
            ambideco_set_coefficients(p, FL(157.5), FL(0.0), 3);
            ambideco_set_coefficients(p, FL(202.5), FL(0.0), 4);
            ambideco_set_coefficients(p, FL(247.5), FL(0.0), 5);
            ambideco_set_coefficients(p, FL(292.5), FL(0.0), 6);
            ambideco_set_coefficients(p, FL(337.5), FL(0.0), 7);
          }
          break;
        }

      case 5:
        {
          if (p->OUTOCOUNT != 8) {
            return csound->InitError(csound, "Wrong number of output cells! "
                                             "There must be 8 output cells.");
          }
          else {
            ambideco_set_coefficients(p, FL(45.0), FL(0.0), 0);
            ambideco_set_coefficients(p, FL(45.0), FL(30.0), 1);
            ambideco_set_coefficients(p, FL(135.0), FL(0.0), 2);
            ambideco_set_coefficients(p, FL(135.0), FL(30.0), 3);
            ambideco_set_coefficients(p, FL(225.0), FL(0.0), 4);
            ambideco_set_coefficients(p, FL(225.0), FL(30.0), 5);
            ambideco_set_coefficients(p, FL(315.0), FL(0.0), 6);
            ambideco_set_coefficients(p, FL(315.0), FL(30.0), 7);
          }
          break;
        }

      default:
          return csound->InitError(csound, "Not supported setup number!");
    }
    return OK;
}

static int aambideco(CSOUND *csound, AMBID *p)
{
    int nn = csound->ksmps;  /* array size from orchestra */
    int i = 0;

    /* init input array pointer 0th order */
    MYFLT *inptp_w = p->aw;

    /* init input array pointer 1st order */
    MYFLT *inptp_x = p->ax;
    MYFLT *inptp_y = p->ay;
    MYFLT *inptp_z = p->a[0];

    /* init input array pointer 2nd order */
    MYFLT *inptp_r = p->a[1];
    MYFLT *inptp_s = p->a[2];
    MYFLT *inptp_t = p->a[3];
    MYFLT *inptp_u = p->a[4];
    MYFLT *inptp_v = p->a[5];

    /* init input array pointer 3rd order */
    MYFLT *inptp_k = p->a[6];
    MYFLT *inptp_l = p->a[7];
    MYFLT *inptp_m = p->a[8];
    MYFLT *inptp_n = p->a[9];
    MYFLT *inptp_o = p->a[10];
    MYFLT *inptp_p = p->a[11];
    MYFLT *inptp_q = p->a[12];

    /* init output array pointer */
    MYFLT *rsltp[8];

    rsltp[0] = p->m0;
    rsltp[1] = p->m1;
    rsltp[2] = p->m2;
    rsltp[3] = p->m3;
    rsltp[4] = p->m4;
    rsltp[5] = p->m5;
    rsltp[6] = p->m6;
    rsltp[7] = p->m7;

    if (p->INOCOUNT == 5) {
      do {
        /* 1st order */
        for (i = 0; i < p->OUTOCOUNT; i++) {
          /* calculate output for every used loudspeaker */
          *rsltp[i]++ = *inptp_w * p->w[i] + *inptp_x * p->x[i] +
            *inptp_y * p->y[i] + *inptp_z * p->z[i];
        }

        /* increment input array pointer 0th order */
        inptp_w++;

        /* increment input array pointer 1st order */
        inptp_x++;
        inptp_y++;
        inptp_z++;

      } while (--nn);
    }
    else if (p->INOCOUNT == 10) {
      do {
        /* 2nd order */
        for (i = 0; i < p->OUTOCOUNT; i++) {
          /* calculate output for every used loudspeaker */
          *rsltp[i]++ = *inptp_w * p->w[i] + *inptp_x * p->x[i] +
            *inptp_y * p->y[i] + *inptp_z * p->z[i] +
            *inptp_r * p->r[i] + *inptp_s * p->s[i] +
            *inptp_t * p->t[i] + *inptp_u * p->u[i] + *inptp_v * p->v[i];
        }

        /* increment input array pointer 0th order */
        inptp_w++;

        /* increment input array pointer 1st order */
        inptp_x++;
        inptp_y++;
        inptp_z++;

        /* increment input array pointer 2nd order */
        inptp_r++;
        inptp_s++;
        inptp_t++;
        inptp_u++;
        inptp_v++;

      } while (--nn);
    }
    else if (p->INOCOUNT == 17) {
      do {
        /* 3rd order */
        for (i = 0; i < p->OUTOCOUNT; i++) {
          /* calculate output for every used loudspeaker */
          *rsltp[i]++ = *inptp_w * p->w[i] + *inptp_x * p->x[i] +
            *inptp_y * p->y[i] + *inptp_z * p->z[i] + *inptp_r * p->r[i] +
            *inptp_s * p->s[i] + *inptp_t * p->t[i] + *inptp_u * p->u[i] +
            *inptp_v * p->v[i] + *inptp_k * p->k[i] + *inptp_l * p->l[i] +
            *inptp_m * p->m[i] + *inptp_n * p->n[i] + *inptp_o * p->o[i] +
            *inptp_p * p->p[i] + *inptp_q * p->q[i];
        }

        /* increment input array pointer 0th order */
        inptp_w++;

        /* increment input array pointer 1st order */
        inptp_x++;
        inptp_y++;
        inptp_z++;

        /* increment input array pointer 2nd order */
        inptp_r++;
        inptp_s++;
        inptp_t++;
        inptp_u++;
        inptp_v++;

        /* increment input array pointer 3rd order */
        inptp_k++;
        inptp_l++;
        inptp_m++;
        inptp_n++;
        inptp_o++;
        inptp_p++;
        inptp_q++;
      } while (--nn);
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "bformenc", S(AMBIC), 5, "mmmmmmmmmmmmmmmm", "akz",
                            (SUBR)iambicode, NULL, (SUBR)aambicode },
  { "bformdec", S(AMBID), 5, "mmmmmmmm", "iaaay",
                            (SUBR)iambideco, NULL, (SUBR)aambideco }
};

LINKAGE

