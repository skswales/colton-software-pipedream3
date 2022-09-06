/* misc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       misc.c - odds & sods for CWimp library
 * Author:      Stuart K. Swales 07-Mar-1989
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "trace.h"

#include "misc.h"


/* exported functions */

#if !defined(WATCH_ALLOCS)
extern void dispose(void **v);
#endif
extern char *leafname(const char *filename);
extern int max(int a, int b);
extern int min(int a, int b);
extern char *strset(char **v, const char *str);


/* ----------------------------------------------------------------------- */

extern int
max(int a, int b)
{
    return(a > b ? a : b);
}


extern int
min(int a, int b)
{
    return(a < b ? a : b);
}


/* allocate a new string variable */

extern char *
strset(char **v, const char *str)
{
    int len = strlen(str) + 1;
    char *ptr;

    dispose((void **) v);

    ptr = malloc(len);
    if(ptr != NULL)
        {
        strcpy(ptr, str);
        *v = ptr;
        }

    return(ptr);
}


/* find the leafname in a filename */

extern char *
leafname(const char *filename)
{
    const char *leaf = filename + strlen(filename); /* point to null */
    int ch; 

    while(leaf > filename)
        if(((ch = *--leaf) == '.')  ||  (ch == ':'))
            return((char *) leaf+1);

    return((char *) leaf);
}


/* free a variable */

#if defined(dispose)
#   undef   dispose
#endif

extern void
dispose(void **v)
{
    tracef2("dispose(&%p -> &%p", v, *v);

    if(v  &&  *v)
        {
        *v = NULL;
        free(*v);
        }

    tracef0(")\n");
}

/* end of misc.c */
