/* riscdialog.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       riscdialog.c - dialog box handling for RISC OS PipeDream
 * Author:      Stuart K. Swales 14-Mar-1989
*/

/* standard header files */
#include "flags.h"


#if RISCOS
/* module only compiled if RISCOS */

#include "swinumbers.h"

#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "wimpt.h"
#include "win.h"
#include "menu.h"
#include "event.h"
#include "dbox.h"
#include "werr.h"
#include "fileicon.h"
#include "xfersend.h"
#include "flex.h"
#include "print.h"

#include "datafmt.h"

#include "ext.riscos"
#include "ext.pd"
#include "riscmenu.h"

#include "riscdialog.h"

#include "monotime.h"


/* ----------------------------------------------------------------------- */

static dbox dialog__dbox = NULL;


/* whether dialog box filled in ok */
static BOOL ok;


/* whether dialog box wants to persist */
static BOOL dialog__may_persist;


/****************************
*                           *
* create a named dialog box *
*                           *
****************************/

static BOOL
dialog__create_reperr(const char *dboxname, dbox *dd)
{
    char *errorp;
    dbox d = dbox_new(dboxname, &errorp);

    *dd = d;

    if(!d)
        {
        if(errorp)
            rep_fserr(errorp);
        else
            reperr_null(ERR_NOROOMFORDBOX);
        }

    return((BOOL) d);
}


static BOOL
dialog__create(const char *dboxname)
{
    tracef1("dialog__create(%s)\n", dboxname);

    if(!dialog__dbox)
        (void) dialog__create_reperr(dboxname, &dialog__dbox);
    else
        tracef1("dialog__dbox already exists: &%8.8X\n", dialog__dbox);

    return((BOOL) dialog__dbox);
}


/****************************************************
*                                                   *
* fillin a dialog box until any action is returned  *
*                                                   *
****************************************************/

static dbox_field
dialog__fillin_for(dbox d)
{
    dbox_field f;
    dochandle doc = current_document_handle();

    f = dbox_fillin(d);

    select_document_using_handle(doc);  /* in case document swapped */

    tracef2("dialog__fillin_for(&%p) yields %d\n", d, f);
    return(f);
}


static dbox_field
dialog__fillin(void)
{
    dbox d = dialog__dbox;
    dbox_field f;

    tracef0("dialog__fillin()\n");

    dbox_show(d);

    f = dialog__fillin_for(d);

    ok = (f == dbox_OK);

    tracef2("dialog__fillin returns field %d, ok = %s\n",
                f, trace_boolstring(ok));
    return(f);
}


/****************************************************
*                                                   *
* fillin a dialog box until either closed or 'OK'ed *
*                                                   *
****************************************************/

static void
dialog__simple_fillin(void)
{
    dbox_field f;

    tracef0("dialog__simple_fillin()\n");

    do { f = dialog__fillin(); } while((f != dbox_CLOSE)  &&  (f != dbox_OK));

    tracef1("dialog__simple_fillin returns ok = %s\n", trace_boolstring(ok));
}


/****************************
*                           *
*  dispose of a dialog box  *
*                           *
****************************/

static void
dialog__dispose(void)
{
    tracef1("dialog__dispose(): dialog__dbox = &%p\n", dialog__dbox);

    dbox_dispose(&dialog__dbox);
}


/************************************************************
*                                                           *
*  explicit disposal of a dialog box: kill menu tree too    *
*                                                           *
************************************************************/

extern void
riscdialog_dispose(void)
{
    dialog__dispose();

    /* stop dialog box recreation */
    riscmenu_clearmenutree();
}


/********************************
*                               *
* should we persist or go away? *
*                               *
********************************/

extern intl
riscdialog_ended(void)
{
    BOOL ended;

    if(!dialog__dbox)
        {
        ended = TRUE;               /* already disposed of */
        tracef0("riscdialog_ended = TRUE as dialog__dbox already disposed\n");
        }
    else
        {
        ended = !dialog__may_persist;
        tracef1("riscdialog_ended = %s\n", trace_boolstring(ended));

        if(ended)
            /* kill menu tree too (probably dead but don't take chances) */
            riscdialog_dispose();
        }

    return(ended);
}


/************************************************************
*                                                           *
*  ensure created, encode, fillin and decode a dialog box   *
*                                                           *
************************************************************/

extern intl
dialog__raw_eventhandler(dbox d, void *v_e, void *handle)
{
    wimp_eventstr *e = (wimp_eventstr *) v_e;
    BOOL processed = FALSE;

    IGNOREPARM(d);
    IGNOREPARM(handle);

    switch(e->e)
        {
        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            if(e->data.msg.hdr.action == wimp_MHELPREQUEST)
                {
                tracef0("help request on pd dialog box\n");
                riscos_sendhelpreply(help_dialog_window,
                                     (riscos_eventstr *) e);
                processed = TRUE;
                }
            break;

        default:
            break;
        }

    return(processed);
}


static void
dialog__register_help_for(dbox d)
{
    dbox_raw_eventhandler(d, dialog__raw_eventhandler, NULL);
}


extern intl
riscdialog_execute(dialog_proc dproc, const char *dname,
                    DIALOG *dptr, intl boxnumber)
{
    const char *title;

    ok = FALSE;

    if(!dname)
        {
        tracef2("riscdialog_execute(%s, \"\", &%8.8X)\n",
                    trace_procedure_name((trace_proc) dproc), dptr);

        tracef0("calling dialog procedure without creating dialog__dbox\n");
        dproc(dptr);
        }
    else
        {
        tracef3("riscdialog_execute(%s, \"%s\", &%8.8X)\n",
                    trace_procedure_name((trace_proc) dproc), dname, dptr);

        switch(boxnumber)
            {
            #if !defined(SPELL_OFF)

            /* cropcldict */
            case D_USER_CREATE:
                title = Create_user_dictionary_STR;
                break;

            case D_USER_OPEN:
                title = Open_user_dictionary_STR;
                break;

            case D_USER_CLOSE:
                title = Close_user_dictionary_STR;
                break;


            /* unlockdict */
            case D_USER_LOCK:
                title = Lock_dictionary_STR;
                break;

            case D_USER_UNLOCK:
                title = Unlock_dictionary_STR;
                break;


            /* anasubgram */
            case D_USER_ANAG:
                title = Anagrams_STR;
                break;

            case D_USER_SUBGRAM:
                title = Subgrams_STR;
                break;


            /* insdelword */
            case D_USER_INSERT:
                title = Insert_word_in_user_dictionary_STR;
                break;

            case D_USER_DELETE:
                title = Delete_word_from_user_dictionary_STR;
                break;

            #endif  /* SPELL_OFF */


            /* onefile */
            case D_SAVEINIT:
                title = Save_initialisation_file_STR;
                break;

            case D_EXECFILE:
                title = Do_macro_file_STR;
                break;

            case D_MACRO_FILE:
                title = Record_macro_file_STR;
                break;


            /* insremhigh */
            case D_INSHIGH:
                title = Insert_highlights_STR;
                break;

            case D_REMHIGH:
                title = Remove_highlights_STR;
                break;


            default:
                title = NULL;
                break;
            }

        if(title)
            dbox_setinittitle(dname, title);

        if(dialog__create(dname))
            {
            dialog__register_help_for(dialog__dbox);

            dproc(dptr);

            if(!ok)             /* never persist if faulty */
                {
                tracef0("disposing because of faulty fillin etc.\n");
                dialog__dispose();
                }
            }
        else
            tracef0("failed to create dialog__dbox\n");
        }

    dialog__may_persist = ok ? dbox_persist() : FALSE;

    tracef1("riscdialog_execute returns %s\n", trace_boolstring(ok));
    return(ok);
}


#define dboxquery_FYes      0   /* action */
#define dboxquery_FMsg      1   /* string output */
#define dboxquery_FNo       2   /* action */
#define dboxquery_FCancel   3   /* optional cancel button */

extern intl
mydboxquery(const char *question, const char *dboxname, dbox *dd)
{
    intl res;
    dbox d;
    dbox_field f;

    tracef1("riscdialog_query(%s)\n", question);

    if(!*dd)
        {
        if(!dialog__create_reperr(dboxname, dd))
            /* out of space - embarassing */
            return(riscdialogquery_CANCEL); 
        }

    if(!question)
        return(riscdialogquery_YES);

    d = *dd;

    dbox_setfield(d, 1, question);

    dialog__register_help_for(d);

    dbox_show(d);

    f = dialog__fillin_for(d);

    dbox_hide(d);           /* don't dispose !!! */

    switch(f)
        {
        case dboxquery_FYes:
            res = riscdialogquery_YES;
            ok = TRUE;
            break;

        case dboxquery_FNo:
            res = riscdialogquery_NO;
            ok = TRUE;
            break;

    /*  case dboxquery_FCancel: */
        default:
            res = riscdialogquery_CANCEL;
            ok = FALSE;
            break;
        }

    tracef2("riscdialog_query(%s) returns %d\n", question, res);
    return(res);
}


/********************************************************
*                                                       *
*                                                       *
********************************************************/

static dbox query_dbox = NULL;

extern void
riscdialog_initialise_once(void)
{
    /* create at startup so we can always prompt user (for most things) */

    if(mydboxquery(NULL, "query", &query_dbox) == riscdialogquery_CANCEL)
        exit(EXIT_FAILURE);
}


extern intl
riscdialog_query(const char *mess)
{
    return(mydboxquery(mess, "query", &query_dbox));
}


