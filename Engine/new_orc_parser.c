 /*
    new_orc_parser.c:

    Copyright (C) 2006
    Steven Yi

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
#include "csound_orcparse.h"
#include "csound_orc.h"

#define ST(x)   (((RDORCH_GLOBALS*) csound->rdorchGlobals)->x)

extern FILE *csound_orcin;

extern int csound_orcdebug;

extern int csound_orcparse(CSOUND*, TREE*);
extern void init_symbtab();

void new_orc_parser(CSOUND *csound)
{
    void *t;
    int retVal;
    TREE* astTree;

    astTree = (TREE *)mcalloc(csound, sizeof(TREE));

    OPARMS *O = csound->oparms;

    init_symbtab(csound);

    csound->Message(csound, "Testing...\n");

    if ((t = csound->FileOpen(csound, &csound_orcin, CSFILE_STD,
                              csound->orchname, "rb", NULL)) == NULL)
      csoundDie(csound, Str("cannot open orch file %s"), csound->orchname);

    csound_orcdebug = O->odebug;

    csound_orcrestart(csound_orcin);

    retVal = csound_orcparse(csound, astTree);

    if (retVal == 0) {
      csound->Message(csound, "Parsing successful!\n");
    } else if (retVal == 1){
      csound->Message(csound, "Parsing failed due to invalid input!\n");
    } else if (retVal == 2){
      csound->Message(csound, "Parsing failed due to memory exhaustion!\n");
    }

    print_tree(csound, astTree);

    verify_tree(csound, astTree);
    astTree = csound_orc_expand_expressions(csound, astTree);

    print_tree(csound, astTree);

    astTree = csound_orc_optimize(csound, astTree);
    csound_orc_compile(csound, astTree);

}

