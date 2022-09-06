/* file.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#if ARTHUR || RISCOS
#ifndef __cmodules_handlist_ext
#include "ext.handlist"
#endif

#ifndef __os_h
#include "os.h"
#endif

#define OSArgs_ReadPointer  0
#define OSArgs_WritePointer 1
#define OSArgs_ReadExtent   2
#define OSArgs_WriteExtent  3
#define OSArgs_Flush        0xFF

#define OSFind_CloseFile    0x00
#define OSFind_OpenRead     0x40
#define OSFind_CreateUpdate 0x80
#define OSFind_OpenUpdate   0xC0

#define OSGBPB_WriteAtPtr   2
#define OSGBPB_ReadFromPtr  4

#elif MS || WINDOWS
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <errno.h>

#ifndef __cmodules_handlist_ext
#include "handlist.ext"
#endif
#endif


/* fields in _flags */
#define _FILE_READ     0x0001
#define _FILE_WRITE    0x0002
#define _FILE_BUFDIRTY 0x0004
#define _FILE_CLOSED   0x0008


/* extradata struct */

typedef struct
{
    file_handle next;
    #if WINDOWS
    intl        openmode;
    OFSTRUCT    of;
    #endif
}
_file_extradata;

/* end of file.h */
