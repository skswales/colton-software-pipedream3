/* riscmenu.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       riscmenu.c - RISC OS specific menu code for PipeDream
 * Author:      Stuart K. Swales 05 Jul 1989
*/

#include "flags.h"


#if !RISCOS
#   error   This module can only be used to build RISC OS code
#endif


/* local header file - shared with riscos.c */
#include "riscos.h"


/* external header file */
#include "riscmenu.h"


/* exported functions */

extern void riscmenu_attachmenutree(void);
extern void riscmenu_buildmenutree(BOOL short_m);
extern void riscmenu_clearmenutree(void);
extern void riscmenu_detachmenutree(void);
extern void riscmenu_initialise_once(void);


/* internal functions */

/*static void createmainmenu(BOOL short_m);*/
static void opensubmenu(wimp_menustr *submenuptr, int x, int y);


/* ------------------------------------------------------------------------ */

/********************************************
*                                           *
* icon bar menu definition (shared globals) *
*                                           *
********************************************/

static menu iconbar_menu = NULL;

#define mo_iconbar_info     1
#define mo_iconbar_windows  2
#define mo_iconbar_tidyup   3
#define mo_iconbar_quit     4

#if TRACE
#if defined(DEBUG)
#define MO_STR_DEBUG ",Debug"
#else
#define MO_STR_DEBUG ""
#endif

static const char iconbar_menu_entries_trace[] = ",Trace,>Command" MO_STR_DEBUG;

#define mo_iconbar_trace    5
#define mo_iconbar_command  6

#if defined(DEBUG)
#define mo_iconbar_debug    7
#endif
#endif


/* iconbar 'Windows' submenu */

static menu       iw_menu           = NULL;
static dochandle *iw_menu_array     = NULL;
static intl       iw_menu_array_upb = 0;

#define mo_iw_newdoc    1
#define mo_iw_dynamic   2

#define IW_MENU_MAXWIDTH    48


#if TRACE
/* iconbar 'Command' submenu */

static menu       ic_menu            = NULL;
static const char ic_menu_title[]    = "Command";
static const char ic_menu_entries[]  = "Command";
static char       ic_menu_buffer[256];

#define mo_iconbar_command_command 1
#endif


/************************************************************
*                                                           *
* icon bar MENU maker - called when MENU pressed on an icon *
*                                                           *
************************************************************/

static menu
iconbar_menu_maker(void *handle)
{
#if TRACE
    BOOL currtrace = trace_is_on();
    BOOL tron      = trace_enabled  &&  akbd_pollsh();

    if(tron)
        trace_on();
#endif

    IGNOREPARM(handle);         /* this handle is useless */

    tracef0("iconbar_menu_maker()\n");

    if(!installed_ok)
        exit(EXIT_FAILURE);

    /* riscos_non_null_event(); not needed - menu creation safe */


    /* create submenu if needed */

    if(!event_is_menu_being_recreated())
        {
        tracef0("dispose of old & then create new windows menu list\n");

        menu_dispose(&iw_menu);

        iw_menu = menu_new(iw_menu_title, iw_menu_entries);

        if(iw_menu)
            {
            intl nDocuments = 0;
            intl i;
            intl max_i;
            window_data *wdp;

            wdp = NULL;
            while((wdp = next_document(wdp)) != NULL)
                nDocuments++;

            /* enlarge array as needed */
            if(nDocuments > iw_menu_array_upb)
                {
                intl res;
                dochandle *ptr = realloc_ptr_using_cache(iw_menu_array, (word32) nDocuments * sizeof(dochandle *), &res);

                if(ptr)
                    {
                    iw_menu_array       = ptr;
                    iw_menu_array_upb   = nDocuments;
                    }
                elif(res < 0)
                    reperr_null(res);

                max_i = iw_menu_array_upb;
                }
            else
                max_i = nDocuments;

            i  = 0;
            wdp = NULL;

            while((i < max_i)  &&  ((wdp = next_document(wdp)) != NULL))
                {
                const char *name = riscos_obtainfilename(wdp->Xcurrentfilename,
                                                         FALSE);
                intl len = strlen(name);
                char tempstring[256];

                tempstring[0] = ',';
                tempstring[1] = '\0';

                if(len > IW_MENU_MAXWIDTH)
                    {
                    strcpy(tempstring + 1, iw_menu_prefix);
                    strcat(tempstring + 1,
                            name + len - IW_MENU_MAXWIDTH + strlen(iw_menu_prefix));
                    }
                else
                    strcpy(tempstring + 1, name);

                if(menu_extend(&iw_menu, tempstring))
                    iw_menu_array[i++] = wdp->DocHandle;
                }

            (void) menu_submenu(iconbar_menu, mo_iconbar_windows, iw_menu);
            }
        }


    /* encode menus */

#if TRACE
    if(trace_enabled)
        menu_setflags(iconbar_menu, mo_iconbar_trace, currtrace, FALSE);
#endif


    tracef1("iconbar menus encoded: returning menu &%p\n", iconbar_menu);

#if TRACE
    if(tron)
        trace_off();
#endif

    return(iconbar_menu);
}


