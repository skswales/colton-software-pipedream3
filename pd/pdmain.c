/* pdmain.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       pdmain.c - module that drives PipeDream
 * Author:      RJM August 1987
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include <locale.h>
#include "esch.h"
#include "eventh.h"
#include "os.h"
#include "bbc.h"
#include "werr.h"
#include "flex.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#elif MS
#else
    assert(0);
#endif


#include "datafmt.h"



#if RISCOS
#include "ext.riscos"
#include "ext.pd"
#include "kernel.h"
#endif


/* exported functions */

extern BOOL act_on_c(intl c);
extern intl install_pipedream(char *username);
extern int  main(int argc, char *argv[]);
extern intl read_username(char *buffer);


/* internal functions */

#if FALSE
static void application__atexit(void);
static intl decrypt_byte(intl c);
static intl encrypt_byte(intl c);
static void getinstalloptions_sayhello(void);
#endif

#if !FREEBIE
static intl install_image(FILE *pdimage, intl usernamelen);
#endif
static intl check_valid_reg_number(intl installed);

/* installed flag */

#if FREEBIE
intl installed_ok = TRUE;
#else
intl installed_ok = FALSE;
#endif


/* registration addresses */

#if FREEBIE
#else
#define USER_NAME_OFFSET    0x10
#define REG_NUMBER_OFFSET   0x60
#define USER_NAME_ADDRESS   (char *) (0x8000 + USER_NAME_OFFSET)
#define REG_NUMBER_ADDRESS  (char *) (0x8000 + REG_NUMBER_OFFSET)
#endif


/* ----------------------------------------------------------------------- */

/************************************
*                                   *
* called by atexit() on exit(rc)    *
*                                   *
* NB. MS 5.1 library aborts do not  *
*   do atexit() processing !        *
*                                   *
************************************/

static void
application__atexit(void)
{
    reset_mc();

    #if RISCOS
    riscos_finalise_once();

    /* lose all font handles */
    font_close_file(DOCHANDLE_NONE);

    /* lose all external links */
    graph_close_file(DOCHANDLE_NONE);
    #endif

/* On RISCOS || ARTHUR, don't bother to release the escape & event handlers
 * as the C runtime library restores the caller's handlers prior to exiting
 * to the calling program.
 * It also gives us a new ms more protection against SIGINT.
*/
    #if defined(MS_HUGE)
    release();      /* at the last possible moment */
    #endif

/* On RISC OS wimpt__exit() is now called iff wimpt_init has been executed
 * as the CWimp library will have done an atexit() call too.
*/
}


#if !RISCOS

/************************************************
*                                               *
* tell the punter to install PipeDream and stop *
*                                               *
************************************************/

static void
exitbecauseinvalid(void)
{
#if RELEASED
    #if RISCOS
    werr_fatal(run_the_install_program_STR);
    #elif MS || ARTHUR
    puts(run_the_install_program_STR);
    exit(EXIT_FAILURE);
    #endif
#endif
}

#endif


/********************************************************************
*                                                                   *
* Say hello to the punter and check the registation number          *
* The registration number is four banks of four digits              *
* The sum of each digit from the corresponding sets adds up to 9    *
*                                                                   *
********************************************************************/

static void
getinstalloptions_sayhello(void)
{
#if RISCOS
    #if FREEBIE
    /* fake clean registration number */
    strcpy(registration_number, "0000 0000 0000 0000");
    #endif

    #if !RELEASED || defined(TUTU_BODGE)
    installed_ok = TRUE;
    #else
    if(!check_valid_reg_number(TRUE))
        {
        if(!check_valid_reg_number(FALSE))
            {
            werr(program_invalid_STR);
            exit(EXIT_FAILURE);
            }
        }
    #endif

#else

    #if !RELEASED || defined(TUTU_BODGE)
    /* fake clean registration number */
    strcpy(registration_number, "0000 0000 0000 0000");
    #endif


/* extract information registered at install time */

    #if !defined(SPELL_OFF)  &&  !defined(SPELL_BOUND)
    spell_installed = 0x7F;
    #endif


    #if MS
    /* get the screen display flags */
    fastdraw        = 0x7F;     /* fake values */
    multiple_pages  = 0x7F;
    sync            = 0;
    #endif


    #if MS || ARTHUR
    printf("\n%s\n%s\n\n%s",
            d_about[0].tag, d_about[1].tag, d_about[2].tag);
    #endif

    if(!check_valid_reg_number(TRUE))
        exitbecauseinvalid();

    #if ARTHUR
    putchar('A');
    #endif

    printf(registration_number);


    /* Hang about if Shift is pressed to see the above information */

    while(depressed_shift())
        ;

#endif  /* RISCOS */
}


