/* defineos.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* some target-specific definitions for the target machine and OS */

#ifndef _defineos_h
#define _defineos_h

#if !(MS || WINDOWS || ARTHUR || RISCOS)
#error Unable to compile code for this OS 
#endif


#if MS || WINDOWS
/* use wherever possible */
#define LINT_ARGS
#endif


/* sizes of data pointers */

#if MS || WINDOWS
#define FAR     far
#define NEAR    near
#else
#define FAR
#define NEAR
#endif

#define FARP    FAR *
#define NEARP   NEAR *


/* language calling conventions */

#if MS || WINDOWS
#define CDECL   cdecl
#define PASCAL  pascal
#else
#define CDECL
#define PASCAL
#endif


/* addressability of function pointers */

#if WINDOWS
    /* main entry point for a Windows program:
     * if mixed model then we must enable the OS to far call this function.
    */ 
#   if LLIB && (defined(M_I86SM) || defined(M_I86CM))
#       define WINENTRY FAR PASCAL
#   else
#       define WINENTRY PASCAL
#   endif
#else
    /* main entry point for a DOS/ARTHUR/RISC OS program:
     * if mixed model then we must enable the C library to far call this function.
    */ 
#   if LLIB && (defined(M_I86SM) || defined(M_I86CM))
#       define MAINENTRY FAR CDECL
#   else
#       define MAINENTRY CDECL
#   endif
#endif


/* function may only be exported to and called back from the OS:
 * it may NOT be called directly from any part of the program.
*/
#define EXPENTRY    FAR PASCAL


/* function may be called from outwith the current segment:
 * however it is still not exported to the OS.
*/
#define EXTERNAL    FAR


/* function only called from within current segment: 
 * may have several object files combined in one segment.
*/
#define INTERNAL    NEAR


/* function pointers */

#if LLIB && (defined(M_I86SM) || defined(M_I86CM))
    /* Application-Library-Interface Entry */
#   define ALIENTRY FAR CDECL

    /* Library-Application-Interface Entry (eg. qsort callback) */
#   define LAIENTRY FAR CDECL
#else
#   define ALIENTRY CDECL
#   define LAIENTRY CDECL
#endif


/* data pointers */

#if LLIB && (defined(M_I86SM) || defined(M_I86MM))
    /* A pointer suitable for passing to the library */
#   define ALIP     FARP
#else
#   define ALIP     *
#endif


/* include patch functions/macros for compiler internal procs */
#if ARTHUR || RISCOS
#include "ext.mscpatch"
#else
#include "mscpatch.ext"
#endif


/* define this as needed for ARM targets */
#if defined(__CC_NORCROFT)
#if !defined(__CC_NORCROFT_VERSION)
#if defined(__APCS_32)
/* 32-bit target: assume we are using C Release 5, which doesn't set this (later than cc 5.11 do) */
#define __CC_NORCROFT_VERSION 509
#else
/* 26-bit target: assume we are using C Release 3.1B (to target RISC OS 2), which doesn't set this */
#define __CC_NORCROFT_VERSION 311
#endif
#endif /* __CC_NORCROFT_VERSION */
#endif /* __CC_NORCROFT */

#endif /* _defineos_h */

/* end of defineos.h */
