/* copymem.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       copymem.h - local header for copymem.s
 * Author:      Stuart K. Swales 25-Sep-1989
*/

#ifndef __wimplib__copymem_h
#define __wimplib__copymem_h

/* copies n characters from the object pointed to by s2 into the object
 * pointed to by s1. Copying takes place as if the n characters from the
 * object pointed to by s2 are first copied into a temporary array of n
 * characters that does not overlap the objects pointed to by s1 and s2,
 * and then the n characters from the temporary array are copied into the
 * object pointed to by s1.
*/
extern void copymem(void *s1, const void *s2, size_t n);

#if defined(OWN_MEMCPY)
/* note: important difference is that copymem does not yield s1 as result */
#define memcpy(s1, s2, n)   copymem(s1, s2, n)
#define memmove(s1, s2, n)  copymem(s1, s2, n)
#endif

#endif  /* __wimplib__copymem_h */

/* end of copymem.h */
