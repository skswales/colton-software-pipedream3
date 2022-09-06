/* txtwin.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __txtwin_h
#define __txtwin_h
#ifndef __txt_h
#include "txt.h"
#endif
 
void txtwin_new(txt t);
int txtwin_number(txt t);
void txtwin_dispose(txt t);
void txtwin_setcurrentwindow(txt t);
#endif
