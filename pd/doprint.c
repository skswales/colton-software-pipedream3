/* doprint.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       doprint.c - module that does printing from PipeDream
 * Author:      RJM August 1987
 * History:
 *  0.01    31-Jan-89   SKS derived from scinfo.c
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "drawfdiag.h"
#include "bbc.h"
#include "swinumbers.h"
#include "ext.spell"

#if RISCOS
#include "wimp.h"
#include "wimpt.h"
#include "font.h"
#include "drawmod.h"
#endif

#elif MS
#include <dos.h>
#include "spell.ext"
#else
    assert(0);
#endif


#include "datafmt.h"

#if RISCOS
#include "ext.riscos"
#include "riscdraw.h"
#endif


/* exported functions */

extern BOOL just_one_ch(intl ch);
extern void mspace(intl n);
extern void off_highlights(void);
extern BOOL print_complain(os_error *err);
extern intl print_page(void);
extern BOOL prnout(intl ch);
extern void setpvc(void);


/* internal functions */

#if !defined(PRINT_OFF)

/*static BOOL macrocall(void);*/
/*static void pause(void);*/
static intl printx(void);
#define print_newline() putchar('\n')
#define prrslx()
static void prncan(BOOL ok);
static void prnoff(BOOL ok);
static BOOL prnon(void);
static void out_h_string_value_0(LIST *lptr);

#if RISCOS
static void finalise_saved_print_state(void);
static void initialise_saved_print_state(void);
#endif


#define baseline_offset ((global_font_leading * 1) / 8)


/****************************************************************************
* print routines
****************************************************************************/

static BOOL prwbit = FALSE;     /* printer waiting */
static BOOL out_h_on = FALSE;   /* output PON highlight string? */
static BOOL out_h_off = FALSE;  /* output POFF highlight string */

static BOOL escape_pressed = FALSE;

#if ARTHUR || RISCOS
static intl oldfx3 = -1;
#endif

#if RISCOS
#define MILLIPOINTS_PER_OS (72000/180)

static intl riscos_copies;
static intl riscos_scale;
static intl riscos_seqnum;
#endif


#define MACROFILE_BUFSIZ 1024
static FILE *macrofile = NULL;              /* input TAB parameter file */
static void *macro_buffer = NULL;           /* where it's buffered */

static FILE *printer_output = NULL;         /* where print and spooling goes */
static FILE *saved_printer_output = NULL;   /* hold print stream while missing
                                             * page in sheets */

static BOOL had_top;                        /* printed top of page */
static BOOL new_file;                       /* new file loaded */
static intl pagcnt;                         /* lines printed on page so far not including margins */

static BOOL sheets_bit;                     /* pause between pages */
static BOOL screen_bit;                     /* print to screen */
static BOOL allowed_to_actind;
static BOOL print_failure;
#if MS
static BOOL tfastdraw;
#endif
static BOOL allow_print;

static coord linessofar;    /* number of lines draw on screen in screen command */
static coord maxwidth;
static uchar h_on = 0;      /* 8 bits specifying which highlights are on */
static intl prnspc = 0;     /* spaces waiting to be printed */


/* a printer driver is an array of output routines */

/* prnvec is pointer to printer driver */
static DRIVER *prnvec = BAD_POINTER;


static BOOL defoff(BOOL ok);
static BOOL defon(void);
static BOOL defout(intl ch);

static DRIVER default_driver =
    {
    defout,
    defon,
    defoff
    };


static BOOL spooff(BOOL ok);
static BOOL spoon(void);
static BOOL spoutc(intl ch);

static DRIVER spool_driver =
    {
    spoutc,
    spoon,
    spooff
    };


static BOOL drvoff(BOOL ok);
static BOOL drvon(void);
static BOOL drvout(intl ch);

static DRIVER loaded_driver =
    {
    drvout,
    drvon,
    drvoff
    };


#if RISCOS
static BOOL riscos_drvon(void);
static BOOL riscos_drvoff(BOOL ok);
static BOOL riscos_drvout(intl ch);

static DRIVER riscos_driver =
    {
    riscos_drvout,
    riscos_drvon,
    riscos_drvoff
    };
#endif


#define block_option (d_print[P_BLOCK].option == 'Y')

#define two_sided (d_print[P_TWOSIDE].option == 'Y')
static coord two_sided_margin;

#define left_page (two_sided && ((curpnm & 1) == 0))

#define landscape_option (d_print[P_ORIENT].option == 'L')


/* ----------------------------------------------------------------------- */

/**************************************
*                                     *
* small module to initialise a serial *
* printer on MSDOS or Archie          *
*                                     *
**************************************/

static intl
init_serial_print(intl port, intl baudrate, intl databits,
                  intl stopbits, intl parity)
{
    #if MS
    union REGS inregs, outregs;

    inregs.h.ah = 0;
    inregs.h.al = (uchar) ((baudrate << 5) | (parity << 3) |
                  (stopbits << 2) | databits);
    inregs.x.dx = port;
    int86(0x14, &inregs, &outregs);

    return((intl) inregs.h.ah);

    #elif ARTHUR || RISCOS

    intl rs423config;

    IGNOREPARM(port);

    /* set default RS423 state */
    os_swi3(OS_Byte, 181, 1, 0);

    /* set baud rates */
    os_swi2(OS_Byte, 7, baudrate);
    os_swi2(OS_Byte, 8, baudrate);

    /* set RS423 configuration */
    if(parity & 1)
        {
        rs423config = ((databits & 1) << 4) |
                      ((stopbits & 1) << 3) |
                      (((parity >> 1) ^ 1) << 2);
        }
    else
        /* handle no parity case */
        {
        rs423config = (1 << 4) | (((stopbits & 1) ^ 1) << 2);
        }
    os_swi3(OS_Byte, 156, rs423config, 0xE3);

    /* select serial printing */
    os_swi2(OS_Byte, 5, 2);

    return(0);

    #endif
}


/********************************
*                               *
* pause until SHIFT is pressed  *
*                               *
********************************/

#if defined(__APCS_32)
#ifndef XPortable_Idle
#define XPortable_Idle (0x42FC6 | (1U << 17))
#endif
extern int __swi(XPortable_Idle) __swi_XPortable_Idle(void);
#endif

static void
pause(void)
{
    BOOL tprnbit;

    tracef0("pause()\n");

    if(ctrlflag  ||  escape_pressed)
        {
        ack_esc();
        escape_pressed = TRUE;
        return;
        }

    tprnbit = prnbit;
    prnoff(TRUE);

    escape_enable();

    clearkeyboardbuffer();

    while(!ctrlflag)
    {
        if(depressed_shift())
            break;

#if defined(__APCS_32)
        __swi_XPortable_Idle(); /* Shift not pressed - this might help? */
#endif
    }

    if(escape_disable())
        escape_pressed = TRUE;

    print_newline();

    if(tprnbit)
        (void) prnon();

    linessofar = 0;
}


/********************************
*                               *
* print the text to the printer *
*                               *
********************************/

extern void
overlay_Print_fn(void)
{
    reset_print_options();

    if(!dialog_box(D_PRINT))
        return;

    dialog_box_end();

    print_document();
}


extern void
reset_print_options(void)
{
    DIALOG *dptr;

    /* reset dialog box options */
    for(dptr = d_print; dptr <= d_print + P_WAIT; dptr++)
        dptr->option = *(dptr->optionlist);

    d_print[P_COPIES].option = 1;
}


/******************************************
*                                         *
* MRJC created this fudger routine 9/8/89 *
* to fix problems with wait between pages *
* and multiple file print                 *
*                                         *
******************************************/

#if RISCOS

static void
wait_vdu_reset(BOOL clear)
{
    if(sheets_bit || screen_bit)
        {
        wimpt_safe(bbc_vdu(bbc_TextToText));
        wimpt_safe(wimp_textcolour(7));
        wimpt_safe(wimp_textcolour(0 | 0x80));
        if(clear)
            wimpt_safe(bbc_cls());
        wimpt_safe(bbc_tab(0, scrnheight()));
        }
}

#endif


/* row selection to print (updated once per row) */
static char *printing_rowselection = NULL;

/* column range to print */
static COLRANGE printing_colrange;

static char *printing_macro_buffer = NULL;


