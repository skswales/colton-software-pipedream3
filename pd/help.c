/* help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       help.c - Help system for PipeDream
 * Author:      MRJC, RJM
 * History:
 *   0.01 26-Jan-89 SKS split off from pdmain.c, made conditional HELP_OFF
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"
#endif


#include "datafmt.h"


/* exported functions */

extern void overlay_Help_fn(void);


/* ----------------------------------------------------------------------- */

#if !defined(HELP_OFF)  &&  !defined(HELP_FILE)

static const char instruction_str[] =
#if ARTHUR || RISCOS
    "Press \x83\x82 for previous/next screens, Enter to return";
#elif MS
    "Press \x1B\x1A for previous/next screens, Enter to return";
#endif


static const SCREENITEM menutext[] =
    {
    { 4,    "After pressing Esc to leave the help system"           },
    { 0,    NULLSTR                                                 },
    { 6,    "Press and release Alt to bring down the first menu"    },
#if ARTHUR || RISCOS
    { 10,   "Use \x81 \x8D and Enter to select an item"             },
    { 10,   "Use \x82 \x83 to move between menus"                   },
#elif MS
    { 10,   "Use \x19 \x18 and Enter to select an item"             },
    { 10,   "Use \x1A \x1B to move between menus"                   },
#endif
    { 0,    NULLSTR                                                 },
    { 6,    "Alternatively:"                                        },
    { 10,   "Alt-F for files menu, Alt-B for blocks menu, ..."      },
    { 6,    "or"                                                    },
    { 10,   "Alt-FL for load, Alt-PO for print, ..."                },
    { 0,    NULL                                                    }
    };


static const char *substring[] =
    {
    "(n)",
    "(list)",
    " 'condition')",
    "(file,",
    "column,row",
    "value",
    "range,",
    "(cost,salvage,life",
    "(payment,interest,"
    };


static const SCREENITEM funcs1[] =
    {
    { 6,    "abs0"                  },
    { 6,    "acs0"                  },
    { 6,    "asn0"                  },
    { 6,    "atn0"                  },
    { 6,    "avg1"                  },
    { 6,    "choose(5,list)"        },
    { 6,    "col"                   },
    { 6,    "cos0"                  },
    { 6,    "count1"                },
    { 6,    "cterm(interest,fv,pv)" },
    { 6,    "date"                  },
    { 6,    "davg(62"               },
    { 6,    "day0"                  },
    { 6,    "dcount(62"             },
    { 6,    "ddb7,period)"          },
    { 6,    "deg0"                  },
    { 6,    "dmax(62"               },
    { 6,    "dmin(62"               },
    { 6,    "dstd(62"               },
    { 6,    "dsum(62"               },
    { 6,    "dvar(62"               },
    { 6,    "exp0"                  },
    { 0,    NULL                    }
    };


static const SCREENITEM funcs2[] =
    {
    { 6,    "fv8term)"              },
    { 6,    "hlookup(5,6offset)"    },
    { 6,    "if(condition,5,5)"     },
    { 6,    "index(4)"              },
    { 6,    "int0"                  },
    { 6,    "irr(guess,range)"      },
    { 6,    "ln0"                   },
    { 6,    "log0"                  },
    { 6,    "lookup(5,6range)"      },
    { 6,    "max1"                  },
    { 6,    "min1"                  },
    { 6,    "mod(5,5)"              },
    { 6,    "month0"                },
    { 6,    "npv(interest,range)"   },
    { 6,    "pi"                    },
    { 6,    "pmt(principal,interest,term)" },
    { 6,    "pv8term)"              },
    { 6,    "rad0"                  },
    { 6,    "rate(fv,pv,term)"      },
    { 6,    "read34)"               },
    { 6,    "row"                   },
    { 6,    "sgn0"                  },
    { 0,    NULL                    }
    };

static const SCREENITEM funcs3[] =
    {
    { 6,    "sin0"                  },
    { 6,    "sln7)"                 },
    { 6,    "sqr0"                  },
    { 6,    "std1"                  },
    { 6,    "sum1"                  },
    { 6,    "syd7,period)"          },
    { 6,    "tan0"                  },
    { 6,    "term8fv)"              },
    { 6,    "var1"                  },
    { 6,    "vlookup(5,6offset)"    },
    { 6,    "write34,5)"            },
    { 6,    "year0"                 },
    { 0,    NULL                    }
    };


/***********************************************************
*                                                          *
* this defines the help screen for hidden cursor movements *
*                                                          *
***********************************************************/

static const SCREENITEM hiddenmenu1[] =
    {
    { 8,    "Ccr - Cursor right"        },
    { 8,    "Ccl - Cursor left"         },
    { 8,    "Ccu - Cursor up"           },
    { 8,    "Ccd - Cursor down"         },
    { 8,    "Cen - Enter"               },
    { 8,    "Crb - Rubout"              },
    { 8,    "Cnc - Next column"         },
    { 8,    "Cpc - Previous column"     },
    { 8,    "Cpu - Page up"             },
    { 8,    "Cpd - Page down"           },
    { 8,    "Ces - End of slot"         },
    { 8,    "Cbs - Start of slot"       },
    { 8,    "Ctc - Top of column"       },
    { 8,    "Cbc - Bottom of column"    },
    { 8,    "Cx  - Escape"              },
    { 8,    "Ccp - Pause"               },
    { 8,    "Brp - Replace"             },
    { 0,    NULL                        }
    };