#define CLICK_GIVES_DIALOG TRUE

/*************************************************************************
*                                                                        *
* icon bar MENU selection - called when selection is made from icon MENU *
*                                                                        *
*************************************************************************/

static BOOL
iconbar_menu_handler(void *handle, char *hit, BOOL submenurequest)
{
    intl selection      = hit[0];
    intl subselection   = hit[1];
    BOOL processed      = TRUE;

#if TRACE
    BOOL currtrace = trace_is_on();
    BOOL tron      = trace_enabled  &&  akbd_pollsh();

    if(tron)
        trace_on();
#endif

    tracef3("iconbar_menu_handler([%d][%d]): submenurequest = %s\n",
            selection, subselection, trace_boolstring(submenurequest));

    IGNOREPARM(handle);     /* this handle is useless */

    #if defined(SB_GLOBAL_REGISTER)
    select_document(NO_DOCUMENT);
    #endif


    riscos_non_null_event();

    switch(selection)
        {
        case mo_iconbar_info:
            if(CLICK_GIVES_DIALOG  ||  submenurequest)
                riscdialog_execute(dproc_aboutprog,
                                   "aboutprog", d_about, D_ABOUT);
            /* ignore clicks on 'Info' entry if !CLICK_GIVES_DIALOG */
            break;


        case mo_iconbar_windows:
            switch(subselection)
                {
                case mo_noselection:
                    /* ignore clicks on 'Windows' entry */
                    processed = FALSE;
                    break;

                case mo_iw_newdoc:
                    if(create_new_untitled_document())
                        draw_screen();
                    break;

                default:
                    front_document_using_handle(iw_menu_array[subselection - mo_iw_dynamic]);
                    break;
                }
            break;


        case mo_iconbar_tidyup:
            application_process_command(N_TidyUp);
            break;


        case mo_iconbar_quit:
            #if defined(BACKTRACE)
                wimpt_backtrace_on_exit(FALSE);
            #endif
            application_process_command(N_Quit);
            break;


#if TRACE
        case mo_iconbar_trace:
            /* toggle trace state */
            if(currtrace)
                trace_off();
            else
                trace_on();
            break;


        case mo_iconbar_command:
            switch(subselection)
                {
                case mo_noselection:
                    /* ignore clicks on 'Command' entry */
                    processed = FALSE;
                    break;

                default:
                    trace_on();
                    trace_system(riscos_cleanupstring(ic_menu_buffer));
                    trace_off();
                    break;
                }
            break;


        #if defined(DEBUG)
        case mo_iconbar_debug:
            list_toggle_validate();
            trace_on();
            (void) list_allochandle(0);     /* tickle interface */
            trace_off();
            break;
        #endif

#endif  /* TRACE */


        default:
            tracef1("unprocessed iconbar menu hit %d\n", hit[0]);
            break;
        }

#if TRACE
    if(tron)
        trace_off();
#endif

    return(processed);
}


extern void
Debug_fn(void)
{
    #if defined(DEBUG)
    list_toggle_validate();
    trace_on();
    (void) list_allochandle(0);     /* tickle interface */
    trace_off();
    list_toggle_validate();
    #endif
}


/****************************************************************************
*                                                                           *
*             dynamically create menu structures for main window            *
*                                                                           *
****************************************************************************/

/* main menu */

static menu main_menu = NULL;


