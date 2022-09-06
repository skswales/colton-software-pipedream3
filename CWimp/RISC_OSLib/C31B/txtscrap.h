/* txtscrap.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __txtscrap_h
#define __txtscrap_h
#ifndef __txt_h
#include "txt.h"
#endif
void txtscrap_setselect(txt t, txt_index from, txt_index to);
txt txtscrap_selectowner(void);
#endif
