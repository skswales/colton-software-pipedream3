/* riscos.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       c.riscos - RISC OS specifics for PipeDream
 * Author:      Stuart K. Swales 24 Jan 1989
*/

#include "flags.h"


#if !RISCOS
#   error   This module can only be used to build RISC OS code
#endif


/* local header file */
#include "riscos.h"
#include "riscmenu.h"


/* external header file */
#include "ext.riscos"


/* exported functions */

extern BOOL riscos_adjustclicked(void);
extern char *riscos_cleanupstring(char *str);
extern BOOL riscos_createmainwindow(void);
extern void riscos_destroymainwindow(void);
extern void riscos_finalise(void);
extern void riscos_finalise_once(void);
extern void riscos_frontmainwindow(BOOL immediate);
extern intl riscos_getbuttonstate(void);
extern intl riscos_go(void);
extern BOOL riscos_initialise(void);
extern void riscos_initialise_once(void);
extern void riscos_invalidatemainwindow(void);
extern BOOL riscos_keyinbuffer(void);
extern void riscos_non_null_event(void);
extern const char *riscos_obtainfilename(const char *filename, BOOL forsaveop);
extern BOOL riscos_quit_okayed(intl nmodified);
extern BOOL riscos_readfileinfo(riscos_fileinfo *rip /*out*/, const char *name);
extern void riscos_readtime(riscos_fileinfo *rip /*inout*/);
extern void riscos_removecaret(void);
extern void riscos_resetwindowpos(void);
extern void riscos_restorecaret(void);
extern void riscos_restorecurrentwindowpos(void);
extern void riscos_savecurrentwindowpos(void);
extern void riscos_sendhelpreply(const char *msg, void *m);
extern void riscos_sendsheetclosed(const graphlinkp glp);
extern void riscos_setcaretpos(riscos_window w, intl x, intl y);
extern void riscos_settitlebar(const char *filename);
extern void riscos_settype(riscos_fileinfo *rip /*inout*/, intl filetype);
extern void riscos_updatearea(riscos_redrawproc proc, riscos_window w,
                              intl x0, intl y0, intl x1, intl y1);
extern void riscos_visdelay_on(void);
extern void riscos_visdelay_off(void);
extern intl riscos_visdelay_set(intl level);
extern void riscos_visdelay_stop(void);
extern void riscos_writefileinfo(const riscos_fileinfo *rip, const char *name);


/* exported variables */

extern intl      dragtype;
extern dochandle draghandle;


/* internal functions */

#if TRUE
static BOOL default_event_handler(wimp_eventstr *e, void *handle);
/*static BOOL main_event_handler(wimp_eventstr *e, void *handle);*/
/*static BOOL pd_can_print(int rft);*/
/*static BOOL pd_can_run(int rft);*/
static void send_end_markers(void);
/*static void actions_before_entering(void);*/
/*static void actions_before_exiting(void);*/
#endif


/* ----------------------------------------------------------------------- */

#define TRACE_DRAW      (TRACE && FALSE)
#define TRACE_NULL      (TRACE && TRUE)

#define tubetracef tracef


/* what sort of drag is in progress */
int       dragtype   = NO_DRAG_ACTIVE;
dochandle draghandle = DOCHANDLE_NONE;

static BOOL last_event_was_null = FALSE;

static wimp_openstr saved_window_pos;


/* hourglass maintenance */
static intl vis_state = 0;
static intl vis_delay = -1;


/* ------------------------ file import/export --------------------------- */

/* ask for scrap file load: all errors have been reported locally */

static void
scraptransfer_file(wimp_msgdatasave *ms)
{
    intl size;
    intl filetype = xferrecv_checkimport(&size);

    switch(filetype)
        {
        case DIRECTORY_FILETYPE:
        case APPLICATION_FILETYPE:
            reperr(ERR_ISADIR, ms->leaf);
            break;

        case DRAW_FILETYPE:
        case SPRITE_FILETYPE:
            /* fault these as we can't store them anywhere safe */
            reperr_null(ERR_BADSCRAPLOAD);
            break;

        default:
            xferrecv_doimport(0, 0, NULL);  /* can't manage ram load */
            break;
        }
}


/* reallyload -> kill current document, load new file at same position as old */

static BOOL
riscos_LoadFile(const char *filename, BOOL merge, BOOL reallyload, char option)
{
    BOOL temp_file = !xferrecv_file_is_safe();
    BOOL res;

    if(reallyload)
        merge = FALSE;      /* make incompatible option sensible */

    tracef5("Asked to %s file '%s', reallyload = %s, temp = %s; dochandle = %d\n",
            merge ? "merge" : "load", filename, trace_boolstring(reallyload), trace_boolstring(temp_file), current_document_handle());

    res = init_dialog_box(D_LOAD);

    if(res)
        res = str_setc(&d_load[0].textfield, filename);

    if(res)
        {
        if(reallyload)
            {
            tracef0("reallyload causing document destruction\n");
            riscos_savecurrentwindowpos();
            destroy_current_document();
            }

        if(merge)
            d_load[1].option = 'Y'; /* Insert */

        d_load[3].option = option;

        /* loadfile will now create its own document if not really inserting */

        res = loadfile(d_load[0].textfield, NEW_WINDOW, reallyload);

        tracef1("loadfile returned %d\n", res);
        }

    if(res)
        {
        if(temp_file)
            {
            if(!merge)
                {
                set_untitled_document();
                exp_rename_file();
                }
            else
                tracef0("not inserting/list file itself was scrap!\n");
            }
        else
            {
            if(reallyload)
                {
                tracef1("really loading file, so set name to %s\n", d_load[0].textfield);
                (void) str_setc(&currentfilename, d_load[0].textfield);
                riscos_settitlebar(currentfilename);
                exp_rename_file();
                filealtered(FALSE);
                }
            }

        if(reallyload)
            {
            tracef0("put newly loaded document at same place that old one was\n");
            riscos_resetwindowpos();
            riscos_restorecurrentwindowpos();
            }

        draw_screen();
        }

    return(res);
}


extern void
print_file(const char *filename)
{
    BOOL ok;

    tracef1("asked to print %s\n", filename);

    ok = riscos_LoadFile(filename, FALSE, FALSE, 'A');      /* load as new file */

    if(ok)
        {
        #if !defined(PRINT_OFF)
        reset_print_options();

        force_calshe();

        print_document();
        #endif

        destroy_current_document();
        }
}


/* ----------------------------------------------------------------------- */

extern BOOL
riscos_quit_okayed(intl nmodified)
{
    dochandle doc = current_document_handle();
    dochandle ndoc;
    intl res;
    char mess[256];

    sprintf(mess, Zd_Zs_edited_are_you_sure_Zs_STR,
            nmodified,
            (nmodified == 1) ? file_STR: files_STR,
            applicationname);

    res = riscdialog_queryexit(mess);

    /* Yes      -> ok, quit application */
    /* No       -> save files, then quit application */
    /* Cancel   -> abandon closedown */

    if(res == riscdialogquery_NO)
        {
        ndoc = DOCHANDLE_NONE;

        /* loop over all documents in sequence trying to save them */

        while((ndoc = next_document_handle(ndoc)) != DOCHANDLE_NONE)
            {
            select_document_using_handle(ndoc);

            /* If punter cancels any save, he means abort the shutdown */
            if(riscdialog_save_existing() == riscdialogquery_CANCEL)
                {
                res = riscdialogquery_CANCEL;
                break;
                }
            }

        /* if not aborted then all modified documents either saved or ignored - delete all documents (must be at least one) */

        if(res != riscdialogquery_CANCEL)
            {
            ndoc = next_document_handle(DOCHANDLE_NONE);

            do  {
                select_document_using_handle(ndoc);

                ndoc = next_document_handle(ndoc);      /* get next handle before we destroy this document and it's link! */

                destroy_current_document();
                }
            while(ndoc != DOCHANDLE_NONE);
            }
        }

    select_document_using_handle(doc);

    return(res != riscdialogquery_CANCEL);
}


/* ----------------------------------------------------------------------- */

typedef intl riscos_icon;


/* has this module been initialised? (shared global) */

static BOOL riscos__initialised = FALSE;
static BOOL seen_my_birth       = FALSE;


/*******************************
*                              *
* main window (shared globals) *
*                              *
*******************************/

static const char fake_dboxname[] = "fake_dbox";
static const char main_dboxname[] = "main_dbox";
static wimp_wind *main_window_definition;       /* ptr to actual dbox def */
static intl       main_window_initial_y0;
static intl       main_window_default_height;
#define           main_window_y_bump           (title_height + 16)


/* can we print a file of this filetype? */

static BOOL
pd_can_print(intl rft)
{
    switch(rft)
        {
        case PIPEDREAM_FILETYPE:
            return(TRUE);

        default:
            return(FALSE);
        }
}


/* can we 'run' a file of this filetype? */

