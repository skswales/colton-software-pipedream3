/* z88com.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*************************************
*                                    *
* header file for Z88 communications *
*                                    *
* MRJC                               *
* March 1988                         *
*                                    *
*************************************/

/* character definition */
#define ESC 0x1B

/******************************
*                             *
* constants for use on MS-DOS *
*                             *
******************************/

#if MS

/* BIOS interrupts */
#define SERIAL_IO       0x14

/* SERIAL_IO subfunctions */
#define SERIAL_INIT 0
#define SERIAL_WRITE 1
#define SERIAL_READ 2
#define SERIAL_STATUS 3

/* serial register addresses */
#define COM1_BASE 0x3F8
#define COM2_BASE 0x2F8

/* Z88 initialisation value */
#define Z88_INIT 0xE3

#endif

/****************************************
*                                       *
* constants for use on ARTHUR or RISCOS *
*                                       *
****************************************/

#if ARTHUR || RISCOS

#include "os.h"
#include "swic.h"
#include "swinumbers.h"

#endif

/****************
*               *
* Z88 constants *
*               *
****************/

/* receive timeout (seconds + 1) */
#define TIME_RCV 6
#define TIME_XMT 6

/* escape character base */
#define ESCBAS 256

/* size of Z88 filename (+1 for NULL) */
#define Z88NAMSIZ 16+1

/* maximum number of names per directory */
#define MAXNAM 100

/*
errors returned by Z88
*/

#define EZ88_NSP 1

/* end of z88com.h */