static const SCREENHEADER screentext[] =
/* the number of entries in this structure is given by NO_OF_SCREENS */
    {
    { menutext,     "Using menus"           },
    { funcs1,       "Spreadsheet functions" },
    { funcs2,       "...functions..."       },
    { funcs3,       "...functions"          },
    { hiddenmenu1,  "Cursor commands"       }
    };
#define NO_OF_SCREENS (sizeof(screentext) / sizeof(SCREENHEADER)) 

#endif /* !HELP_OFF && !HELP_FILE */


/**************
*             *
* Help system *
*             *
**************/

extern void
overlay_Help_fn(void)
{
#if defined(HELP_OFF)
    reperr_not_installed(ERR_GENFAIL);
#else

#if defined(HELP_FILE)  &&  defined(MANY_DOCUMENTS)

    dochandle doc   = current_document_handle();
    window_data *tb = NULL;
    BOOL help_is_loaded = FALSE;
    char array[LIN_BUFSIZ+1];

    while((tb = next_document(tb)) != NULL)
        if(tb->Xfile_is_help)
            {
            help_is_loaded = TRUE;
            break;
            }

    if(help_is_loaded)
        {
        tracef0("help file already loaded - bring to front\n");

        front_document_using_handle(tb->DocHandle);
        }
    else
        {
        if(add_path(array, HELPFILE_STR, FALSE))
            if(init_dialog_box(D_LOAD))
                if(loadfile(array, NEW_WINDOW, FALSE))
                    file_is_help = TRUE;
        }

    #if !MS
    select_document_using_handle(doc);
    #endif

#else

    coord xpos, ypos, xsize, ysize, i, maxdepth;
    uchar scrn_array[80];
    intl screen_no = 0;
    intl slen;

    bodge_funnies_for_master_font((uchar *) instruction_str);
    bodge_funnies_for_master_font((uchar *) menutext[3].text);
    bodge_funnies_for_master_font((uchar *) menutext[4].text);

    xpos = 4;
    ypos = 3;
    xsize = pagwid - xpos*2;
    ysize = 19;

    maxdepth = ysize-9;

    clip_menu(0, 0, 0, 0);
    save_screen(xpos, ypos, xsize, ysize);
    setcolour(MENU_HOT, MENU_BACK);
    my_rectangle(xpos, ypos, xsize, ysize);

    in_dialog_box = TRUE;

    setcolour(MENU_FORE, MENU_BACK);
    at(xpos+1, ypos+1);
    ospca(xsize-2);
    at(xpos+2, ypos+1);
    stringout((uchar *) applicationname);
    stringout((uchar *) " help system");

    at(xpos+1, ypos+ysize-2);
    ospca(xsize-2);
    at(xpos + (xsize-2-sizeof(instruction_str))/2, ypos+ysize-2);
    stringout(instruction_str);

    setcolour(MENU_HOT, MENU_BACK);
    draw_bar(xpos, ypos+2, xsize);
    draw_bar(xpos, ypos+ysize-3, xsize);

    for(;;)
        {
        coord lastwidth = 0;
        coord thiswidth = 0;
        const SCREENHEADER *scrhead = screentext + screen_no;
        intl c;

        setcolour(MENU_FORE, MENU_BACK);
        for(i = ypos+3; i < ypos+ysize-3; i++)
            {
            at(xpos+1, i);
            ospca(xsize-2);
            }

        slen = sprintf((char *) scrn_array, screen_Zd_of_Zd_STR, screen_no+1, NO_OF_SCREENS);
        at(xpos+xsize-slen-2, ypos+1);
        stringout(scrn_array);

        /* draw the text in here */

        at(xpos+2, ypos+3);
        stringout((uchar *) screentext[screen_no].heading);

        for(i = 0; scrhead->screen[i].text; i++)
            {
            uchar *ptr = (uchar *) scrhead->screen[i].text;
            uchar array[80];
            uchar *to;
            intl len;
            coord depth;

            for(to = array ; *ptr; ptr++)
                if(isdigit(*ptr))
                    {
                    strcpy((char *) to, substring[(intl) (*ptr-'0')]);
                    to += strlen((char *) to);
                    }
                else
                    *to++ = *ptr;
            *to = '\0';

            len = strlen((char *) array);
            if(len > thiswidth)
                thiswidth = len;

            depth = ypos+5+ ((i > maxdepth) ? (i-maxdepth-1) : i);

            at(xpos+1+lastwidth+scrhead->screen[i].offset, depth);
            stringout(array);

            if(i == maxdepth)
                {
                lastwidth = thiswidth + 5;
                thiswidth = 0;
                }
            }

        sb_show_if_fastdraw();

        lastwidth += thiswidth + 8;

        clearkeyboardbuffer();

        c = inpchr(FALSE);

        switch(c < 0 ? -c : c)
            {
            case N_Escape:
                ack_esc();      /* Clear before returning */

            case N_Return:
                goto breakout;

            case N_CursorLeft:
                screen_no--;
                break;

            default:
                screen_no++;
                break;
            }

        if(screen_no > NO_OF_SCREENS-1)
            screen_no = 0;
        elif(screen_no < 0)
            screen_no = NO_OF_SCREENS-1;
        }

breakout:

    in_dialog_box = FALSE;

    clip_menu(0, 0, 0, 0);

    #if MS
    if(!fastdraw)
        clearscreen();
    #endif

#endif  /* HELP_FILE && MANY_DOCUMENTS*/

#endif  /* HELP_OFF */
}

/* end of help.c */
