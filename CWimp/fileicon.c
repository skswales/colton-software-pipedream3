/* > c.fileicon */

/* Title:   fileicon.c
 * Purpose: general display of a file icon in a dialog box.
 * Status:  Arthur-specific, part of ArcEdit.
 * History:
 *  15-Mar-88   WRS carved off from xfersend.
*/

#define BOOL int
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <string.h>

#include "trace.h"

#include "os.h"
#include "wimp.h"
#include "wimpt.h"
#include "sprite.h"
#if TRACE
#include "werr.h"
#endif

#include "fileicon.h"


/* Set up a sprite icon with the icon that represents the relevant file
 * type. The user may now drag this icon away, if he so desires.
*/

extern void
fileicon(wimp_w w, wimp_i i, int filetype)
{
    wimp_icreate icreate;
    wimp_i newi;

    tracef3("fileicon(%d, %d, %3.3X)\n", w, i, filetype);

    icreate.w = w;

    if( !wimpt_complain(wimp_get_icon_info(w, i, &icreate.i))   &&
        !wimpt_complain(wimp_delete_icon(w, i))                 )
    {
        /* the icon must initially be an indirect text(or sprite) icon. */
        icreate.i.flags = (icreate.i.flags & ~wimp_ITEXT) | wimp_ISPRITE;
        icreate.i.data.indirectsprite.spritearea = (void *) 1;
        sprintf((char *) icreate.i.data.indirectsprite.name,
                "file_%03x", filetype);
        /* one of those rare occasions this is written to */

        /* now to check if the sprite exists. */
        /* do a sprite_select on the Wimp sprite pool */

        if(wimp_spriteop(24, icreate.i.data.indirectsprite.name) != 0)
            /* the sprite does not exist: print general don't-know icon. */
            strcpy((char *) icreate.i.data.indirectsprite.name, "file_xxx");

        tracef1("sprite name is %s.\n", icreate.i.data.indirectsprite.name);

        /* will recreate with same number. */
        (void) wimpt_safe(wimp_create_icon(&icreate, &newi));
#if TRACE
        if (newi != i)
          werr("INTERNAL: fi-1");
#endif
        tracef2("new icon no %i (should be %i).\n", newi, i);
    }
}


/* end of fileicon.c */