extern intl
riscdialog_queryexit(const char *mess)
{
    dbox d = NULL;
    intl res = mydboxquery(mess, "queryexit", &d);

    dbox_dispose(&d);

    return(res);
}


extern intl
riscdialog_save_existing(void)
{
    char tempstring[256];
    intl res;

    if(!xf_filealtered)
        {
        tracef0("riscdialog_save_existing() returns NO because file not altered\n");
        return(riscdialogquery_NO);
        }

    sprintf(tempstring, save_edited_file_Zs_STR, fndfcl());

    res = riscdialog_query(tempstring);

    switch(res)
        {
        case riscdialogquery_CANCEL:
            #if TRACE
            tracef0("riscdialog_save_existing() returns CANCEL\n");
            return(res);
            #else
            /* deliberate drop thru ... */
            #endif

        case riscdialogquery_NO:
            tracef0("riscdialog_save_existing() returns NO\n");
            return(res);

    /*  case dboxquery_YES: */
        default:
            tracef0("riscdialog_save_existing() got YES; saving file\n");
            break;
        }

    application_process_command(N_SaveFile);

    /* may have failed to save or saved to unsafe receiver */
    if(been_error  ||  xf_filealtered)
        res = riscdialogquery_CANCEL;

    tracef1("riscdialog_save_existing() returns %d\n", res);
    return(res);
}


/****************************************************************************
*                                                                           *
*               dialog box field manipulation procedures                    *
*                                                                           *
****************************************************************************/

/********************************
*                               *
*  get/set text fields of icons *
*                               *
********************************/

static BOOL
dialog__getfield(dbox_field f, DIALOG *dptr)
{
    char **var = &dptr->textfield;
    char tempstring[256];

    dbox_getfield(dialog__dbox, f, tempstring, sizeof(tempstring));

    tracef3("dialog__getfield(%d, &%8.8X) yields \"%s\"\n",
            f, var, tempstring);

    /* translate ctrl chars too */

    return((BOOL) str_setc(var, tempstring));
}


static BOOL
dialog__getfield_high(dbox_field f, DIALOG *dptr)
{
    char **var = &dptr->textfield;
    char tempstring[256];
    char *src;
    char *dst;
    char ch;

    dbox_getfield(dialog__dbox, f, tempstring, sizeof(tempstring));

    tracef3("dialog__getfield(%d, &%8.8X) yields \"%s\"\n",
            f, var, tempstring);

    /* translate ctrl chars */

    src = dst = tempstring;

    do  {
        if((ch = *src++) == '^')
            {
            ch = *src++;
            if(ishighlighttext(ch))
                ch = (ch - FIRST_HIGHLIGHT_TEXT) + FIRST_HIGHLIGHT;
            elif(ch != '^')
                *dst++ = '^';
            }

        *dst++ = ch;
        }
    while(ch != '\0');

    return((BOOL) str_setc(var, tempstring));
}