static BOOL
pd_can_run(intl rft)
{
    switch(rft)
        {
        case PIPEDREAM_FILETYPE:
        case PDMACRO_FILETYPE:
            return(TRUE);

        default:
            return(FALSE);
        }
}


/****************************************************************************
*                                                                           *
*                           icon bar processing                             *
*                                                                           *
****************************************************************************/

/******************************************
*                                         *
* process button click event for icon bar *
*                                         *
******************************************/

static void
iconbar_button_click(wimp_eventstr *e)
{
    tracef0("iconbar_button_click()\n");

    if(installed_ok)
        {
        riscos_non_null_event();

        if(e->data.but.m.bbits == wimp_BLEFT)
            {
            if(create_new_untitled_document())
                draw_screen();
            }
        else
            tracef0("wierd button click - ignored");
        }
    else
        riscos_install_pipedream();
}


/*************************************
*                                    *
* initialise RISC OS icon bar system *
*                                    *
*************************************/

static char iconbar_spritename[16] = "!";

static void
iconbar_initialise(const char *appname)
{
    tracef1("iconbar_initialise(%s)\n", appname);

    /* make name of icon bar icon sprite */
    strcat(iconbar_spritename, appname);

    win_register_event_handler(win_ICONBAR, default_event_handler, NULL);

    /* new icon (sprite/no text) */
    (void) baricon_new(iconbar_spritename, NULL);
}


/********************************************
*                                           *
*  do a little browsing in the background   *
*                                           *
********************************************/

static void
continue_browse(void)
{
    if(pausing_doc != DOCHANDLE_NONE)
        {
        select_document_using_handle(pausing_doc);
        pausing_null();

        /* whilst pausing, stop a few others from getting nulls */
        return;
        }

    /* all these are low priority things - we wouldn't have got them if key or mouse coming */

    #if !defined(SPELL_OFF)

    if(browsing_doc != DOCHANDLE_NONE)
        {
        select_document_using_handle(browsing_doc);
        browse_null();
        }

    if(anagram_doc != DOCHANDLE_NONE)
        {
        select_document_using_handle(anagram_doc);
        anagram_null();
        }

    if(dumpdict_wants_nulls)
        dumpdict_null();

    if(mergedict_wants_nulls)
        mergedict_null();

    #endif  /* SPELL_OFF */
}


/****************************************
*                                       *
* do a little drawing in the background *
*                                       *
****************************************/

static void
continue_draw(void)
{
    window_data *wdp = NULL;

    while((wdp = next_document(wdp)) != NULL)
        if(wdp->Xxf_interrupted)
            {
            tracef1("continuing with interrupted draw for document &%p\n", wdp);
            select_document(wdp);
            draw_screen();
            break;          /* only do one per event */
            }
}


/****************************************
*                                       *
* do a little recalc in the background  *
*                                       *
****************************************/

static void
continue_recalc(void)
{
    window_data *wdp = NULL;

    while((wdp = next_document(wdp)) != NULL)
        if(wdp->Xrecalc_forced  ||  (wdp->Xrecalc_bit  &&  wdp->Xrcobit))
            {
            tracef1("continuing with interrupted recalc for document &%p\n", wdp);
            select_document(wdp);

            (void) calshe(CALC_DRAW, last_event_was_null
                                            ? CALC_CONTINUE
                                            : CALC_RESTART);

            break;
            }
}


/****************************************
*                                       *
* drag processor, called on null events *
*                                       *
****************************************/

static void
process_drag(void)
{
    wimp_wstate wstate;
    wimp_mousestr m;
    int x, y;
    BOOL shiftpressed;

    if(dragtype != NO_DRAG_ACTIVE)
        {
        shiftpressed = depressed_shift();

        wimpt_safe(wimp_get_point_info(&m));
        x = m.x;
        y = m.y;

        tracef4("mouse pointer at w %d i %d x %d y %d\n",
                m.w, m.i, m.x, m.y);

        /* send this to the right guy */
        select_document_using_handle(draghandle);

        if(m.bbits & (wimp_BLEFT | wimp_BRIGHT))
            {
            tracef0("continuing drag: button still held\n");

            /* scroll text if outside this guy's window */
            wimpt_safe(wimp_get_wind_state(main__window, &wstate));

              if(x <  wstate.o.box.x0)
                application_process_command(N_ScrollLeft);
            elif(x >= wstate.o.box.x1)
                application_process_command(N_ScrollRight);

              if(y <  wstate.o.box.y0)
                application_process_command(shiftpressed ? N_PageDown : N_ScrollDown);
            elif(y >= wstate.o.box.y1)
                application_process_command(shiftpressed ? N_PageUp : N_ScrollUp);

            application_drag(x, y, FALSE);
            }
        else
            {
            tracef0("stopping drag: button released\n");
            application_drag(x, y, TRUE);
            dragtype = NO_DRAG_ACTIVE;      /* will release nulls */
            }
        }
}


/************************************
*                                   *
*  see if anyone needs null events  *
*                                   *
************************************/

static void
check_if_null_events_wanted(void)
{
    BOOL required = FALSE;
    BOOL alreadygot = (win_idle_event_claimer() != win_IDLE_OFF);
    window_data *wdp;

    if(dragtype != NO_DRAG_ACTIVE)
        {
        required = TRUE;
        vtracef0(TRACE_NULL, "nulls wanted because of drag: ");
        }
    else
        {
        wdp = NULL;

        while((wdp = next_document(wdp)) != NULL)
            /* does any document want recalc in background? */
            if(wdp->Xrecalc_forced  ||  (wdp->Xrecalc_bit  &&  wdp->Xrcobit))
                {
                vtracef0(TRACE_NULL, "nulls wanted because of recalc: ");
                required = TRUE;
                break;
                }
            elif(wdp->Xxf_interrupted)
                {
                vtracef0(TRACE_NULL, "nulls wanted for interrupted drawing: ");
                required = TRUE;
                break;
                }

          if(browsing_doc != DOCHANDLE_NONE)
            {
            vtracef0(TRACE_NULL, "nulls wanted for browse: ");
            required = TRUE;
            }
        elif(anagram_doc != DOCHANDLE_NONE)
            {
            vtracef0(TRACE_NULL, "nulls wanted for anagram: ");
            required = TRUE;
            }
        elif(dumpdict_wants_nulls)
            {
            vtracef0(TRACE_NULL, "nulls wanted for dump dictionary: ");
            required = TRUE;
            }
        elif(mergedict_wants_nulls)
            {
            vtracef0(TRACE_NULL, "nulls wanted for merge dictionary: ");
            required = TRUE;
            }
        elif(pausing_doc != DOCHANDLE_NONE)
            {
            vtracef0(TRACE_NULL, "nulls wanted for pause: ");
            required = TRUE;
            }
        }

    vtracef1(TRACE_NULL, "already got nulls = %s\n",
                trace_boolstring(alreadygot));

      if(!alreadygot  &&   required)
        win_claim_idle_events(win_ICONBARLOAD);
    elif( alreadygot  &&  !required)
        win_claim_idle_events(win_IDLE_OFF);
}


/***********************************
*                                  *
* Tidy up before returning control *
* back to the Window manager       *
*                                  *
***********************************/

static void
actions_before_exiting(void)
{
    /* anyone need background events? */
    check_if_null_events_wanted();

    /* anyone need end of data sending? */
    send_end_markers();

    /* forget about any fonts we have handles on */
    font_close_file(DOCHANDLE_NONE);

    /* turn off hourglass */
    riscos_visdelay_off();

    /* destroy static base pointer */
    select_document(NO_DOCUMENT);

    /* everything processed here */
    been_error = FALSE;
}


static void
actions_before_entering(void)
{
    /* turn on hourglass */
    riscos_visdelay_on();
}


/********************************************
*                                           *
* tell client this sheet is no longer open  *
*                                           *
********************************************/

extern void
riscos_sendsheetclosed(const graphlinkp glp)
{
    wimp_msgstr msg;
    
    if(glp)
        {
        msg.hdr.size    = offsetof(wimp_msgstr, data.pd_dde.type.b)
                        + sizeof(wimp_msgpd_ddetypeB);
        msg.hdr.my_ref  = 0;        /* fresh msg */
        msg.hdr.action  = wimp_MPD_DDE;

        msg.data.pd_dde.id              = wimp_MPD_DDE_SheetClosed;
        msg.data.pd_dde.type.b.handle   = glp->ghan;

        wimpt_send_message(wimp_ESEND, &msg, glp->task);
        }
}


/****************************************
*                                       *
*  send client the contents of a slot   *
*                                       *
****************************************/