static intl
print_document_core(const char **errp)
{
    char parmfile_array[MAX_FILENAME];
    char *parmfile;
    char outfile_array[MAX_FILENAME];
    char *outfile;
    const char *colrange;
    intl res = 0;
    BOOL mustredraw = FALSE;
    #if RISCOS
    riscos_fileinfo printerfileinfo;
    BOOL triscos_fonts;
    #endif

    #if MS
    tfastdraw = fastdraw;
    #endif

    *errp = NULL;

    currently_inverted = currently_protected = FALSE;

    saved_printer_output = NULL;
    printer_output = stdout;

    ack_esc();
    escape_pressed = FALSE;

    allow_print = TRUE;
    print_failure = FALSE;

    screen_bit = FALSE;
    sheets_bit = (d_print[P_WAIT].option == 'Y');


    if(sheets_bit  &&  (d_poptions_PL == 0))
        return(ERR_NOPAGES);


    /* range of columns? */
    colrange = (d_print[P_COLS].option == 'Y')
                        ? d_print[P_COLS].textfield
                        : NULL;

    if(colrange)
        {
        if(block_option)
            return(ERR_BAD_SELECTION);

        buff_sofar = (char *) colrange;

        printing_colrange.stt = getcol();
        printing_colrange.end = getcol();

        if( printing_colrange.end == NO_COL)
            printing_colrange.end = printing_colrange.stt;

        if( (printing_colrange.stt == NO_COL)  ||
            (printing_colrange.stt > printing_colrange.end))
                return(ERR_BAD_RANGE);

trace_on();
tracef2("[printing colrange stt %d end %d]\n", printing_colrange.stt, printing_colrange.end);
trace_off();
        }
    else
        printing_colrange.stt = NO_COL;


    /* row condition? */
    if((d_print[P_ROWS].option == 'Y')  &&  !str_isblank(d_print[P_ROWS].textfield))
        {
        intl len, res;
        char oprstb[COMPILE_BUFSIZ];

        /* abuse linbuf temporarily */
        if(!mergebuf_nocheck())
            return(ERR_NOROOM);

        /* set up row selection */
        strcpy((char *) linbuf, d_print[P_ROWS].textfield);

        /* compile expression, and copy result to printing_rowselection */
        xf_inexpression = TRUE;
        len = cplent(oprstb, TRUE);
        xf_inexpression = FALSE;

        filbuf();

        if(len < 0)
            return(ERR_BAD_SELECTION);

        printing_rowselection = alloc_ptr_using_cache(len, &res);

        if(!printing_rowselection)
            return((res < 0) ? res : ERR_NOROOM);

        memcpy(printing_rowselection, oprstb, len);

trace_on();
tracef1("[printing row selection %s]\n", d_print[P_ROWS].textfield);
trace_off();
        }
    else
        printing_rowselection = NULL;


    if(d_print[P_PARMFILE].option == 'Y')
        {
        parmfile = d_print[P_PARMFILE].textfield;

        if(parmfile)
            {
            intl res;

            res = add_path(parmfile_array, parmfile, TRUE)
                        /* doing macros - check it's a tab file */
                        ? find_file_type(parmfile_array)
                        : '\0';

            if(res != 'T')
                {
                res = (res == '\0') ? ERR_NOTFOUND : ERR_NOTTABFILE;
                /* must pass back a pointer to a non-auto object */
                *errp = parmfile;
                return(res);
                }
            }
        }
    else
        parmfile = NULL;


    /* if block specified, check block marked */
    if(block_option  &&  !MARKER_DEFINED())
        return(MARKER_SOMEWHERE() ? ERR_NOBLOCKINDOC : ERR_NOBLOCK);


    /* setup printer destination */
    switch(d_print[P_PSF].option)
        {
        case 'F':
            {
            /* it's going to a file */
            outfile = d_print[P_PSF].textfield;

            if(str_isblank(outfile))
                return(ERR_BAD_NAME);

            (void) add_prefix_to_name(outfile_array, outfile, TRUE);

            if(!checkoverwrite(outfile_array))
                return(0);

            printer_output = myfopen(outfile_array, write_str);
            if(!printer_output)
                {
                /* must pass back a pointer to a non-auto object */
                *errp = outfile;
                return(ERR_NOTFOUND);
                }

            mysetvbuf(printer_output, NULL, 0);

            #if RISCOS
            /* Make file a 'Printout' type file so we can drag it to the Printer app. */
            if(riscos_readfileinfo(&printerfileinfo, outfile_array))
                {
                riscos_settype(&printerfileinfo, PRINTOUT_FILETYPE);
                riscos_writefileinfo(&printerfileinfo, outfile_array);
                }
            #endif

            spobit = TRUE;
            sheets_bit = FALSE;
            #if RISCOS
            riscos_printing = FALSE;
            #endif
            }
            break;


        /* it's going to the screen */
        case 'S':
            printer_output = stdout;
            screen_bit = TRUE;
            sheets_bit = FALSE;
            linessofar = 0;
            #if RISCOS
            riscos_printing = FALSE;
            #endif
            break;


        /* it's going to printer */
        default:
            #if MS
            {
            uchar *name = * (((char **) d_driver[1].optionlist) + d_driver[1].option);

            printer_output = myfopen(name, write_str);
            if(!printer_output)
                {
                *errp = name;
                return(ERR_CANNOTOPEN);
                }

            mysetvbuf(printer_output, NULL, 0);

            /* if serial port, initialize it */
            if( d_driver[1].option == driver_com1  ||
                d_driver[1].option == driver_com2)
                {
                intl port     = d_driver[1].option - driver_com1;
                intl baudrate = 7 - (intl) d_driver[2].option;
                intl databits = d_driver[3].option - '5';
                intl stopbits = d_driver[5].option - '1';
                intl parity   = (d_driver[4].option == 'N') ? 0 :
                                (d_driver[4].option == 'O') ? 1 : 2;

                if(init_serial_print(port, baudrate, databits, stopbits, parity))
                    {
                    *errp = serial_port_STR;
                    return(ERR_CANNOTOPEN);
                    }
                }

            /* set up error handler */
            _harderr(myhandler);
            }

            #elif ARTHUR || RISCOS

            tracef1("outputting to driver type %d\n", d_driver[1].option);
#if RISCOS
            riscos_printing = (d_driver[1].option == driver_riscos);
#endif

            escape_enable();

            switch(d_driver[1].option)
                {
                #if RISCOS
                case driver_riscos:
                    break;
                #endif

                case driver_serial:
                    init_serial_print(0, /*port*/
                                      d_driver[3].option,       /*baudrate*/
                                      d_driver[4].option - '5', /*databits*/
                                      d_driver[6].option - '1', /*stopbits*/
                                      (d_driver[5].option == 'N') ? 0 :
                                      (d_driver[5].option == 'O') ? 1 : 2   /*parity*/);
                    break;

                case driver_parallel:
                    fx_x2(5, 1);
                    break;

                case driver_network:
                    if(!str_isblank(d_driver[2].textfield))
                        {
                        char array[LIN_BUFSIZ+1];
                        char *str;

                        sprintf(array, PS_Zs_STR, d_driver[2].textfield);

                        str = mysystem(array);
                        if(str)
                            {
                            escape_disable_nowinge();
                            *errp = str;
                            return(ERR_PRINTER);
                            }
                        }

                    fx_x2(5, 4);
                    break;

            /*  case driver_user:   */
                default:
                    break;
                }

            if(escape_disable_nowinge())
                return(ERR_ESCAPE);

            #endif

            break;
        }

    two_sided_margin = (!two_sided  ||  str_isblank(d_print[P_TWOSIDE].textfield))
                                ? 0
                                : atoi(d_print[P_TWOSIDE].textfield);

    allowed_to_actind = (!screen_bit  &&  !sheets_bit);

    if(sheets_bit  ||  screen_bit)
        {
        #if RISCOS
        wait_vdu_reset(TRUE);
        #else
        clearscreen();
        #endif
        mustredraw = TRUE;

        #if MS
        sb_show_if_fastdraw();
        fastdraw = FALSE;
        #endif

        print_newline();

        if(sheets_bit)
            puts(Initialising_STR);
        }


    if(parmfile)
        {
        macrofile = myfopen(parmfile_array, read_str);

        if(macrofile)
            {
            intl res;
            printing_macro_buffer = alloc_ptr_using_cache(MACROFILE_BUFSIZ, &res);
            if(res < 0)
                return(res);
            mysetvbuf(macrofile, printing_macro_buffer, MACROFILE_BUFSIZ);
            }
        else
            {
            *errp = parmfile;
            return(ERR_NOTFOUND);
            }
        }


    #if RISCOS
    triscos_fonts = font_stack(riscos_fonts);

    /* does multiple copies differently */
    if(riscos_printing)
        {
        riscos_copies = d_print[P_COPIES].option;
        d_print[P_COPIES].option = 1;
        riscos_scale  = d_print[P_SCALE].option;
        }
    else
        riscos_fonts = FALSE;

    draw_to_screen = FALSE;
    #endif


    if(d_print[P_COPIES].option)
        {
        #if RISCOS
        initialise_saved_print_state();
        #endif

        do  {
            if(macrofile)
                fseek(macrofile, 0, SEEK_SET);

            prwbit = out_h_on = sqobit = TRUE;
            tracef0("had_top := FALSE\n");
            had_top = FALSE;

            if(!prnon())
                break;

            res = printx();

            if(screen_bit)
                pause();
            }
        while(!res  &&  (--d_print[P_COPIES].option > 0));

        /* never call if prnon not been called */
        prncan(TRUE);

        #if RISCOS
        finalise_saved_print_state();
        #endif
        }


    /* get rid of any leftovers */
    dispose((void **) &printing_rowselection);


    /* close macro file */
    if(macrofile)
        {
        myfclose(macrofile);
        macrofile = NULL;
        dispose(&macro_buffer);
        }


    /* get screen redrawn if needed */
    #if RISCOS
    if(is_current_document())
        riscos_fonts = font_unstack(triscos_fonts);

    riscos_printing = FALSE;
    draw_to_screen = TRUE;

    if(mustredraw)
        /* we blew everyone's windows away */
        wimpt_forceredraw();
    #else
    xf_draweverything = TRUE;

    #if MS
    fastdraw = tfastdraw;
    #endif
    #endif

    if(print_failure)
        res = ERR_GENFAIL;

    return(res);
}