#if RISCOS

/********************************************
*                                           *
* RISC OS has sent us some input we have    *
* interpreted as a command, so process it   *
*                                           *
********************************************/

extern void
application_process_command(intl c)
{
    in_execfile = command_expansion = been_error = FALSE;
    allow_output = TRUE;

    (void) act_on_c(-c);
}


/********************************************************
*                                                       *
* RISC OS has sent us some keyboard input so process it *
*                                                       *
********************************************************/

extern BOOL
application_process_key(intl c)
{
    BOOL res;

    in_execfile = command_expansion = been_error = FALSE;
    allow_output = TRUE;

    c = inpchr_riscos(translate_received_key(c));

    if(c)
        {
        res = act_on_c(c);

        /* may have key expansion going etc. */
        while((c = inpchr_riscos(0)) != 0)
            (void) act_on_c(c);
        }
    else
        res = TRUE;

    return(res);
}


#elif MS || ARTHUR

/****************************************
*                                       *
* loop getting input and processing it  *
*                                       *
****************************************/

static void
get_and_process_input(void)
{
    intl c;

    for(;;)
        {
        in_execfile = command_expansion = been_error = FALSE;
        allow_output = TRUE;

        #if MS
        if(autorepeat  &&  depressed_shift()  &&  !check_error)
            ;   /* use last input again */
        else
            c = inpchr(TRUE);

        check_error = autorepeat = FALSE;
        #elif ARTHUR
        c = inpchr(TRUE);
        #endif

        act_on_c(c);
        }
}

#endif  /* RISCOS */


/********************************
*                               *
* --in--                        *
*   +ve     normal characters   *
*   0       do nothing          *
*   -ve     a function number   *
*                               *
********************************/

extern BOOL
act_on_c(intl c)
{
    BOOL processed = TRUE;      /* unless otherwise specified */

    if(c < 0)
        {
        tracef1("act_on_c(%d) (command)\n", -c);
        processed = do_command(-c, TRUE);
#if TRACE
        if(ctrlflag)                /* should have all been caught by now */
            {
            char array[256];
            sprintf(array, ": command %d left escape unprocessed\n", -c);
            ack_esc();
            reperr(ERR_ESCAPE, array);
            }
#else
        ack_esc();
#endif
        }
    elif(c > 0)
        {
        inschr((uchar) c);          /* insert char in buffer */
        chkwrp();                   /* format if necessary */
        }
    #if RISCOS
    /* recalculation always done on null events */
    #else
    elif(calshe_break > 0)          /* c == 0 */
        {
        calshe_break = calshe(CALC_DRAW, CALC_RESTART);
        draw_screen();
        }
    #endif

    if(processed)
        {
        if(is_current_document())
            {
            #if RISCOS
            /* recalculation always done on null events */
            #else
            if( recalc_forced  ||
                (recalc_bit  &&  rcobit  &&  (calshe_break <= 0)))
                calshe_break = calshe(CALC_DRAW, CALC_RESTART);
            #endif

            draw_screen();
            }
        else
            tracef0("document disappeared from under our feet\n");
        }

    return(processed);
}


/********************************************************
*                                                       *
*  exec pd.key if it exists, in its own little domain   *
*                                                       *
********************************************************/

static void
exec_pd_key_once(void)
{
    char array[MAX_FILENAME];

    if(add_path(array, INITEXEC_STR, FALSE))
        exec_file(array);
}


