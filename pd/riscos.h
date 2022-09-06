/* riscos.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       riscos.h - local header file for riscos.c
 * Author:      Stuart K. Swales 25 Jan 1989
*/

#ifndef __pd__riscos_h
#define __pd__riscos_h

#if RISCOS

#include "swinumbers.h"

#include "trace.h"

#include "os.h"
#include "bbc.h"
#include "res.h"
#include "sprite.h"
#include "resspr.h"
#include "akbd.h"
#include "wimp.h"
#include "wimpt.h"
#include "werr.h"
#include "win.h"
#include "menu.h"
#include "event.h"
#include "baricon.h"
#include "dbox.h"
#include "flex.h"
#include "xferrecv.h"
#include "visdelay.h"
#include "print.h"
#include "drawfdiag.h"
#include "font.h"


/* #includes for bits of pd we need to call */

#include "datafmt.h"

#include "ext.riscos"
#include "riscdraw.h"
#include "ext.pd"
#include "riscdialog.h"


/* menu offset for no selection */

#define mo_noselection 0


#define OSFile_WriteLoad    2
#define OSFile_WriteExec    3
#define OSFile_ReadNoPath   17
#define OSFile_MakeError    19

#endif  /* RISCOS */

#endif

/* end of riscos.h */