extern void
print_document(void)
{
    const char *errp;
    intl res = print_document_core(&errp);

    if(res < 0)
        reperr(res, errp);
}


/***************************
*                          *
* set the pitch to n units *
*                          *
* NOP if RISC OS printing  *
*                          *
***************************/

extern void
set_pitch(intl n)
{
    char buffer[32];
    char *ptr;
    char ch;
    intl offset;
    LIST *lptr;

    if(riscos_printing  ||  !prnbit  ||  !micbit)
        return;

    /* send prefix code */

    lptr = search_list(&highlight_list, HMI_P);
    if(!lptr)
        return;

    out_h_string_value_0(lptr);

    /* is there an offset ? */
    lptr = search_list(&highlight_list, HMI_O);
    if(lptr)
        {
        offset = (intl) *lptr->value;

        if(offset > 128)
            n -= (256 - offset);
        else
            n += offset;
        }

    /* send length of gap as byte or text form? */
    if(hmi_as_text)
        {
        /* send length of gap as text form */
        sprintf(buffer, "%d", n);
        ptr = buffer;
        while((ch = *ptr++) != '\0')
            rawprint(ch);
        }
    else
        /* send length of gap as byte */
        rawprint(n);

    /* send suffix if it has one */
    out_h_string_value_0(search_list(&highlight_list, HMI_S));
}


#if RISCOS

static void
at_print(void)
{
    /* system font printing pos at top of char */
    /* + extra correction for system font baseline */
    print_complain(bbc_move(riscos_font_xad/MILLIPOINTS_PER_OS,
                            riscos_font_yad/MILLIPOINTS_PER_OS
                            + (charheight*(7-1))/8));
}

#endif  /* RISCOS */


/***************************************
*                                      *
* output a gap n microspace units long *
*                                      *
***************************************/

extern void
mspace(intl n)
{
    tracef1("mspace(%d)\n", n);

    #if RISCOS
    if(riscos_printing)
        {
        print_complain(bbc_plot(bbc_MoveCursorRel, n, 0));
        return;
        }
    #endif

    set_pitch(n);

    /* send gap */
    prnout(SPACE);

    /* reset to standard pitch */
    set_pitch(smispc);
}


/***********************************
*                                  *
* update page number from new file *
*                                  *
***********************************/

static void
getfpn(void)
{
    if(encpln == 0)
        {
        curpnm = 0;
        tracef1("[getfpn: curpnm := %d (encpln == 0)]\n", curpnm);
        }
    elif(new_file)
        {
        curpnm = filpnm;
        tracef1("[getfpn: curpnm := %d (filpnm)]\n", curpnm);

        /* check we haven't gone over a soft break in between the files,
         * cos then curpnm gets updated twice.
        */
        if(!pagcnt  &&  curfil)
            {
            curpnm--;
            tracef1("[getfpn: --curpnm := %d\n", curpnm);
            }
        }

    new_file = FALSE;
}


#if RISCOS

/* move print head down a number of lines */

static void
drop_n_lines(intl nlines)
{
    tracef1("drop_n_lines(%d)\n", nlines);

    if(nlines > 0)
        {
        if(riscos_printing)
            {
            /* drop baseline n lines */
            riscos_font_yad -= nlines * global_font_leading;
            tracef3("riscos_font_yad = %d (mp) after dropping %d line%s\n",
                    riscos_font_yad, nlines, (nlines == 1) ? "" : "s");
            }
        else
            wrchrep(CR, nlines);
        }
}

#else

#define drop_n_lines(nlines) wrchrep(CR, nlines)

#endif  /* RISCOS */


static intl
left_margin_width(void)
{
    intl nspaces = d_poptions_LM + ((left_page) ? 0 : two_sided_margin);

    tracef1("print_left_margin() giving %d spaces\n", nspaces);

    return(nspaces);
}


static void
print_left_margin(void)
{
    intl nspaces = left_margin_width();

    #if RISCOS
    if(riscos_printing)
        {
        riscos_font_xad = nspaces*(charwidth*MILLIPOINTS_PER_OS);
        return;
        }
    #endif

    ospca(nspaces);
}


/***************************
*                          *
* process header or footer *
*                          *
***************************/

static void
prchef(char *field)
{
    char array[PAINT_STRSIZ];

    tracef2("prchef(%s), page number = %d\n", trace_string(field), curpnm);

    if(curpnm)
        {
        if(str_isblank(field))
            return;

        /* set pitch if microspacing */
        setssp();
        set_pitch(smispc);

        print_left_margin();

        #if RISCOS
        if(riscos_printing)
            {
            print_setcolours(FORE, BACK);

            if(riscos_fonts)
                print_setfontcolours();
            else
                at_print();
            }
        #endif

        /* expand the lcr field and send it off */
        expand_lcr(field, -1, array, maxwidth, TRUE, TRUE, TRUE, TRUE);

        lcrjust(array, maxwidth, left_page);
        }

    prnout(EOS);        /* switch off highlights */
    prnout(CR);
}


/********************
*                   *
* eject top of page *
*                   *
********************/

static BOOL
topejc(void)
{
    tracef0("topejc()\n");

    getfpn();

    tracef0("had_top := TRUE\n");
    had_top = TRUE;

    /* output top margin */
    drop_n_lines(d_poptions_TM);

    /* output header */
    prchef(d_poptions_HE);

    /* output header margin */
    drop_n_lines(d_poptions_HM);

    return(!been_error);
}


/**************************************************************************
*                                                                         *
* end of page string                                                      *
*                                                                         *
* if the string doesn't contain a formfeed, only output if do_something   *
* is set, ie at the real end of page, not a trial one                     *
*                                                                         *
* return TRUE if anything output                                          *
*                                                                         *
**************************************************************************/

static BOOL
outff(BOOL do_something)
{
    tracef1("outff(%s)\n", trace_boolstring(do_something));

    if(riscos_printing)
        return(TRUE);

    if( !screen_bit
        #if !defined(SPOOL_HIGHLIGHTS)
        &&  !spobit
        #endif
      )
        {
        LIST *lptr = search_list(&highlight_list, E_P);

        if(lptr)
            {
            /* we have string, can we output it? */

            if(do_something  ||  strchr((char *) lptr->value, FORMFEED))
                {
                out_h_string_value_0(lptr);
                return(TRUE);
                }
            }
        }

    return(FALSE);
}


/***********************
*                      *
* eject bottom of page *
*                      *
***********************/

static void
botejc(void)
{
    tracef0("botejc()\n");

    tracef0("had_top := FALSE\n");
    had_top = FALSE;

    /* if there is no footer and end of page string contains a FORMFEED,
     * just do end of page string.
     * check also we haven't output all lines on page
    */
    if(pagcnt  ||  (d_poptions_FM > 0)  ||  (d_poptions_BM > 0))
        if(str_isblank(d_poptions_FO)  &&  outff(FALSE))
            return;

    /* output lines to footer margin */
    if(pagcnt)
        drop_n_lines(real_pagelength - real_pagcnt);

    /* output footer margin */
    drop_n_lines(d_poptions_FM);

    /* output footer */
    prchef(d_poptions_FO);

    if(d_poptions_BM > 0)
        {
        /* if can do FORMFEED, do it */
        if(!outff(FALSE))
            {
            /* must output blanks to end of page */
            drop_n_lines(d_poptions_BM);

            outff(TRUE);
            }
        }
}


/*
*/

static BOOL
pagejc(void)
{
    tracef0("pagejc()\n");

    if(!encpln)
        return(TRUE);

    if(had_top)
        {
        botejc();
        if(been_error)
            return(FALSE);
        getfpn();
        curpnm++;
        tracef1("[pagejc: ++curpnm := %d]\n", curpnm);
        return(TRUE);
        }

    return(topejc());
}


/********************************************
*                                           *
* read a TAB separated line from macrofile  *
* and stick parameters on list              *
*                                           *
********************************************/

static intl
macrocall(void)
{
    char buffer[LIN_BUFSIZ];
    int res;
    word32 key = 0;
    intl mres;

    tracef0("[macrocall()]\n");

    if(!macrofile)
        return(0);

    /* delete old macro list */
    delete_list(&first_macro);

    for(;;)
        {
        res = getfield(macrofile, buffer, FALSE);

        switch(res)
            {
            case EOF:
                return(0);

            case CR:
            case TAB:
                if(!str_isblank(buffer))
                    {
                    mres = add_to_list(&first_macro, key, buffer, NULL);
                    if(mres <= 0)
                        return(mres ? mres : ERR_NOROOM);
                    }
                /* omit blank fields ? */
                elif(d_print[P_OMITBLANK].option == 'Y')
                    --key;

                if(res == CR)
                    return(1);
                break;

            default:
                break;
            }

        ++key;
        }
}