/* offsets starting from 1 */

#define mo_main_dynamic 1


/* 'Printer font' submenu */

static menu   pf_menu           = NULL;
static char **pf_menu_array     = NULL;
static intl   pf_menu_array_upb = 0;    /* number of elements in array */

#define mo_pf_system  1
#define mo_pf_dynamic 2


/* 'Font size' submenu */

static menu fs_menu = NULL;

typedef enum
{
    mo_fs_fontwidth = 1,
    mo_fs_first,
    mo_fs_ten,
    mo_fs_twelve,
    mo_fs_fourteen,
    mo_fs_writeable,
    mo_fs_fontheight,
    mo_fs_firstH,
    mo_fs_tenH,
    mo_fs_twelveH,
    mo_fs_fourteenH,
    mo_fs_writeableH
}
fs_menu_offsets;

static intl fontsize;
static intl fontheight;

static char fs_writeable_buffer[10]     =  "6.40";
static char fs_writeableH_buffer[10]    = "12.80";

static const short fs_sizes_array[]     = {  8 * 16, 10 * 16,
                                            12 * 16, 14 * 16    };


/* 'Printer line spacing' submenu */

static menu ld_menu = NULL;

typedef enum
{
    mo_ld_first = 1,
    mo_ld_second,
    mo_ld_third,
    mo_ld_last,
    mo_ld_writeable
}
ld_menu_offsets;

/* in millipoints */
static intl fontleading;

static char ld_writeable_buffer[10];

static const intl ld_sizes_array[]      = {  8 * 1000, 10 * 1000,
                                            12 * 1000, 14 * 1000    };


