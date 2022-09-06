/* flags.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       flags.h - defines the compile-time flags for PipeDream
 * Author:      Stuart K. Swales 14-Feb-1989
*/

#ifndef __pd__flags_h
#define __pd__flags_h

#if !defined(RELEASED)
#define RELEASED 0
#endif


/* target machine definition */

#include "selectos.h"

#include "defineos.h"


/* ------------------ features that are now standard --------------------- */

#define EXT_REFS
#define MACRO_RECORDER
#define MANY_WINDOWS
#define MANY_DOCUMENTS
#define PROTECTION
#define SCROLL_WINDOW
#define NEW_CSV
#define SPOOL_HIGHLIGHTS
#define NEW_WRAP
#define RECALC_BOX
#define SAVE_FIXES
#define SAVE_DELETED_BLOCK
#define FAST_SORT
#define UNDERLINE_SPACES
#define BROWSE_FOREIGN_CHARS
#define EXPORT_FIXED_ALLOCS
#define DONT_CLEAR_DRAWFILES
#define NOLIMIT
#define SHOW_CURRENT_ROW

#define SG_MENU_ITEMS /* SKS 05sep22 */

#if ARTHUR || RISCOS
#define DTP_EXPORT
#define VIEW_IO
#endif

#if MS
#define BYTES_FREE
#define VIEWSHEET_OFF
#endif


/* -------------- new features, not in release version yet --------------- */

#if !RELEASED
#endif


/* ---------- paranoid checks to ensure kosher release version ----------- */

#if RELEASED && !defined(DEBUG)
/* Turn off assert checking */
#define NDEBUG

/* Turn off any trace information */
#define TRACE 0

#if defined(SMALLPD)
#   error   Cannot release version with SMALLPD defined
#endif

#if defined(POINTER_CHECK)
#   error   Cannot release version with POINTER_CHECK defined
#endif

#if defined(WATCH_ALLOCS)
#   error   Cannot release version with WATCH_ALLOCS defined
#endif

#endif  /* RELEASED */


/* ----------------------- SMALLPD specific flags ------------------------ */

#if defined(SMALLPD)

#define LOTUS_OFF
#define MULTI_OFF 
#define SPELL_OFF
#define Z88_OFF 

#endif  /* SMALLPD */


/* ------------------------ DEMO specific flags -------------------------- */

#if defined(DEMO)

#define PRINT_OFF
#ifndef LOTUS_OFF
#define LOTUS_OFF
#endif
#define SAVE_OFF
/* #define SEARCH_OFF */
#define SPELL_OFF

#endif  /* DEMO */


/* ----------------------- RISC OS specific flags ------------------------ */

#if RISCOS
#define HEADLINE_OFF
#define SHORT_BORDER
#define GRID_ON
#define HELP_FILE
#if !defined(SPELL_OFF)
#define SPELL_BOUND
#endif
#define Z88FS

#if !defined(MANY_DOCUMENTS)
#   define  MANY_DOCUMENTS
#endif

#if !defined(MANY_WINDOWS)
#   define  MANY_WINDOWS
#endif


/* Permanently out */
#define FILE_SELECTOR_OFF
#define Z88_OFF
#ifndef LOTUS_OFF
#define LOTUS_OFF
#endif

#endif  /* RISCOS */


/* ----------------------- MS-DOS specific flags ------------------------- */

#if MS

#define OVERLAYS

#define HELP_OVERLAY
#define SPELL_OVERLAY
#define SEARCH_OVERLAY
#define PRINT_OVERLAY
#define FILES_OVERLAY

#endif  /* MS */


#if defined(POINTER_CHECK)
/* Check pointer values are plausible before dereference */
#   if RISCOS
#       pragma -c1
#   endif
#endif


#endif

/* standard header files after all flags defined */
#include "include.h"

/* end of flags.h */