/* do once initially and once for each subsequent
 * file in a multi-file document print
*/

/* range of slots to print */
static SLR printing_first;
static SLR printing_last;

static intl
init_print_selection(void)
{
    rowt trow;

trace_on();
    tracef0("*** init_print_selection\n");

    /* use marked block? */
    if(block_option)
        {
        printing_first = blkstart;
        printing_last  = (blkend.col == NO_COL) ? blkstart : blkend;
        }
    else
        {
        /* use column range? (all rows) */
        if(printing_colrange.stt != NO_COL)
            {
            printing_first.col = printing_colrange.stt;
            printing_last.col  = printing_colrange.end;
            }
        /* use whole document */
        else
            {
            printing_first.col = (colt) 0;
            printing_last.col  = numcol - 1;
            }

        printing_first.row = (rowt) 0;
        printing_last.row  = numrow - 1;
        }

    tracef4("[init_print_selection srow: %d, erow: %d, scol %d, ecol %d]\n",
            printing_first.row, printing_last.row, printing_first.col, printing_last.col);

    tracef0("[init_print_selection checking last.col]\n");

    /* check column range valid for this file */
    if(printing_last.col >= numcol)
        return(ERR_BAD_RANGE);


    /* find length of headers, footers */
    maxwidth = header_length(printing_first.col, printing_last.col + 1);

    tracef1("[init_print_selection maxwidth: %d]\n", maxwidth);

    /* make sure slot references start at beginning of marked block */
    if(printing_rowselection)
        for(trow = 0; trow < printing_first.row; trow++)
            inc_rows(printing_rowselection);

    tracef0("[init_print_selection initing block]\n");

    init_block(&printing_first, &printing_last);
trace_off();

    return(0);
}


/* print the page */

#if RISCOS
/* a huge graphics window for Draw file printing */
static draw_box printing_draw_box = { -(1<<15), -(1<<15), (1<<15), (1<<15) };

static drawmod_line printing_draw_line_style =
{
    /* flatness */          0,
    /* thickness */         0,
    /* spec */              {
    /* spec.join */             join_mitred,
    /* spec.leadcap */          cap_butt,
    /* spec.trailcap */         cap_butt,
    /* spec.reserved8 */        0,
    /* spec.mitrelimit */       0x00010000
                            },
    /* dash_pattern */      NULL
};
#endif

#define P_RETURN0   0
#define P_DONE_PAGE 1
#define P_FINISHED  2

extern intl
print_page(void)
{
    /* for each file in multi-file document */
    for(;;)
        {
        coord this_colstart;
        coord drawn_sofar = 0;      /* only used for PD printing */
        rowt previous_row = -1;     /* an invalid row number */
        slotp tslot;
        coord fwidth, colwid;
        intl res;

        tracef0("[print_page loop]\n");

        /* for each slot in file or block */
        while(next_in_block(ACROSS_ROWS))
            {
            tracef2("print_page loop... col %d, row %d\n", in_block.col, in_block.row);

            /* see if user wants to stop */
            if(ctrlflag  ||  escape_pressed)
                {
                ack_esc();
                escape_pressed = TRUE;
                return(ERR_ESCAPE);
                }

            if(previous_row != in_block.row)
                {
                /* it's a new row so check for hard break */
                if(chkrpb(in_block.row))
                    {
                    /* dont do hard break if coincides with soft break */
                    if( (in_block.row > 0)  &&  (pagcnt != enclns)  &&
                        chkpbs(in_block.row, pagcnt))
                        {
                        trace_on();
                        tracef0("[found active hard break line]\n");
                        trace_off();

                        /* this code does not cater, like does VP, for
                         * hard/soft combinations and file boundaries
                        */
                        if(!pagejc())
                            return(0);

                        pagcnt = enclns;
                        real_pagcnt = 0;
                        }
                    #if TRACE
                    else
                        {
                        trace_on();
                        tracef0("[found inactive hard break line]\n");
                        trace_off();
                        }
                    #endif

                    while(in_block.col < printing_last.col)
                        next_in_block(ACROSS_ROWS);

                    if(printing_rowselection)
                        inc_rows(printing_rowselection);

                    continue;       /* get another row to print */
                    }

                /* check if row selected */
                #if TRACE
                if(printing_rowselection)
                    {
                    trace_on();
                    trace_system("memory b &%p + 20", printing_rowselection);
                    trace_off();
                    }
                #endif
                if( printing_rowselection  &&
                    !selrow(printing_rowselection, in_block.row, TRUE))
                    {
                    trace_on();
                    tracef1("row %d not selected so go to new row\n", in_block.row);
                    trace_off();

                    while(in_block.col < printing_last.col)
                        next_in_block(ACROSS_ROWS);

                    continue;
                    }

                previous_row = in_block.row;
                this_colstart = drawn_sofar = 0;

                /* this seems to be user abandoning print before start */
                if(!had_top  &&  !pagejc())             /* prnrow1:  */
                    return(P_RETURN0);

                getfpn();                               /* prnrow2: */

                /* set pitch if microspacing */
                setssp();
                set_pitch(smispc);

                print_left_margin();

                if(been_error)
                    return(P_RETURN0);
                }

            if(colwidth(in_block.col))
                {
                /* update to start of column */
                if(!riscos_printing)
                    while(drawn_sofar < this_colstart)
                        {
                        prnout(SPACE);

                        if(been_error)
                            return(P_RETURN0);

                        drawn_sofar++;
                        }

                /* draw slot */
                tslot = travel_in_block();

                /* get width of slot */
                fwidth = chkolp(tslot, in_block.col, in_block.row);
                colwid = colwidth(in_block.col);

                #if RISCOS
                if(riscos_printing  &&  grid_on)
                    {
                    /* OS units */
                    intl x0 = riscos_font_xad/MILLIPOINTS_PER_OS;
                    intl y0 = (riscos_font_yad - global_font_leading/4)/MILLIPOINTS_PER_OS;
                    intl x = colwid*charwidth;
                    intl y = global_font_leading/MILLIPOINTS_PER_OS;
                    int path[4*3 + 2];
                    int *ptr = path;
                    drawmod_pathelemptr admit_defeat;

                    admit_defeat.wordp = path;

                    /* convert to Draw units */
                    x0 = x0 * 256;
                    y0 = y0 * 256;
                    x = x * 256;
                    y = y * 256;

                    *ptr++ = path_move_2;       /* MoveTo */
                    *ptr++ = x0;
                    *ptr++ = y0;

                    *ptr++ = path_lineto;       /* DrawTo */
                    *ptr++ = x0 + x;
                    *ptr++ = y0;

                    *ptr++ = path_lineto;       /* DrawTo */
                    *ptr++ = x0 + x;
                    *ptr++ = y0 + y;

                    *ptr++ = path_lineto;       /* DrawTo */
                    *ptr++ = x0;
                    *ptr++ = y0 + y;

                    *ptr++ = path_closeline;    /* EndOfPath */

                    *ptr++ = path_term;         /* EndOfObject */

                    print_setcolours(FORE, BACK);

                    print_complain(drawmod_stroke(admit_defeat, fill_Default, NULL, &printing_draw_line_style));
                    }
                #endif

                if(tslot)
                    {
                    #if RISCOS
                    drawfep dfep;
                    drawfrp dfrp;
                    BOOL flag;
                    draw_error err;
                    draw_redrawstr r;

                    if( riscos_printing  &&
                        draw_find_file(current_document_handle(),
                                        in_block.col, in_block.row,
                                        &dfep, &dfrp))
                        {
                        /* pictures fill line, not just down from baseline */
                        tracef2("found picture at %d, %d\n",
                                    in_block.col, in_block.row);

                        /* origin of draw file at x0, y1 */
                        r.box.x0 = riscos_font_xad/MILLIPOINTS_PER_OS;
                        r.box.y1 =
                            (riscos_font_yad
                             + (global_font_leading - baseline_offset)
                             - (dfrp->ysize_os * MILLIPOINTS_PER_OS)
                            ) / MILLIPOINTS_PER_OS;
                        r.scx    = 0;
                        r.scy    = 0;
                        r.g      = printing_draw_box;

                        tracef6("draw_render_diag(&%p, %d; box %d, %d, %d, %d; ",
                                dfep->diag.data, dfep->diag.length,
                                r.box.x0, r.box.y0, r.box.x1, r.box.y1);
                        tracef5("gw %d, %d, %d, %d; scale %g\n",
                            r.g.x0, r.g.y0, r.g.x1, r.g.y1, dfrp->xfactor);
                        flag = draw_render_diag((draw_diag *) &dfep->diag,
                                                &r, dfrp->xfactor, &err);
                        if(!flag)
                            {
                            if(err.type == DrawOSError)
                                {
                                tracef2("Draw: os error &%8.8X, \"%s\"\n",
                                    err.err.os.errnum, err.err.os.errmess);
                                print_complain(&err.err.os);
                                }
                            else
                                tracef2("Draw: draw error &%8.8X, \"%s\"\n",
                                    err.err.draw.code, err.err.draw.location);
                            }

                        /* Draw rendering has invalidated our colour cache */
                        killcolourcache();
                        }
                    else
                    #endif
                        {
                        tracef2("printing slot %d, %d\n", in_block.col, in_block.row);

                        if((fwidth > colwid)  &&  (tslot->type != SL_TEXT))
                            fwidth = colwid;

                        #if RISCOS
                        if(riscos_printing)
                            {
                            BOOL neg =  (tslot->type == SL_NUMBER)  &&
                                        (tslot->content.number.result.value < 0.0);

                            print_setcolours(neg ? NEGATIVEC : FORE, BACK);

                            if(riscos_fonts)
                                print_setfontcolours();
                            else
                                at_print();
                            }
                        #endif

                        drawn_sofar += outslt(tslot, in_block.row, fwidth);

                        prnout(EOS);
                        }
                    }

                if(been_error)
                    return(P_RETURN0);

                #if RISCOS
                if(riscos_printing)
                    riscos_font_xad += colwid*(charwidth*MILLIPOINTS_PER_OS);
                else
                #endif
                    this_colstart += colwid;
                }

            /* if at last column draw carriage returns */
            if(in_block.col == printing_last.col)
                {
                /* send out line spacing, checking not going over page break */
                intl i;

                for(i = 0; i < enclns; i++)
                    {
                    if((real_pagcnt < real_pagelength)  ||  (real_pagelength == 0))
                        {
                        prnout(CR);
                        real_pagcnt++;
                        }

                    pagcnt++;

                    if((real_pagelength > 0)  &&  (real_pagcnt >= real_pagelength))
                        {
                        pagcnt = 0;
                        real_pagcnt = 0;
                        break;
                        }
                    }

                if(pagcnt == 0)
                    {
                    if(!pagejc())       /* bottom of page */
                        return(P_RETURN0);

                    pagcnt = enclns;
                    real_pagcnt = 0;

                    return(P_DONE_PAGE);
                    }

                /* do some activity indicator */
                #if RISCOS
                actind(ACT_PRINT, percent_in_block(ACROSS_ROWS));
                #else
                if(allowed_to_actind)
                    {
                    BOOL tsqobit = sqobit;
                    BOOL tspobit = spobit;
                    BOOL tprnbit = prnbit;

                    /* do it on screen, not printer */
                    spobit = sqobit = FALSE;

                    prnoff(TRUE);

                    if(!actind(ACT_PRINT, percent_in_block(ACROSS_ROWS)))
                        {
                        prncan(FALSE);
                        return(ERR_ESCAPE);
                        }

                    /* restore flags for printer */
                    spobit = tspobit;
                    sqobit = tsqobit;

                    if(tprnbit)
                        (void) prnon();
                    }
                #endif
                }
            }

        tracef0("ran out of slots in this file: ");

        if(glbbit  &&  !block_option)
            {
            intl tprnbit = prnbit;
            #if RISCOS
            intl save_yad = riscos_font_yad;
            #endif

            tracef0("load next file\n");

            prnoff(TRUE);
            tracef1("[riscos_font_yad: %d]\n", riscos_font_yad);

            res = nxfloa();

            #if RISCOS
            wait_vdu_reset(FALSE);
            riscos_font_yad = save_yad;
            #endif

            tracef1("[riscos_font_yad: %d]\n", riscos_font_yad);
            if(tprnbit)
                (void) prnon();

            if(!res)
                {
                force_calshe();
                new_file = TRUE;

                /* initialise block, column range, row selection */
                if((res = init_print_selection()) < 0)
                    {
                    prncan(FALSE);
                    return(res);
                    }

                tracef0("[print_page done init - continue round main loop]\n");
                continue;       /* round main loop */
                }

            if(res == ERR_ENDOFLIST)
                tracef0("reached end of list file - complete current page\n");
            else
                {
                tracef0("nxfloa failed - current document destroyed\n");
                prncan(FALSE);
                return(res);
                }
            }
        else
            tracef0("complete current page\n");

        off_highlights();

        /* do bottom of page */
        if(had_top  &&  !pagejc())
            return(0);

        return(P_FINISHED);
        }
}


