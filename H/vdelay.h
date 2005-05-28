/*
    vdelay.h:

    Copyright (C) 1994 Paris Smaragdis, John ffitch

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

/*      vdelay, multitap, reverb2 coded by Paris Smaragdis              */
/*      Berklee College of Music Csound development team                */
/*      Copyright (c) December 1994.  All rights reserved               */

typedef struct {
        OPDS    h;
        MYFLT   *sr, *ain, *adel, *imaxd, *istod;
        AUXCH   aux;
        long    left;
} VDEL;

typedef struct {
        OPDS    h;
        MYFLT   *sr1, *sr2, *sr3, *sr4;
        MYFLT   *ain1, *ain2, *ain3, *ain4, *adel, *imaxd, *iquality, *istod;
        AUXCH   aux1, aux2, aux3, aux4;
        int     interp_size;
        long    left;
} VDELXQ;

typedef struct {
        OPDS    h;
        MYFLT   *sr1, *sr2, *ain1, *ain2, *adel, *imaxd, *iquality, *istod;
        AUXCH   aux1, aux2;
        int     interp_size;
        long    left;
} VDELXS;

typedef struct {
        OPDS    h;
        MYFLT   *sr1, *ain1, *adel, *imaxd, *iquality, *istod;
        AUXCH   aux1;
        int     interp_size;
        long    left;
} VDELX;

typedef struct {
        OPDS    h;
        MYFLT   *sr, *ain, *ndel[VARGMAX];
        AUXCH   aux;
        long    left, max;
} MDEL;

#define Combs   6
#define Alpas   5

typedef struct  {
        OPDS    h;
        MYFLT   *out, *in, *time, *hdif, *istor;
        MYFLT   *cbuf_cur[Combs], *abuf_cur[Alpas];
        MYFLT   c_time[Combs], c_gain[Combs], a_time[Alpas], a_gain[Alpas];
        MYFLT   z[Combs], g[Combs];
        AUXCH   temp;
        AUXCH   caux[Combs], aaux[Alpas];
        MYFLT   prev_time, prev_hdif;
} STVB;

/*      nreverb coded by Paris Smaragdis 1994 and Richard Karpen 1998 */
#define Combs   6
#define Alpas   5
typedef struct  {
        OPDS    h;
        MYFLT   *out, *in, *time, *hdif, *istor;
        MYFLT   *cbuf_cur[Combs], *abuf_cur[Alpas];
        MYFLT   c_time[Combs], c_gain[Combs], a_time[Alpas], a_gain[Alpas];
        MYFLT   z[Combs], g[Combs];
        AUXCH   temp;
        AUXCH   caux[Combs], aaux[Alpas];
        MYFLT   prev_time, prev_hdif;
} NREV;

/*
 * Based on nreverb coded by Paris Smaragdis 1994 and Richard Karpen 1998.
 * Changes made to allow user-defined comb and alpas constant in a ftable.
 * Sept 2000, by rasmus ekman.
 * Memory allocation fixed April 2001 by JPff
 */
typedef struct  {
        OPDS    h;
        MYFLT   *out, *in, *time, *hdif, *istor;
        MYFLT   *inumCombs, *ifnCombs, *inumAlpas, *ifnAlpas;
        /* Used to be [Combs]- and [Alpas]-sized arrays */
        int     numCombs, numAlpas;
        MYFLT   **cbuf_cur, **abuf_cur;
        MYFLT   **pcbuf_cur, **pabuf_cur;
        MYFLT   *c_time, *c_gain, *a_time, *a_gain;
        MYFLT   *c_orggains, *a_orggains;
        MYFLT   *z, *g;    /* [Combs] */
        AUXCH   temp;
        AUXCH   caux, aaux;
        AUXCH   caux2, aaux2;  /* Used to hold space for all dynamized arrays */
        MYFLT   prev_time, prev_hdif;
} NREV2;

int vdelset(ENVIRON*,VDEL *p);
int vdelay(ENVIRON*,VDEL *p);
int vdelay3(ENVIRON*,VDEL *p);
int vdelxset(ENVIRON*,VDELX *p);
int vdelxsset(ENVIRON*,VDELXS *p);
int vdelxqset(ENVIRON*,VDELXQ *p);
int vdelayx(ENVIRON*,VDELX *p);
int vdelayxw(ENVIRON*,VDELX *p);
int vdelayxs(ENVIRON*,VDELXS *p);
int vdelayxws(ENVIRON*,VDELXS *p);
int vdelayxq(ENVIRON*,VDELXQ *p);
int vdelayxwq(ENVIRON*,VDELXQ *p);
int multitap_set(ENVIRON*,MDEL *p);
int multitap_play(ENVIRON*,MDEL *p);
int nreverb_set(ENVIRON*,NREV *p);
int nreverb(ENVIRON*,NREV *p);
int reverbx_set(ENVIRON*,NREV2 *p);
int reverbx(ENVIRON*,NREV2 *p);