static void
dialog__setfield_high(dbox_field f, const DIALOG *dptr)
{
    char array[256];
    intl i = 0;
    char ch;
    const char *str = dptr->textfield;

    tracef3("dialog__setfield_high(%d, (&%p) \"%s\")\n",
                                    f, str, trace_string(str));

    if(!str)
        str = (const char *) NULLSTR;

    /* translate ctrl chars */

    do  {
        if(i >= sizeof(array) - 1)
            ch = '\0';
        else
            ch = *str++;

        if((ch == '^')  ||  ishighlight(ch))
            {
            array[i++] = '^';
            if(ch != '^')
                ch = (ch - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT;
            }

        array[i++] = ch;
        }
    while(ch != '\0');

    dbox_setfield(dialog__dbox, f, array);
}


static void
dialog__setfield_str(dbox_field f, const char *str)
{
    tracef3("dialog__setfield_str(%d, (&%p) \"%s\")\n",
                                    f, str, trace_string(str));

    if(!str)
        str = (const char *) NULLSTR;

    dbox_setfield(dialog__dbox, f, str);
}


static void
dialog__setfield(dbox_field f, const DIALOG *dptr)
{
    dialog__setfield_str(f, dptr->textfield);
}


/* no getchar() needed */

static void
dialog__setchar(dbox_field f, const DIALOG *dptr)
{
    char tempstring[2];

    tempstring[0] = iscntrl(dptr->option) ? ' ' : dptr->option;
    tempstring[1] = '\0';

    dbox_setfield(dialog__dbox, f, tempstring); /* no translation */
}


/* no getcolour() needed */

static void
dialog__setcolour(dbox_field f, const DIALOG *dptr)
{
    wimp_w w = dbox_syshandle(dialog__dbox);
    wimp_i i = dbox_field_to_icon(dialog__dbox, f);

    tracef4("dialog__setcolour(%d, %d) w %d i %d",
                f, dptr->option & 0x0F, w, i);

    wimpt_safe(wimp_set_icon_state(w, i,
                        /* EOR */   ((dptr->option & 0x0F) * wimp_IBACKCOL),
                        /* BIC */   0x0F * wimp_IBACKCOL));
}


/* a bumpable item is composed of an Inc, Dec and Value fields */

/********************************************************
*                                                       *
* --in--                                                *
*   field actually hit, two fields to select between    *
*                                                       *
* --out--                                               *
*   field we pretend to have hit                        *
*                                                       *
********************************************************/

static BOOL
dialog__adjust(dbox_field *fp, dbox_field val)
{
    dbox_field f   = *fp;
    dbox_field dec = val - 1;
    dbox_field inc = dec - 1;

    /* make ADJUST do the other thing if hit one of these two */
    if((f == dec)  ||  (f == inc))
        {
        if(riscos_adjustclicked())
            {
            tracef1("ADJUST pressed: returning other button hit %d\n",
                    f ^ inc ^ dec);
            *fp = f ^ inc ^ dec;
            }
        else
            tracef1("SELECT pressed: returning current button hit %d\n", f);

        return(TRUE);   /* hit one of these two */
        }

    return(FALSE);
}


/************************************************
*                                               *
*  get/set text fields of numeric icons         *
*  for F_NUMBER, F_COLOUR, F_CHAR type entries  *
*                                               *
************************************************/

static void
dialog__getnumericlimited(dbox_field f, DIALOG *dptr, intl minval, intl maxval)
{
    int num = dbox_getnumeric(dialog__dbox, f);

    num = max(num, minval);
    num = min(num, maxval);

    dptr->option = num;

    tracef2("dialog__getnumeric(%) yields %d\n", f, dptr->option);
    assert((dptr->type==F_NUMBER) || (dptr->type==F_COLOUR) || (dptr->type==F_CHAR) || (dptr->type==F_LIST));
}


static void
dialog__getnumeric(dbox_field f, DIALOG *dptr)
{
    dialog__getnumericlimited(f, dptr, 0, 255);
}


static void
dialog__setnumeric(dbox_field f, DIALOG *dptr)
{
    int num = dptr->option;

    tracef2("dialog__setnumeric(%d, %d)\n", f, num);
    assert((dptr->type==F_NUMBER) || (dptr->type==F_COLOUR) || (dptr->type==F_CHAR) || (dptr->type==F_LIST));

    dbox_setnumeric(dialog__dbox, f, num);
}


static void
dialog__bumpnumericlimited(dbox_field valuefield, DIALOG *dptr,
                           dbox_field hit, intl minval, intl maxval)
{
    intl delta = depressed_shift() ? 5 : 1;
    intl range = maxval - minval + 1;           /* ie 0..255 is 256 */
    intl num;

    /* one is allowed to bump a composite textfield as a number */
    assert((dptr->type==F_NUMBER) || (dptr->type==F_COLOUR) || (dptr->type==F_CHAR) || (dptr->type==F_LIST) || (dptr->type=F_COMPOSITE));

    /* read current value back */
    dialog__getnumericlimited(valuefield, dptr, minval, maxval);

    /* always inc,dec,value */
    if(hit+2 == valuefield)
        {
        num = dptr->option + delta;
        if(num > maxval)
            num -= range;
        }
    else
        {
        num = dptr->option - delta;
        if(num < minval)
            num += range;
        }

    dptr->option = num;

    dialog__setnumeric(valuefield, dptr);
}


/* Normal numeric fields range from 0x00..0xFF */

static void
dialog__bumpnumeric(dbox_field valuefield, DIALOG *dptr, dbox_field hit)
{
    dialog__bumpnumericlimited(valuefield, dptr, hit, 0, 255);
}


/************************************
*                                   *
* get/set a text option             *
* for F_SPECIAL type entries        *
*                                   *
* --in--                            *
*   dptr->option is char to set     *
*                                   *
* --out--                           *
*   dptr->option = char that is set *
*                                   *
************************************/

static const char *
dialog__getspecial(dbox_field f, DIALOG *dptr)
{
    dbox d = dialog__dbox;
    char tempstring[2];
    const char *optptr;
    char ch;

    dbox_getfield(d, f, tempstring, sizeof(tempstring));

    ch = toupper(tempstring[0]);

    optptr = (ch == '\0') ? NULL : (const char *) strchr(dptr->optionlist, ch);

    if(!optptr)
        {
        optptr = dptr->optionlist;
        dptr->option = *optptr; /* ensure sensible */
        }
    else
        dptr->option = ch;

    tracef3("dialog__getspecial(%d) returns option '%c' optptr &%p\n",
                f, dptr->option, optptr);
    assert(dptr->type == F_SPECIAL);

    return(optptr);
}


static void
dialog__setspecial(dbox_field f, const DIALOG *dptr)
{
    char tempstring[2];
    assert(dptr->type == F_SPECIAL);

    tempstring[0] = (char) dptr->option;
    tempstring[1] = '\0';

    dialog__setfield_str(f, tempstring);
}


static void
dialog__bumpspecial(dbox_field valuefield, DIALOG *dptr, dbox_field hit)
{
    /* Read current value */
    const char *optptr = dialog__getspecial(valuefield, dptr);

    /* always inc,dec,value */
    if(hit+2 == valuefield)
        {
        if(*++optptr == '\0')
            optptr = dptr->optionlist;
        }
    else
        {
        if(optptr-- == dptr->optionlist)
            optptr = dptr->optionlist + strlen(dptr->optionlist) - 1;
        }

    dptr->option = *optptr;

    dialog__setspecial(valuefield, dptr);
}


/************************************
*                                   *
* get/set text fields               *
* for F_ARRAY type entries          *
* option is index into optionlist[] *
*                                   *
************************************/

static void
dialog__getarray(dbox_field f, DIALOG *dptr)
{
    const char **array = (const char **) dptr->optionlist;
    const char **ptr   = array;
    char tempstring[256];
    int res = 0;

    dbox_getfield(dialog__dbox, f, tempstring, sizeof(tempstring));
    tracef1("dialog__getarray got \"%s\"\n", tempstring);
    assert(dptr->type == F_ARRAY);

    do  {
        tracef1("comparing with \"%s\"\n", *ptr);
        if(!stricmp(*ptr, tempstring))
            {
            res = ptr - array;
            break;
            }
        }
    while(*++ptr);

    tracef3("dialog__getarray(%d, &%8.8X) yields %d\n",
            f, array, res);
    dptr->option = (char) res;
}


static void
dialog__setarray(dbox_field f, const DIALOG *dptr)
{
    const char **array = (const char **) dptr->optionlist;

    tracef3("dialog__setarray(%d, &%p) option is %d --- ",f,dptr,dptr->option);
    assert(dptr->type == F_ARRAY);

    dialog__setfield_str(f, array[dptr->option]);
}


static intl
dialog__lastarrayopt(const char **array)
{
    const char **ptr = array - 1;

    do { /* nothing */; } while(*++ptr);

    tracef1("last array element is array[%d]\n", ptr - array - 1);
    return(ptr - array - 1);
}


static void
dialog__bumparray(dbox_field valuefield, DIALOG *dptr, dbox_field hit)
{
    const char **array = (const char **) dptr->optionlist;

    assert(dptr->type == F_ARRAY);

    /* always inc,dec,value */
    if(hit+2 == valuefield)
        {
        if(dptr->option++ == dialog__lastarrayopt(array))
            dptr->option = 0;
        }
    else
        {
        if(dptr->option-- == 0)
            dptr->option = dialog__lastarrayopt(array);
        }

    dialog__setfield_str(valuefield, array[dptr->option]);
}


/********************************
*                               *
* get/set on/off fields         *
* for F_SPECIAL type entries    *
* NB. the optionlist is ignored *
*                               *
* --in--                        *
*   'N' to clear, otherwise set *
*                               *
* --out--                       *
*   'N' if clear, 'Y' otherwise *
*                               *
********************************/

static void
dialog__getonoff(dbox_field f, DIALOG *dptr)
{
    int option = (dbox_getnumeric(dialog__dbox, f) == 0) ? 'N' : 'Y';

    tracef2("dialog__getonoff(%d) yields '%c'\n", f, option);
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(1 == strlen(dptr->optionlist) - 1);

    dptr->option = (char) option;
}


static void
dialog__setonoff(dbox_field f, const DIALOG *dptr)
{
    tracef2("dialog__setonoff(%d, '%c')\n", f, dptr->option);
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(1 == strlen(dptr->optionlist) - 1);

    /* only 'N' sets false */
    dbox_setnumeric(dialog__dbox, f, dptr->option ^ 'N');
}


/********************************************
*                                           *
* get/set a group of radio buttons          *
* for F_SPECIAL type entries                *
*                                           *
* --in--                                    *
*   dptr->option == button to set           *
*                                           *
* --out--                                   *
*   dptr->option == button that is set      *
*                   first if none set       *
*                                           *
********************************************/

static dbox_field
dialog__whichradio(dbox_field start, dbox_field end)
{
    dbox d = dialog__dbox;
    dbox_field f = end + 1;

    do { f--; } while((f > start)  &&  (dbox_getnumeric(d, f) == 0));
    /* if none set, gets first */

    tracef3("dialog__whichradio(%d, %d) returns field %d\n", start, end, f);

    return(f);
}


static void
dialog__getradio(dbox_field start, dbox_field end, DIALOG *dptr)
{
    dbox_field f = dialog__whichradio(start, end);

    dptr->option = (dptr->optionlist)[f - start];

    tracef5("dialog__getradio(%d, %d, \"%s\") returns field %d, option '%c'\n",
            start, end, dptr->optionlist, f, dptr->option); 
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(end - start == strlen(dptr->optionlist) - 1);
}


static void
dialog__setradio(dbox_field start, dbox_field end, const DIALOG *dptr)
{
    dbox d = dialog__dbox;
    dbox_field f    = start;
    dbox_field this = start + (dbox_field) (strchr(dptr->optionlist, dptr->option) - (char *) (dptr->optionlist));

    tracef5("dialog__setradio(%d, %d, \"%s\", '%c'), index %d\n",
            start, end, dptr->optionlist, dptr->option, this);
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(end - start == strlen(dptr->optionlist) - 1);

    do { dbox_setnumeric(d, f, f == this); } while(++f <= end);
}


/****************************************
*                                       *
* get/set on/off field & text value     *
* for F_COMPOSITE with Y/N type entries *
*                                       *
****************************************/

static void
dialog__getcomponoff(dbox_field f, DIALOG *dptr)
{
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    dialog__getonoff(f,     dptr);
    dialog__getfield(f+1,   dptr);
}


static void
dialog__setcomponoff(dbox_field f, const DIALOG *dptr)
{
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    dialog__setonoff(f,     dptr);
    dialog__setfield(f+1,   dptr);
}


/****************************************************************************
*                                                                           *
*                               dialog boxes                                *
*                                                                           *
****************************************************************************/

/********************************
*                               *
*  single text value dialog box *
*                               *
********************************/

#define onetext_Name        1

extern void
dproc_onetext(DIALOG *dptr)
{
    assert_dialog(0, D_SAVEINIT);
    assert_dialog(0, D_NAME);
    assert_dialog(0, D_EXECFILE);
    assert_dialog(0, D_GOTO);
    #if !defined(SPELL_OFF)
    assert_dialog(0, D_USER_CREATE);
    assert_dialog(0, D_USER_OPEN);
    assert_dialog(0, D_USER_CLOSE);
    #endif

    dialog__setfield(onetext_Name,  &dptr[0]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getfield(onetext_Name,  &dptr[0]);
}


/********************************
*                               *
*  two text values dialog box   *
*                               *
********************************/

#define twotext_First       1
#define twotext_Second      2

extern void
dproc_twotext(DIALOG *dptr)
{
    assert_dialog(1, D_REPLICATE);
    #if !defined(SPELL_OFF)
    assert_dialog(1, D_USER_DELETE);
    assert_dialog(1, D_USER_INSERT);
    assert_dialog(1, D_USER_MERGE);
    assert_dialog(1, D_USER_PACK);
    #endif
    assert_dialog(1, D_DEF_CMD);

    dialog__setfield(twotext_First,     &dptr[0]);
    dialog__setfield(twotext_Second,    &dptr[1]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getfield(twotext_First,     &dptr[0]);
    dialog__getfield(twotext_Second,    &dptr[1]);
}


/****************************************
*                                       *
* single numeric value (+/-) dialog box *
*                                       *
****************************************/

#define onenumeric_Inc      1
#define onenumeric_Dec      2
#define onenumeric_Value    3

extern void
dproc_onenumeric(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(0, D_INSPAGE);
    assert_dialog(0, D_DELETED);

    dialog__setnumeric(onenumeric_Value,    &dptr[0]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,   onenumeric_Value))
            dialog__bumpnumeric(onenumeric_Value,   &dptr[0], f);
        else
            tracef1("unprocessed onenumeric action %d\n", f);
        }

    dialog__getnumeric(onenumeric_Value,    &dptr[0]);
}


/****************************************
*                                       *
* single special value (+/-) dialog box *
*                                       *
****************************************/

#define onespecial_Inc      1
#define onespecial_Dec      2
#define onespecial_Value    3

extern void
dproc_onespecial(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(0, D_INSHIGH);
    assert_dialog(0, D_REMHIGH);

    dialog__setspecial(onespecial_Value,    &dptr[0]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,   onespecial_Value))
            dialog__bumpspecial(onespecial_Value,   &dptr[0], f);
        else
            tracef1("unprocessed onespecial action %d\n", f);
        }

    dialog__getspecial(onespecial_Value,    &dptr[0]);
}


/********************************************
*                                           *
*  number (+/-) and text value dialog box   *
*                                           *
********************************************/

#define numtext_Inc         1
#define numtext_Dec         2
#define numtext_Number      3
#define numtext_Text        4

extern void
dproc_numtext(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_WIDTH);
    assert_dialog(1, D_MARGIN);

    dialog__setnumeric(numtext_Number,  &dptr[0]);
    dialog__setfield(numtext_Text,      &dptr[1]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,   numtext_Number))
            dialog__bumpnumeric(numtext_Number, &dptr[0], f);
        else
            tracef1("unprocessed numtext action %d\n", f);
        }

    dialog__getnumeric(numtext_Number,  &dptr[0]);
    dialog__getfield(numtext_Text,      &dptr[1]);
}