#if RISCOS

/* state to preserve - vague effort to reduce code on ARM */
typedef struct
{
    SLR  saved_in_block;
    BOOL saved_start_block;
    intl saved_pagcnt;
    intl saved_real_pagcnt;
    intl saved_new_file;
    intl saved_curpnm;
    intl saved_curfil;
    fpos_t saved_macrofilepos;
    list_block *saved_macrolist;
    BOOL saved_had_top;
}
riscos_print_save_str;

static riscos_print_save_str riscos_print_save;


static void
initialise_saved_print_state(void)
{
    riscos_print_save.saved_macrolist = NULL;
}


static void
save_print_state(void)
{
    riscos_print_save.saved_in_block    = in_block;
    riscos_print_save.saved_start_block = start_block;
    riscos_print_save.saved_pagcnt      = pagcnt;
    riscos_print_save.saved_real_pagcnt = real_pagcnt;
    riscos_print_save.saved_new_file    = new_file;
    riscos_print_save.saved_curpnm      = curpnm;
    if(macrofile)
        {
        fgetpos(macrofile, &riscos_print_save.saved_macrofilepos);

        delete_list(&riscos_print_save.saved_macrolist);
        riscos_print_save.saved_macrolist = first_macro;
        first_macro = NULL;
        }
    riscos_print_save.saved_had_top     = had_top;
    tracef1("saved had_top %s\n", trace_boolstring(had_top));
}


static intl
restore_saved_print_state(void)
{
    intl res = 1;

    in_block            = riscos_print_save.saved_in_block;
    start_block         = riscos_print_save.saved_start_block;
    pagcnt              = riscos_print_save.saved_pagcnt;
    real_pagcnt         = riscos_print_save.saved_real_pagcnt;
    new_file            = riscos_print_save.saved_new_file;
    curpnm              = riscos_print_save.saved_curpnm;
    if(macrofile)
        {
        fsetpos(macrofile, &riscos_print_save.saved_macrofilepos);

        res = duplicate_list(&first_macro, &riscos_print_save.saved_macrolist);
        }
    had_top             = riscos_print_save.saved_had_top;
    tracef1("restored had_top := %s\n", trace_boolstring(had_top));

    return(res);
}


static void
finalise_saved_print_state(void)
{
    delete_list(&riscos_print_save.saved_macrolist);
}


/* called to print a bit of a page from RISC OS */

static intl riscos_res;

static void
application_printpage(void)
{
    tracef0("application_printpage()\n");

    riscos_res = restore_saved_print_state();

    if(riscos_res > 0)
        riscos_res = print_page();

    tracef1("application_printpage got result %d\n", riscos_res);
}

#endif  /* RISCOS */


/*******************************************************
*                                                      *
* do the print or spool                                *
* printx does the bulk of the work for spool and print *
*                                                      *
*******************************************************/

static intl
printx(void)
{
    BOOL firstmacro = TRUE;
    intl res = 0;

    /* do a print of all files for each macro call */
    while(macrocall()  ||  firstmacro)
        {
        tracef0("in printx macrocall loop...\n");

        firstmacro = FALSE;

        /* reset page numbers */
        new_file = TRUE;
        pagcnt = enclns;
        real_pagcnt = 0;

        /* ensure highlight flags are disabled */
        h_waiting = h_on = 0;

        /* check for marked block or multi-file print */
        if(!block_option  &&  glbbit)
            {
            intl res;
            intl tprnbit = prnbit;
            #if RISCOS
            intl save_yad = riscos_font_yad;
            #endif

            prnoff(TRUE);
            tracef1("[riscos_font_yad: %d]\n", riscos_font_yad);

            res = iniglb();

            #if RISCOS
            wait_vdu_reset(FALSE);
            riscos_font_yad = save_yad;
            #endif

            tracef1("[riscos_font_yad: %d]\n", riscos_font_yad);
            if(tprnbit)
                (void) prnon();

            if(!res)
                {
                tracef0("iniglb failed - current document has been destroyed\n");
                prncan(FALSE);
                return(ERR_CANNOTOPEN);
                }

            force_calshe();
            }

        #if RISCOS
        riscos_seqnum = 1;
        #endif

        /* initialise block, column range, row selection */
        if((res = init_print_selection()) < 0)
            {
            prncan(FALSE);
            return(res);
            }

        tracef0("[printx done init]\n");

        /* keep printing pages until this macro call complete */
        for(;;)
            {
            tracef0("[printx about to call getfpn]\n");
            getfpn();

            /* does the user want wait between sheets etc. ? */
            if(encpln  &&  sheets_bit)
                {
                int c;
                char array[LIN_BUFSIZ];

                prnoff(TRUE);

                sprintf(array, Page_Zd_wait_between_pages_STR, curpnm);

                #if RISCOS
                wait_vdu_reset(FALSE);
                #endif
                strout(array, 80, TRUE);

                clearkeyboardbuffer();

                c = rdch(TRUE, TRUE);

                switch(toupper(c))
                    {
                    case ESCAPE:
                        bleep();
                        escape_pressed = TRUE;
                        return(1);              /* end printing please */

                    case 'M':
                        print_newline();
                        break;

                    case 'A':
                        sheets_bit = FALSE;
                        allowed_to_actind = TRUE;
                        #if MS
                        fastdraw = tfastdraw;
                        #endif

                        /* deliberate fall thru */

                    default:
                        print_newline();

                        if(!prnon())
                            return(ERR_GENFAIL);

                        #if RISCOS
                        if(riscos_printing)
                            {
                            print_setcolours(FORE, BACK);

                            if(riscos_fonts)
                                print_setfontcolours();
                            else
                                at_print();
                            }
                        #endif

                        break;
                    }
                }

            escape_enable();

            #if RISCOS
            if(riscos_printing  &&  prnbit)
                {
                save_print_state();

                res = riscprint_page(riscos_copies, landscape_option, riscos_scale,
                                     riscos_seqnum++, application_printpage);

                /* no error, or error either from giving page or printing slices */
                res = res ? riscos_res : P_RETURN0;
                }
            else
            #endif
                {
                #if RISCOS
                intl triscos_printing = riscos_printing;

                riscos_printing = FALSE;
                setpvc();
                #endif

                res = print_page();

                #if RISCOS
                riscos_printing = triscos_printing;
                setpvc();
                #endif
                }

            if(escape_disable())
                res = ERR_ESCAPE;

            tracef1("print_page returns %d\n", res);

            switch(res)
                {
                case P_DONE_PAGE:
                    continue;

                case P_FINISHED:
                    break;

                case P_RETURN0:
                    return(0);

                default:
#ifdef ROB
                    if(res > 0)
                        {
                        printf("error in printx - res = _%d_", res);
                        bleep();
                        exit(2);
                        }
                    else
#endif
                /* it's an error */
                return(res);
                }

            tracef0("this macro call finished\n");
            break;
            }

        }   /* each macro */

    return(0);
}


