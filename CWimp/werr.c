/* > c.werr */

/* Title  : werr.c
 * Purpose: provide error reporting in wimp programs
 * Version: 0.2
*/

#include "include.h"

#include <stdio.h>

#include "wimp.h"
#include "wimpt.h"

#include "werr.h"


static void
vwerr(BOOL fatal, const char *format, va_list va)
{
    os_error err;
    err.errnum = 0;

    vsprintf(&err.errmess[0], format, va);

    if(fatal)
        wimpt_noerr(&err);
    else
        wimpt_complain(&err);
}


extern void
werr_fatal(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    vwerr(TRUE, format, va);
    #if !defined(VA_END_SUPERFLUOUS)
    va_end(va);
    #endif
}


extern void
werr(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    vwerr(FALSE, format, va);
    #if !defined(VA_END_SUPERFLUOUS)
    va_end(va);
    #endif
}


/* end of werr.c */
