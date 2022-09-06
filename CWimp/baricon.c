/* > c.baricon */

/* Title:   baricon.c
 * Purpose: Support placing of an icon on the icon bar
 * History:
 *   21-Mar-88 WRS extracted from ArcEdit
 *   02-Mar-89 SKS serverly hacked to allow user to install his own eventprocs
*/

#include "include.h"

#include "wimp.h"
#include "wimpt.h"
#include "win.h"
#include "sprite.h"
#include "resspr.h"
#include "misc.h"

#include "baricon.h"


#define iconbar_whandle ((wimp_w) (-1)) /* icon bar, right hand side */

#define iconbar_iflags ( (wimp_IFORECOL * 7) | (wimp_IBACKCOL * 1) | \
                         wimp_INDIRECT | wimp_IFILLED | wimp_IHCENTRE | \
                         (wimp_BCLICKDEBOUNCE * wimp_IBTYPE) )


#define charheight   32
#define charwidth    16
#define spriteheight 68
#define spritewidth  68


typedef struct baricon__str
{
    wimp_i i;
    const sprite_area *spritearea;
    char *spritename;
    char *str;
} baricon__str;


static void
baricon__mixed(wimp_icreate *i, baricon bi)
{
    int len = strlen(bi->str);

    i->w        = iconbar_whandle; 

    i->i.box.x0 = 0;
    i->i.box.y0 = -(charheight/2);
    i->i.box.x1 = max(spritewidth, charwidth * len);
    i->i.box.y1 = -(charheight/2) + charheight + 4 + spriteheight;

    i->i.flags  = iconbar_iflags | wimp_ITEXT | wimp_ISPRITE;

    i->i.data.indirecttext.buffer      = bi->str;
    i->i.data.indirecttext.validstring = bi->spritename;
    i->i.data.indirecttext.bufflen     = len + 1;
}


static void
baricon__sprite(wimp_icreate *i, baricon bi)
{
    i->w        = iconbar_whandle; 

    i->i.box.x0 = 0;
    i->i.box.y0 = 0;
    i->i.box.x1 = spritewidth;
    i->i.box.y1 = spriteheight;

    i->i.flags  = iconbar_iflags | wimp_ISPRITE;

    i->i.data.indirectsprite.name       = bi->spritename + 1;
    i->i.data.indirectsprite.spritearea = bi->spritearea;
    i->i.data.indirectsprite.nameisname = TRUE;
}


/* create an icon on the icon bar, either a sprite or text & sprite */

extern baricon
baricon_new(const char *spritename, const char *str)
{
    baricon bi = malloc(sizeof(baricon__str));
    wimp_icreate i; /* icon structure */

    tracef2("baricon_new(%s, %s)\n", spritename, trace_string(str));

    if(bi == NULL)
        return(bi);

    bi->spritename = malloc(strlen(spritename) + 2);
    if(bi->spritename == NULL)
        return(NULL);

    bi->spritearea = resspr_area();

    *bi->spritename = 'S';
    strcpy(bi->spritename + 1, spritename);

    if(str == NULL)
        {
        /* sprite only, no text */
        bi->str = NULL;

        baricon__sprite(&i, bi);
        }
    else
        {
        /* text & sprite icon */

        if(strset(&bi->str, str) == NULL)
            {
            dispose((void **) &bi->spritename);
            return(NULL);
            }

        baricon__mixed(&i, bi);
        }

    if(wimpt_complain(wimp_create_icon(&i, &bi->i)))
        return(NULL);


    /* sucessfully added, so lock application in core */
    win_activeinc();

    return(bi);
}


#if defined(BARICON_EXTRAS)

/* remove the given icon from the icon bar */

extern void
baricon_dispose(baricon *bip)
{
    baricon bi = *bip;

    tracef2("baricon_dispose(&%p, &%p)\n", bip, bi);

    if(bi != NULL)
        {
        win_activedec();

        *bip = NULL;

        wimpt_complain(wimp_delete_icon(iconbar_whandle, bi->i));

        dispose(&bi->spritename);
        dispose(&bi->str);
        free(bi);
        }
}


/* alter text state of icon bar icon: str == NULL -> make sprite only */

extern void
baricon_settext(baricon bi, const char *str)
{
    wimp_icreate i;

    tracef2("baricon_settext(&%p, %s)\n", bi, trace_string(str));

    if(str == NULL)
        {
        if(bi->str != NULL)
            {
            /* was text & sprite, now sprite only */
            dispose(&bi->str);

            baricon__sprite(&i, bi);
            }
        else
            /* was already just sprite */
            return;
        }
    else
        {
        dispose(&bi->str);

        if(strset(&bi->str, str) == NULL)
            return;

        baricon__mixed(&i, bi);
        }

    wimpt_complain(wimp_delete_icon(iconbar_whandle, bi->i));

    wimpt_complain(wimp_create_icon(&i, &bi->i));
}


extern int
baricon_syshandle(baricon bi)
{
    return((int) bi->i);
}

#endif


/* end of baricon.c */