/****************************************************************************
*                                                                           *
*                       low level print routines                            *
*                                                                           *
****************************************************************************/

/************************************
*                                   *
* set the driver address in prnvec  *
*                                   *
************************************/

extern void
setpvc(void)
{
    prnvec =
                #if RISCOS
                (riscos_printing)
                    ? &riscos_driver
                    :
                #endif

                (spobit)
                    ? &spool_driver
                    :

                (driver_loaded  &&  !screen_bit)
                    ? &loaded_driver
                    : &default_driver;
}


/***************************** printer driver ******************************/

#if RISCOS && TRACE

static void
riscos_printing_abort(const char *procname)
{
    if(riscos_printing)
        {
        prncan(FALSE);

        werr_fatal("procedure %s called during RISC OS printing", procname);
        }
}

#else
#define riscos_printing_abort(procname)
#endif


#if RISCOS

static BOOL
riscos_drvon(void)
{
    tracef0("riscos_drvon()\n");

    /* only turn on right at start */
    if(out_h_on)
        {
        out_h_on = FALSE;
        return(riscprint_start());
        }

    /* otherwise just resume */
    riscprint_resume();
    tracef0("[riscos_drvon: print resumed]\n");
    return(TRUE);
}


static BOOL
riscos_drvoff(BOOL ok)
{
    tracef0("riscos_drvoff()\n");

    /*  only end job when fully finished */
    if(out_h_off)
        riscprint_end(ok);
    else
        /* otherwise just suspend */
        {
        riscprint_suspend();
        tracef0("[riscos_drvoff: print suspended]\n");
        }

    return(FALSE);
}


static BOOL
riscos_drvout(intl ch)
{
    BOOL res = TRUE;

    tracef2("riscos_drvout(%d ('%c'))\n", ch, ch ? ch : '0');

    switch(ch)
        {
        case CR:
            /* drop baseline one line; doesn't reposition xad */
            riscos_font_yad -= global_font_leading;
            tracef1("riscos_font_yad = %d (mp) due to CR\n", riscos_font_yad);
            break;

        case EOS:
            /* switch off highlights (for system font) */
            highlights_on = 0;
            break;

        default:
            res = print_complain(bbc_vdu(ch));
        }

    return(res);
}

#endif  /* RISCOS */


extern void
out_h_string(uchar *str, intl thisch)
{
    uchar ch;

    riscos_printing_abort("out_h_string");

    if(!prnbit)
        return;

    while((ch = *str++) != '\0')
        {
        if(ch == ESCAPE)
            {
            ch = *str++;

            if(ch == QUERY)
                {
                just_one_ch(thisch);
                continue;
                }
            elif(ch == 0xFF)
                ch = 0;
            }

        rawprint(ch);
        }
}


/* a most common call to the above funcion */

static void
out_h_string_value_0(LIST *lptr)
{
    if(lptr)
        out_h_string(lptr->value, 0);
}


/************************************************
*                                               *
* send character to printer, nothing in the way *
*                                               *
************************************************/

extern void
rawprint(intl ch)
{
    FILE *pout = printer_output;

    riscos_printing_abort("rawprint");

    #if ARTHUR || RISCOS
    /* don't send VDU 1 to files */
    if(prnbit  &&  !ctrlflag  &&  (pout == stdout))
        myfputc(1, pout);
    #endif

    if(!ctrlflag)
        myfputc(ch, pout);
}


/********************************************
*                                           *
* output one character to printer, perhaps  *
* substituting a printer driver string      *
*                                           *
********************************************/

extern BOOL
just_one_ch(intl ch)
{
    /* try for character substitution */
    LIST *lptr = search_list(&highlight_list, (word32) ch);

    riscos_printing_abort("just_one_ch");
    tracef1("[just_one_ch: %x]\n", ch);

    if(lptr)
        {
        out_h_string_value_0(lptr);
        return(TRUE);
        }

    rawprint(ch);

    if((ch == CR)  &&  send_linefeeds)
        return(drvout(LF));

    return(TRUE);
}


/********************************
*                               *
* printer driver - printer on   *
*                               *
********************************/

extern BOOL
drvon(void)
{
    riscos_printing_abort("drvon");

    tracef0("drvon()\n");

    escape_enable();

    #if ARTHUR || RISCOS
    if(out_h_on)
        print_complain(bbc_vdu(2));
    elif(oldfx3 >= 0)
        {
        fx_x2(3, oldfx3);
        oldfx3 = -1;
        }
    #elif MS
    if(saved_printer_output)
        printer_output = saved_printer_output;
    #endif

    /* if the driver has a printer on string, send it out */

    if(out_h_on)
        {
        out_h_string_value_0(search_list(&highlight_list, P_ON));

        out_h_on = FALSE;
        }

    return(!escape_disable());
}


/********************************
*                               *
* printer driver - printer off  *
*                               *
********************************/

extern BOOL
drvoff(BOOL ok)
{
    riscos_printing_abort("drvoff");

    IGNOREPARM(ok);

    escape_enable();

    #if ARTHUR || RISCOS
    if(out_h_off  &&  (oldfx3 >= 0))
        {
        fx_x2(3, oldfx3);
        oldfx3 = -1;
        }
    #endif

    if(out_h_off)
        out_h_string_value_0(search_list(&highlight_list, P_OFF));

    #if ARTHUR || RISCOS
    if(out_h_off)
        {
        print_complain(bbc_vdu(3));
        out_h_off = FALSE;
        }
    else
        {
        oldfx3 = fx_x(236, 0, 255);
        fx_x2(3, oldfx3 | 0x04);
        }

    if(d_driver[1].option == driver_serial)
        {
        fx_x2(21, 1);           /* flush RS423 buffers */
        fx_x2(21, 2);
        }
    #endif

    return(!escape_disable());
}


/************************************
*                                   *
* printer driver - output character *
*                                   *
************************************/

