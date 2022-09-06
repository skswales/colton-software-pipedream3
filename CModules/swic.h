/* swic.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       swic.h - exported objects from swic.s
 * Author:      Stuart K. Swales 31-Jan-1989
*/

#ifndef __swic_h
#define __swic_h

/* flag to mask SWI number with to create X-form (status returning) SWI */
#define ErrBit (1 << 17)


struct swierror
    {
    intl errnum;
    char errmsg[252];
    };

typedef struct swierror *errp;


typedef struct
    {
    intl    r[10];
    uword32 pc;
    } swic_regset;


/* bits in the ARM psr */
#define ARM_C_BIT 0x20000000
#define ARM_V_BIT 0x10000000


/* exported functions */

/* Call an ARM SWI from C with a block of registers: r0..r9
 * Returns r0..r9, pc: V bit set if error returned.
 *
 * Take care not to use SWI calls that will cause an error to be raised
 * as the ARM C runtime library fakes these into V bit set returns but
 * with an invalid r0.
*/
extern errp SwiC(int SWInum, void *addressofregisterset);

#endif  /* __swic_h */

/* end of swic.h */
