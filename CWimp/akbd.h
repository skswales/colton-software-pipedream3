/*
 * Title: akbd.h
 * Purpose: Access to Archimedes keyboard under the Wimp.
 * Author: W. Stoye
 * Status: Arthur-specific
 * History:
 *   13-Oct-87: started
 *   13-Dec-87: converted to C
*/

/* Standard key codes produced by Wimp, for special keys. */
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

int akbd_pollsh(void); /* TRUE if Shift pressed down */

int akbd_pollctl(void); /* TRUE if Ctrl pressed down */

int akbd_pollalt(void); /* TRUE if ALT pressed down */

int akbd_pollkey(int *keycode /*out*/);
/* 1 if he has typed ahead, in which case the next keycode is returned.
Function keys will appear here as values more than 256, as produced by the
window system and as described by the constants above. */

/* end of akbd.h */