extern void
riscos_sendslotcontents(const graphlinkp glp, intl xoff, intl yoff)
{
    rowt row = glp->row + yoff;
    slotp tslot;
    intl type = wimp_MPD_DDE_typeC_type_Text;
    char buffer[LIN_BUFSIZ];
    const char *ptr = NULL;
    char *to, ch;
    intl t_justify, t_opt_DF;
    intl nbytes = 0;
    wimp_msgstr msg;

    if(glp)
        {
        glp->datasent = TRUE;

        tslot = travel(glp->col + xoff, row);

        if(tslot)
            switch(tslot->type)
                {
                case SL_NUMBER:
                    type = wimp_MPD_DDE_typeC_type_Number;
                    msg.data.pd_dde.type.c.content.number = tslot->content.number.result.value;
                    nbytes = sizeof(double);
                    break;


                default:
                    {
                    t_justify = tslot->justify;
                    t_opt_DF  = d_options_DF;
                    tslot->justify = J_LEFT;
                    if(tslot->type == SL_DATE)
                        d_options_DF = 'E';
                    curpnm = 0;     /* we have really no idea */
                    expand_slot(tslot, row, buffer, LIN_BUFSIZ, TRUE, FALSE, TRUE);
                    /* remove highlights & funny spaces etc */
                    ptr = buffer;
                    to  = buffer;
                    do  {
                        ch = *ptr++;
                        if((ch >= SPACE)  ||  !ch)
                            *to++ = ch;
                        }
                    while(ch);
                    ptr = buffer;
                    tslot->justify = t_justify;
                    d_options_DF   = t_opt_DF;
                    }
                    break;
                }
        else
            ptr = NULLSTR;

        if(ptr)
            {
            *msg.data.pd_dde.type.c.content.text = '\0';
            strncat(msg.data.pd_dde.type.c.content.text,
                    ptr,
                    sizeof(wimp_msgpd_ddetypeC_text));
            nbytes = strlen(msg.data.pd_dde.type.c.content.text) + 1;
            }

        msg.hdr.size    = offsetof(wimp_msgstr, data.pd_dde.type.c.content)
                        + nbytes;
        msg.hdr.my_ref  = 0;        /* fresh msg */
        msg.hdr.action  = wimp_MPD_DDE;

        msg.data.pd_dde.id              = wimp_MPD_DDE_SendSlotContents;
        msg.data.pd_dde.type.c.handle   = glp->ghan;
        msg.data.pd_dde.type.c.xoff     = xoff;
        msg.data.pd_dde.type.c.yoff     = yoff;
        msg.data.pd_dde.type.c.type     = type;

        wimpt_send_message(wimp_ESENDWANTACK, &msg, glp->task);
        }
}


extern void
riscos_sendallslots(const graphlinkp glp)
{
    intl xoff, yoff;

    xoff = 0;
    do  {
        yoff = 0;
        do  {
            riscos_sendslotcontents(glp, xoff, yoff);
            }
        while(++yoff <= glp->ysize);
        }
    while(++xoff <= glp->xsize);
}


/****************************************
*                                       *
*  send client an end of data message   *
*                                       *
****************************************/

extern void
riscos_sendendmarker(const graphlinkp glp)
{
    wimp_msgstr msg;
    
    if(glp)
        {
        glp->datasent = FALSE;

        msg.hdr.size    = offsetof(wimp_msgstr, data.pd_dde.type.c)
                        + sizeof(wimp_msgpd_ddetypeC);
        msg.hdr.my_ref  = 0;        /* fresh msg */
        msg.hdr.action  = wimp_MPD_DDE;

        msg.data.pd_dde.id              = wimp_MPD_DDE_SendSlotContents;
        msg.data.pd_dde.type.c.handle   = glp->ghan;
        msg.data.pd_dde.type.c.type     = wimp_MPD_DDE_typeC_type_End;

        wimpt_send_message(wimp_ESENDWANTACK, &msg, glp->task);
        }
}


static void
send_end_markers(void)
{
    LIST *lptr;
    graphlinkp glp;

    for(lptr = first_in_list(&graphics_link_list);
        lptr;
        lptr = next_in_list(&graphics_link_list))
        {
        glp = (graphlinkp) lptr->value;

        if(glp->datasent)
            riscos_sendendmarker(glp);
        }
}


/****************************************
*                                       *
*  event processing for broadcasts      *
*  and other such events that don't go  *
*  to a main window handler             *
*                                       *
****************************************/

/*************************
*                        *
* process message events *
*                        *
*************************/

