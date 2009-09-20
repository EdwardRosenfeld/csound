/*
    (c) Victor Lazzarini, 2007

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
#include "pstream.h"

typedef struct {
  PVSDAT  header;
  float   *data;
  uint32  frames;
} FSIG_HANDLE;

typedef struct {
  OPDS h;
  MYFLT  *hptr;
  MYFLT  *ktime;
  PVSDAT *fin;
  MYFLT  *len;
  uint32 nframes;
  uint32 cframes;
  FSIG_HANDLE handle;
  AUXCH  buffer;
  uint32 lastframe;
} PVSBUFFER;

static int pvsbufferset(CSOUND *csound, PVSBUFFER *p)
{
    int N, hop;
#ifndef OLPC
    if (UNLIKELY(p->fin->sliding))
      return csound->InitError(csound, Str("SDFT case not implemented yet"));
#endif
    p->handle.header.N = N = p->fin->N;
    p->handle.header.overlap = hop = p->fin->overlap;
    p->handle.header.winsize = p->fin->winsize;
    p->handle.header.wintype = p->fin->wintype;
    p->handle.header.format  = p->fin->format;
    p->handle.header.framecount = p->fin->framecount;
    p->nframes = p->handle.frames = (*p->len) * csound->esr/hop;
    if (p->buffer.auxp == NULL ||
        p->buffer.size < sizeof(float) * (N + 2) * p->nframes)
      csound->AuxAlloc(csound, (N + 2) * sizeof(float) * p->nframes, &p->buffer);
    else
      memset(p->buffer.auxp, 0, (N + 2) * sizeof(float) * p->nframes);

    p->handle.header.frame.auxp = p->buffer.auxp;
    p->handle.header.frame.size = p->buffer.size;
    p->handle.data = (float *)  p->buffer.auxp;
    *p->hptr = (MYFLT) ((long)&p->handle);
    p->lastframe = 0;
    p->cframes = 0;
    *p->ktime = FL(0.0);
    return OK;
}

static int pvsbufferproc(CSOUND *csound, PVSBUFFER *p)
{
     float *fin = p->fin->frame.auxp;

    if (p->lastframe < p->fin->framecount) {
      int32 framesize = p->fin->N + 2, i;
      float *fout = (float *) p->buffer.auxp;
      fout += framesize*p->cframes;
      for(i=0;i < framesize; i+=2) {
        fout[i] = fin[i];
        fout[i+1] = fin[i+1];
      }
      p->handle.header.framecount = p->lastframe = p->fin->framecount;
      p->cframes++;
      if(p->cframes == p->nframes)p->cframes = 0;
    }
     *p->ktime = p->cframes/(csound->esr/p->fin->overlap);

    return OK;
}


typedef struct {
  OPDS h;
  PVSDAT *fout;
  MYFLT  *ktime;
  MYFLT *hptr;
  MYFLT *strt;
  MYFLT *end;
  int scnt;
} PVSBUFFERREAD;

static int pvsbufreadset(CSOUND *csound, PVSBUFFERREAD *p)
{
    int N;
    FSIG_HANDLE *handle;
    handle = (FSIG_HANDLE *) ((long)*p->hptr);
    if (handle != NULL) {
      p->fout->N = N = handle->header.N;
      p->fout->overlap = handle->header.overlap;
      p->fout->winsize = handle->header.winsize;
      p->fout->wintype = handle->header.wintype;
      p->fout->format  = handle->header.format;
      p->fout->framecount = 1;
    }
    else {
      p->fout->N = N = 1024;
      p->fout->overlap = 256;
      p->fout->winsize = 1024;
      p->fout->wintype = 1;
      p->fout->format  = PVS_AMP_FREQ;
      p->fout->framecount = 1;
    }

    if (p->fout->frame.auxp == NULL ||
         p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
#ifndef OLPC
    p->fout->sliding = 0;
#endif
    p->scnt = p->fout->overlap;
    return OK;
}

 static int pvsbufreadproc(CSOUND *csound, PVSBUFFERREAD *p){

    unsigned int posi, frames;
    MYFLT pos, sr = csound->esr;
    FSIG_HANDLE *handle = (FSIG_HANDLE *) ((long)*p->hptr);
    MYFLT frac;
    float *fout, *buffer;
    int strt = *p->strt, end = *p->end, overlap, i, N;
    if(handle == NULL) goto err1;

    fout = (float *) p->fout->frame.auxp,
    buffer = handle->data;
    N = p->fout->N;
    overlap = p->fout->overlap;

    if(p->scnt >= overlap){
      float *frame1, *frame2;
      strt /= (sr/N);
      end /= (sr/N);
      strt = (int)(strt < 0 ? 0 : strt > N/2 ? N/2 : strt);
      end = (int)(end <= strt ? N/2 + 2 : end > N/2 + 2 ? N/2 + 2 : end);
      frames = handle->frames-1;
      pos = *p->ktime*(sr/overlap) - 1;

      while(pos >= frames) pos -= frames;
      while(pos < 0) pos += frames;
      posi = (int) pos;
      if(N == handle->header.N &&
         overlap == handle->header.overlap){
        frame1 = buffer + (N + 2) * posi;
        frame2 = buffer + (N + 2)*(posi != frames-1 ? posi+1 : 0);
      frac = pos - posi;

      for(i=strt; i < end; i+=2){
        fout[i] = frame1[i] + frac*(frame2[i] - frame1[i]);
        fout[i+1] = frame1[i+1] + frac*(frame2[i+1] - frame1[i+1]);
        }
      }
      else
       for(i=0; i < N+2; i+=2){
        fout[i] = 0.0f;
        fout[i+1] = 0.0f;
      }
      p->scnt -= overlap;
      p->fout->framecount++;
    }
    p->scnt += csound->ksmps;

    return OK;
 err1:
    return csound->PerfError(csound, Str("Invalid buffer handle"));
  }

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"pvsbuffer", S(PVSBUFFER), 3, "ik", "fi", (SUBR)pvsbufferset, (SUBR)pvsbufferproc, NULL},
  {"pvsbufread", S(PVSBUFFERREAD), 3, "f", "kkOO", (SUBR)pvsbufreadset, (SUBR)pvsbufreadproc, NULL}
};

LINKAGE
