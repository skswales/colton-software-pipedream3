/* misc.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       misc.h - odds & sods for CWimp library
 * Author:      Stuart K. Swales 07-Mar-1989
*/

#ifndef __wimplib__misc_h
#define __wimplib__misc_h

/* exported functions */

#if !defined(WATCH_ALLOCS)
extern void dispose(void **v);
#endif
extern char *leafname(const char *filename);
#if !defined(max)
extern int   max(int a, int b);
#endif
#if !defined(min)
extern int   min(int a, int b);
#endif
extern char *strset(char **v, const char *str);


/* exported macro definitions */

#if !defined(elif)
#   define  elif        else if
#endif

#if !defined(IGNOREPARM)
#   define  IGNOREPARM(p)   p = p
#endif

#if defined(WATCH_ALLOCS)
#   include "tralloc.h"
#else
#   if !defined(OLD_ALLOCS)
#       include "ext.alloc"
#   endif
#endif


#define FLEX_FAKE_PAGESIZE 1

#endif  /* __wimplib__misc_h */

/* end of misc.h */
