/*
    winFLTK.c: graphs using FLTK library

    Copyright (C) 2002 John ffitch

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
                                                /*    winFLTK.c         */
                                                /* Csound FLTK/X graphs */
                                                /*   jpff,06Oct02       */
#include <stdio.h>
#include "cwindow.h"

extern long MakeWindow(char *);
extern void kill_graph(int);
extern int  myFLwait(void);
extern void *XOpenDisplay(char *);

int Graphable_(CSOUND *csound)      /* called during program initialisation */
{                   /* decides whether to use X or not; initializes X if so */
    int         rc = 0;             /* default : don't use X, use tty ascii */
#if defined(USE_FLTK)
    rc = 1;
#else
    if (XOpenDisplay(NULL)) rc = 1;  /*       so set return code  */
#endif
    return(rc);
}

void MakeGraph_(CSOUND *csound, WINDAT *wdptr, char *name)
{
    wdptr->windid = MakeWindow(name);
}

void KillGraph_(CSOUND *csound, WINDAT *wdptr)
{
    kill_graph(wdptr->windid);
}

/* print click-Exit message in most recently active window */

int ExitGraph_(CSOUND *csound)
{
    const char *env = csoundGetEnv(csound, "CSNOSTOP");
    if (env == NULL || strcmp(env, "yes") == 0)
      myFLwait();
    return 0;
}