static menu
pf_menu_maker(BOOL create)
{
    BOOL iscurrent, hadcurrent;
    intl i;
    char *currentfontname;

#if TRACE
    BOOL tron = trace_enabled  &&  akbd_pollsh();

    if(tron)
        trace_on();
#endif

    tracef1("printer_font_menu_maker(create = %s)\n", trace_boolstring(create));


    /* encode 'Printer line spacing' menu */

    fontleading = global_font_leading;

    if(ld_menu)
        {
        hadcurrent = FALSE;
        for(i = mo_ld_first; i <= mo_ld_last; i++)
            {
            iscurrent = (fontleading == ld_sizes_array[i - mo_ld_first]);
            hadcurrent = hadcurrent || iscurrent;
            menu_setflags(ld_menu, i, iscurrent, FALSE);
            }

        sprintf(ld_writeable_buffer, pointsize_STR,
                (double) fontleading / 1000.0);

        menu_setflags(ld_menu, mo_ld_writeable, !hadcurrent, FALSE);
        }


    /* encode 'Font size' menu */

    if(!event_is_menu_being_recreated())
        {
        fontsize    = global_font_x;
        fontheight  = global_font_y;
        }

    if(fs_menu)
        {
        hadcurrent = FALSE;
        for(i = mo_fs_first; i < mo_fs_writeable; i++)
            {
            iscurrent = (fontsize == fs_sizes_array[i - mo_fs_first]);
            hadcurrent = hadcurrent || iscurrent;
            menu_setflags(fs_menu, i, iscurrent, FALSE);
            }

        if(!hadcurrent)
            sprintf(fs_writeable_buffer, pointsize_STR,
                    fontsize / 16.0);

        menu_setflags(fs_menu, mo_fs_writeable, !hadcurrent, FALSE);


        hadcurrent = FALSE;
        for(i = mo_fs_firstH; i < mo_fs_writeableH; i++)
            {
            iscurrent = (fontheight == fs_sizes_array[i - mo_fs_firstH]);
            hadcurrent = hadcurrent || iscurrent;
            menu_setflags(fs_menu, i, iscurrent, FALSE);
            }

        if(!hadcurrent)
            sprintf(fs_writeableH_buffer, pointsize_STR,
                    fontheight / 16.0);

        menu_setflags(fs_menu, mo_fs_writeableH, !hadcurrent, FALSE);
        }


    /* create 'Printer font' menu if needed - once only operation */

    if(create  &&  !pf_menu)
        {
        intl dummy;

        tracef0("create new printer font menu list\n");

        pf_menu = menu_new(pf_menu_title, pf_menu_entries);

        if(!pf_menu)
            /* complain a little */
            reperr_null(ERR_NOROOM);
        elif(!font_cacheaddress(&dummy, &dummy, &dummy))
            {
            intl nFonts = 0;
            intl fontindex = 0;

            for(;;)
                {
                char name[64];
                char *fontname = name + 2;

                name[0] = ',';
                name[1] = '>';

                if(wimpt_complain(font_list(fontname, &fontindex)))
                    break;

                if(fontindex == -1)
                    break;

                if(nFonts == pf_menu_array_upb)
                    {
                    intl res;
                    char **ptr = realloc_ptr_using_cache(pf_menu_array, ((word32) nFonts + 1) * sizeof(char *), &res);

                    if(ptr)
                        {
                        pf_menu_array           = ptr;
                        pf_menu_array[nFonts]   = NULL;
                        pf_menu_array_upb       = nFonts + 1;
                        }
                    else
                        {
                        tracef0("failed to extend pf menu array\n");
                        if(res < 0)
                            reperr_null(res);
                        break;
                        }
                    }

                if(!str_set(&pf_menu_array[nFonts], fontname))
                    break;

                if( !menu_extend(&pf_menu, name)                    ||
                    !menu_submenu(pf_menu, mo_pf_dynamic + nFonts, fs_menu)
                    )
                    {
                    /* complain a little */
                    reperr_null(ERR_NOROOM);
                    break;
                    }

                ++nFonts;
                }
            }
        }


    /* attach 'Printer font' menu to 'Print' menu if needed */

    if( !printer_font_attached  &&  pf_menu  &&
        printer_font_mhptr  &&  printer_font_mhptr->m)
        {
        (void) menu_submenu(printer_font_mhptr->m, printer_font_offset,     pf_menu);

        (void) menu_submenu(printer_font_mhptr->m, printer_font_offset + 1, pf_menu);

        printer_font_attached = TRUE;
        }


    /* encode 'Printer font' menu */

    if(pf_menu)
        {
        BOOL systemfont = (global_font == NULL);

        currentfontname = systemfont ? NULLSTR : global_font;

        menu_setflags(pf_menu, mo_pf_system, systemfont, FALSE);

        for(i = 0; i < pf_menu_array_upb; i++)
            {
            tracef2("comparing %s and %s\n", currentfontname, pf_menu_array[i]);
            iscurrent = !systemfont  &&  !stricmp(currentfontname, pf_menu_array[i]);
            menu_setflags(pf_menu, mo_pf_dynamic + i, iscurrent, FALSE);
            }
        }


    tracef1("printer font menu encoded: returning menu &%p\n", pf_menu);

#if TRACE
    if(tron)
        trace_off();
#endif

    return(pf_menu);
}


/*********************************
*                                *
* create submenu for headline[i] *
*                                *
*********************************/

static menu
createsubmenu(int head_i, BOOL short_m)
{
    menu m;
    MENU_HEAD *mhptr = &headline[head_i];
    MENU *mptr       = mhptr->tail;
    MENU *last_mptr  = mptr + mhptr->items;
    char description[256];
    char sep = '\0';

    tracef2("createsubmenu(%d, short = %s)\n", head_i, trace_boolstring(short_m));

    do  {
        char *ptr    = &description[4]; /* could be 2 but 4 saves code */
        BOOL process = !short_m  ||  (mptr->flags & SHORT);

        if(!mptr->title)
            {
            tracef2("looking at item line: short = %s -> process = %s\n",
                    trace_boolstring(mptr->flags & SHORT),
                    trace_boolstring(process));

            if(process)
                sep = '|';
            }
        else
            {
            tracef3("looking at command %s: short = %s -> process = %s\n",
                    trace_string(mptr->title),
                    trace_boolstring(mptr->flags & SHORT),
                    trace_boolstring(process));

            if(process)
                {
                if(get_menu_item(mhptr, mptr, ptr))
                    /* menu item has a submenu/dbox */
                    *--ptr = '>';

                if(sep == '\0')
                    {
                    /* first item */
                    m = menu_new(mhptr->name, ptr);
                    if(!m)
                        break;
                    }
                else
                    {
                    *--ptr = sep;
                    if(!menu_extend(&m, ptr))
                        break;
                    }

                sep = ',';
                }
            }
        }
    while(++mptr < last_mptr);

    return(m);
}