extern void
exec_file(const char *filename)
{
    BOOL resetwindow;

    #if RISCOS
    window_data *wdp = find_document_using_window(caret_window);

    if(wdp)
        {
        dochandle cdoc = current_document_handle();

        select_document(wdp);

        do_execfile((char *) filename);

        if(is_current_document())
            draw_screen();

        select_document_using_handle(cdoc);
        }
    else
    #endif
    if(create_new_untitled_document())
        {
        resetwindow = TRUE;

        draw_screen();

        do_execfile((char *) filename);

        if(is_current_document())
            {
            (void) mergebuf_nocheck();

            if(xf_filealtered)          /* watch out for strange people */
                {
                draw_screen();
                resetwindow = FALSE;
                }
            else
                destroy_current_document();
            }

        if(resetwindow)
            {
            #if RISCOS
            riscos_resetwindowpos();
            #endif

            --NextUntitledNumber;
            }
        }
}


/****************************************************
*                                                   *
*  create a new document and load the file into it  *
*                                                   *
****************************************************/

extern BOOL
load_new_document(const char *filename);
extern BOOL
load_new_document(const char *filename)
{
    BOOL ok;

    tracef1("load_new_document(%s)\n", filename);

    ok = init_dialog_box(D_LOAD);

    if(ok)
        ok = loadfile((char *) filename, NEW_WINDOW, FALSE);

    if(ok)
        new_screen();

    return(ok);
} 


static void
pd_report_and_trace_enable(void)
{
    /* allow application to run without any report info */
    char env_value[256/*BUF_MAX_PATHSTRING*/];

    report_enable(NULL == _kernel_getenv("PipeDrea3$ReportEnable", env_value, sizeof(env_value)));

    #if TRACE
    /* allow trace version to run without any trace info */
    if(getenv("PipeDrea3$TraceEnable"))
        {
        trace_enabled = TRUE;
        trace_on();
        trace_clearscreen();
        tracef1("sp ~= &%p\n", &i);
        #if !defined(SB_GLOBAL_REGISTER)
        tracef1("&sb = &%p\n", &sb);
        #endif
        if(!depressed_shift())
            trace_off();
        }
    #endif
}

/****************************
*                           *
* main program starts here  *
*                           *
****************************/

extern void riscdialog_initialise_once(void);

