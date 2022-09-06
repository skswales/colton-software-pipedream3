/* vsload.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*******************************
*                              *
* ViewSheet loader header file *
* MRJC                         *
* May 1989                     *
*                              *
*******************************/

struct vsfileheader
    {
    uchar maxrow;
    uchar scmode;
    uchar curwin;
    uchar scrwin[120];
    uchar prnwin[100];
    uchar stracc[14];
    uchar prttab[64];
    uchar rowpn1;         /* this because the ARM can't byte align ANYTHING */
    uchar rowpn2;
    uchar rtbpn1;
    uchar rtbpn2;
    uchar ctbpn1;
    uchar ctbpn2;
    uchar sltpn1;
    uchar sltpn2;
    uchar frepn1;
    uchar frepn2;
    uchar fileid;
    };

struct rowtabentry
    {
    uchar colsinrow;
    uchar offtoco1;
    uchar offtoco2;
    };

#define VS_MAXSLOTLEN 255

/* end of vsload.h */