extern BOOL
drvout(intl ch)
{
    LIST *lptr;

    riscos_printing_abort("drvout");

    if(ishighlight(ch))
        {
        uchar mask = (uchar) (1 << (ch - FIRST_HIGHLIGHT));

        /* if it's waiting to go on, just turn it off */
        if(h_waiting & mask)
            {
            h_waiting &= ~mask;
            return(FALSE);
            }

        /* if its not on, make it wait */
        if(!(h_on & mask))
            {
            h_waiting |= mask;
            return(FALSE);
            }

        /* it must be on, so turn it off */
        h_on &= ~mask;

        /* if it's a query field don't output anything */
        if(h_query & mask)
            return(FALSE);

        /* if not a query field, use off code if it exists, otherwise on code */
        lptr = search_list(&highlight_list, ((word32) ch) + 256);

        if(!lptr)
            lptr = search_list(&highlight_list, (word32) ch);

        out_h_string_value_0(lptr);

        return(FALSE);
        }

    /* if at end of slot, switch highlights off, maybe */
    if(ch == EOS)
        {
        h_waiting &= ~off_at_cr;

        if(h_on)
            {
            uchar mask = 1;
            intl i;

            for(i = FIRST_HIGHLIGHT; i <= LAST_HIGHLIGHT; i++, mask <<= 1)
                {
                /* perhaps don't switch this one off */

                if((off_at_cr & mask) == 0)
                    continue;

                /* if it's waiting, just stop it waiting */
                if(h_waiting & mask)
                    h_waiting &= ~mask;
                elif((h_on & mask))
                    {
                    /* if it's on, turn it off, turn it off, turn it off, ...
                            turn it off again */

                    h_on &= ~mask;

                    /* if it's a query field, don't need to do anything */
                    if(h_query & mask)
                        continue;

                    lptr = search_list(&highlight_list, ((word32) i) + 256);

                    if(!lptr)
                        lptr = search_list(&highlight_list, (word32) i);

                    out_h_string_value_0(lptr);
                    }
                }
            }

        return(FALSE);
        }

    if(ch == CR)
        {
        if(!prnbit)     /* if outputting to screen */
            {
            print_newline();
            return(TRUE);
            }
        }
    elif(h_waiting)
        {
        /* if something waiting, switch it on */
        uchar mask;
        intl i;

        for(mask = 1, i = FIRST_HIGHLIGHT; i <= LAST_HIGHLIGHT; i++, mask <<= 1)
            if(h_waiting & mask)
                {
                /* if itsa query field, don't output anything */
                if(h_query & mask)
                    h_on |= mask;
                else
                    {
                    lptr = search_list(&highlight_list, (word32) i);

                    if(lptr)
                        {
                        out_h_string_value_0(lptr);
                        h_on |= mask;
                        }
                    }

                }
        h_waiting = 0;
        }

    /* output the character.  If any funny highlights expand the strings */
    if(h_query & h_on)
        {
        uchar mask = 1;
        int i;

        for(i = FIRST_HIGHLIGHT; i <= LAST_HIGHLIGHT; i++, mask <<= 1)
            if(h_query & h_on & mask)
                {
                /* if it's a query field, don't output anything */
                lptr = search_list(&highlight_list, (word32) i);
                if(lptr)
                    out_h_string(lptr->value, ch);
                }

        return(TRUE);
        }

    return(just_one_ch(ch));
}


/************************* default printer driver **************************/

/***************************
*                          *
* default output character *
*                          *
***************************/

extern BOOL
defout(intl ch)
{
    riscos_printing_abort("defout");

    if((ch == EOS)  ||  ishighlight(ch))
        return(FALSE);

    rawprint(ch);

    if((ch == CR)  &&  !prnbit)
        {
        /* now output end of page string which doesn't contain formfeeds */

        print_newline();

        if(screen_bit)
            {
            if(++linessofar >= scrnheight())
                pause();
            }
        }

    return(TRUE);
}


/********************************
*                               *
* default driver - printer on   *
*                               *
********************************/

extern BOOL
defon(void)
{
    riscos_printing_abort("defon");

    tracef0("defon()\n");

    escape_enable();

    #if ARTHUR || RISCOS
    if(!screen_bit)
        {
        if(out_h_on)
            print_complain(bbc_vdu(2));
        elif(oldfx3 >= 0)
            {
            fx_x2(3, oldfx3);
            oldfx3 = -1;
            }
        }
    #elif MS
    if(saved_printer_output)
        printer_output = saved_printer_output;
    #endif

    out_h_on = FALSE;

    return(!escape_disable());
}


/********************************
*                               *
* default driver - printer off  *
*                               *
********************************/

extern BOOL
defoff(BOOL ok)
{
    IGNOREPARM(ok);

    escape_enable();

    #if ARTHUR || RISCOS
    if(out_h_off)
        {
        if(oldfx3 >= 0)
            {
            fx_x2(3, oldfx3);
            oldfx3 = -1;
            }

        print_complain(bbc_vdu(3));

        out_h_off = FALSE;
        }
    else
        {
        oldfx3 = fx_x(236, 0, 255);
        fx_x2(3, oldfx3 | 0x04);
        }

    if(d_driver[1].option == driver_serial)
        {
        fx_x2(21, 1);           /* flush RS423 buffers */
        fx_x2(21, 2);
        }
    #endif

    return(!escape_disable());
}


/************************** spool printer driver ***************************/

/************************************
*                                   *
*  spool driver - output character  *
*                                   *
************************************/

extern BOOL
spoutc(intl ch)
{
    FILE *pout = printer_output;

    riscos_printing_abort("spoutc");

    #if defined(SPOOL_HIGHLIGHTS)

    if(!prnbit)
        return(FALSE);

    if(!drvout(ch)  &&  ferror(pout))
        return(reperr(ERR_CANNOTWRITE, d_print[P_PSF].textfield));

    if(ch == EOS || ishighlight(ch))
        return(FALSE);

    #else

    if(!prnbit  ||  (ch == EOS)  ||  ishighlight(ch))
        return(FALSE);

    if((myfputc(ch, pout) == EOF)  &&  ferror(pout))
        return(reperr(ERR_CANNOTWRITE, d_print[P_PSF].textfield));
    #endif

    return(TRUE);
}


/********************************
*                               *
*  spool driver - printer on    *
*                               *
********************************/

extern BOOL
spoon(void)
{
    riscos_printing_abort("spoon");

    tracef0("spoon()\n");

    escape_enable();

    if(saved_printer_output)
        printer_output = saved_printer_output;

    #if defined(SPOOL_HIGHLIGHTS)
    /* if the driver has a printer on string, send it out */

    if(out_h_on)
        {
        out_h_string_value_0(search_list(&highlight_list, P_ON));

        out_h_on = FALSE;
        }
    #else
    out_h_on = FALSE;
    #endif

    return(!escape_disable());
}


/********************************
*                               *
*  spool driver - printer off   *
*                               *
********************************/

extern BOOL
spooff(BOOL ok)
{
    #if !MS
    IGNOREPARM(ok);
    #endif

    riscos_printing_abort("spooff");

    escape_enable();

    #if defined(SPOOL_HIGHLIGHTS)
    if(out_h_off)
        out_h_string_value_0(search_list(&highlight_list, P_OFF));
    #endif

    #if MS
    defoff(ok);
    #endif

    tracef3("[spoof out_h_off: %d, printer_output: %x, stdout: %x]\n",
            out_h_off, printer_output, stdout);

    if(out_h_off)
        if(printer_output)
            {
            if(printer_output != stdout)
                {
                if(spobit  &&  fflush(printer_output))
                    reperr(ERR_CANNOTWRITE, d_print[P_PSF].textfield);

                tracef0("[closing printer_output]\n");
                myfclose(printer_output);
                }

            printer_output = NULL;
            }

    saved_printer_output = printer_output;
    printer_output = stdout;

    return(!escape_disable());
}


/****************************************
*                                       *
* send character to printer_output      *
* queues spaces for later output/strip  *
* returns TRUE if printable character   *
*                                       *
****************************************/

extern BOOL
prnout(intl ch)
{
    intl nspaces;
    BOOL (*out)(intl ch);

    tracef2("prnout(%d ('%c'))\n", ch, ch ? ch : '0');

    if((ch == SPACE)  &&  !microspacing)
        {
        tracef0("absorbing space\n");
        prnspc++;
        return(TRUE);
        }

    out = prnvec->out;
    nspaces = prnspc;

    /* if not end of slot, output spaces cos of printable character */
    if((ch != EOS)  &&  nspaces)
        {
        prnspc = 0;

        /* ignore space queue at end of line */
        if(ch != CR)
            do { out(SPACE); } while(--nspaces > 0);
        else
            tracef1("discarding %d trailing spaces at eol\n", nspaces);
        }

    return(out(ch));        /* output character via driver */
}


/****************************************
*                                       *
* turn printer on                       *
* printer turned on at beginning        *
* and at start of each page in sheets   *
*                                       *
****************************************/

static BOOL
prnon(void)
{
    BOOL res;

    prnspc = 0;

    if(!prwbit)
        return(TRUE);

    if(!screen_bit  &&  (d_print[P_PSF].option != 'S'))
        prnbit = TRUE;

    setpvc();

    res = (*prnvec->on)();      /* switch printer on via driver */

    if(!res)
        prnbit = FALSE;

    return(res);
}


/********************************************************
*                                                       *
* printer turned off at end and maybe end of each page  *
*                                                       *
********************************************************/

extern void
prnoff(BOOL ok)
{
    tracef0("prnoff()\n");

    if(!prnbit  &&  !out_h_off)
        return;

    (void) (*prnvec->off)(ok);      /* switch printer off via driver */

    prnbit = out_h_off = FALSE;
}


/************************
*                       *
* switch off highlights *
*                       *
************************/

extern void
off_highlights(void)
{
    if(!riscos_printing)
        {
        uchar mask = 1;
        intl i = FIRST_HIGHLIGHT;

        while(h_on  &&  (i <= LAST_HIGHLIGHT))
            {
            if(h_on & mask)
                {
                /* turn highlight off with either off string or on string */

                LIST *lptr = search_list(&highlight_list, ((word32) i) + 256);

                if(!lptr)
                    lptr = search_list(&highlight_list, (word32) i);

                out_h_string_value_0(lptr);
                }

            mask <<= 1;
            i++;
            }

        h_on = 0;
        }
}


/*******************************************
*                                          *
* cancel print:                            *
* switch off printer, close spool file etc *
*                                          *
*******************************************/