extern int
main(int argc, char *argv[])
{
    intl i;

    wimptx_os_version_determine(); /* very early indeed */

    pd_report_and_trace_enable();

#if FREEBIE
#else
    /* Trap ESCAPE as soon as possible */
    #if defined(MS_HUGE)
    capture(&ctrlflag);
    #elif (ARTHUR || RISCOS)
    #   if defined(NO_SURRENDER)
            /* ensure ESCAPE enabled for trapping */
            fx_x2(229, 0);
    #   else
            EscH(&ctrlflag);
    #   endif
    #endif

    #if (ARTHUR || RISCOS)
    /* block events from reaching C runtime system where they cause enormous chugging */
    EventH();
    #endif
#endif

    if(atexit(application__atexit)) /* closedown proc called on exit */
        return(EXIT_FAILURE);

    if(!alloc_init())
        reperr_fatal(reperr_getstr(ERR_NOTINDESKTOP));

    /* we want notification of memory moving */
    flex_set_notify((flex_notifyproc) list_unlockpools);

    #if defined(WATCH_ALLOCS)
    tralloc_init();
    #endif

    #if defined(SB_GLOBAL_REGISTER)
    select_document(NO_DOCUMENT); 
    trace_sb();
    #endif


    #if ARTHUR
    /* decode command line arguments early to set screen mode */

    for(i = 1; i < argc; i++)
        {
        char *arg = argv[i];
        char ch   = *arg;

        switch(ch)
            {
            case FORESLASH:
            case BACKSLASH:
                ch = *++arg;
                if(isdigit(ch))
                    (void) bbc_mode(atoi(arg));
                else
                    tracef1("unable to set mode to %s\n", arg);
                break;

            default:
                break;
            }
        }
    #endif

    /* once off initialisations */

    #if RISCOS
    riscos_initialise_once();

    riscdialog_initialise_once();   /* may werr() if really duff */
    #endif

    getinstalloptions_sayhello();   /* may werr()/puts() if not installed */

    #if RISCOS
    riscos_visdelay_on();
    #endif

    init_mc();                      /* old style error reporting now possible */

    /* which parts of PipeDream are present */
    headline_initialise_once();

    /* initialise window data template */
    init_window_data_once();

    /* loads pd.ini if we have one */
    constr_initialise_once();

    /* exec pd.key if we have one */
    exec_pd_key_once();

#if RISCOS
#define ARG_STROP_CHAR '-'
#else
#define ARG_STROP_CHAR '/'
#endif

    /* decode command line arguments */

    for(i = 1; i < argc; ++i)
        {
        char *arg = argv[i];
        char ch   = *arg;

        trace_on();
        tracef2("*** got arg %d, '%s'\n", i, arg);
        trace_sb();
        trace_off();

        switch(ch)
            {
            #if ARTHUR && 0
            case FORESLASH:
            case BACKSLASH:
                /* already processed */
                break;
            #endif

            case ARG_STROP_CHAR:
                ch = *++arg;
                ch = tolower(ch);
                switch(ch)
                    {
                    case 'h':
                        #if RISCOS
                        application_process_command(N_Help);
                        #else
                        #if !defined(HELP_OFF)
                        Help_fn();
                        #endif
                        #endif
                        break;

                    #if RISCOS
                    case 'l':
                        setlocale(LC_CTYPE, argv[++i]);
                        break;
                    #endif

                    case 'm':
                        exec_file(argv[++i]);
                        break;

                    case 'n':
                        if(create_new_untitled_document())
                            draw_screen();
                        break;

                    #if RISCOS
                    case 'p':
                            print_file(argv[++i]);
                        break;
                    #endif

                    case 'q':
                        Quit_fn();
                        break;

                    #if TRACE
                    case 't':
                        trace_on();
                        break;
                    #endif

                    default:
                        #if RISCOS
                        werr(Unrecognised_arg_Zs_STR, arg);
                        #endif
                        break;
                    }
                break;


            default:
                if(installed_ok)
                    load_new_document(arg);

                break;
            }   /* esac */
        }   /* od */

    #if MS || ARTHUR
    /* must have one document to start us off */

    if(nDocuments == 0)
        {
        if(create_new_untitled_document())
            draw_screen();
        else
            return(EXIT_FAILURE);   /* most unlikely */
        }
    #endif

    #if RISCOS
/*  riscos_visdelay_off();      - do NOT do this; we will turn it off when polling starts */
    #endif

    /* Go and find something to do now */

    #if RISCOS
    return(riscos_go());
    #elif MS || ARTHUR
    get_and_process_input();
    return(EXIT_SUCCESS);
    #endif
}


/******************************************
*                                         *
* encrypt a byte for putting in the image *
*                                         *
******************************************/

#if RISCOS && !FREEBIE

static intl
encrypt_byte(intl c)
{
    intl x;

    x = c / 2;
    if(c % 2)
        x |= 0x80;

    return(x ^ 0x55);
}

#endif


/******************************************
*                                         *
* decrypt a byte for putting in the image *
*                                         *
******************************************/

#if RISCOS && !FREEBIE

static intl
decrypt_byte(intl c)
{
    intl x;

    c ^= 0x55;
    x = (c * 2) & 0xFF;
    if(c & 0x80)
        x |= 1;

    return(x);
}

#endif


/*********************************************
*                                            *
* install the PipeDream image on the disk by *
* poking in the users name                   *
*                                            *
*********************************************/

#if RISCOS && !FREEBIE

