/*  
    lpc_import.c

    Copyright (C) 1995 John ffitch

    This file is part of Csound.

    Csound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* ***************************************************************** */
/* ******** Program to import lpanal files in tabular format. ****** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1998 Nov 15                                           */
/* ***************************************************************** */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "cs.h"
#include "lpc.h"

void lpc_import_usage(void)
{
    exit(1);
}

int main(int argc, char **argv)
{
    FILE *inf;
    FILE *outf;
    LPHEADER hdr;
    int i, j;
    char *str;
    MYFLT *coef;

    init_getstring(0, NULL);
    csoundPreCompile(csoundCreate(NULL));
    if (argc!= 3)
      lpc_import_usage();
    inf = fopen(argv[1], "rb");
    if (inf == NULL) {
      fprintf(stderr, Str("Cannot open input file %s\n"), argv[1]);
      exit(1);
    }
    outf = fopen(argv[2], "w");
    if (outf == NULL) {
      fprintf(stderr, Str("Cannot open output file %s\n"), argv[2]);
      exit(1);
    }
    if (fread(&hdr, sizeof(LPHEADER)-4, 1, inf) != 1 ||
        (hdr.lpmagic != LP_MAGIC && hdr.lpmagic != LP_MAGIC2)) {
      fprintf(stderr, Str("Failed to read LPC header\n"));
      exit(1);
    }
    fprintf(outf, "%ld,%ld,%ld,%ld,%f,%f,%f",
            hdr.headersize, hdr.lpmagic, hdr.npoles, hdr.nvals,
            hdr.framrate, hdr.srate, hdr.duration);
    str = (char *)malloc(hdr.headersize-sizeof(LPHEADER)+4);
    fread(&hdr, sizeof(char), hdr.headersize-sizeof(LPHEADER)+4, inf);
    for (i=0; i<hdr.headersize-sizeof(LPHEADER)+4; i++)
      putc(str[i],outf);
    putc('\n', outf);
    coef = (MYFLT *)malloc((hdr.npoles+hdr.nvals)*sizeof(MYFLT));
    for (i = 0; i<hdr.nvals; i++) {
      fread(&coef[0], sizeof(MYFLT), hdr.npoles, inf);
      for (j=0; j<hdr.npoles; j++)
        fprintf(outf, "%f%c", coef[j], (j==hdr.npoles-1 ? '\n' : ','));
    }
    fclose(outf);
    fclose(inf);
    return 0;
}