/****************************************
*                                       *
*  single on/off composite dialog box   *
*                                       *
****************************************/

#define onecomponoff_OnOff  1
#define onecomponoff_Text   2

extern void
dproc_onecomponoff(DIALOG *dptr)
{
    #if !defined(SPELL_OFF)
    assert_dialog(0, D_USER_LOCK);
    assert_dialog(0, D_USER_UNLOCK);
    #endif

    dialog__setcomponoff(onecomponoff_OnOff,    &dptr[0]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getcomponoff(onecomponoff_OnOff,    &dptr[0]);
}


#define list_Inc        1
#define list_Dec        2
#define list_Value      3
#define list_Definition 4

extern void
dproc_list(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_PARAMETER);
    assert_dialog(1, D_DEFKEY);

    dialog__setnumeric(list_Value,      &dptr[0]);
    dialog__setfield(list_Definition,   &dptr[1]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,   list_Value))
            dialog__bumpnumeric(list_Value, &dptr[0], f);
        else
            tracef1("unprocessed list action %d\n", f);
        }

    dialog__getnumeric(list_Value,      &dptr[0]);
    dialog__getfield(list_Definition,   &dptr[1]);
}


/****************************************************
*                                                   *
* generic yes/no/cancel dialog box                  *
*                                                   *
* NB. can't be called directly as it needs a string *
*                                                   *
****************************************************/

static void
dproc__query(DIALOG *dptr, const char *mess)
{
    intl res = riscdialog_query(mess);

    switch(res)
        {
        case riscdialogquery_YES:
            dptr[0].option = 'Y';
            break;

        case riscdialogquery_NO:
            dptr[0].option = 'N';
            break;

        case riscdialogquery_CANCEL:
            break;
        }
}


extern intl
currentfiletype(optiontype filetype)
{
    return( (filetype != 'T')
                ? rft_from_option(filetype)
                : ((initial_filetype == 'T')
                        ? ((currentfileinfo.load >> 8) & 0xFFF)
                        : TEXT_FILETYPE)
            );
}


/************************
*                       *
* about file dialog box *
*                       *
************************/

#define aboutfile_Icon      0
#define aboutfile_Modified  1
#define aboutfile_Type      2
#define aboutfile_Name      3
#define aboutfile_Date      4
#define aboutfile_BytesFree 5

extern void
dproc_aboutfile(DIALOG *dptr)
{
    char tempstring[256];
    intl filetype = currentfiletype(d_save_FORMAT);

    IGNOREPARM(dptr);

    dialog__setfield_str(aboutfile_Name,
                        riscos_obtainfilename(currentfilename, FALSE));


    dialog__setfield_str(aboutfile_Modified,
                        xf_filealtered ? YES_STR : NO_STR);


    os_swi4r(XOS_MASK | OS_FSControl, 18, 0, filetype, 0,
             NULL, NULL, (int *) &tempstring[0], (int *) &tempstring[4]);
    tempstring[8] = '\0';
    sprintf(&tempstring[12], filetype_expand_Zs_ZX_STR, &tempstring[0], filetype);

    dialog__setfield_str(aboutfile_Type, &tempstring[12]);


    if(wimpt_complain(os_swi3(XOS_MASK | OS_ConvertStandardDateAndTime,
        (int) &currentfileinfo, (int) tempstring, sizeof(tempstring))) )
        tempstring[sizeof(tempstring)-1] = '\0';    /* Ensure terminated */

    dialog__setfield_str(aboutfile_Date, tempstring);


    /* make appropriate file icon in dbox */
    fileicon(dbox_syshandle(dialog__dbox), aboutfile_Icon, filetype);


    sprintf(tempstring, memory_size_Zd_STR,
    (flex_extrastore() + flex_storefree() + list_howmuchpoolspace())/ 1024);
    dialog__setfield_str(aboutfile_BytesFree, tempstring);

    dialog__simple_fillin();
}


/****************************
*                           *
* about program dialog box  *
*                           *
****************************/

#define aboutprog_Name      0
#define aboutprog_Author    1
#define aboutprog_Version   2
#define aboutprog_User      3
#define aboutprog_RegNo     4

extern void
dproc_aboutprog(DIALOG *dptr)
{
    #if RELEASED || defined(DEMO)
    char nambuf[LIN_BUFSIZ];
    #endif

    IGNOREPARM(dptr);

    #if FALSE
    /* These are already set in the progInfo dbox template */
    dialog__setfield_str(aboutprog_Name,    applicationname);
    dialog__setfield_str(aboutprog_Author,  "© 1989, Colton Software");
    #endif

    #if defined(DEMO)
    strcpy(nambuf, applicationversion);
    strcat(nambuf, " Demonstration");
    dialog__setfield_str(aboutprog_Version, nambuf);
    #else
    dialog__setfield_str(aboutprog_Version, applicationversion);
    #endif

    #if RELEASED
    read_username(nambuf);
    dialog__setfield_str(aboutprog_User,    nambuf);
    dialog__setfield_str(aboutprog_RegNo,   registration_number);
    #else
    dialog__setfield_str(aboutprog_User,    "Colton Software - Development");
    dialog__setfield_str(aboutprog_RegNo,   "R0123 4567 8901 2345");
    #endif

    dialog__simple_fillin();
}


/************
*           *
* sort dbox *
*           *
************/

#define sort_FirstColumn        1       /* pairs of on/off(ascending) & text(column) */
#define sort_UpdateReferences   sort_FirstColumn + 2*SORT_FIELD_DEPTH + 0
#define sort_MultiRowRecords    sort_FirstColumn + 2*SORT_FIELD_DEPTH + 1

extern void
dproc_sort(DIALOG *dptr)
{
    intl i;

    assert_dialog(SORT_MULTI_ROW, D_SORT);

    i = 0;
    do  {
        dialog__setfield(2*i + sort_FirstColumn + 1,    &dptr[2*i + SORT_FIELD_COLUMN]);
        dialog__setonoff(2*i + sort_FirstColumn + 0,    &dptr[2*i + SORT_FIELD_ASCENDING]);
        }
    while(++i < SORT_FIELD_DEPTH);

    dialog__setonoff(sort_UpdateReferences, &dptr[SORT_UPDATE_REFS]);
    dialog__setonoff(sort_MultiRowRecords,  &dptr[SORT_MULTI_ROW]);

    dialog__simple_fillin();

    if(!ok)
        return;

    i = 0; 
    do  {
        dialog__getfield(2*i + sort_FirstColumn + 1,    &dptr[2*i + SORT_FIELD_COLUMN]);
        dialog__getonoff(2*i + sort_FirstColumn + 0,    &dptr[2*i + SORT_FIELD_ASCENDING]);
        }
    while(++i < SORT_FIELD_DEPTH);

    dialog__getonoff(sort_UpdateReferences, &dptr[SORT_UPDATE_REFS]);
    dialog__getonoff(sort_MultiRowRecords,  &dptr[SORT_MULTI_ROW]);
}


/****************************
*                           *
*  create linking file dbox *
*                           *
****************************/

#define createlinking_FInc  1
#define createlinking_FDec  2
#define createlinking_File  3
#define createlinking_CInc  4
#define createlinking_CDec  5
#define createlinking_Cols  6
#define createlinking_RInc  7
#define createlinking_RDec  8
#define createlinking_Rows  9

extern void
dproc_createlinking(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(2, D_CREATE);

    dialog__setnumeric(createlinking_File,  &dptr[0]);
    dialog__setnumeric(createlinking_Cols,  &dptr[1]);
    dialog__setnumeric(createlinking_Rows,  &dptr[2]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
          if(dialog__adjust(&f, createlinking_File))
            dialog__bumpnumeric(    createlinking_File, &dptr[0], f);
        elif(dialog__adjust(&f, createlinking_Cols))
            dialog__bumpnumeric(    createlinking_Cols, &dptr[1], f);
        elif(dialog__adjust(&f, createlinking_Rows))
            dialog__bumpnumeric(    createlinking_Rows, &dptr[2], f);
        else
            tracef1("unprocessed createlinking action %d\n", f);
        }

    dialog__getnumeric(createlinking_File,  &dptr[0]);
    dialog__getnumeric(createlinking_Cols,  &dptr[1]);
    dialog__getnumeric(createlinking_Rows,  &dptr[2]);
}


/********************
*                   *
*  load file dbox   *
*                   *
********************/

typedef enum
{
    loadfile_Name = 1,
    loadfile_Slot,
    loadfile_SlotSpec,
    loadfile_RowRange,
    loadfile_RowRangeSpec,
    loadfile_Auto,
    loadfile_PipeDream,
    loadfile_VIEW,
    loadfile_CSV,
    loadfile_Tab,
    loadfile_ViewSheet,
    loadfile_1WP

} loadfile_offsets;

