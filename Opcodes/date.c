/*
    date.c:

    Copyright (C) 2006 John ffitch

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
#include <time.h> 

typedef struct {
   OPDS h;
   MYFLT *time_;
} DATEMYFLT;

typedef struct {
   OPDS h;
   MYFLT *Stime_;
   MYFLT *timstmp;
} DATESTRING;

static int datemyfltset(CSOUND *csound, DATEMYFLT *p)
{
    *p->time_ = (MYFLT) time(NULL);
    return OK;
}

static int datestringset(CSOUND *csound, DATESTRING *p)
{
    time_t temp_time;
    char *time_string;
    int32 tmp;
#if defined(MSVC) || (defined(__GNUC__) && defined(__i386__))
    tmp = (int32) MYFLT2LRND(*(p->timstmp));
#else
    tmp = (int32) (*(p->timstmp) + FL(0.5));
#endif
    if (tmp < 0) {
      temp_time = time(NULL);
/*       printf("Timestamp = %f\n", *p->timstmp); */
    } else {
      temp_time = (time_t)tmp;
/*       printf("Timestamp = %f\n", *p->timstmp); */
    }
    time_string = ctime(&temp_time);
    ((char*) p->Stime_)[0] = '\0';
    if ((int) strlen(time_string) >= csound->strVarMaxLen) {
      return csound->InitError(csound, Str("dates: buffer overflow"));
    }
    strcpy((char*) p->Stime_, time_string);
    return OK;
}

static OENTRY localops[] = 
{
    { "date",    sizeof(DATEMYFLT),     1,     "i",    "",(SUBR)datemyfltset, NULL, NULL },
    { "dates",   sizeof(DATESTRING),    1,     "S",    "j",(SUBR)datestringset, NULL, NULL },
};

LINKAGE

