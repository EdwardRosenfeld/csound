/*
    insert.h:

    Copyright (C) 1991, 2002 Barry Vercoe, Istvan Varga

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

typedef struct {                        /*       INSERT.H                */
    OPDS    h;
    LBLBLK  *lblblk;
} GOTO;

typedef struct {
    OPDS    h;
    int     *cond;
    LBLBLK  *lblblk;
} CGOTO;

typedef struct {
    OPDS    h;
    MYFLT   *ndxvar, *incr, *limit;
    LBLBLK  *l;
} LOOP_OPS;

typedef struct {
    OPDS    h;
    MYFLT   *idel, *idur;
    LBLBLK  *lblblk;
    long    cnt1, cnt2;
} TIMOUT;

typedef struct {
    OPDS    h;
} LINK;

/* the number of optional outputs defined in entry.c */
#define SUBINSTNUMOUTS  8

typedef struct {
    void    *uopcode_struct;
    INSDS   *parent_ip;
    MYFLT   *iobufp_ptrs[4];            /* expandable IV - Oct 26 2002 */
} OPCOD_IOBUFS;

typedef struct {                        /* IV - Oct 16 2002 */
    OPDS    h;
    MYFLT   *ar[VARGMAX];
    INSDS   *ip, *parent_ip;
    AUXCH   saved_spout;
    OPCOD_IOBUFS    buf;
} SUBINST;

typedef struct {                        /* IV - Oct 20 2002 */
    OPDS    h;
    MYFLT   *i_insno, *iname;
} NSTRNUM;

typedef struct {
    OPDS    h;
    MYFLT   *kInsNo, *kFlags, *kRelease;
} TURNOFF2;

typedef struct {
    OPDS    h;
    MYFLT   *insno;
} DELETEIN;

