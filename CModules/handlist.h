/* handlist.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************
*                                         *
* header file for moveable memory manager *
*                                         *
* MRJC                                    *
* February 1989                           *
*                                         *
******************************************/

#define itemp   list_itemp      /* internal name for item pointer */
#define itemno  list_itemno     /* internal name for list_itemno */

/* overhead for item allocation */
#define ITEMOVH  (sizeof(list_item) - sizeof(union guts))
#define FILLSIZE (sizeof(union guts))

#define quickptr(han) (handleblock ? *(handleblock + (han)) : NULL)

#if WINDOWS
extern void EXTERNAL sysexit(char *message);
#endif

/* end of handlist.h */