extern void
dproc_loadfile(DIALOG *dptr)
{
    assert_dialog(3, D_LOAD);

    dialog__setfield(loadfile_Name,                 &dptr[0]);
    dialog__setcomponoff(loadfile_Slot,             &dptr[1]);
    dialog__setcomponoff(loadfile_RowRange,         &dptr[2]);
    dialog__setradio(loadfile_Auto, loadfile_1WP,   &dptr[3]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getfield(loadfile_Name,                 &dptr[0]);
    dialog__getcomponoff(loadfile_Slot,             &dptr[1]);
    dialog__getcomponoff(loadfile_RowRange,         &dptr[2]);
    dialog__getradio(loadfile_Auto, loadfile_1WP,   &dptr[3]);
}


/********************
*                   *
*  save file dbox   *
*                   *
********************/

#define savefile_Name               1       /* == xfersend_FName */
#define savefile_Icon               2       /* == xfersend_FIcon */
#define savefile_ColRange           3
#define savefile_ColRangeSpec       4
#define savefile_RowSelection       5
#define savefile_RowSelectionSpec   6
#define savefile_MarkedBlock        7
#define savefile_SeparatorInc       8
#define savefile_SeparatorDec       9
#define savefile_Separator          10
#define savefile_PipeDream          11
#define savefile_VIEW               12
#define savefile_CSV                13
#define savefile_Tab                14
#define savefile_DTP                15

#if !defined(SAVE_OFF)

/* xfersend calling us to handle clicks on the dbox fields */

static void
savefile_clickproc(dbox d, dbox_field f, int *filetypep, void *handle)
{
    DIALOG *dptr;

    IGNOREPARM(d);

    tracef3("savefile_clickproc(%d, &%p, &%p)\n", f, filetypep, handle);

    select_document_using_handle((dochandle) handle);

    /* now data^ valid */
    dptr = d_save;

    if(dialog__adjust(&f, savefile_Separator))
        dialog__bumparray(savefile_Separator, &dptr[SAV_LINESEP], f);
    else
        {
        dbox_field oldf;

        switch(f)
            {
            case savefile_PipeDream:
            case savefile_VIEW:
            case savefile_CSV:
            case savefile_Tab:
            case savefile_DTP:
                oldf = dialog__whichradio(savefile_PipeDream, savefile_DTP);
                if(f != oldf)
                    {
                    /* manually process radio buttons */
                    dbox_setnumeric(d, oldf, FALSE);
                    dbox_setnumeric(d,    f,  TRUE);
                    *filetypep = currentfiletype((dptr[SAV_xxxFORMAT].optionlist)[f - savefile_PipeDream]);
                    }
                break;

            default:
                tracef1("unprocessed savefile_clickproc action %d\n", f);
                break;
            }
        }
}


/* xfersend calling us to save a file */

static BOOL
savefile_saveproc(const char *filename /*low lifetime*/, void *handle)
{
    char buffer[216];
    DIALOG *dptr;
    wimp_eventstr *e;
    wimp_mousestr ms;
    BOOL res = TRUE;

    tracef2("savefile_saveproc(%s, %d)\n", filename, handle);

    select_document_using_handle((dochandle) handle);

    strcpy(buffer, filename);

    /* now data^ valid */
    dptr = d_save;

    dialog__getfield(savefile_Name,                     &dptr[SAV_NAME]);
    dialog__getcomponoff(savefile_ColRange,             &dptr[SAV_COLRANGE]);
    dialog__getcomponoff(savefile_RowSelection,         &dptr[SAV_ROWCOND]);
    dialog__getonoff(savefile_MarkedBlock,              &dptr[SAV_BLOCK]);
    dialog__getarray(savefile_Separator,                &dptr[SAV_LINESEP]);
    dialog__getradio(savefile_PipeDream, savefile_DTP,  &dptr[SAV_xxxFORMAT]);

    /* try to look for wally saves to the same window */
    e = wimpt_last_event();

    tracef1("last wimp event was %s\n", trace_wimp_event(e));

    if(e->e != wimp_EKEY)
        {
        wimpt_safe(wimp_get_point_info(&ms));

        tracef4("last mouse event at %d %d, window %d, icon %d\n",
                ms.x, ms.y, ms.w, ms.i);

        if(ms.w == fake__window)
            if(!saving_part_file())
                /* could do more checking, eg. curpos not in marked block for save */
                res = FALSE;
        }

    if(res)
        res = excsav(buffer, dptr[SAV_xxxFORMAT].option, FALSE);
        /* not init file */
    else
        res = reperr_null(ERR_CANTSAVETOITSELF);

    return(res);
}


/* xfersend calling us to print a file */

static BOOL
savefile_printproc(const char *filename, void *handle)
{
    BOOL res;

    tracef2("savefile_saveproc(%s, %d)\n", filename, handle);

    select_document_using_handle((dochandle) handle);

    force_calshe();

    reset_print_options();

    print_document();

    res = been_error ? xfersend_printFailed : xfersend_printPrinted;

    been_error = FALSE;

    return(res);
}

#endif  /* SAVE_OFF */


extern void
dproc_savefile(DIALOG *dptr)
{
    #if defined(SAVE_OFF)
    ok = FALSE;
    #else

    dochandle doc = current_document_handle();
    const char *filename = riscos_obtainfilename(dptr[SAV_NAME].textfield, FALSE);
    intl filetype = currentfiletype(dptr[SAV_xxxFORMAT].option);
    intl estsize  = 42;

    assert_dialog(SAV_xxxFORMAT, D_SAVE);

    dialog__setfield_str(savefile_Name,                 filename);
    dialog__setcomponoff(savefile_ColRange,             &dptr[SAV_COLRANGE]);
    dialog__setcomponoff(savefile_RowSelection,         &dptr[SAV_ROWCOND]);
    dialog__setonoff(savefile_MarkedBlock,              &dptr[SAV_BLOCK]);
    dialog__setarray(savefile_Separator,                &dptr[SAV_LINESEP]);
    dialog__setradio(savefile_PipeDream, savefile_DTP,  &dptr[SAV_xxxFORMAT]);

    dialog__register_help_for(dialog__dbox);

    dbox_show(dialog__dbox);

    /* all the action takes place in here */
    ok = xfersend(filetype, filename, estsize,
                  savefile_saveproc, (void *) doc,  /* save file */
                  NULL,                             /* send file - RAM transmit */
                  savefile_printproc,               /* print file */
                  dialog__dbox,
                  savefile_clickproc, (void *) doc);

    select_document_using_handle(doc);

    if(ok)
        {
        dialog__getfield(savefile_Name,                     &dptr[SAV_NAME]);
        dialog__getcomponoff(savefile_ColRange,             &dptr[SAV_COLRANGE]);
        dialog__getcomponoff(savefile_RowSelection,         &dptr[SAV_ROWCOND]);
        dialog__getonoff(savefile_MarkedBlock,              &dptr[SAV_BLOCK]);
        dialog__getarray(savefile_Separator,                &dptr[SAV_LINESEP]);
        dialog__getradio(savefile_PipeDream, savefile_DTP,  &dptr[SAV_xxxFORMAT]);
        }

    #endif  /* SAVE_OFF */
}


extern void
dproc_save_existing(DIALOG *dptr)
{
    intl res;

    IGNOREPARM(dptr);

    assert_dialog(1, D_SAVE_EXISTING);

    res = riscdialog_save_existing();

    ok = (res != riscdialogquery_CANCEL);

    /* all actions been and gone in the above proc */
    d_save_existing[0].option = 'N';
}


/****************************
*                           *
* overwrite file dialog box *
*                           *
****************************/

extern void
dproc_overwrite(DIALOG *dptr)
{
    assert_dialog(0, D_OVERWRITE);

    dproc__query(dptr, Overwrite_existing_file_STR);
}


/************************************************
*                                               *
*  continue deletion on store fail dialog box   *
*                                               *
************************************************/

extern void
dproc_save_deleted(DIALOG *dptr)
{
    assert_dialog(0, D_SAVE_DELETED);

    dproc__query(dptr, Cannot_store_block_STR);
}


/************************
*                       *
*  colours dialog box   *
*                       *
************************/

#define colours_Inc     1
#define colours_Dec     2
#define colours_Number  3
#define colours_Patch   4

extern void
dproc_colours(DIALOG *dptr)
{
    dbox_field f;
    intl i;
    DIALOG temp_dialog[N_COLOURS];      /* update temp copy! */

    assert_dialog(N_COLOURS - 1, D_COLOURS);

    for(i = 0; i < N_COLOURS; i++)
        {
        dialog__setnumeric( 4*i + colours_Number,   &dptr[i]);
        dialog__setcolour(  4*i + colours_Patch,    &dptr[i]);
        temp_dialog[i] = dptr[i];
        }

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        for(i = 0; i < N_COLOURS; i++)
            if(dialog__adjust(&f, 4*i + colours_Number))
                {
                dialog__bumpnumericlimited(4*i + colours_Number,
                                                        &temp_dialog[i], f, 0x00, 0x0F);
                dialog__setcolour(4*i + colours_Patch,  &temp_dialog[i]);
                }
        }

    /* only update colour globals if ok */
    if(ok)
        for(i = 0; i < N_COLOURS; i++)
            dialog__getnumericlimited(4*i + colours_Number, &dptr[i], 0x00, 0x0F);
}


/************************
*                       *
*  options dialog box   *
*                       *
************************/

#define options_Title           1
#define options_SlotText        2
#define options_SlotNumbers     3
#define options_InsWrapRow      4
#define options_InsWrapCol      5
#define options_Borders         6
#define options_Justify         7
#define options_Wrap            8
#define options_DecimalInc      9
#define options_DecimalDec      10
#define options_Decimal         11
#define options_NegMinus        12
#define options_NegBrackets     13
#define options_ThousandsInc    14
#define options_ThousandsDec    15
#define options_Thousands       16
#define options_InsertOnReturn  17
#define options_DateEnglish     18
#define options_DateAmerican    19
#define options_DateText        20
#define options_LeadingChars    21
#define options_TrailingChars   22
#define options_Grid            23

