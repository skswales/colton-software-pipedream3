/* cs-wimptx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Borrowed and cut down from PipeDream 4 */

#include "include.h"

#include "cs-wimptx.h"

#include "kernel.h"

static int wimptx__os_version;

static int wimptx__platform_features; /* 19aug96 */

extern void
wimptx_os_version_determine(void)
{
    wimptx__os_version = (_kernel_osbyte(0x81, 0, 0xFF) & 0xFF);

    { /* 19aug96 */
    _kernel_swi_regs rs;
    rs.r[0] = 0; /*Read code features*/
    if(NULL == _kernel_swi(/*OS_PlatformFeatures*/ 0x6D, &rs, &rs))
        wimptx__platform_features = rs.r[0];
    } /*block*/
}

extern int
wimptx_os_version_query(void)
{
    return(wimptx__os_version);
}

extern int
wimptx_platform_features_query(void)
{
    return(wimptx__platform_features);
}

/* end of cs-wimptx.c */
