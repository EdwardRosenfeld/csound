/*
    auxfd.c:

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

#include "cs.h"                                 /*      AUXFD.C         */
#include <sndfile.h>

static void auxrecord(void*, AUXCH *), auxchprint(INSDS *), fdchprint(INSDS *);

void csoundAuxAlloc(void *csound, long nbytes, AUXCH *auxchp)
     /* allocate an auxds, or expand an old one */
     /*    call only from init (xxxset) modules */
{
    void *auxp;

    if ((auxp = auxchp->auxp) != NULL)  /* if size change only,      */
      mfree(csound, auxp);              /*      free the old space   */
    else auxrecord(csound, auxchp);     /* else linkin new auxch blk */
    auxp = mcalloc(csound, nbytes);     /* now alloc the space       */
    auxchp->size = nbytes;              /* update the internal data  */
    auxchp->auxp = auxp;
    auxchp->endp = (char*) auxp + nbytes;
    if (O.odebug) auxchprint(((ENVIRON*) csound)->curip_);
}

static void auxrecord(void *csound, AUXCH *auxchp)
     /* put auxch into chain of xp's for this instr */
     /* called only from auxalloc       */
{
    AUXCH       *prvchp, *nxtchp;

    prvchp = &(((ENVIRON*) csound)->curip_->auxch); /* from current insds,  */
    while ((nxtchp = prvchp->nxtchp) != NULL)   /* chain through xplocs */
      prvchp = nxtchp;
    prvchp->nxtchp = auxchp;                    /* then add this xploc  */
    auxchp->nxtchp = NULL;                      /* & terminate the chain */
}

void fdrecord(FDCH *fdchp)    /* put fdchp into chain of fd's for this instr */
                              /*      call only from init (xxxset) modules   */
{
    FDCH        *prvchp, *nxtchp;

    prvchp = &curip->fdch;                      /* from current insds,  */
    while ((nxtchp = prvchp->nxtchp) != NULL)   /* chain through fdlocs */
      prvchp = nxtchp;
    prvchp->nxtchp = fdchp;                     /* then add this fdloc  */
    fdchp->nxtchp = NULL;                       /* & terminate the chain */
    if (O.odebug) fdchprint(curip);
}

void fdclose(FDCH *fdchp)       /* close a file and remove from fd chain */
                                /*  call only from inits, after fdrecord */
{
    FDCH        *prvchp, *nxtchp;

    prvchp = &curip->fdch;                      /* from current insds,  */
    while ((nxtchp = prvchp->nxtchp) != NULL) { /* chain through fdlocs */
      if (nxtchp == fdchp) {                    /*   till find this one */
        if (fdchp->fd)
          sf_close(fdchp->fd);                  /* then close the file  */
        else
          close(fdchp->fdc);                    /* then close the file  */
        fdchp->fd = 0;                          /*   delete the fd &    */
        prvchp->nxtchp = fdchp->nxtchp;         /* unlnk from fdchain   */
        if (O.odebug) fdchprint(curip);
        return;
      }
      else prvchp = nxtchp;
    }
    fdchprint(curip);
    csoundDie(&cenviron, Str("fdclose: no record of fd %d"), fdchp->fd);
}

void auxchfree(void *csound, INSDS *ip) /* release all xds in instr auxp chain*/
                                        /*   called by insert at orcompact    */
{
    AUXCH       *curchp = &ip->auxch;
    char        *auxp;

    if (O.odebug) auxchprint(ip);
    while ((curchp = curchp->nxtchp) != NULL) { /* for all xp's in chain: */
      if ((auxp = curchp->auxp) == NULL) {
        auxchprint(ip);
        csoundDie(csound, Str("auxchfree: illegal auxp %lx in chain"), auxp);
      }
      mfree(csound, auxp);                      /*      free the space  */
      curchp->auxp = NULL;              /*      & delete the pntr */
    }
    ip->auxch.nxtchp = NULL;            /* finally, delete the chain */
    if (O.odebug) auxchprint(ip);
}

void fdchclose(INSDS *ip)   /* close all files in instr fd chain     */
                            /*   called by insert on deact & expire  */
{                           /*   (also musmon on s-code, & fgens for gen01) */
    FDCH        *curchp = &ip->fdch;

    if (O.odebug) fdchprint(ip);
      while ((curchp = curchp->nxtchp) != NULL) { /* for all fd's in chain: */
        if (curchp->fd)
          sf_close(curchp->fd);
        else
          close(curchp->fdc);           /*      close the file  */
        curchp->fd = NULL;
        curchp->fd = 0;                 /*      & delete the fd */
      }
      ip->fdch.nxtchp = NULL;           /* finally, delete the chain */
      if (O.odebug) fdchprint(ip);
}

static void auxchprint(INSDS *ip)   /* print the xp chain for this insds blk */
{
    AUXCH       *curchp = &ip->auxch;

    printf(Str("auxlist for instr %d (%lx):\n"), ip->insno, ip);
    while ((curchp = curchp->nxtchp) != NULL)         /* chain through auxlocs */
      printf(Str("\tauxch at %lx: size %lx, auxp %lx, endp %lx\n"),
             curchp, curchp->size, curchp->auxp, curchp->endp);
}

static void fdchprint(INSDS *ip)   /* print the fd chain for this insds blk */
{
    FDCH        *curchp = &ip->fdch;

    printf(Str("fdlist for instr %d (%lx):"), ip->insno, ip);
    while ((curchp = curchp->nxtchp) != NULL)    /* chain through fdlocs */
      printf(Str("  fd %p/%d in %lx"), curchp->fd, curchp->fdc, curchp);
    printf("\n");
}