extern void
dproc_options(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(13, D_OPTIONS);

    dialog__setfield_high(options_Title,                        &dptr[0]);
    dialog__setradio(options_SlotText, options_SlotNumbers,     &dptr[1]);
    dialog__setradio(options_InsWrapRow, options_InsWrapCol,    &dptr[2]);
    dialog__setonoff(options_Borders,                           &dptr[3]);
    dialog__setonoff(options_Justify,                           &dptr[4]);
    dialog__setonoff(options_Wrap,                              &dptr[5]);
    dialog__setspecial(options_Decimal,                         &dptr[6]);
    dialog__setradio(options_NegMinus, options_NegBrackets,     &dptr[7]);
    dialog__setarray(options_Thousands,                         &dptr[8]);
    dialog__setonoff(options_InsertOnReturn,                    &dptr[9]);
    dialog__setradio(options_DateEnglish, options_DateText,     &dptr[10]);
    dialog__setfield_high(options_LeadingChars,                 &dptr[11]);
    dialog__setfield_high(options_TrailingChars,                &dptr[12]);
    dialog__setonoff(options_Grid,                              &dptr[13]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
          if(dialog__adjust(&f, options_Decimal))
            dialog__bumpspecial(options_Decimal,    &dptr[6], f);
        elif(dialog__adjust(&f, options_Thousands))
            dialog__bumparray(options_Thousands,    &dptr[8], f);
        else
            tracef1("unprocessed options action %d\n", f);
        }

    dialog__getfield_high(options_Title,                        &dptr[0]);
    dialog__getradio(options_SlotText, options_SlotNumbers,     &dptr[1]);
    dialog__getradio(options_InsWrapRow, options_InsWrapCol,    &dptr[2]);
    dialog__getonoff(options_Borders,                           &dptr[3]);
    dialog__getonoff(options_Justify,                           &dptr[4]);
    dialog__getonoff(options_Wrap,                              &dptr[5]);
    dialog__getspecial(options_Decimal,                         &dptr[6]);
    dialog__getradio(options_NegMinus, options_NegBrackets,     &dptr[7]);
    dialog__getarray(options_Thousands,                         &dptr[8]);
    dialog__getonoff(options_InsertOnReturn,                    &dptr[9]);
    dialog__getradio(options_DateEnglish, options_DateText,     &dptr[10]);
    dialog__getfield_high(options_LeadingChars,                 &dptr[11]);
    dialog__getfield_high(options_TrailingChars,                &dptr[12]);
    dialog__getonoff(options_Grid,                              &dptr[13]);
}


/****************************
*                           *
*  page layout dialog box   *
*                           *
****************************/

#define pagelayout_LengthInc    1
#define pagelayout_LengthDec    2
#define pagelayout_Length       3
#define pagelayout_SpacingInc   4
#define pagelayout_SpacingDec   5
#define pagelayout_Spacing      6
#define pagelayout_StartPage    7
#define pagelayout_TopInc       8
#define pagelayout_TopDec       9
#define pagelayout_Top          10
#define pagelayout_HeaderInc    11
#define pagelayout_HeaderDec    12
#define pagelayout_Header       13
#define pagelayout_FooterInc    14
#define pagelayout_FooterDec    15
#define pagelayout_Footer       16
#define pagelayout_BottomInc    17
#define pagelayout_BottomDec    18
#define pagelayout_Bottom       19
#define pagelayout_LeftInc      20
#define pagelayout_LeftDec      21
#define pagelayout_Left         22
#define pagelayout_HeaderText   23
#define pagelayout_FooterText   24

extern void
dproc_pagelayout(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(9, D_POPTIONS);

    dialog__setnumeric(pagelayout_Length,           &dptr[0]);
    dialog__setnumeric(pagelayout_Spacing,          &dptr[1]);
    dialog__setfield(pagelayout_StartPage,          &dptr[2]);
    dialog__setnumeric(pagelayout_Top,              &dptr[3]);
    dialog__setnumeric(pagelayout_Header,           &dptr[4]);
    dialog__setnumeric(pagelayout_Footer,           &dptr[5]);
    dialog__setnumeric(pagelayout_Bottom,           &dptr[6]);
    dialog__setnumeric(pagelayout_Left,             &dptr[7]);
    dialog__setfield_high(pagelayout_HeaderText,    &dptr[8]);
    dialog__setfield_high(pagelayout_FooterText,    &dptr[9]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
          if(dialog__adjust(&f, pagelayout_Length))
            dialog__bumpnumeric(pagelayout_Length,  &dptr[0], f);
        elif(dialog__adjust(&f, pagelayout_Spacing))
            dialog__bumpnumeric(pagelayout_Spacing, &dptr[1], f);
        elif(dialog__adjust(&f, pagelayout_Top))
            dialog__bumpnumeric(pagelayout_Top,     &dptr[3], f);
        elif(dialog__adjust(&f, pagelayout_Header))
            dialog__bumpnumeric(pagelayout_Header,  &dptr[4], f);
        elif(dialog__adjust(&f, pagelayout_Footer))
            dialog__bumpnumeric(pagelayout_Footer,  &dptr[5], f);
        elif(dialog__adjust(&f, pagelayout_Bottom))
            dialog__bumpnumeric(pagelayout_Bottom,  &dptr[6], f);
        elif(dialog__adjust(&f, pagelayout_Left))
            dialog__bumpnumeric(pagelayout_Left,    &dptr[7], f);
        else
            tracef1("unprocessed pagelayout action %d\n", f);
        }

    dialog__getnumeric(pagelayout_Length,           &dptr[0]);
    dialog__getnumeric(pagelayout_Spacing,          &dptr[1]);
    dialog__getfield(pagelayout_StartPage,          &dptr[2]);
    dialog__getnumeric(pagelayout_Top,              &dptr[3]);
    dialog__getnumeric(pagelayout_Header,           &dptr[4]);
    dialog__getnumeric(pagelayout_Footer,           &dptr[5]);
    dialog__getnumeric(pagelayout_Bottom,           &dptr[6]);
    dialog__getnumeric(pagelayout_Left,             &dptr[7]);
    dialog__getfield_high(pagelayout_HeaderText,    &dptr[8]);
    dialog__getfield_high(pagelayout_FooterText,    &dptr[9]);
}


/****************************
*                           *
*  recalculate dialog box   *
*                           *
****************************/

#define recalc_Auto             1
#define recalc_Manual           2
#define recalc_Columns          3
#define recalc_Rows             4
#define recalc_Natural          5
#define recalc_Iteration        6
#define recalc_IterationNumber  7
#define recalc_IterationChange  8

extern void
dproc_recalc(DIALOG *dptr)
{
    assert_dialog(4, D_RECALC);

    dialog__setradio(recalc_Auto, recalc_Manual,        &dptr[0]);
    dialog__setradio(recalc_Columns, recalc_Natural,    &dptr[1]);
    dialog__setonoff(recalc_Iteration,                  &dptr[2]);
    dialog__setfield(recalc_IterationNumber,            &dptr[3]);
    dialog__setfield(recalc_IterationChange,            &dptr[4]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getradio(recalc_Auto, recalc_Manual,        &dptr[0]);
    dialog__getradio(recalc_Columns, recalc_Natural,    &dptr[1]);
    dialog__getonoff(recalc_Iteration,                  &dptr[2]);
    dialog__getfield(recalc_IterationNumber,            &dptr[3]);
    dialog__getfield(recalc_IterationChange,            &dptr[4]);
}


/********************************
*                               *
*  decimal places dialog box    *
*                               *
********************************/

#define decimal_Inc     1
#define decimal_Dec     2
#define decimal_Value   3

extern void
dproc_decimal(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(0, D_DECIMAL);

    dialog__setspecial(decimal_Value,   &dptr[0]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
          if(dialog__adjust(&f, decimal_Value))
            dialog__bumpspecial(decimal_Value,  &dptr[0], f);
        else
            tracef1("unprocessed options action %d\n", f);
        }

    dialog__getspecial(decimal_Value,   &dptr[0]);
}


/************************
*                       *
* insert character dbox *
*                       *
************************/

#define insertchar_Inc      1
#define insertchar_Dec      2
#define insertchar_Number   3
#define insertchar_Char     4

extern void
dproc_insertchar(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(0, D_INSCHAR);

    dialog__setnumeric(insertchar_Number,   &dptr[0]);
    dialog__setchar(insertchar_Char,        &dptr[0]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f, insertchar_Number))
            dialog__bumpnumericlimited(insertchar_Number,   &dptr[0], f, 0x01, 0xFF);
        elif(f == insertchar_Char)
            dialog__getnumericlimited(insertchar_Number,    &dptr[0], 0x01, 0xFF);
        else
            tracef1("unprocessed insertchar action %d\n", f);

        dialog__setchar(insertchar_Char, &dptr[0]);
        }

    dialog__getnumericlimited(insertchar_Number,    &dptr[0], 0x01, 0xFF);
}


/********************
*                   *
*  define key dbox  *
*                   *
********************/

/* ordered this way to get caret into definition at start */
#define defkey_Definition   1
#define defkey_Inc          2
#define defkey_Dec          3
#define defkey_Number       4
#define defkey_Char         5