/*********************************
*                                *
* encode submenu for headline[i] *
*                                *
*********************************/

static void
encodesubmenu(int head_i, BOOL short_m)
{
    MENU_HEAD *mhptr = &headline[head_i];
    MENU *mptr       = mhptr->tail;
    MENU *last_mptr  = mptr + mhptr->items;
    intl offset      = 1;
    BOOL tick, fade;

    tracef2("encodesubmenu(%d, %s)\n", head_i, trace_boolstring(short_m));

    do  {
        if(mptr->title) /* ignore gaps */
            {
            BOOL flag = mptr->flags;

            if(!short_m  ||  (flag & SHORT))
                {
                if(flag & (TICKABLE | GREYABLE))
                    {
                    tick = fade = FALSE;

                    if(flag & TICKABLE)
                        tick = flag & TICK_STATUS;

                    if(flag & GREYABLE)
                        fade = flag & GREY_STATUS;

                    if(mhptr->m)
                        menu_setflags(mhptr->m, offset, tick, fade);
                    }

                offset++;
                }
            }
        }
    while(++mptr < last_mptr);
}


/****************************************
*                                       *
*  Create main menu once at startup     *
* and whenever long/short form changed  *
*                                       *
****************************************/

static void
createmainmenu(BOOL short_m)
{
    char description[256];
    intl head_i;
    menu submenu;
    char *ptr;
    BOOL needsmain = (main_menu == NULL);

    tracef1("createmainmenu(%s)\n", trace_boolstring(short_m));

    /* create dynamic main menu bits */

    for(head_i = 0; head_i < head_size; head_i++)
        if(headline[head_i].installed)
            {
            /* create main menu if needed - once only operation */
            if(needsmain)
                {
                ptr = description;
                *++ptr = '>';
                strcpy(ptr + 1, headline[head_i].name);

                /* create/extend the top level menu */
                if(!main_menu)
                    main_menu = menu_new(applicationname, ptr);
                else
                    {
                    *--ptr = ',';
                    (void) menu_extend(&main_menu, ptr);
                    }

                if(!main_menu)
                    break;
                }

            /* dispose of existing submenu at this point */
            menu_dispose((menu *) &headline[head_i].m);

            /* create the submenu and attach it here, and remember it too */
            headline[head_i].m = submenu = createsubmenu(head_i, short_m);

            /* complain a little */
            if(!submenu)
                reperr_null(ERR_NOROOM);

            (void) menu_submenu(main_menu, mo_main_dynamic + head_i, submenu);
            }


    /* will need to attach printer font again */
    printer_font_attached = FALSE;


    /* create 'Printer line spacing' menu if needed - once only operation */

    if(!ld_menu)
        {
        tracef0("create line spacing menu\n");

        ld_menu = menu_new(ld_menu_title, ld_menu_entries);

        if(!ld_menu)
            /* complain a little */
            reperr_null(ERR_NOROOM);
        else
            menu_make_writeable(ld_menu, mo_ld_writeable,
                                ld_writeable_buffer,
                                sizeof(ld_writeable_buffer),
                                fp_only_validation_STR);
        }


    /* attach menu now if short menu state permits it */

    if( ld_menu  &&
        printer_font_mhptr  &&  printer_font_mhptr->m)
            (void) menu_submenu(printer_font_mhptr->m,
                                printer_font_offset + 2,
                                ld_menu);


    /* create 'Font size' menu if needed - once only operation */

    if(!fs_menu)
        {
        tracef0("create font size menu\n");

        fs_menu = menu_new(fs_menu_title, fs_menu_entries);

        if(!fs_menu)
            /* complain a little */
            reperr_null(ERR_NOROOM);
        else
            {
            menu_make_writeable(fs_menu, mo_fs_writeable,
                                fs_writeable_buffer,
                                sizeof(fs_writeable_buffer),
                                fp_only_validation_STR);

            menu_make_writeable(fs_menu, mo_fs_writeableH,
                                fs_writeableH_buffer,
                                sizeof(fs_writeableH_buffer),
                                fp_only_validation_STR);
            }
        }

    /* will be attached at same time as printer font menu */
}


