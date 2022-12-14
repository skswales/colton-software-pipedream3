/* akbd.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __akbd_h
#define __akbd_h
#define akbd_Fn (256 + 128)
#define akbd_Sh (16)
#define akbd_Ctl (32)
#define akbd_TabK (akbd_Fn + 10)
#define akbd_CopyK (akbd_Fn + 11)
#define akbd_LeftK (akbd_Fn + 12)
#define akbd_RightK (akbd_Fn + 13)
#define akbd_DownK (akbd_Fn + 14)
#define akbd_UpK (akbd_Fn + 15)
#define akbd_Fn10 (0x1CA)
#define akbd_Fn11 (0x1CB)
#define akbd_Fn12 (0x1CC)
#define akbd_InsertK (0x1CD)
#define akbd_PrintK (akbd_Fn+0)
#define akbd_PageUpK (akbd_Sh+akbd_UpK)
#define akbd_PageDownK (akbd_Sh+akbd_DownK)
int akbd_pollsh(void); 
int akbd_pollctl(void);
int akbd_pollkey(int *keycode);
#endif