extern void
dproc_defkey(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_DEFKEY);

    dialog__setnumeric(defkey_Number,           &dptr[0]);
    dialog__setchar(defkey_Char,                &dptr[0]);
    dialog__setfield_high(defkey_Definition,    &dptr[1]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,   defkey_Number))
            {
            dialog__bumpnumeric(defkey_Number,  &dptr[0], f);
            dialog__setchar(defkey_Char,        &dptr[0]);
            }
        elif(f == defkey_Char)
            {
            dialog__getnumeric(defkey_Number,   &dptr[0]);
            dialog__setchar(defkey_Char,        &dptr[0]);
            }
        else
            tracef1("unprocessed defkey action %d\n", f);
        }

    dialog__getnumeric(defkey_Number,           &dptr[0]);
    dialog__getfield_high(defkey_Definition,    &dptr[1]);
}


/****************************
*                           *
* define function key dbox  *
*                           *
****************************/

#define deffnkey_Inc        1
#define deffnkey_Dec        2
#define deffnkey_Key        3
#define deffnkey_Definition 4

extern void
dproc_deffnkey(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_DEF_FKEY);

    dialog__setarray(deffnkey_Key,              &dptr[0]);
    dialog__setfield_high(deffnkey_Definition,  &dptr[1]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f, deffnkey_Key))
            dialog__bumparray(deffnkey_Key, &dptr[0], f);
        else
            tracef1("unprocessed deffnkey action %d\n", f);
        }

    dialog__getarray(deffnkey_Key,              &dptr[0]);
    dialog__getfield_high(deffnkey_Definition,  &dptr[1]);
}


/****************
*               *
*  print dbox   *
*               *
****************/

#define print_PrinterType       1
#define print_Copies            2
#define print_Portrait          3
#define print_Landscape         4
#define print_Printer           5
#define print_Screen            6
#define print_File              7
#define print_FileName          8
#define print_ParmFile          9
#define print_ParmFileName      10
#define print_OmitBlankFields   11
#define print_ColRange          12
#define print_ColRangeSpec      13
#define print_RowSelection      14
#define print_RowSelectionSpec  15
#define print_MarkedBlock       16
#define print_TwoSided          17
#define print_TwoSidedSpec      18
#define print_WaitBetween       19
#define print_ScaleInc          20
#define print_ScaleDec          21
#define print_Scale             22

extern void
dproc_print(DIALOG *dptr)
{
    #if defined(PRINT_OFF)
    ok = FALSE;
    #else

    print_infostr pinfo;
    char tempstring[256];
    const char *description;    
    dbox_field f;

    assert_dialog(P_SCALE, D_PRINT);

    dialog__setradio(print_Printer, print_File,         &dptr[P_PSF]);
    dialog__setfield(print_FileName,                    &dptr[P_PSF]);
    dialog__setcomponoff(print_ParmFile,                &dptr[P_PARMFILE]);
    dialog__setonoff(print_OmitBlankFields,             &dptr[P_OMITBLANK]);
    dialog__setcomponoff(print_ColRange,                &dptr[P_COLS]);
    dialog__setcomponoff(print_RowSelection,            &dptr[P_ROWS]);
    dialog__setonoff(print_MarkedBlock,                 &dptr[P_BLOCK]);
    dialog__setcomponoff(print_TwoSided,                &dptr[P_TWOSIDE]);
    dialog__setnumeric(print_Copies,                    &dptr[P_COPIES]);
    dialog__setonoff(print_WaitBetween,                 &dptr[P_WAIT]);
    dialog__setradio(print_Portrait, print_Landscape,   &dptr[P_ORIENT]);
    dialog__setnumeric(print_Scale,                     &dptr[P_SCALE]);

      if(d_driver[1].option != driver_riscos)
        description = applicationname;
    elif(print_info(&pinfo))
        description = No_RISC_OS_STR;
    else
        description = pinfo.description;

    sprintf(tempstring, Zs_printer_driver_STR, description);

    dialog__setfield_str(print_PrinterType,             tempstring);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,           print_Scale))
            dialog__bumpnumericlimited( print_Scale,    &dptr[P_SCALE], f, 1, 999);
        else
            tracef1("unprocessed print action %d\n", f);
        }

    if(!ok)
        return;

    dialog__getradio(print_Printer, print_File,         &dptr[P_PSF]);
    dialog__getfield(print_FileName,                    &dptr[P_PSF]);
    dialog__getcomponoff(print_ParmFile,                &dptr[P_PARMFILE]);
    dialog__getonoff(print_OmitBlankFields,             &dptr[P_OMITBLANK]);
    dialog__getcomponoff(print_ColRange,                &dptr[P_COLS]);
    dialog__getcomponoff(print_RowSelection,            &dptr[P_ROWS]);
    dialog__getonoff(print_MarkedBlock,                 &dptr[P_BLOCK]);
    dialog__getcomponoff(print_TwoSided,                &dptr[P_TWOSIDE]);
    dialog__getnumericlimited(print_Copies,             &dptr[P_COPIES], 1, 999);
    dialog__getonoff(print_WaitBetween,                 &dptr[P_WAIT]);
    dialog__getradio(print_Portrait, print_Landscape,   &dptr[P_ORIENT]);
    dialog__getnumericlimited(print_Scale,              &dptr[P_SCALE],  1, 999);

    #endif  /* PRINT_OFF */
}


/************************************
*                                   *
* printer configuration dialog box  *
*                                   *
************************************/

#define printconfig_Driver      1
#define printconfig_TypeInc     2
#define printconfig_TypeDec     3
#define printconfig_Type        4
#define printconfig_Server      5
#define printconfig_BaudInc     6
#define printconfig_BaudDec     7
#define printconfig_Baud        8
#define printconfig_DataInc     9
#define printconfig_DataDec     10
#define printconfig_Data        11
#define printconfig_ParityInc   12
#define printconfig_ParityDec   13
#define printconfig_Parity      14
#define printconfig_StopInc     15
#define printconfig_StopDec     16
#define printconfig_Stop        17

extern void
dproc_printconfig(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(6, D_DRIVER);

    dialog__setfield(printconfig_Driver,    &dptr[0]);
    dialog__setarray(printconfig_Type,      &dptr[1]);
    dialog__setfield(printconfig_Server,    &dptr[2]);
    dialog__setarray(printconfig_Baud,      &dptr[3]);
    dialog__setspecial(printconfig_Data,    &dptr[4]);
    dialog__setspecial(printconfig_Parity,  &dptr[5]);
    dialog__setspecial(printconfig_Stop,    &dptr[6]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
          if(dialog__adjust(&f, printconfig_Type))
            dialog__bumparray(  printconfig_Type,   &dptr[1], f);
        elif(dialog__adjust(&f, printconfig_Baud))
            dialog__bumparray(  printconfig_Baud,   &dptr[3], f);
        elif(dialog__adjust(&f, printconfig_Data))
            dialog__bumpspecial(printconfig_Data,   &dptr[4], f);
        elif(dialog__adjust(&f, printconfig_Parity))
            dialog__bumpspecial(printconfig_Parity, &dptr[5], f);
        elif(dialog__adjust(&f, printconfig_Stop))
            dialog__bumpspecial(printconfig_Stop,   &dptr[6], f);
        else
            tracef1("unprocessed printconfig action %d\n", f);
        }

    dialog__getfield(printconfig_Driver,    &dptr[0]);
    dialog__getarray(printconfig_Type,      &dptr[1]);
    dialog__getfield(printconfig_Server,    &dptr[2]);
    dialog__getarray(printconfig_Baud,      &dptr[3]);
    dialog__getspecial(printconfig_Data,    &dptr[4]);
    dialog__getspecial(printconfig_Parity,  &dptr[5]);
    dialog__getspecial(printconfig_Stop,    &dptr[6]);
}


/********************
*                   *
*  microspace dbox  *
*                   *
********************/

#define microspace_OnOff    1
#define microspace_Inc      2
#define microspace_Dec      3
#define microspace_Pitch    4

extern void
dproc_microspace(DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_MSPACE);

    dialog__setonoff(microspace_OnOff,      &dptr[0]);
    dialog__setnumeric(microspace_Pitch,    &dptr[1]);

    while(((f = dialog__fillin()) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,   microspace_Pitch))
            dialog__bumpnumeric(microspace_Pitch,   &dptr[1], f);
        else
            tracef1("unprocessed microspace action %d\n", f);
        }

    dialog__getonoff(microspace_OnOff,      &dptr[0]);
    dialog__getnumeric(microspace_Pitch,    &dptr[1]);
}


/****************
*               *
*  search dbox  *
*               *
****************/

#define search_TargetString     1
#define search_Replace          2
#define search_ReplaceString    3
#define search_Confirm          4
#define search_UpperLower       5
#define search_ExpSlots         6
#define search_MarkedBlock      7
#define search_ColRange         8
#define search_ColRangeSpec     9
#define search_AllFiles         10
#define search_FromThisFile     11

