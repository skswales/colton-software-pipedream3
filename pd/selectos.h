/* selectos.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __selectos_h
#define __selectos_h

/* target machine type - mutually exclusive options */

#define ARTHUR  0

#define RISCOS  1

#define MS      0

#define WINDOWS 0

#define LLIB    (WINDOWS && 0)

#if MS && defined(M_I86LM)
#define MS_HUGE
/* required to build full PC system, not for QuickC */
#endif

#endif  /* __selectos_h */

/* end of selectos.h */
