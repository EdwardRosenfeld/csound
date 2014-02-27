/*
 csound_standard_types.c:

 Copyright (C) 2012,2013 Steven Yi

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
#include "csound_standard_types.h"
#include "pstream.h"
#include <stdlib.h>

//#define Wfloats (((int) sizeof(SPECDAT) + 7) / (int) sizeof(MYFLT))
//#define Pfloats (((int) sizeof(PVSDAT) + 7) / (int) sizeof(MYFLT))

void updateAsigMemBlock(void* csound, CS_VARIABLE* var) {
    CSOUND* cs = (CSOUND*)csound;
    int ksmps = cs->ksmps;
    var->memBlockSize = ksmps * sizeof (MYFLT);
}

void varInitMemory(CS_VARIABLE* var, MYFLT* memblock) {
    memset(memblock, 0, var->memBlockSize);
}


CS_VARIABLE* createAsig(void* cs, void* p) {
    int ksmps;
    CSOUND* csound = (CSOUND*)cs;
    IGN(p);

    //FIXME - this needs to take into account local ksmps, once
    //context work is complete
//    if(instr != NULL) {
//      OPDS* p = (OPDS*)instr;
//      ksmps = CS_KSMPS;
//    } else {
      ksmps = csound->ksmps;
//    }

    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = ksmps * sizeof (MYFLT);
    var->updateMemBlockSize = &updateAsigMemBlock;
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createMyflt(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof (MYFLT);
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createBool(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof (MYFLT);
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createWsig(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof(SPECDAT);
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createFsig(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof(PVSDAT);
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

void arrayInitMemory(CS_VARIABLE* var, MYFLT* memblock) {
    ARRAYDAT* dat = (ARRAYDAT*)memblock;
    dat->arrayType = var->subType;
}

CS_VARIABLE* createString(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof(STRINGDAT);
    return var;
}

CS_VARIABLE* createArray(void* csnd, void* p) {
    CSOUND* csound = (CSOUND*)csnd;
    ARRAY_VAR_INIT* state = (ARRAY_VAR_INIT*)p;


    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = sizeof(ARRAYDAT);
    var->initializeVariableMemory = &arrayInitMemory;

    if(state) { // NB: this function is being called with p=NULL
    CS_TYPE* type = state->type;
    var->subType = type;
    var->dimensions = state->dimensions;
    }
    return var;
}


//#define ARGTYP_S        0x00000040L     /* string constant or variable */
//#define ARGTYP_l        0x00000800L     /* label */

const CS_TYPE CS_VAR_TYPE_A = {
  "a", "audio rate vector", CS_ARG_TYPE_BOTH, createAsig, NULL
};

const CS_TYPE CS_VAR_TYPE_K = {
  "k", "control rate var", CS_ARG_TYPE_BOTH, createMyflt, NULL
};

const CS_TYPE CS_VAR_TYPE_I = {
  "i", "init time var", CS_ARG_TYPE_BOTH, createMyflt, NULL
};

const CS_TYPE CS_VAR_TYPE_S = {
    "S", "String var", CS_ARG_TYPE_BOTH, createString, NULL
};

const CS_TYPE CS_VAR_TYPE_P = {
  "p", "p-field", CS_ARG_TYPE_BOTH, createMyflt, NULL
};

const CS_TYPE CS_VAR_TYPE_R = {
  "r", "reserved symbol", CS_ARG_TYPE_BOTH, createMyflt, NULL
};

const CS_TYPE CS_VAR_TYPE_C = {
  "c", "constant", CS_ARG_TYPE_IN, createMyflt, NULL
};

const CS_TYPE CS_VAR_TYPE_W = {
  "w", "spectral", CS_ARG_TYPE_BOTH, createWsig, NULL
};

const CS_TYPE CS_VAR_TYPE_F = {
  "f", "f-sig", CS_ARG_TYPE_BOTH, createFsig, NULL
};

const CS_TYPE CS_VAR_TYPE_B = {
  "B", "boolean", CS_ARG_TYPE_BOTH, createBool, NULL
};

const CS_TYPE CS_VAR_TYPE_b = {
  "b", "boolean", CS_ARG_TYPE_BOTH, createBool, NULL
};

const CS_TYPE CS_VAR_TYPE_ARRAY = {
   "[", "array", CS_ARG_TYPE_BOTH, createArray, NULL
};



void csoundAddStandardTypes(CSOUND* csound, TYPE_POOL* pool) {

    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_K);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_I);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_S);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_P);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_R);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_C);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_W);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_F);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_B);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_b);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_ARRAY);

}


/* Type maps for poly, optional, and var arg types
 * format is in pairs of specified type and types it can resolve into,
 * termintated by a NULL */
const char* POLY_IN_TYPES[] = {
    "x", "kacpri",
    "T", "Sicpr",
    "U", "Sikcpr",
    "i", "cpri",
    "k", "cprki",
    "B", "Bb", NULL};
const char* OPTIONAL_IN_TYPES[] = {
    "o", "icpr",
    "p", "icpr",
    "q", "icpr",
    "v", "icpr",
    "j", "icpr",
    "h", "icpr",
    "O", "kicpr",
    "J", "kicpr",
    "V", "kicpr",
    "P", "kicpr", NULL
};
const char* VAR_ARG_IN_TYPES[] = {
    "m", "icrp",
    "M", "icrpka",
    "N", "icrpkaS",
    "n", "icrp",   /* this one requires odd number of args... */
    "W", "S",
    "y", "a",
    "z", "kicrp",
    "Z", "kaicrp",  NULL  /* this one needs to be ka alternatating... */
};

const char* POLY_OUT_TYPES[] = {
    "s", "ka",
    "i", "pi", NULL
};

const char* VAR_ARG_OUT_TYPES[] = {
    "m", "a",
    "z", "k",
    "I", "Sip", /* had comment of (not implemented yet) in entry1.c */
    "X", "akip",
    "N", "akipS",
    "F", "f", NULL
};