/************************************
*                                   *
* encode main menu each MENU press  *
*                                   *
************************************/

static void
encodemainmenu(BOOL short_m)
{
    intl head_i;

    /* encode main menu */

    for(head_i = 0; head_i < head_size; head_i++)
        if(headline[head_i].installed)
            encodesubmenu(head_i, short_m);


    /* encode 'Printer font' menu and friends */

    pf_menu_maker(FALSE);
}


/************************************
*                                   *
*  process main window MENU press   *
*                                   *
************************************/

static menu
main_menu_maker(void *handle)
{
#if TRACE
    BOOL tron = trace_enabled  &&  akbd_pollsh();

    if(tron)
        trace_on();
#endif

    tracef1("main_menu_maker(%d)\n", handle);

    select_document_using_handle((dochandle) handle);

    xf_acquirecaret = TRUE;
    position_cursor();

/*  riscos_non_null_event(); not needed - menu creation safe */


    /* do menu encoding */

    encodemainmenu(short_menus());

    tracef1("main menus encoded: returning menu &%p\n", main_menu);

#if TRACE
    if(tron)
        trace_off();
#endif

    return(main_menu);
}


static BOOL
insertfontsize(intl size)
{
    char array[32];

    sprintf(array, ",%.3g", size / 16.0);

    return(insert_string(array, FALSE));
}


static void
fontaction(intl funcnum, intl nextoffset)
{
    char *fontname  = pf_menu_array[nextoffset - mo_pf_dynamic];

    if(funcnum == N_PRINTERFONT)
        {
        /* printer font selection */
        str_set(&global_font, fontname);
        riscos_fonts = TRUE;
        riscos_font_error = FALSE;
        xf_acquirecaret = TRUE;
        filealtered(TRUE);
        new_screen();
        }
    else
        {
        /* insert font reference */
        filbuf();

        if( insert_string("@F:", FALSE)     &&
            insert_string(fontname, FALSE)  &&
            insertfontsize(fontsize)        )
            {
            if(fontheight != fontsize)
                insertfontsize(fontheight);

            insert_string("@", FALSE);
            }

        out_currslot = TRUE;
        filealtered(TRUE);
        draw_screen();
        }
}


/****************************************
*                                       *
* process main window menu selection    *
* caused by click on submenu entry      *
* or wandering off the right => of one  *
*                                       *
****************************************/

