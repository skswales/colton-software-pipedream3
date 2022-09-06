/* baricon.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __baricon_h
#define __baricon_h
#ifndef __wimp_h
#include "wimp.h"
#endif
typedef void (*baricon_clickproc)(wimp_i);
 
wimp_i baricon(char *spritename, int spritearea, baricon_clickproc p);
 
wimp_i baricon_left(char *spritename, int spritearea, baricon_clickproc p);
wimp_i baricon_newsprite(char *newsprite);
#endif