static BOOL
default_message(wimp_msgstr *m)
{
    wimp_t task     = m->hdr.task;          /* caller's task id */
    int action      = m->hdr.action;
    BOOL processed  = TRUE;

    switch(action)
        {
        case wimp_MPREQUIT:
            {
            int size  = m->hdr.size;
            /* flags word only present if RISC OS+ or better */
            int flags = (size >= 24) ? m->data.prequitrequest.flags : 0;
            int count;

            mergebuf_all();         /* ensure modified buffers to docs */

            count = documents_modified();

            /* if any modified, it gets hard */
            if(count)
                {
                /* First, acknowledge the message to stop it going any further */
                wimpt_ack_message(m);

                /* And then tell the user. */
                if(riscos_quit_okayed(count)  &&  !(flags & wimp_MPREQUIT_flags_killthistask))
                    {
                    /* Start up the closedown sequence again if OK and all tasks are to die. (RISC OS+) */
                    /* We assume that the sender is the Task Manager,
                     * so that Ctrl-Shf-F12 is the closedown key sequence.
                    */
                    dochandle ndoc = DOCHANDLE_NONE;
                    wimp_eventdata ee;

                    wimpt_safe(wimp_get_caret_pos(&ee.key.c));
                    ee.key.chcode = akbd_Sh + akbd_Ctl + akbd_Fn12;
                    wimpt_send_message(wimp_EKEY, (wimp_msgstr *) &ee, task);

                    #if TRUE
                    /* 3.07 */
                    while((ndoc = next_document_handle(DOCHANDLE_NONE)) != DOCHANDLE_NONE)
                        {
                        select_document_using_handle(ndoc);

                        destroy_current_document();
                        }
                    #else
                    /* Now tidy up so that we don't object the next time
                     * the message comes round. Don't actually delete
                     * each document or update the title bar
                     * as this is a gross waste of time.
                    */
                    ndoc = DOCHANDLE_NONE;

                    while((ndoc = next_document_handle(ndoc)) != DOCHANDLE_NONE)
                        {
                        select_document_using_handle(ndoc);

                        xf_filealtered = FALSE;

                        /* lose all fonts for this document */
                        font_close_file(ndoc);
                        }
                    #endif
                    }
                }

        /* may get CLOSEDOWN soon, this just does exit(EXIT_SUCCESS) in lib */
        #if defined(BACKTRACE)
        wimpt_backtrace_on_exit(FALSE);
        #endif
        }
        break;


        case wimp_MDATASAVE:
            /* initial drag of data from somewhere to icon */
            if(installed_ok)
                scraptransfer_file(&m->data.datasave);
            break;


        case wimp_MDATAOPEN:
            {
            /* double-click on object */
            char *filename;
            intl filetype = xferrecv_checkinsert(&filename);

            tracef1("ukprocessor got asked if it can 'run' a file of type &%4.4X\n", filetype);

            if(filetype == PDMACRO_FILETYPE)
                {
                xferrecv_insertfileok();
                if(!installed_ok)
                    riscos_install_pipedream();
                else
                    exec_file(filename);
                }
            elif(pd_can_run(filetype))
                {
                xferrecv_insertfileok();
                if(!installed_ok)
                    riscos_install_pipedream();
                else
                    (void) riscos_LoadFile(filename, FALSE, FALSE, 'A');        /* load as new file */
                }
            else
                tracef0("we can't run files of this type\n");
            }
            break;


        case wimp_MDATALOAD:
            if(!installed_ok)
                riscos_install_pipedream();
            else
            {
            /* object dragged to icon bar - load regardless of type */
            char *filename;
            intl filetype = xferrecv_checkinsert(&filename);

            tracef1("ukprocessor asked to load a file of type &%4.4X\n", filetype);

            switch(filetype)
                {
                case DIRECTORY_FILETYPE:
                case APPLICATION_FILETYPE:
                    reperr(ERR_ISADIR, filename);
                    break;

                case DRAW_FILETYPE:
                case SPRITE_FILETYPE:
                    /* ignore these as we can't do anything sensible */
                    break;

                default:
                    (void) riscos_LoadFile(filename, FALSE, FALSE,
                                (filetype == PDMACRO_FILETYPE) ? 'T' : 'A');        /* load as new file */
                    xferrecv_insertfileok();
                    break;
                }
            }
            break;


        #if !defined(PRINT_OFF)

        case wimp_MPrintTypeOdd:
            {
            /* printer broadcast */
            char *filename;
            intl filetype = xferrecv_checkprint(&filename);

            tracef1("ukprocessor got asked if it can print a file of type &%4.4X\n", filetype);

            if(pd_can_print(filetype))
                {
                if(!installed_ok)
                    riscos_install_pipedream();
                else
                    print_file(filename);

                xferrecv_printfileok(-1);   /* print all done */
                }
            }
            break;


        case wimp_MPrinterChange:
            tracef0("ukprocessor got informed of printer driver change\n");
            riscprint_set_printer_data();
            break;

        #endif  /* PRINT_OFF */


        case wimp_MMODECHANGE:
            cachemodevariables();
            cachepalettevariables();
            font_tidy_up();
            break;


        case wimp_PALETTECHANGE:
            cachepalettevariables();
            break;


        case wimp_MHELPREQUEST:
            tracef0("ukprocessor got help request for icon bar icon\n");
            m->data.helprequest.m.i = 0;
            riscos_sendhelpreply(help_iconbar, m);
            break;


        case wimp_MINITTASK:
            {
            const char *taskname = (char *) &m->data.words[2];
            tracef1("ukprocessor got wimp_MINITTASK for %s\n", taskname);
            if(!strcmp(taskname, applicationname))
                {
                if(seen_my_birth)
                    {
                    tracef0("Another PipeDream is trying to start! I'll kill it\n");

                    /* reuse message block */
                    m->hdr.size     = sizeof(wimp_msghdr);
                    m->hdr.your_ref = 0;                /* fresh msg */
                    m->hdr.action   = wimp_MCLOSEDOWN;
                    wimpt_send_message(wimp_ESEND, m, task);
                    }
                else
                    {
                    tracef0("Witnessing our own birth\n");
                    seen_my_birth = TRUE;
                    if(!installed_ok)
                        riscos_install_pipedream();
                    }
                }
            }
            break;


        case wimp_MPD_DDE:
            {
            intl id = m->data.pd_dde.id;

            tracef1("ukprocessor got PD DDE message %d\n", id);

            switch(id)
                {
                case wimp_MPD_DDE_IdentifyMarkedBlock:
                    {
                    tracef0("[IdentifyMarkedBlock]\n");

                    if((blkstart.col != NO_COL)  &&  (blkend.col != NO_COL))
                        {
                        wimp_msgstr msg;
                        char *ptr = msg.data.pd_dde.type.a.text;
                        size_t nbytes = sizeof(wimp_msgpd_ddetypeA_text);
                        ghandle ghan;
                        intl xsize = (intl) (blkend.col - blkstart.col);
                        intl ysize = (intl) (blkend.row - blkstart.row);
                        const char *leaf;
                        const char *tag;
                        intl taglen;
                        slotp tslot;

                        select_document_using_handle(blkdochandle);

                        (void) mergebuf_nocheck();
                        filbuf();

                        leaf = leafname(currentfilename);
                        *ptr = '\0';
                        strncatind(ptr, leaf, &nbytes);
                        ptr += strlen(ptr) + 1;

                        tslot = travel(blkstart.col, blkstart.row);

                        if(tslot  &&  (tslot->type == SL_TEXT))
                            tag = tslot->content.text;
                        else
                            tag = ambiguous_tag_STR;

                        taglen = strlen(tag);

                        if(taglen < nbytes)
                            {
                            /* need to copy tag before adding to list to avoid moving memory problems */
                            strcpy(ptr, tag);
                            tag = ptr;
                            ptr += taglen + 1;

                            /* create entry on list - even if already there */
                            ghan = graph_add_entry(m->hdr.my_ref,       /* unique number */
                                                   blkdochandle, blkstart.col, blkstart.row,
                                                   xsize, ysize, leaf, tag,
                                                   task);

                            if(ghan > 0)
                                {
                                tracef5("[IMB: ghan %d xsize %d ysize %d leafname %s tag %s]",
                                        ghan, xsize, ysize, leaf, tag);

                                msg.data.pd_dde.id              = wimp_MPD_DDE_ReturnHandleAndBlock;
                                msg.data.pd_dde.type.a.handle   = ghan;
                                msg.data.pd_dde.type.a.xsize    = xsize;
                                msg.data.pd_dde.type.a.ysize    = ysize;

                                /* send message as ack to his one */
                                msg.hdr.size     = ptr - (char *) &msg;
                                msg.hdr.your_ref = m->hdr.my_ref;
                                msg.hdr.action   = wimp_MPD_DDE;
                                wimpt_send_message(wimp_ESENDWANTACK, &msg, task);
                                }
                            elif(ghan  &&  (ghan != ERR_NOROOM))
                                reperr_null(ghan);
                            else
                                tracef0("[IMB: failed to create ghandle - caller will get bounced msg]\n");
                            }
                        else
                            tracef0("[IMB: leafname/tag too long - caller will get bounced msg]\n");
                        }
                    else
                        tracef0("[IMB: no block marked/only one mark set - caller will get bounced msg]\n");
                    }
                    break;


                case wimp_MPD_DDE_EstablishHandle:
                    {
                    ghandle ghan;
                    intl xsize       = m->data.pd_dde.type.a.xsize;
                    intl ysize       = m->data.pd_dde.type.a.ysize;
                    const char *leaf = m->data.pd_dde.type.a.text;
                    const char *tag  = leaf + strlen(leaf) + 1;
                    dochandle doc;
                    colt col;
                    rowt row;
                    slotp tslot;

                    tracef4("[EstablishHandle: xsize %d, ysize %d, leaf %s, tag %s]\n", xsize, ysize, leaf, tag);

                    doc = find_document_using_leafname(leaf);

                    if(doc == DOCHANDLE_NONE)
                        break;

                    if(doc == DOCHANDLE_SEVERAL)
                        break;

                    select_document_using_handle(doc);

                    (void) mergebuf_nocheck();
                    filbuf();

                    /* find tag in document */
                    init_doc_as_block();

                    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
                        {
                        if(tslot->type != SL_TEXT)
                            continue;

                        if(stricmp(tslot->content.text, tag))
                            continue;

                        col = in_block.col;
                        row = in_block.row;

                        /* add entry to list */
                        ghan = graph_add_entry(m->hdr.my_ref,       /* unique number */
                                               doc, col, row,
                                               xsize, ysize, leaf, tag,
                                               task);

                        if(ghan > 0)
                            {
                            tracef5("[EST: ghan %d xsize %d ysize %d leafname %s tag %s]",
                                    ghan, xsize, ysize, leaf, tag);

                            m->data.pd_dde.id               = wimp_MPD_DDE_ReturnHandleAndBlock;
                            m->data.pd_dde.type.a.handle    = ghan;

                            /* send same message as ack to his one */
                            m->hdr.your_ref = m->hdr.my_ref;
                            m->hdr.action   = wimp_MPD_DDE;
                            wimpt_send_message(wimp_ESENDWANTACK, m, task);
                            }
                        else
                            tracef0("[EST: failed to create ghandle - caller will get bounced msg]\n");

                        break;
                        }
                    }
                    break;


                case wimp_MPD_DDE_RequestUpdates:
                case wimp_MPD_DDE_StopRequestUpdates:
                    {
                    ghandle han = m->data.pd_dde.type.b.handle;
                    graphlinkp glp;

                    tracef2("[%sRequestUpdates: handle %d]\n", (id == wimp_MPD_DDE_StopRequestUpdates) ? "Stop" : "", han);

                    glp = graph_search_list(han);

                    if(glp)
                        {
                        /* stop caller getting bounced msg */
                        wimpt_ack_message(m);

                        /* flag that updates are/are not required on this handle */
                        glp->update = (id == wimp_MPD_DDE_RequestUpdates);
                        }
                    }
                    break;


                case wimp_MPD_DDE_RequestContents:
                    {
                    ghandle han = m->data.pd_dde.type.b.handle;
                    graphlinkp glp;

                    tracef1("[RequestContents: handle %d]\n", han);

                    glp = graph_search_list(han);

                    if(glp)
                        {
                        /* stop caller getting bounced msg */
                        wimpt_ack_message(m);

                        select_document_using_handle(glp->han);

                        (void) mergebuf_nocheck();
                        filbuf();

                        /* fire off all the slots */
                        riscos_sendallslots(glp);

                        riscos_sendendmarker(glp);
                        }
                    }
                    break;


                case wimp_MPD_DDE_GraphClosed:
                    {
                    ghandle han = m->data.pd_dde.type.b.handle;
                    graphlinkp glp;

                    tracef1("[GraphClosed: handle %d]\n", han);

                    glp = graph_search_list(han);

                    if(glp)
                        {
                        /* stop caller getting bounced msg */
                        wimpt_ack_message(m);

                        /* delete entry from list */
                        graph_remove_entry(han);
                        }
                    }
                    break;


                case wimp_MPD_DDE_DrawFileChanged:
                    {
                    const char *drawfilename = m->data.pd_dde.type.d.leafname;

                    tracef1("[DrawFileChanged: name %s]\n", drawfilename);

                    /* don't ack this message: other people may want to see it too */

                    /* look for any instances of this DrawFile; update windows with refs */
                    draw_recache_file(drawfilename);
                    }
                    break;


                default:
                    tracef1("ignoring PD DDE type %d message\n", id);
                    processed = FALSE;
                    break;
                }
            }
            break;


        default:
            tracef1("unprocessed %s message to default handler\n",
                    trace_wimp_message(m));
            processed = FALSE;
            break;
        }

    return(processed);
}


