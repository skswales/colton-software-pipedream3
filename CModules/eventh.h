/* eventh.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       eventh.h - event blocking under C
 * Author:      Stuart K. Swales 29-Jun-1989
*/

#ifndef __myclib__eventh_h
#define __myclib__eventh_h

/* exported functions */

/* Install a new event handler, replacing that set by the C runtime library.
*/
extern void *EventH(void);

#endif

/* end of event.h */