extern intl
install_pipedream(char *username)
{
    FILE *pdimage;
    char pdimage_buffer[256];
    uchar *o, *i;
    intl len, usernamelen, err, err1;

    tracef0("[install_pipedream opening image]\n");

    /* open image for update... dangerous */
    pdimage = fopen(PD_IMAGEFILE_STR, update_str);
    if(!pdimage)
        return(ERR_CANTINSTALL);

    mysetvbuf(pdimage, pdimage_buffer, sizeof(pdimage_buffer));
    tracef0("[install_pipedream image opened]\n");

    /* write user name into memory */
    len = usernamelen = strlen(username) + 1;
    o = USER_NAME_ADDRESS;
    i = username;
    while(--len >= 0)
        {
        *o++ = encrypt_byte(*i++);

        tracef3("[encrypt in: %x, out: %x, decrypt: %x]\n",
                i[-1], o[-1], decrypt_byte(o[-1]));
        }

    /* encrypt registration number */
    o = REG_NUMBER_ADDRESS;
    len = 20;
    while(--len >= 0)
        *o++ = encrypt_byte(*o);

    /* update the image */
    err = install_image(pdimage, usernamelen);
    err1 = fclose(pdimage);

    if(err || err1)
        {
        *registration_number = '\0';
        *USER_NAME_ADDRESS  = '\0';
        *REG_NUMBER_ADDRESS = '\0';
        return(ERR_CANTINSTALL);
        }

    return(check_valid_reg_number(TRUE));
}

#endif


/*****************************************************
*                                                    *
* write the encrypted name and number into the image *
*                                                    *
*****************************************************/

#if RISCOS && !FREEBIE

static intl
install_image(FILE *pdimage, intl usernamelen)
{
    /* write user name into the image */
    if(fseek(pdimage, USER_NAME_OFFSET, SEEK_SET))
        return(-1);

    if(fwrite(USER_NAME_ADDRESS, 1,
              usernamelen, pdimage) != usernamelen)
        return(-1);

    /* update the registration number in the image */
    if(fseek(pdimage, REG_NUMBER_OFFSET, SEEK_SET))
        return(-1);

    if(fwrite(REG_NUMBER_ADDRESS, 1,
              20, pdimage) != 20)
        return(-1);

    return(0);
}

#endif


/************************************
*                                   *
* read the user name from the image *
*                                   *
************************************/

#if RISCOS

extern intl
read_username(char *buffer)
{
#if FREEBIE
    strcpy(buffer, "Unregistered");
    return(0);
#else
    uchar *i;
    char *o;
    intl x;

    i = USER_NAME_ADDRESS;
    o = buffer;
    do  {
        x = decrypt_byte(*i++);
        *o++ = x;
        }
    while(x);

    tracef1("[read_username decrypted: %s]\n", buffer);

    return(buffer - o);
#endif
}

#endif


/******************************************
*                                         *
* check that registration number is valid *
*                                         *
******************************************/

#if RISCOS && !FREEBIE

static intl
check_valid_reg_number(intl installed)
{
    char *ptr, *o;
    intl i, len, x;
    intl total, j;

    /* copy registration number into internal buffer */
    o = registration_number;
    *o++ = 'R';
    for(ptr = REG_NUMBER_ADDRESS, len = 20;
        len;
        ++ptr, ++o, --len)
        {
        x = installed ? decrypt_byte(*ptr) : *ptr;

        tracef2("[check_valid_installed read: %d, %c]\n", x, (x & 0x7F));

        if(x & 0x80)
            *o = (x & 0x7F);
        else
            return(FALSE);
        }
    *o = '\0';

    tracef1("[reg number: %s]\n", registration_number);

    /* check the number adds up */
    for(i = 0; i < 4; i++)
        {
        for(total = 0, j = 0; j < 20; j += 5)
            total += registration_number[j + i + 1] - '0';

        tracef1("[total: %d]\n", total);
        if(total % 9 != 0)
            return(FALSE);
        }

    if(installed)
        installed_ok = TRUE;

    return(TRUE);
}

#else

static intl
check_valid_reg_number(intl installed)
{
    IGNOREPARM(installed);
    return(TRUE);
}

#endif

/* end of pdmain.c */