static BOOL
default_message_bounced(wimp_msgstr *m)
{
    int action      = m->hdr.action;
    BOOL processed  = TRUE;

    switch(action)
        {
        case wimp_MPD_DDE:
            {
            intl id = m->data.pd_dde.id;

            tracef1("ukprocessor got bounced PD DDE message %d\n", id);

            switch(id)
                {
                case wimp_MPD_DDE_SendSlotContents:
                    {
                    ghandle han = m->data.pd_dde.type.c.handle;
                    graphlinkp glp;
                    tracef1("[SendSlotContents on handle %d bounced - receiver dead]\n", han);
                    glp = graph_search_list(han);
                    if(glp)
                        /* delete entry from list */
                        graph_remove_entry(han);
                    }
                    break;


                case wimp_MPD_DDE_ReturnHandleAndBlock:
                    {
                    ghandle han = m->data.pd_dde.type.a.handle;
                    graphlinkp glp;
                    tracef1("[ReturnHandleAndBlock on handle %d bounced - receiver dead]\n", han);
                    glp = graph_search_list(han);
                    if(glp)
                        /* delete entry from list */
                        graph_remove_entry(han);
                    }
                    break;


                default:
                    tracef1("ignoring bounced PD DDE type %d message\n", id);
                    processed = FALSE;
                    break;
                }
            }
            break;


        default:
            tracef1("unprocessed %s bounced message to default handler\n",
                    trace_wimp_message(m));
            processed = FALSE;
            break;
        }

    return(processed);
}


/****************************
*                           *
* process unhandled events  *
*                           *
****************************/

static BOOL
default_event_handler(wimp_eventstr *e, void *handle)
{
    BOOL processed = TRUE;

    IGNOREPARM(handle);

    select_document(NO_DOCUMENT);

    trace_sb();

    if(e->e == wimp_ENULL)
        {
        tracef0("got a null event\n");
        process_drag();
        continue_recalc();
        continue_draw();
        continue_browse();
        last_event_was_null = TRUE;
        }
    else
        {
        last_event_was_null = FALSE;

        switch(e->e)
            {
            case wimp_EBUT:
                /* one presumes only the icon bar stuff gets here ... */
                iconbar_button_click(e);
                break;


            case wimp_EUSERDRAG:
                tracef0("UserDrag: stopping drag as button released\n");

                /* send this to the right guy */
                select_document_using_handle(draghandle);

                application_drag(e->data.dragbox.x0, e->data.dragbox.y0, TRUE);
                dragtype = NO_DRAG_ACTIVE;      /* will release nulls */
                break;


            case wimp_ESEND:
            case wimp_ESENDWANTACK:
                processed = default_message(&e->data.msg);
                break;


            case wimp_EACK:
                processed = default_message_bounced(&e->data.msg);
                break;


            default:
                tracef1("unprocessed wimp event %s\n", trace_wimp_event(e));
                processed = FALSE;
                break;
            }
        }

    return(processed);
}


/* ----------------------------------------------------------------------- */

/************************************************
*                                               *
*  process open window request for fake window  *
*                                               *
************************************************/

static void
fake_open_request(wimp_eventstr *e)
{
    tracef0("fake_open_request()\n");

    application_open_request(e);
}


/************************************************
*                                               *
* process scroll window request for fake window *
*                                               *
************************************************/

static void
fake_scroll_request(wimp_eventstr *e)
{
    tracef0("fake_scroll_request()\n");

    application_scroll_request(e);
}


/************************************************
*                                               *
* process close window request for fake window  *
*                                               *
************************************************/

/* if       select-close then close window */
/* if       adjust-close then open parent directory and close window */
/* if shift-adjust-close then open parent directory */

static void
fake_close_request(wimp_eventstr *e)
{
    BOOL adjustclicked = riscos_adjustclicked();
    BOOL shiftpressed  = depressed_shift();
    BOOL justopening   = (shiftpressed  &&  adjustclicked);
    BOOL wanttoclose   = TRUE;
    intl res;

    IGNOREPARM(e);

    tracef0("fake_close_request()\n");

    if(!justopening)
        {
        (void) mergebuf_nocheck();
        filbuf();
        }

    /* deal with modified files before opening any other windows */
    if(!justopening  &&  xf_filealtered)
        {
        res = riscdialog_save_existing();

#if 1
        wanttoclose = (res != riscdialogquery_CANCEL);
#else
        wanttoclose = ((res != riscdialogquery_CANCEL)  &&
                       (!xf_filealtered  ||  (res == riscdialogquery_NO)));
#endif
        }

    if(!justopening)
        {
        if(wanttoclose)
            wanttoclose = dependent_files_warning();

        if(wanttoclose)
            wanttoclose = dependent_links_warning();
        }

    if(adjustclicked)
        {
        /* obtain dirname to open */
        char *filename = currentfilename;
        char *leaf = leafname(filename);

        if(leaf != filename)
            {
            char a[256];
            char sep = leaf[-1];
            leaf[-1] = '\0';
            sprintf(a, Filer_OpenDir_Zs_STR, filename);
            leaf[-1] = sep;
            (void) wimpt_complain(os_cli(a));
            /* which will pop up ca. 0.5 seconds later ... */
            }
        }

    if(!justopening  &&  wanttoclose)
        {
        close_window();
        if(is_current_document())
            draw_screen();
        }
}


/************************************************
*                                               *
* process redraw window request for main window *
*                                               *
************************************************/

static void
main_redraw_request(wimp_eventstr *e)
{
    os_error *bum;
    wimp_redrawstr redraw;
    intl redrawindex, vdlevel;

    tracef0("main_redraw_request()\n");

    redraw.w = e->data.o.w;

    /* wimp errors in redraw are fatal */
    bum = wimpt_complain(wimp_redraw_wind(&redraw, &redrawindex));

    if(TRACE  &&  !redrawindex)
        tracef0("no rectangles to redraw\n");

    vdlevel = riscos_visdelay_set(0);   /* make hourglass appear later than usual */
    vis_delay = 100;
    riscos_visdelay_set(vdlevel);
    vis_delay = -1;
    
    while(!bum  &&  redrawindex)
        {
        killcolourcache();

        graphics_window = *((coord_box *) &redraw.g);

        #if defined(ALWAYS_PAINT)
        paint_is_update = TRUE;
        #else
        paint_is_update = FALSE;
        #endif

        /* redraw area, not update */
        application_redraw((riscos_redrawstr *) &redraw);

        bum = wimpt_complain(wimp_get_rectangle(&redraw, &redrawindex));
        }
}
    

/********************************************
*                                           *
* process key pressed event for main window *
*                                           *
********************************************/

#define CTRL_H  ('H' & 0x1F)    /* Backspace key */
#define CTRL_M  ('M' & 0x1F)    /* Return/Enter keys */
#define CTRL_Z  ('Z' & 0x1F)    /* Last Ctrl-x key */

static void
main_key_pressed(wimp_eventstr *e)
{
    intl ch = e->data.key.chcode;
    intl kh;

    tracef1("main_key_pressed: key &%3.3X\n", ch);

    /* Translate key from Window manager into what PipeDream expects */

      if(ch == ESCAPE)
        kh = ch;
    elif(ch <= CTRL_Z)              /* RISC OS 'Hot Key' here we come */
        {
        /* Watch out for useful CtrlChars not produced by Ctrl-letter */
          if(((ch == CTRL_H)  ||  (ch == CTRL_M))  &&  !depressed_ctrl())
            kh = ch;
        else
            kh = (ch - 1 + 'A') | ALT_ADDED;    /* Uppercase Alt-letter */
        }
    else
        {
        if(ch & 0x100)
            kh = -(ch & 0xFF);              /* Fn keys become -ve */
        elif((alt_array[0] != '\0')  &&  isalpha(ch))
                                            /* already in Alt-sequence? */
            kh = toupper(ch) | ALT_ADDED;   /* Uppercase Alt-letter */
        else
            kh = ch;
        }

    if(!application_process_key(kh))
        {
        /* if unprocessed, send it back from whence it came */
        tracef1("main_key_pressed: unprocessed app_process_key(&%8X)\n", ch);
        wimpt_safe(wimp_processkey(ch));
        }
}


/********************************************
*                                           *
*  process message events for main window   *
*                                           *
********************************************/

