/* upcall.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       upcall.h - include this if the application is called back
 *                          from a source file.
 * Author:      SKS 8-Apr-1989
*/

#ifndef __wimplib__upcall_h
#define __wimplib__upcall_h

#if defined(CLIENT_GLOBAL_REGISTERS)

/* PipeDream is storing something in v5 */
#pragma -r5
extern void *application_has_v5_as_global;
#pragma -r

#endif

#endif  /* __wimplib__upcall_h */

/* end of upcall.h */
