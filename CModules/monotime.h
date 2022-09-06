/* monotime.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       monotime.h - exported objects from monotime.s
 * Author:      Stuart K. Swales 28-Apr-1989
*/

#ifndef __monotime_h
#define __monotime_h

typedef unsigned long int monotime_t;

/* exported functions */

/* return the current monotonic time.
 * (in centiseconds since the machine last executed a power-on reset)
*/
extern monotime_t monotime(void);

/* return the difference between the current monotonic time and a previous
 * value thereof.
*/
extern monotime_t monotime_diff(monotime_t oldtime);

#endif  /* __monotime_h */

/* end of monotime.h */