static BOOL
decodesubmenu(intl head_i, intl offset, intl nextoffset, intl lastoffset, BOOL submenurequest)
{
    BOOL short_m = short_menus();
    MENU *mptr   = headline[head_i].tail;
    intl i = 1; /* NB. offset == 1 is start of array */
    intl funcnum;
    BOOL processed = TRUE;
    BOOL recreatepending = event_is_menu_recreate_pending();

    tracef4("decodesubmenu(%d, [%d][%d][%d])\n", head_i, offset, nextoffset, lastoffset);

    for(; ; mptr++)
        {
        /* don't count the gaps and the missing items */
        if(!mptr->title)
            continue;

        if(short_m  &&  (mptr->flags & LONG))
            {
            tracef1("--- ignoring command %s\n", trace_string(mptr->title));
            continue;
            }

        tracef2("--- looking at command %s, i = %d\n",
                    trace_string(mptr->title), i);

        if(i == offset)
            break;

        i++;
        }

    tracef1("--- found command %s\n", mptr->title);

    funcnum = mptr->funcnum;

    switch(funcnum)
        {
        case N_PRINTERFONT:
        case N_INSERTFONT:
            switch(nextoffset)
                {
                case mo_noselection:
                    if(submenurequest)
                        {
                        pf_menu_maker(TRUE);

                        if(pf_menu)
                            {
                            intl x, y;
                            event_read_submenudata(NULL, &x, &y);
                            opensubmenu(menu_sysmenu(pf_menu), x, y);
                            }
                        }
                    /* ignore clicks on 'Printer font' (& friend) entry */
                    break;

                case mo_pf_system:
                    /* set system font */
                    if(funcnum == N_PRINTERFONT)
                        {
                        str_clr(&global_font);
                        riscos_fonts = FALSE;
                        filealtered(TRUE);
                        new_screen();
                        }
                    break;

                default:
                    {
                    BOOL change = FALSE;

                    switch(lastoffset)
                        {
                        case mo_noselection:
                            if(submenurequest)
                                processed = FALSE;
                            else
                                change = TRUE;
                            break;

                        case mo_fs_first:
                        case mo_fs_ten:
                        case mo_fs_twelve:
                        case mo_fs_fourteen:
                            fontsize = fontheight =
                                fs_sizes_array[lastoffset - mo_fs_first];

                            if((funcnum == N_PRINTERFONT)  ||  !recreatepending)
                                {
                                global_font_x = global_font_y = fontsize;
                                change = TRUE;
                                }

                            sprintf(fs_writeable_buffer, pointsize_STR,
                                    fontsize / 16.0);
                            strcpy(fs_writeableH_buffer, fs_writeable_buffer);
                            break;

                        case mo_fs_writeable:
                            {
                            double dfontsize;

                            riscos_cleanupstring(fs_writeable_buffer);

                            dfontsize = strtod(fs_writeable_buffer, NULL);

                            if((dfontsize < 1.0)  ||  (dfontsize == HUGE_VAL))
                                reperr_null(ERR_BADFONTSIZE);
                            else
                                {
                                fontsize = fontheight = (intl) (dfontsize * 16.0);

                                if((funcnum == N_PRINTERFONT)  ||  !recreatepending)
                                    {
                                    global_font_x = global_font_y = fontsize;
                                    change = TRUE;
                                    }

                                sprintf(fs_writeable_buffer, pointsize_STR,
                                        fontsize / 16.0);
                                strcpy(fs_writeableH_buffer, fs_writeable_buffer);
                                }
                            }
                            break;

                        case mo_fs_firstH:
                        case mo_fs_tenH:
                        case mo_fs_twelveH:
                        case mo_fs_fourteenH:
                            fontheight =
                                fs_sizes_array[lastoffset - mo_fs_firstH];

                            if(funcnum == N_PRINTERFONT)
                                global_font_y = fontheight;

                            sprintf(fs_writeableH_buffer, pointsize_STR,
                                    fontheight / 16.0);
                            change = TRUE;
                            break;

                        case mo_fs_writeableH:
                            {
                            double dfontheight;

                            riscos_cleanupstring(fs_writeableH_buffer);

                            dfontheight = strtod(fs_writeableH_buffer, NULL);

                            if((dfontheight < 1.0)  ||  (dfontheight == HUGE_VAL))
                                reperr_null(ERR_BADFONTSIZE);
                            else
                                {
                                fontheight = (intl) (dfontheight * 16.0);

                                if(funcnum == N_PRINTERFONT)
                                    global_font_y = fontheight;

                                sprintf(fs_writeableH_buffer, pointsize_STR,
                                        fontheight / 16.0);
                                change = TRUE;
                                }
                            }
                            break;

                    /*  case mo_fs_fontheight: */   /* ignore this entry */
                        default:
                            break;
                        }

                    /* select as current or do insertion */
                    if(change)
                        fontaction(funcnum, nextoffset);
                    }
                    break;
                }
            break;


        case N_PRINTERLINESPACE:
            switch(nextoffset)
                {
                case mo_noselection:
                    /* ignore clicks on 'Printer line spacing' entry */
                    processed = FALSE;
                    break;

                case mo_ld_first:
                case mo_ld_second:
                case mo_ld_third:
                case mo_ld_last:
                    global_font_leading = ld_sizes_array[nextoffset - mo_ld_first];
                    filealtered(TRUE);
                    break;

                case mo_ld_writeable:
                    {
                    double dfontleading;

                    riscos_cleanupstring(ld_writeable_buffer);

                    dfontleading = strtod(ld_writeable_buffer, NULL);

                    if((dfontleading < 1.0)  ||  (dfontleading == HUGE_VAL))
                        reperr_null(ERR_BADLINESPACE);
                    else
                        {
                        global_font_leading = (intl) (dfontleading * 1000);
                        filealtered(TRUE);
                        }
                    }
                    break;

                default:
                    break;
                }
            break;


        case N_SaveFile:
            if(!submenurequest)
                funcnum = N_SaveFileAsIs;

            /* deliberate drop thru */

        default:
            /* call the appropriate function */
            application_process_command(funcnum);
            break;
        }

    return(processed);
}