extern void
dproc_search(DIALOG *dptr)
{
    #if defined(SEARCH_OFF)
    ok = FALSE;
    #else

    assert_dialog(8, D_SEARCH);

    dialog__setfield(search_TargetString,   &dptr[0]);
    dialog__setcomponoff(search_Replace,    &dptr[1]);
    dialog__setonoff(search_Confirm,        &dptr[2]);
    dialog__setonoff(search_UpperLower,     &dptr[3]);
    dialog__setonoff(search_ExpSlots,       &dptr[4]);
    dialog__setonoff(search_MarkedBlock,    &dptr[5]);
    dialog__setcomponoff(search_ColRange,   &dptr[6]);
    dialog__setonoff(search_AllFiles,       &dptr[7]);
    dialog__setonoff(search_FromThisFile,   &dptr[8]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getfield(search_TargetString,   &dptr[0]);
    dialog__getcomponoff(search_Replace,    &dptr[1]);
    dialog__getonoff(search_Confirm,        &dptr[2]);
    dialog__getonoff(search_UpperLower,     &dptr[3]);
    dialog__getonoff(search_ExpSlots,       &dptr[4]);
    dialog__getonoff(search_MarkedBlock,    &dptr[5]);
    dialog__getcomponoff(search_ColRange,   &dptr[6]);
    dialog__getonoff(search_AllFiles,       &dptr[7]);
    dialog__getonoff(search_FromThisFile,   &dptr[8]);

    #endif  /* SEARCH_OFF */
}


#if !defined(SPELL_OFF)

/* --------------------- SPELL related dialog boxes ---------------------- */

/****************************
*                           *
* check document dialog box *
*                           *
****************************/

#define checkdoc_MarkedBlock    1
#define checkdoc_AllFiles       2

extern void
dproc_checkdoc(DIALOG *dptr)
{
    assert_dialog(1, D_CHECKON);

    dialog__setonoff(checkdoc_MarkedBlock,  &dptr[0]);
    dialog__setonoff(checkdoc_AllFiles,     &dptr[1]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getonoff(checkdoc_MarkedBlock,  &dptr[0]);
    dialog__getonoff(checkdoc_AllFiles,     &dptr[1]);
}


/********************************
*                               *
* checking document dialog box  *
*                               *
********************************/

#define checking_ChangeWord         1
#define checking_ChangeWordString   2
#define checking_Browse             3
#define checking_UserDict           4
#define checking_UserDictName       5

extern void
dproc_checking(DIALOG *dptr)
{
    assert_dialog(2, D_CHECK);

    dialog__setcomponoff(checking_ChangeWord,   &dptr[0]);
    dialog__setonoff(checking_Browse,           &dptr[1]);
    dialog__setcomponoff(checking_UserDict,     &dptr[2]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getcomponoff(checking_ChangeWord,   &dptr[0]);
    dialog__getonoff(checking_Browse,           &dptr[1]);
    dialog__getcomponoff(checking_UserDict,     &dptr[2]);
}


/********************************
*                               *
* checked document dialog box   *
*                               *
********************************/

#define checked_Unrecognized    1
#define checked_Added           2

extern void
dproc_checked(DIALOG *dptr)
{
    assert_dialog(1, D_CHECK_MESS);

    dialog__setfield(checked_Unrecognized,  &dptr[0]);
    dialog__setfield(checked_Added,         &dptr[1]);

    dialog__simple_fillin();

    /* no return values */
}


/****************************
*                           *
*  browse word dialog box   *
*                           *
****************************/

#define browse_Word         1
#define browse_UserDict     2
#define browse_UserDictName 3

extern void
dproc_browse(DIALOG *dptr)
{
    assert_dialog(1, D_USER_BROWSE);

    dialog__setfield(browse_Word,           &dptr[0]);
    dialog__setcomponoff(browse_UserDict,   &dptr[1]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getfield(browse_Word,           &dptr[0]);
    dialog__getcomponoff(browse_UserDict,   &dptr[1]);
}


/********************************
*                               *
* anagram/subgram dialog boxes  *
*                               *
********************************/

#define anasubgram_WordTemplate 1
#define anasubgram_UserDict     2
#define anasubgram_UserDictName 3

extern void
dproc_anasubgram(DIALOG *dptr)
{
    assert_dialog(1, D_USER_ANAG);
    assert_dialog(1, D_USER_SUBGRAM);

    dialog__setfield(anasubgram_WordTemplate,   &dptr[0]);
    dialog__setcomponoff(anasubgram_UserDict,   &dptr[1]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getfield(anasubgram_WordTemplate,   &dptr[0]);
    dialog__getcomponoff(anasubgram_UserDict,   &dptr[1]);
}


/****************************
*                           *
* dump user dictionary dbox *
*                           *
****************************/

#define dumpdict_WordTemplate   1
#define dumpdict_FileName       2
#define dumpdict_UserDict       3
#define dumpdict_UserDictName   4

extern void
dproc_dumpdict(DIALOG *dptr)
{
    assert_dialog(2, D_USER_DUMP);

    dialog__setfield(dumpdict_WordTemplate, &dptr[0]);
    dialog__setfield(dumpdict_FileName,     &dptr[1]);
    dialog__setcomponoff(dumpdict_UserDict, &dptr[2]);

    dialog__simple_fillin();

    if(!ok)
        return;

    dialog__getfield(dumpdict_WordTemplate, &dptr[0]);
    dialog__getfield(dumpdict_FileName,     &dptr[1]);
    dialog__getcomponoff(dumpdict_UserDict, &dptr[2]);
}


#endif  /* SPELL_OFF */


/****************************************
*                                       *
* search/replacing dialog box handling  *
*                                       *
****************************************/

#define replace_No      0       /* RETURN(dbox_OK) -> No */
#define replace_Yes     1
#define replace_All     2
#define replace_Stop    3
#define replace_Found   4
#define replace_Hidden  5

extern intl
riscdialog_replace_dbox(const char *mess1, const char *mess2)
{
    intl res;
    dbox d;

    tracef2("riscdialog_replace_dbox(%s, %s)\n",
                trace_string(mess1), trace_string(mess2));

    if(!dialog__dbox)
        if(!dialog__create_reperr("replace", &dialog__dbox))
            /* failed miserably */
            return(ESCAPE);

    d = dialog__dbox;

    dbox_setfield(d, replace_Found,
                  mess1 ? mess1 : (const char *) NULLSTR);

    dbox_setfield(d, replace_Hidden,
                  mess2 ? mess2 : (const char *) NULLSTR);

    dialog__register_help_for(d);

    /* maybe front: note that it's not showstatic */
    dbox_show(d);

    switch(dialog__fillin_for(d))
        {
        case replace_Yes:
            res = 'Y';
            break;

        case replace_All:
            res = 'A';
            break;

        case dbox_CLOSE:
        case replace_Stop:
            res = ESCAPE;
            break;

     /* case dbox_OK: */
     /* case replace_No: */
        default:
            res = 'N';
            break;
        }

    /* note that this dialog box is left hanging around
     * and must be closed explicitly with the below call
     * if the user is foolish enough to go clickaround he gets closed
    */
    tracef1("riscdialog_replace_dbox() returns %d\n", res);
    return(res);
}


extern void
riscdialog_replace_dbox_end(void)
{
    tracef0("riscdialog_replace_dbox_end()\n");

    dialog__dispose();
}


/****************************
*                           *
*  pause function handling  *
*                           *
****************************/

#define pausing_Continue    0       /* RETURN(dbox_OK) -> Continue */
#define pausing_Pause       1

static monotime_t starttime;
static monotime_t lengthtime;

/* pausing null processor */

extern void
pausing_null(void)
{
    tracef1("timesofar = %d\n", monotime_diff(starttime));
    if(monotime_diff(starttime) >= lengthtime)
        /* Cause dbox_CLOSE to be returned to the main process */
        dbox_sendclose(dialog__dbox);
}


extern void
riscdialog_dopause(intl nseconds)
{
    dbox d;
    dbox_field f;

    tracef1("riscdialog_dopause(%d)\n", nseconds);

    if(!dialog__create_reperr("pausing", &dialog__dbox))
        /* failed miserably */
        return;

    d = dialog__dbox;

    dialog__register_help_for(d);

    /* maybe front: note that it's not showstatic */
    dbox_show(d);

    pausing_doc = current_document_handle();

    starttime   = monotime();
    lengthtime  = (monotime_t) 100*nseconds;

    while(((f = dialog__fillin_for(d)) != dbox_CLOSE)  &&  (f != dbox_OK))
        switch(f)
            {
            case pausing_Pause:
                /* stop null events so process never times out */
                /* now needs explicit click (or dbox closure) */
                pausing_doc = DOCHANDLE_NONE;
                break;

        /*  case pausing_Continue:  */
            default:
                break;
            }

    pausing_doc = DOCHANDLE_NONE;

    dialog__dispose();
}


#if !FREEBIE

/************************************
*                                   *
* PipeDream installation dialog box *
*                                   *
************************************/

#define install_Cancel  1
#define install_Name    2
#define install_RegNo   3

extern void
riscos_install_pipedream(void)
{
    intl res;
    dbox d;
    dbox_field f;
    char username[256];

    tracef0("riscos_install_pipedream()\n");

    if(!dialog__create_reperr("install", &d))
        /* failed miserably */
        exit(EXIT_FAILURE);

    dbox_setfield(d, install_Name,  NULLSTR);
    dbox_setfield(d, install_RegNo, registration_number);

    dialog__register_help_for(d);

    dbox_showstatic(d);

    clearkeyboardbuffer();      /* an important event! */
    clearmousebuffer();

    do  {
        while(((f = dialog__fillin_for(d)) != dbox_CLOSE)  &&  (f != dbox_OK))
            switch(f)
                {
                case install_Cancel:
                    /* he is unsure about what he's done */
                    exit(EXIT_FAILURE);

                default:
                    break;
                }

        if(f == dbox_CLOSE)
            break;

        dbox_getfield(d, install_Name, username, sizeof(username));
        }
    while(str_isblank(username));

    if(f == dbox_OK)
        {
        res = install_pipedream(username);

        if(res < 0)
            {
            reperr_null(res);
            exit(EXIT_FAILURE);
            }

        tracef0("the world is alive!");
        }

    dbox_dispose(&d);
}

#endif /* FREEBIE */

#endif /* RISCOS */

/* end of riscdialog.c */