static BOOL
main_message(wimp_msgstr *m)
{
    int action      = m->hdr.action;
    BOOL processed  = TRUE;

    switch(action)
        {
        case wimp_MDATASAVE:
            /* possible object dragged into main window - ask for DATALOAD */
            scraptransfer_file(&m->data.datasave);
            break;


        case wimp_MDATALOAD:
            /* object dragged into main window - insert regardless of type */
            {
            char *filename;
            intl filetype = xferrecv_checkinsert(&filename);
            /* sets up reply too */

            tracef1("main window got a DataLoad for %s\n", filename);

            switch(filetype)
                {
                case DIRECTORY_FILETYPE:
                case APPLICATION_FILETYPE:
                    reperr(ERR_ISADIR, filename);
                    break;


                case DRAW_FILETYPE:
                case SPRITE_FILETYPE:
                    {
                    char array[MAX_FILENAME];
                    intl prefix_len;
                    uchar t_options_IR;

                    /* try for minimal reference */
                    if(get_cwd(array))
                        {
                        prefix_len = strlen(array);

                        if(!strnicmp(filename, array, prefix_len))
                            filename += prefix_len;
                        }

                    filbuf();

                    if( insert_string("@G:", FALSE)     &&
                        insert_string(filename, FALSE)  &&
                        insert_string(",100@", FALSE)   )
                        {
                        t_options_IR = d_options_IR;
                        d_options_IR = 'N';             /* RJM is so pragmatic ... */
                        application_process_command(N_Return);
                        d_options_IR = t_options_IR;
                        }
                    }
                    break;


                case PDMACRO_FILETYPE:
                    do_execfile(filename);
                    break;


                default:
                    (void) riscos_LoadFile(filename, TRUE,
                                    !(xf_filealtered || xf_fileloaded), 'A');
                    break;
                }
            }

            xferrecv_insertfileok();
            break;


        case wimp_MHELPREQUEST:
            tracef0("Help request on main window\n");
            {
            int x = m->data.helprequest.m.x;
            int y = m->data.helprequest.m.y;
            window_data *wdp = find_document_using_window(caret_window);
            BOOL insertref = ((wdp != NULL)  &&  wdp->Xxf_inexpression);
            char  abuffer[256];
            char *buffer;
            coord tx    = tcoord_x(x);  /* text cell coordinates */
            coord ty    = tcoord_y(y);
            coord coff  = calcoff(tx);  /* not _click */
            coord roff  = calroff(ty);  /* not _click */
            coord o_roff = roff;
            rowt  trow;
            SCRROW *rptr;

            tracef4("get_slr_for_point: g(%d, %d) t(%d, %d)", x, y, tx, ty);

            /* stop us wandering off bottom of sheet */
            if(roff >= rowsonscreen)
                roff = rowsonscreen - 1;

            tracef1(" roff %d\n", roff);

            /* default message */
            strcpy(abuffer, help_main_window);
            buffer = abuffer + strlen(abuffer);
            strcpy(buffer, help_drag_file_to_insert);

            if(roff >= 0)
                {
                rptr = vertvec_entry(roff);

                if(rptr->flags & PAGE)
                    sprintf(buffer, "%s%s",
                            help_row_is_page_break,
                            help_drag_file_to_insert);
                else
                    {
                    trow = rptr->rowno;

                    if((coff >= 0)  ||  (coff == OFF_RIGHT))
                        {
                        colt tcol, scol;
                        char sbuf[32];
                        char abuf[32];
                        const char *msg = (insertref)
                                            ? help_insert_a_reference_to
                                            : help_position_the_caret_in;
                        /* if one of us but not this one is editing, give full ref */
                        docno d = (!insertref  ||  (caret_window == main_window))
                                            ? 0
                                            : ensure_cur_docno();

                        if( !insertref  &&
                            chkrpb(trow)  &&  chkfsb()  &&  chkpac(trow))
                            {
                            sprintf(buffer, "%s%s",
                                    help_row_is_hard_page_break,
                                    help_drag_file_to_insert);
                            }
                        else
                            {
                            tcol = col_number(coff);
                            tracef2("in sheet at row #%ld, col #%d\n", trow, tcol);
                            writeref(abuf, d, tcol, trow);

                            if(!insertref)
                                {
                                coff = get_column(tx, trow, 0, TRUE);
                                scol = col_number(coff);
                                tracef2("will position at row #%ld, col #%d\n", trow, scol);
                                writeref(sbuf, d, scol, trow);
                                }
                            else
                                scol = tcol;

                            sprintf(buffer,
                                    (scol != tcol)
                                        ? "%s%s%s%s%s%s%s%s%s%s%s"
                                        : "%s%s%s%.0s%.0s%.0s%.0s%.0s%s%s%s",
                                    help_click_select_to, msg, help_slot,
                                    /* else miss this set of args */
                                    sbuf, help_dot_cr,
                                    /* and this set of args */
                                    help_click_adjust_to, msg, help_slot,
                                    abuf, help_dot_cr,
                                    help_drag_file_to_insert);
                            }
                        }
                    elif(IN_ROW_BORDER(coff))
                        sprintf(buffer, "%s%s%s",
                                help_drag_row_border,
                                (o_roff == roff)
                                    ? help_double_row_border
                                    : (const char *) NULLSTR,
                                help_drag_file_to_insert);
                    else
                        tracef0("off left/right\n");
                    }
                }
            elif(IN_COLUMN_HEADING(roff))
                {
                if((coff >= 0)  ||  IN_ROW_BORDER(coff))
                    sprintf(buffer, "%s%s",
                            (coff >= 0)
                                ? help_col_border
                                : help_top_left_corner,
                            help_drag_file_to_insert);
                else
                    tracef0("off left/right\n");
                }
            elif(ty == EDTLIN)
                {
                if((tx >= 0)  &&  (tx < RHS_X)   &&  xf_inexpression)
                    sprintf(buffer, "%s%s%s%s",
                            help_click_select_to,
                            help_position_the_caret_in,
                            help_edit_line,
                            help_drag_file_to_insert);
                else
                    tracef0("not in valid part of edit line/not editing\n");
                }
            elif(ty == SLOTCOORDS_Y0)
                {
                assert(SLOTCOORDS_Y0 == NUMERICSLOT_Y0);

                if((tx >= SLOTCOORDS_X0)  &&  (tx < SLOTCOORDS_X1))
                    sprintf(buffer, "%s%s",
                            help_slot_coordinates,
                            help_drag_file_to_insert);
                elif((tx >= NUMERICSLOT_X0)  &&
                     (tx < NUMERICSLOT_X0 + numericslotcontents_length))
                    sprintf(buffer, "%s%s",
                            help_numeric_contents,
                            help_drag_file_to_insert);
                else
                    tracef0("not in slot coordinates/contents\n");
                }
            else
                tracef0("above sheet data\n");

            riscos_sendhelpreply(abuffer, m);
            }
            break;


        default:
            tracef1("unprocessed %s message to main window\n",
                     trace_wimp_message(m));
            processed = FALSE;
            break;
        }

    return(processed);
}


/*****************************
*                            *
* process main window events *
*                            *
*****************************/

static BOOL
main_event_handler(wimp_eventstr *e, void *handle)
{
    BOOL processed = TRUE;

    select_document_using_handle((dochandle) handle);

    tracef4("main_event_handler: event %s, dhan %d window %d, document &%p\n",
             trace_wimp_event(e), handle, (int) main_window, current_document());

    switch(e->e)
        {
        case wimp_EOPEN:    /* main window always opened as a pane on top of the fake window */
        case wimp_ECLOSE:   /* main window has no close box */
        case wimp_ESCROLL:  /* or scroll bars - come through fake */
        case wimp_EPTRLEAVE:
        case wimp_EPTRENTER:
            break;


        case wimp_EREDRAW:
            main_redraw_request(e);
            break;


        case wimp_EBUT:
            riscos_non_null_event();

            /* ignore old button state */
            application_button_click(e->data.but.m.x,
                                     e->data.but.m.y,
                                     e->data.but.m.bbits);
            break;


        case wimp_EKEY:
            riscos_non_null_event();
            main_key_pressed(e);
            break;


        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            riscos_non_null_event();
            main_message(&e->data.msg);
            break;


        case wimp_EGAINCARET:
            tracef2("GainCaret: new window %d icon %d",
                    e->data.c.w, e->data.c.i);
            tracef4(" x %d y %d height %8.8X index %d\n",
                    e->data.c.x, e->data.c.y,
                    e->data.c.height, e->data.c.index);
            caret_window = (riscos_window) e->data.c.w;
            break;


        case wimp_ELOSECARET:
            tracef2("LoseCaret: old window %d icon %d",
                    e->data.c.w, e->data.c.i);
            tracef4(" x %d y %d height %X index %d\n",
                    e->data.c.x, e->data.c.y,
                    e->data.c.height, e->data.c.index);

            /* cancel Alt sequence */
            alt_array[0] = '\0';

            /* don't cancel key expansion or else user won't be able to
             * set 'Next window', 'other action' etc. on keys
            */
            (void) mergebuf_nocheck();
            filbuf();

            caret_window = window_NULL;
            break;


        default:
            riscos_non_null_event();
            tracef1("unprocessed wimp event to main window: %s\n",
                    trace_wimp_event(e));
            processed = FALSE;
            break;
        }

    return(processed);
}


