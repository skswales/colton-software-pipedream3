/* riscmenu.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       riscmenu.h - exported objects from riscmenu.c
 * Author:      Stuart K. Swales 05 Jul 1989
*/

#if RISCOS
/* Only export objects if RISCOS */

#ifndef __pd__riscmenu_h
#define __pd__riscmenu_h

/* exported functions */

extern void riscmenu_attachmenutree(void);
extern void riscmenu_buildmenutree(BOOL short_m);
extern void riscmenu_clearmenutree(void);
extern void riscmenu_detachmenutree(void);
extern void riscmenu_initialise_once(void);
extern void riscmenu_tidy_up(void);

#endif  /* __pd__riscmenu_h */

#endif  /* RISCOS */

/* end of riscmenu.h */
