/* Title: >  c.resspr
 * Purpose: Common access to sprite resources
 * Version: 0.1
 */

/****************************************************************************
 * This source file was written by Acorn Computers Limited. It is part of   *
 * the "cwimp" library for writing applications in C for RISC OS. It may be *
 * used freely in the creation of programs for Archimedes. It should be     *
 * used with Acorn's C Compiler Release 2 or later.                         *
 *                                                                          *
 * No support can be given to programmers using this code and, while we     *
 * believe that it is correct, no correspondence can be entered into        *
 * concerning behaviour or bugs.                                            *
 *                                                                          *
 * Upgrades of this code may or may not appear, and while every effort will *
 * be made to keep such upgrades upwards compatible, no guarantees can be   *
 * given.                                                                   *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "trace.h"

#include "os.h"
#include "res.h"
#include "sprite.h"
#include "werr.h"

#include "misc.h"

#include "resspr.h" /* Ensure consistent interface */

static sprite_area *resspr__area = ((sprite_area *) 1);

/* Defaults to using the wimp sprite pool */

/* Having done res_init(argv[0]); the caller should do resspr_init();
 * before dbox_init(); so that the latter can run over the icon defs and
 * rewrite the sprite pointers to use the sprites we've loaded
*/

#define OS_File 0x08

extern void
resspr_init(void)
{
    os_regset r;
    os_error *ur;
    char s[256];

    #if !defined(NO_PROGRAM_SPRITES)
    if(res_findname("Sprites", s)) /* read this program's sprites */
    {
        int arealen;
        os_error *e;
        r.r[0] = 5;
        r.r[1] = (int) s;
        e = os_swix(OS_File, &r);
        if(e != NULL)
            werr_fatal(e->errmess);

        if(r.r[0] != 0)
        {
            arealen = r.r[4] + 4; /* file is sprite area without the length word */
            resspr__area = (sprite_area *) malloc(arealen);
            if(resspr__area == NULL)                /* get core for sprite area */
                werr_fatal("Unable to claim space for sprite file '%s'", s);

            sprite_area_initialise(resspr__area, arealen);
            ur = sprite_area_load(resspr__area, s);
            if(ur != NULL)
            {
                free(resspr__area);
                werr_fatal("Unable to load sprite file '%s'", s);
            }
        }
        else /* file not found */
        {
            tracef1("sprite file (%s) not found - you get the wimp pool.\n", s);
        }

    tracef1("Sprite area at %8.8x\n", (int) resspr__area);
    }
    #endif /* NO_PROGRAM_SPRITES */
}


sprite_area *
resspr_area(void)
{
    return(resspr__area);
}


/* end of c.resspr */