static BOOL
fake_event_handler(wimp_eventstr *e, void *handle)
{
    BOOL processed = TRUE;

    select_document_using_handle((dochandle) handle);

    tracef4("fake_event_handler: event %s, dhan %d window %d, document &%p\n",
             trace_wimp_event(e), handle, (int) fake_window, current_document());

    switch(e->e)
        {
        case wimp_EPTRLEAVE:
        case wimp_EPTRENTER:
            break;

        case wimp_EOPEN:
            fake_open_request(e);
            break;

        case wimp_ECLOSE:
            fake_close_request(e);
            break;

        case wimp_ESCROLL:
            fake_scroll_request(e);
            break;

    /*  case wimp_EREDRAW:  */
        default:
            riscos_non_null_event();
            tracef1("unprocessed wimp event to fake window: %s\n",
                    trace_wimp_event(e));
            processed = FALSE;
            break;
        }

    return(processed);
}


/****************************************************************************
*                                                                           *
*                           exported functions                              *
*                                                                           *
****************************************************************************/

/********************
*                   *
* clean up a string *
*                   *
********************/

extern char *
riscos_cleanupstring(char *str)
{
    char *a = str;

    while(*a++ >= 0x20)
        ;

    a[-1] = '\0';

    return(str);
}


/****************************************
*                                       *
* if window not already in existence    *
* in this domain then make a new window *
*                                       *
****************************************/

extern BOOL
riscos_createmainwindow(void)
{
    intl new_y;
    dochandle doc = current_document_handle();
    char *errorp;

    tracef0("riscos_createmainwindow()\n");

    if(fake_window == window_NULL) /* a window needs creating? */
        {
        fake_dbox = dbox_new(fake_dboxname, &errorp);
        if(!fake_dbox)
            {
            if(errorp)
                rep_fserr(errorp);

            return(FALSE);
            }

        fake_window = dbox_syshandle(fake_dbox);
        tracef1("created fake window %d\n", fake_window);


        /* register event & MENU selection procedures for this window */

        tracef1("calling win_register_event_handler(%d)\n", fake_window);
        if(!win_register_event_handler(fake__window, fake_event_handler,
                                            (void *) doc))
            return(FALSE);

        riscos_settitlebar(currentfilename);

        /* Bump the y coordinate for the next window to be created */
        /* Try not to overlap the icon bar */
        new_y = main_window_definition->box.y0 - main_window_y_bump;
        if(new_y <= (iconbar_height + hscroll_height))
            new_y = main_window_initial_y0;
        main_window_definition->box.y0 = new_y;
        main_window_definition->box.y1 = new_y + main_window_default_height;

        /* get extent and scroll offsets set correctly on initial open */
        out_alteredstate = TRUE;
        }

    if(main_window == window_NULL) /* a window needs creating? */
        {
        main_dbox = dbox_new(main_dboxname, &errorp);
        if(!main_dbox)
            {
            if(errorp)
                rep_fserr(errorp);

            return(FALSE);
            }

        main_window = dbox_syshandle(main_dbox);
        tracef1("created main window %d\n", main_window);


        /* register event & MENU selection procedures for this window */

        tracef1("calling win_register_event_handler(%d)\n", main_window);
        if(!win_register_event_handler(main__window, main_event_handler,
                                            (void *) doc))
            return(FALSE);

        riscmenu_attachmenutree();

        /* now window created at default size, initialise screen bits */
        if(!screen_initialise())
            return(FALSE);
        }

    riscos_frontmainwindow(FALSE);

    return(TRUE);
}


/***********************************
*                                  *
* destroy this domain's mainwindow *
*                                  *
***********************************/

extern void
riscos_destroymainwindow(void)
{
    tracef0("riscos_destroymainwindow()\n");

    if(fake_window != window_NULL)
        {
        /* deregister procedures for the fake window */

        tracef1("calling win_deregister_event_handler(%d)\n", fake_window);
        win_register_event_handler(fake__window, NULL, NULL);

        dbox_dispose((dbox *) &fake_dbox);
        fake_window = window_NULL;
        }

    if(main_window != window_NULL)
        {
        /* deregister procedures for the main window */

        tracef1("calling win_deregister_event_handler(%d)\n", main_window);
        win_register_event_handler(main__window, NULL, NULL);

        riscmenu_detachmenutree();

        dbox_dispose((dbox *) &main_dbox);
        main_window = window_NULL;
        }
}


/*************************************
*                                    *
* RISC OS module domain finalisation *
*                                    *
*************************************/

extern void
riscos_finalise(void)
{
    tracef0("riscos_finalise()\n");

    riscos_destroymainwindow();
}


/************************************
*                                   *
* RISC OS module final finalisation *
*                                   *
************************************/

extern void
riscos_finalise_once(void)
{
    tracef0("riscos_finalise_once()\n");

    if(riscos__initialised)
        {
        riscos__initialised = FALSE;

        wrch_undefinefunnies();

        riscos_visdelay_stop();
        }
}


/*********************************
*                                *
* bring main window to the front *
*                                *
*********************************/

extern void
riscos_frontmainwindow(BOOL immediate)
{
    tracef1("[riscos_frontmainwindow(immediate = %s)]\n", trace_boolstring(immediate));

    if(main_window != window_NULL)
        {
        xf_frontmainwindow = FALSE;                 /* as immediate event raising calls draw_screen() */
        dbox_sendfront(fake__dbox, immediate);
        }
    else
        tracef0("--- no main window!");
}


extern intl
riscos_getbuttonstate(void)
{
    wimp_mousestr m;
    wimpt_safe(wimp_get_point_info(&m));
    return(m.bbits);
}


extern BOOL
riscos_adjustclicked(void)
{
    return(riscos_getbuttonstate() & wimp_BRIGHT);
}


/*****************************
*                            *
*  execute RISC OS module    *
*                            *
*****************************/

extern intl
riscos_go(void)
{
    tracef0("riscos_go()\n");

/* Loop getting events until we are told to curl up and die */

    while(TRUE)
        event_process();

    return(EXIT_SUCCESS);
}


/***************************************
*                                      *
* RISC OS module domain initialisation *
*                                      *
***************************************/

extern BOOL
riscos_initialise(void)
{
    BOOL ok;

    ok = riscos_createmainwindow();

    return(ok);
}


/****************************
*                           *
* initialise RISC OS module *
*                           *
****************************/

#define MIN_X (8 * charwidth)
#define MIN_Y (5 * (charheight + 2*4))

extern void
riscos_initialise_once(void)
{
    tracef0("riscos_initialise_once()\n");

    res_init(applicationname);  /* Resources are in <applicationname$Dir> */
    resspr_init();

    wimpt_init(applicationname);

    #if defined(BACKTRACE)
    if(trace_enabled)
        wimpt_backtrace_on_exit(TRUE);
    #endif

    tracef0("calling dbox_init\n");
    dbox_init();

    tracef0("calling Draw_init\n");
    draw_registerMemoryFunctions(flex_alloc, flex_extend, flex_free);

    /* now get a handle onto the definition after the dbox is loaded */

    main_window_definition = (wimp_wind *) dbox_syswindowptr(main_dboxname);
    main_window_definition->minsize = (MIN_Y << 16) + (MIN_X << 0);
    main_window_definition->colours[wimp_WCWKAREABACK] = 0xFF;


    main_window_definition = (wimp_wind *) dbox_syswindowptr(fake_dboxname);
    main_window_definition->minsize = (MIN_Y << 16) + (MIN_X << 0);
    main_window_definition->colours[wimp_WCWKAREABACK] = 0xFF;

    main_window_initial_y0     = main_window_definition->box.y0;
    main_window_default_height = main_window_definition->box.y1 -
                                 main_window_initial_y0;


    iconbar_initialise(applicationname);

    /* register a callproc for wimp exits */
    wimpt_atentry(actions_before_entering);
    wimpt_atexit(actions_before_exiting);

    /* Direct miscellaneous unknown (and icon bar)
     * events to our default event handler
    */
    win_register_event_handler(win_ICONBARLOAD, default_event_handler, NULL);
    win_claim_unknown_events(win_ICONBARLOAD);

    /* initialise riscmenu */
    riscmenu_initialise_once();

    /* initialise riscdraw */
    cachemodevariables();
    cachepalettevariables();

    riscprint_set_printer_data();

    riscos__initialised = TRUE;
}


/********************************
*                               *
* invalidate all of main window *
*                               *
********************************/

extern void
riscos_invalidatemainwindow(void)
{
    wimp_redrawstr redraw;

    redraw.w      = main__window;
    redraw.box.x0 = -0x07FFFFFF;
    redraw.box.y0 = -0x07FFFFFF;
    redraw.box.x1 = +0x07FFFFFF;
    redraw.box.y1 = +0x07FFFFFF;

    wimpt_safe(wimp_force_redraw(&redraw));
}


/***************************************************
*                                                  *
* derive a filename to use given the current state *
*                                                  *
***************************************************/

extern const char *
riscos_obtainfilename(const char *filename, BOOL forsaveop)
{
    IGNOREPARM(forsaveop);

    if(filename == NULL)
        return("<untitled>");
    else
        return(filename);
}


/************************************
*                                   *
*  read state of RAM fetch buffer   *
*                                   *
************************************/

extern void
riscos_rambufferinfo(char **bufferp, intl *sizep)
{
    *bufferp = NULL;
    *sizep   = 0;

    tracef2("riscos_bufferinfo returns &%p, size %d\n", *bufferp, *sizep);
}


