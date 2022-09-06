/* screen.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * header for PipeDream screen drawing modules
 * RJM August 1987
*/

/*
 * scinfo.c
*/

extern BOOLEAN inrowfixes1(rowt);
#define inrowfixes(trow) ((vertvec->flags & FIX) && inrowfixes1(trow))

extern void cencol(colt);
extern void cenrow(rowt);
#define CALL_FIXPAGE 1
#define DONT_CALL_FIXPAGE 0

extern BOOLEAN incolfixes(colt);
extern coord schcsc(colt);              /* search for col on screen */
extern coord schrsc(rowt);              /* search for row on screen */
extern rowt fstnrx(void);
extern colt fstncx(void);
extern coord calcad(coord);
extern BOOLEAN chkpbs(rowt, intl);
extern BOOLEAN chkpac(rowt);
extern intl calsiz(uchar *);
extern void curosc(void);               /* check cursor on screen */
extern void chkmov(void);

/*
 * c.cursmov
*/

extern void curup(void);
extern void curdown(void);
extern void prevcol(void);
extern void nextcol(void);
extern void mark_row(coord);
extern void cursdown(void);
extern void cursup(void);
extern coord calcoff(coord xpos);
extern coord calcoff_overlap(coord xpos, rowt trow);

/* end of screen.h */
