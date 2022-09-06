/* cs-wimptx.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Borrowed and cut down from PipeDream 4 */

#ifndef __cs_wimptx_h
#define __cs_wimptx_h

#ifndef __wimp_h
#include "wimp.h"
#endif

#ifndef __wimpt_h
#include "wimpt.h"
#endif

/*
cs-wimptx.c
*/

#ifndef RISC_OS_3_5
#define RISC_OS_3_5 0xa5
#endif

extern void
wimptx_os_version_determine(void);

extern int
wimptx_os_version_query(void);

extern int
wimptx_platform_features_query(void);

#endif /* __cs_wimptx_h */

/* end of cs-wimptx.h */