static void
opensubmenu(wimp_menustr *submenuptr, intl x, intl y)
{
    tracef3("opensubmenu(&%p, %d, %d)\n", submenuptr, x, y);
    wimpt_complain(wimp_create_submenu(submenuptr, x, y));
}


/***************************************
*                                      *
* process main window menu selection   *
* caused either by click on menu entry *
* or menu warning when leaving =>      *
*                                      *
***************************************/

static BOOL
main_menu_handler(void *handle, char *hit, BOOL submenurequest)
{
    dochandle doc       = (dochandle) handle;
    intl selection      = hit[0];
    intl subselection   = hit[1];
    BOOL processed      = TRUE;

    #if TRACE
    BOOL tron = trace_enabled  &&  akbd_pollsh();

    if(tron)
        trace_on();
    #endif

    tracef3("main_menu_handler([%d][%d]): submenurequest = %s\n",
            selection, subselection, trace_boolstring(submenurequest));

    select_document_using_handle(doc);

    riscos_non_null_event();

    switch(selection)
        {
        case mo_noselection: /* I don't believe this ever happens */
            break;           /* as it'd be far too useful ... */

        default:
            /* an event to do with the dynamic menu bits */
            switch(subselection)
                {
                case mo_noselection:
                    /* open the given submenu */
                    processed = FALSE;
                    break;

                default:
                    /* always decode the selection: the called function
                     * may put up a dialog box, in which case dbox does
                     * the right thing.
                    */
                    {
                    /* lookup in the dynamic menu structure */
                    int offset  = selection - mo_main_dynamic;
                    int thisoff = 0;
                    int head_i;

                    for(head_i = 0; head_i < head_size; head_i++)
                        if(headline[head_i].installed)
                            if(offset == thisoff++)
                                {
                                processed = decodesubmenu(head_i, subselection, hit[2], hit[3], submenurequest);
                                break;
                                }
                    }
                    break;
                }
            break;
        }

    #if TRACE
    if(tron)
        trace_off();
    #endif

    return(processed);
}


/************************************
*                                   *
*  attach menu tree to this window  *
*                                   *
************************************/

extern void
riscmenu_attachmenutree(void)
{
    event_attachmenumaker(main__window,
                          main_menu_maker,
                          main_menu_handler,
                          (void *) current_document_handle());
}


/************************************************
*                                               *
* (re)build the menu tree in short or long form *
*                                               *
************************************************/

extern void
riscmenu_buildmenutree(BOOL short_m)
{
    createmainmenu(short_m);
}


/********************
*                   *
*  clear menu tree  *
*                   *
********************/

extern void
riscmenu_clearmenutree(void)
{
    tracef0("destroying menu tree\n");

    event_clear_current_menu();
}


/************************************
*                                   *
* detach menu tree from this window *
*                                   *
************************************/

extern void
riscmenu_detachmenutree(void)
{
    event_attachmenumaker(main__window, NULL, NULL, NULL);
}


extern void
riscmenu_initialise_once(void)
{
    /* create menu for icon bar icons */

    iconbar_menu = menu_new(applicationname, iconbar_menu_entries);

    #if TRACE
    if(trace_enabled)
        {
        (void) menu_extend(&iconbar_menu, iconbar_menu_entries_trace);

        ic_menu = menu_new(ic_menu_title, ic_menu_entries);

        if(ic_menu)
            {
            menu_make_writeable(ic_menu,
                                mo_iconbar_command_command,
                                ic_menu_buffer,
                                sizeof(ic_menu_buffer),
                                "");

            (void) menu_submenu(iconbar_menu, mo_iconbar_command, ic_menu);
            }
        }
    #endif

    event_attachmenumaker(win_ICONBAR,
                          iconbar_menu_maker,
                          iconbar_menu_handler,
                          NULL);


    /* main menu created by menu_state_changed soon after */
}


extern void
riscmenu_tidy_up(void)
{
    /* free 'Windows' menu */
    menu_dispose(&iw_menu);

    /* free 'Printer font' menu */
    menu_dispose(&pf_menu);
}

/* end of riscmenu.c */