/************************************************
*                                               *
*  read info from a file into a fileinfo block  *
*                                               *
************************************************/

extern BOOL
riscos_readfileinfo(riscos_fileinfo *rip /*out*/, const char *name)
{
    os_filestr fileblk;
    BOOL res;

    fileblk.action = OSFile_ReadNoPath;
    fileblk.name   = name;

    res = !wimpt_complain(os_file(&fileblk))  &&  (fileblk.action == 1);

    if(!res)
        {
        riscos_readtime(rip);
        riscos_settype(rip, PIPEDREAM_FILETYPE);
        rip->length = 0;
        }
    else
        {
        rip->exec   = fileblk.execaddr;
        rip->load   = fileblk.loadaddr;
        rip->length = fileblk.start;
        }

    return(res);
}


/********************************
*                               *
* read the current time from OS *
* into a fileinfo block         *
*                               *
********************************/

extern void
riscos_readtime(riscos_fileinfo *rip /*inout*/)
{
    rip->exec = 3;           /* cs since 1900 */
    (void) os_word(14, rip); /* sets exec and LSB of load */
}


/************************************
*                                   *
*  start windows from one up again  *
*                                   *
************************************/

extern void
riscos_resetwindowpos(void)
{
    if(main_window_definition->box.y0 != main_window_initial_y0)
        {
        plusab(main_window_definition->box.y0, main_window_y_bump);
        plusab(main_window_definition->box.y1, main_window_y_bump);
        }
}


/****************************************************
*                                                   *
* restore the current window to the saved position  *
*                                                   *
****************************************************/

extern void
riscos_restorecurrentwindowpos(void)
{
    wimp_eventdata ed;

    ed.o = saved_window_pos;

    /* send myself an open request for this new window */
    ed.o.w      = (wimp_w) fake_window;
    ed.o.scx    = 0;
    ed.o.scy    = 0;
    wimpt_safe(wimp_sendwmessage(wimp_EOPEN, (wimp_msgstr *) &ed,
                                 ed.o.w, -1));
}


/****************************************
*                                       *
*  save the current window's position   *
*                                       *
****************************************/

extern void
riscos_savecurrentwindowpos(void)
{
    wimp_eventdata ed;

    /* try to preserve the window coordinates */
    wimpt_safe(wimp_get_wind_state((wimp_w) fake_window,
                                   (wimp_wstate *) &ed.o));

    saved_window_pos = ed.o;
}


/********************
*                   *
* send a help reply *
*                   *
********************/

extern void
riscos_sendhelpreply(const char *msg, void *v)
{
    wimp_msgstr *m = (wimp_msgstr *) v;
    intl size;

    if(m->data.helprequest.m.i >= -1)
        {
        size = offsetof(wimp_msgstr, data.helpreply.text) + strlen(msg) + 1;

        m->hdr.size     = size;
        m->hdr.your_ref = m->hdr.my_ref;
        m->hdr.action   = wimp_MHELPREPLY;

        tracef2("helpreply is %d long, %s\n", size, msg);

        if(!RELEASED  &&  (size > 256))
            reperr_fatal("help message \"%.128s...\" too big to send\n", msg);

        strcpy(m->data.helpreply.text, msg);

        wimpt_send_message(wimp_ESEND, m, m->hdr.task);
        }
    else
        tracef0("no reply for system icons\n");
}


/***************************************************
*                                                  *
* Set title bar of main window given current state *
*                                                  *
***************************************************/

extern void
riscos_settitlebar(const char *filename)
{
    const char *documentname = riscos_obtainfilename(filename, FALSE);
    char *str;
    size_t count;
    char winfo[sizeof(wimp_winfo) + 50*sizeof(wimp_icon)]; /*BPDM!*/
    wimp_winfo *win = (wimp_winfo *) &winfo;

    tracef2("riscos_settitlebar(%s): fake_window %d\n", documentname, fake_window);

    if(fake_window != window_NULL)
        {
        win->w = fake__window;

        if(!wimpt_safe(wimp_get_wind_info(win)))
            {
            /* carefully copy information into the title string */
            str   = win->info.title.indirecttext.buffer;
            count = win->info.title.indirecttext.bufflen;
            *str  = '\0';
            strncatind(str, applicationname, &count);
            strncatind(str, ": ",            &count);
            strncatind(str, documentname,    &count);
            if(xf_filealtered)
                strncatind(str, " *", &count);
            #if defined(TRACE_DOC)
                {
                char buffer[256];
                sprintf(buffer, " (dochan %d)", current_document_handle());
                strncatind(str, buffer, &count);
                }
            #endif
            tracef1("poked main document title to be '%s'\n", str);

            if(win->info.flags & wimp_WOPEN)
                {
                wimp_redrawstr redraw;
                redraw.w = -1;
                redraw.box.x0 = win->info.box.x0;
                redraw.box.y0 = win->info.box.y1 + dy;
                redraw.box.x1 = win->info.box.x1;
                redraw.box.y1 = redraw.box.y0 + title_height - 2*dy;
                tracef0("forcing global redraw of inside of title bar area\n");
                wimpt_safe(wimp_force_redraw(&redraw));
                }
            }
        }
}


/*****************************
*                            *
* set the type of a fileinfo *
*                            *
*****************************/

extern void
riscos_settype(riscos_fileinfo *rip /*inout*/, int filetype)
{
    rip->load = (rip->load & 0x000000FF) | (filetype << 8) | 0xFFF00000;
}


extern void
riscos_non_null_event(void)
{
    last_event_was_null = FALSE;
}


/****************************************
*                                       *
* invalidate part of main window and    *
* call application back to redraw it    *
* NB. this takes offsets from xorg/yorg *
*                                       *
****************************************/

extern void
riscos_updatearea(riscos_redrawproc redrawproc, riscos_window w,
                  int x0, int y0, int x1, int y1)
{
    os_error *bum;
    wimp_redrawstr redraw;
    intl redrawindex;

    tracef6("riscos_updatearea(%s, %d, %d, %d, %d, %d)\n",
             trace_procedure_name((trace_proc) redrawproc), w, x0, y0, x1, y1);

    /* RISC OS graphics primitives all need coordinates limited to s16 */
    x0 = max(x0, SHRT_MIN);
    y0 = max(y0, SHRT_MIN);
    x1 = min(x1, SHRT_MAX);
    y1 = min(y1, SHRT_MAX);

    redraw.w      = (wimp_w) w;
    redraw.box.x0 = x0;
    redraw.box.y0 = y0;
    redraw.box.x1 = x1;
    redraw.box.y1 = y1;

    /* wimp errors in update are fatal */
    bum = wimpt_complain(wimp_update_wind(&redraw, &redrawindex));

    if(TRACE  &&  !redrawindex)
        tracef0("no rectangles to update\n");

    while(!bum  &&  redrawindex)
        {
        killcolourcache();

        graphics_window = *((coord_box *) &redraw.g);

        #if !defined(ALWAYS_PAINT)
        paint_is_update = TRUE;
        #endif

        /* we are updating area */
        redrawproc((riscos_redrawstr *) &redraw);

        bum = wimpt_complain(wimp_get_rectangle(&redraw, &redrawindex));
        }
}


/****************************
*                           *
* maintain hourglass state  *
*                           *
****************************/

extern intl
riscos_visdelay_set(intl level)
{
    intl oldlevel = vis_state;

    while(level != vis_state)
        if(level > vis_state)
            riscos_visdelay_on();
        else
            riscos_visdelay_off();

    return(oldlevel);
}


extern void
riscos_visdelay_on(void)
{
    tracef2("riscos_visdelay_on(): vis_state = %d -> %d\n",
            vis_state, vis_state+1);

    if(vis_state++ == 0)
        {
        if(vis_delay != -1)
            visdelay_beginafter(vis_delay);
        else
            visdelay_begin();
        }
}


extern void
riscos_visdelay_off(void)
{
    tracef2("riscos_visdelay_off(): vis_state = %d -> %d\n",
            vis_state, vis_state-1);

    /* note that -ve state is quite reasonable temporarily */
    if(--vis_state == 0)
        visdelay_end();
}


extern void
riscos_visdelay_stop(void)
{
    if(vis_state > 0)
        visdelay_end();
}


/********************************************
*                                           *
*  write a fileinfo block data onto a file  *
*                                           *
********************************************/

extern void
riscos_writefileinfo(const riscos_fileinfo *rip, const char *name)
{
    os_filestr fileblk;

    fileblk.action      = 2;                 /* OSFile_WriteLoad */
    fileblk.name        = name;
    fileblk.loadaddr    = rip->load;

    if(!wimpt_complain(os_file(&fileblk)))
        {
        fileblk.action      = 3;            /* OSFile_WriteExec */
        fileblk.execaddr    = rip->exec;
        (void) wimpt_complain(os_file(&fileblk));
        }
}

/* end of riscos.c */