static void
prncan(BOOL ok)
{
    tracef1("prncan(%s)\n", trace_boolstring(ok));

    actind_end();

    off_highlights();

    /* delete the macro list */
    delete_list(&first_macro);

    sqobit = prwbit = FALSE;
    out_h_off = TRUE;

    prnoff(ok);

    spobit = FALSE;
}


#else   /* PRINT_OFF */


extern void
overlay_Print_fn(void)
{
    reperr_not_installed(ERR_GENFAIL);
}

#endif  /* PRINT_OFF */


/***********************
*                      *
* set standard spacing *
*                      *
***********************/

extern void
setssp(void)
{
    smispc = (prnbit  &&  (micbit  ||  riscos_printing))
                        ? d_mspace[1].option : 1;
}


#if RISCOS

extern BOOL
print_complain(os_error *err)
{
    if(err)
        {
        if(riscos_printing)
            reperr(ERR_PRINTER, err->errmess);
        else
            rep_fserr(err->errmess);
        }

    return((BOOL) err);
}

#endif  /* RISCOS */


#if MS

/********************************************************
*                                                       *
* if disc not ready or something, load,print comes here *
*                                                       *
********************************************************/

extern void
myhandler(void)
{
    print_failure = TRUE;

    /* can't print error message here cos printer is on */
    if(!prnbit)
        reperr_null(ERR_GENFAIL);

    _hardretn(36);
}

#endif  /* MS */


extern void
force_calshe(void)
{
    recalc_bit = recalc_forced = TRUE;
    calshe(CALC_NOINTORDRAW, CALC_RESTART);
}


extern coord
header_length(colt first, colt last)
{
    coord maxwidth = 0;
    coord this_colstart = 0;
    coord newwrap;
    colt tcol;

    for(tcol = first; tcol < last; tcol++)
        {
        newwrap = this_colstart + colwrapwidth(tcol);

        maxwidth = max(maxwidth, newwrap);

        this_colstart += colwidth(tcol);
        }

    return(max(this_colstart, maxwidth));
}


/************************************************
*                                               *
* get a field from the driver into a buffer     *
* if end of file or field too long return false *
*                                               *
************************************************/

extern int
getfield(FILE *input, uchar *array /*out*/, BOOL ignore_lead_spaces)
{
    int ch;
    uchar *ptr;

    if(ignore_lead_spaces)
        while((ch = myfgetc(input)) == (int) SPACE)
            ;
    else
        ch = myfgetc(input);

    for(ptr = array; ; ch = myfgetc(input))
        {
        switch(ch)
            {
            case EOF:
                *ptr = '\0';
                return(EOF);

            case LF:
            case CR:
                {
                /* read next character - if CR,LF or LF,CR throw
                 * character away, otherwise put character back in file
                */
                int newch = myfgetc(input);

                if((newch != EOF)  &&  ((newch + ch) != (LF + CR)))
                    {
                    if(ungetc(newch, input) == EOF)
                        {
                        reperr_null(ERR_CANNOTREAD);
                        *ptr = '\0';
                        return(EOF);
                        }
                    }
                }
                ch = CR;

                /* deliberate fall through */

            case TAB:
                *ptr = '\0';
                return(ch);

            default:
                *ptr++ = (uchar) ch;
                break;
            }
        }
}


/****************************
*                           *
*  page layout dialog box   *
*                           *
****************************/

extern void
overlay_PageLayout_fn(void)
{
    do  {
        (void) dialog_box(D_POPTIONS);

        update_variables();

        filealtered(TRUE);
        }
    while(!dialog_box_ended());
}


/************************************
*                                   *
* printer configuration dialog box  *
*                                   *
************************************/

extern void
overlay_PrinterConfig_fn(void)
{
#if defined(PRINT_OFF)
    reperr_not_installed(ERR_GENFAIL);
#else
    while(dialog_box(D_DRIVER))
        {
        load_driver();

        if(dialog_box_ended())
            break;
        }
#endif
}


/****************************
*                           *
* has to set micbit, smispc *
*                           *
****************************/

extern void
overlay_MicrospacePitch_fn(void)
{
#if !defined(PRINT_OFF)
    micbit = FALSE;

    if(!driver_loaded)
        {
        reperr_null(ERR_NO_DRIVER);
        return;
        }

    if(!search_list(&highlight_list, HMI_P))
        {
        reperr_null(ERR_NO_MICRO);
        return;
        }

    while(dialog_box(D_MSPACE))
        {
        micbit = FALSE;

        if(d_mspace[0].option == 'Y')
            {
            if(!d_mspace[1].option)
                {
                reperr_null(ERR_BAD_OPTION);
                dialog_box_end();
                return;
                }

            micbit = TRUE;
            }

        if(dialog_box_ended())
            break;
        }
#endif  /* PRINT_OFF */
}


/****************************************
*                                       *
*  define one of the macro parameters   *
*                                       *
****************************************/

extern void
overlay_SetMacroParm_fn(void)
{
    window_data *wdp;
    dochandle doc;
    intl res;

    while(dialog_box(D_PARAMETER))
        {
        delete_from_list(&first_macro, (word32) d_parameter[0].option);

        res = add_to_list(&first_macro,
                          (word32)  d_parameter[0].option,
                          (uchar *) d_parameter[1].textfield,
                          &res);

        if(res <= 0)
            {
            dialog_box_end();
            reperr_null(res ? res : ERR_NOROOM);
            break;
            }

        /* reflect changes in macro list to all windows */
        wdp = NULL;
        doc = current_document_handle();

        while((wdp = next_document(wdp)) != NULL)
            {
            select_document(wdp);
            out_screen = TRUE;
            draw_screen();
            }

        select_document_using_handle(doc);

        if(dialog_box_ended())
            break;
        }
}


/**************************************
*                                     *
* estimate how much core is left free *
*                                     *
**************************************/

#if MS && defined(BYTES_FREE)

static long
bytes_free(void)
{
    long free;
    struct _heapinfo heapb;

    list_freepoolspace(-1);

    free = 0;
    heapb._pentry = NULL;

    while(_heapwalk(&heapb) == _HEAPOK)
        {
        if(heapb._useflag == _FREEENTRY)
            free += heapb._size;
        }

    return(free);
}

#endif


#if !RISCOS

/****************************************************************************
*                                                                           *
*  display status of pages, insert/overtype, printer driver in dialog box   *
*                                                                           *
****************************************************************************/

extern void
overlay_PrintStatus_fn(void)
{
    char array[20];
    #if defined(BYTES_FREE)
    char array0[20];
    #endif
    #if defined(HEAP_INFO)
    uchar array1[20];
    uchar array2[20];
    uchar array3[20];
    uchar array4[20];
    uchar array5[20];
    uchar array6[20];
    uchar array7[20];
    int cache_blocks, largest_block;
    unsigned long total_cache_size;
    intl freeblks, usedblks;
    long maxfree, maxused;
    struct _heapinfo heapb;
    intl res;

    static char deadmes[] = "Heap dead";
    #endif

    sprintf(array, "%d", (!encpln || n_rowfixes) ? 0 : curpnm);

    d_status[0].textfield = array;
    d_status[1].textfield = xf_iovbit ? "Insert" : "Overtype";
    d_status[2].textfield = d_driver[0].textfield;

    #if defined(BYTES_FREE)
    sprintf(array0, "%ldk", bytes_free()/1000);
    d_status[3].textfield = array0;
    #endif

    #ifdef HEAP_INFO
    #if !defined(SPELL_OFF)
    spell_stats(&cache_blocks, &largest_block, &total_cache_size);
    sprintf(array1, "%d", cache_blocks);
    sprintf(array2, "%d", largest_block);
    sprintf(array3, "%ld", total_cache_size);

    d_status[4].textfield = array1;
    d_status[5].textfield = array2;
    d_status[6].textfield = array3;
    #else
    d_status[4].textfield = NULL;
    d_status[5].textfield = NULL;
    d_status[6].textfield = NULL;
    #endif

    freeblks = usedblks = 0;
    maxfree = maxused = 0;
    heapb._pentry = NULL;
    while((res = _heapwalk(&heapb)) == _HEAPOK)
        {
        if(heapb._useflag == _USEDENTRY)
            {
            ++usedblks;
            maxused = max(maxused, heapb._size);
            }
        else
            {
            ++freeblks;
            maxfree = max(maxfree, heapb._size);
            }
        }

    if(res == _HEAPEND)
        {
        sprintf(array4, "%d", freeblks);
        sprintf(array5, "%ld", maxfree);
        sprintf(array6, "%d", usedblks);
        sprintf(array7, "%ld", maxused);
        d_status[7].textfield = array4;
        d_status[8].textfield = array5;
        d_status[9].textfield = array6;
        d_status[10].textfield = array7;
        }
    else
        {
        d_status[7].textfield = deadmes;
        d_status[8].textfield = deadmes;
        d_status[9].textfield = deadmes;
        d_status[10].textfield = deadmes;
        }

    #endif

    if(dialog_box(D_STATUS))
        dialog_box_end();
}

#endif  /* RISCOS */

/* end of doprint.c */
