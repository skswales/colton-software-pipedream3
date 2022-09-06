/* esch.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       esch.h - escape handling under C
 * Author:      SKS 13-Jan-1989
*/

#ifndef __myclib__esch_h
#define __myclib__esch_h

/* exported functions */

/* Install a new ESCAPE handler, replacing that set by the C runtime library.
 *
 * When an ESCAPE condition is detected, the flag word is set non-zero.
 * When the ESCAPE condition is cleared, the flag word is set to zero.
 *
 * To clear an ESCAPE condition, use OS_Byte 124.
 * To acknowledge (and clear) an ESCAPE condition, use OS_Byte 126.
 * To generate an ESCAPE condition, use OS_Byte 125.
*/
extern void EscH(int *addressofflag);

#endif

/* end of esch.h */
