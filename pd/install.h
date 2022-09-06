/* install.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
installation
*/

#define INSTALL_STRING_STR    "PipeDream: unregistered copy   "
#define INSTALL_STRING_LENGTH (sizeof(INSTALL_STRING_STR)+1)

#define I_SPELL 0

#if MS

#define I_VSYNC 1
#define I_PAGES 2
#define I_FAST  3
#define I_STRING_OFFSET 4

#else

#define I_STRING_OFFSET 1

#endif

/* end of install.h */
