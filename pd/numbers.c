/* numbers.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       numbers.c - module that handles spreadsheet bits
 * Author:      RJM September 1987
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "sprite.h"
#include "bbc.h"
#include "drawfdiag.h"
#include "drawfobj.h"
#endif

#include "datafmt.h"

#if RISCOS
#include "ext.riscos"
#include "riscdialog.h"
#include "visdelay.h"
#endif


/* exported functions */

extern BOOL actind(intl message, intl number);
extern void actind_end(void);
extern intl compile_text_slot(char *array /*out*/, const char *from, uchar *slot_refsp /*out*/);
extern intl cplent(uchar *oprstb, intl hash_to_dollar);
extern void draw_close_file(dochandle han);
extern intl draw_find_file(dochandle han, colt col, rowt row,
                           drawfep *drawfile, drawfrp *drawref);
extern void draw_recache_file(const char *drawfilename);
extern void draw_removeblock(dochandle han, colt scol, rowt srow, colt ecol, rowt erow);
extern void draw_removeslot(colt col, rowt row);
extern LIST *draw_search_cache(char *name, intl namlen, word32 *maxkey);
extern intl draw_str_insertslot(colt col, rowt row);
extern void draw_swaprefs(rowt row1, rowt row2, colt scol, colt ecol);
extern void draw_tidy_up(void);
extern intl draw_tree_str_insertslot(colt col, rowt row, intl sort);
extern void draw_tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow);
extern void draw_tree_removeslot(colt col, rowt row);
extern void draw_updref(colt mrksco, rowt mrksro,
                        colt mrkeco, rowt mrkero,
                        colt coldiff, rowt rowdiff);
extern void endeex(void);
extern void endeex_nodraw(void);
extern void filbuf(void);
extern colt getcol(void);
extern BOOL graph_active_present(dochandle han);
extern ghandle graph_add_entry(ghandle ghan, dochandle han, colt col, rowt row,
                            intl xsize, intl ysize, const char *leaf, const char *tag,
                            intl task);
extern void graph_close_file(dochandle han);
extern void graph_draw_tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow);
extern void graph_draw_tree_removeslot(colt col, rowt row);
extern void graph_removeblock(dochandle han, colt scol, rowt srow, colt ecol, rowt erow);
extern void graph_remove_entry(ghandle ghan);
extern void graph_removeslot(colt col, rowt row);
extern graphlinkp graph_search_list(ghandle ghan);
extern void graph_send_block(colt scol, rowt srow, colt ecol, rowt erow);
extern void graph_send_xblock(colt scol, rowt srow, colt ecol, rowt erow, dochandle han);
extern void graph_send_slot(colt col, rowt row);
extern void graph_send_split_blocks(colt scol, rowt srow, colt ecol, rowt erow);
extern void graph_updref(colt mrksco, rowt mrksro,
                         colt mrkeco, rowt mrkero,
                         colt coldiff, rowt rowdiff);
extern intl is_draw_file_slot(dochandle han, colt col, rowt row);
extern void mark_slot(slotp tslot);
extern void merexp(void);
extern BOOL mergebuf(void);
extern BOOL mergebuf_nocheck(void);
extern BOOL merst1(colt tcol, rowt trow);
extern void prccon(uchar *target, slotp ptr);
extern intl readfxy(intl id, char **pfrom, char **pto, char **name,
                    double *xp, double *yp);
extern void seteex(void);
extern void seteex_nodraw(void);
extern void splat(uchar *to, word32 num, intl size);
extern word32 talps(uchar *from, intl size);
extern intl writecol(uchar *array, colt col);
extern intl writeref(uchar *ptr, docno doc, colt tcol, rowt trow);


/* internal functions */

/*static void bash_slots_about_a_bit(BOOL ent);*/
/*static intl draw_add_file_ref(word32 key, dochandle han, colt col, rowt row,
                              double xp, double yp);*/
static void draw_adjust_file_ref(drawfep dfp, drawfrp dfrp);
/*static void draw_adjust_file_refs(drawfep dfp, word32 key);*/
/*static intl draw_cache_file(char *name, intl namlen, BOOL adjust_refs,
                            double xp, double yp, colt col, rowt row);*/
static BOOL slot_ref_here(const char *ptr);


/* ------------------------------------------------------------------------- */

/************************************************
*                                               *
* Activity indicator                            *
* Every second switch on or off inverse message *
*                                               *
************************************************/

#if MS || ARTHUR

static struct activ_mess
    {
    char *ptr;
    intl length;
    } activity_message[] =
    {
    { "Loading",        3   },
    { "Saving",         3   },
    { "Sorting",        5   },
    { "Calculating",    5   },
    { "Copying",        0   },
    { "Printing",       5   }
#if !defined(SPELL_OFF)
,   { "Checking",       5   }
#endif
,   { "Bashing",        0   }
    };

#endif


extern BOOL
actind(intl message, intl number)
{
#if RISCOS
    static intl lastpercent = -1;
#elif MS || ARTHUR
    static intl lastpercent = 0;
    static BOOL notdrawnyet = TRUE;
    static intl taglength;
    static coord xpos, ypos, xsize, ysize;
    static time_t lasttime = (time_t) 0;
#endif

    tracef2("actind(%d, %d)\n", message, number);

#if MS || ARTHUR
    if(!allow_output)
        return(TRUE);
#endif


    /* time to go home? */

    if( ctrlflag  ||  (message == DEACTIVATE)
#if ARTHUR
        || depressed(DEPRESSED_ESCAPE)
#endif
        )
        {
        if(activity_indicator)
            {
            /* switch off and reset statics */
            activity_indicator = FALSE;
#if RISCOS
            #if defined(DO_VISDELAY_BY_STEAM)
            tracef0("visdelay_end()\n");
            riscos_visdelay_off();
            #endif
            lastpercent = -1;

#elif MS || ARTHUR

            lastpercent = 0;
            notdrawnyet = TRUE;
            lasttime = 0;
            clip_menu(0,0,0,0);
#endif
            }

        return(FALSE);
        }


#if MS || ARTHUR
    /* first time in for this operation */
    if(lasttime == 0)
        {
        /* remember time of first call */
        lasttime = time(NULL);
        ack_esc();
        activity_indicator = TRUE;
        return(TRUE);
        }
#endif


    if( number > 99)
        number = 99;

    /* don't bother with 0% change, saves calling time() a bit */
    if(number == lastpercent)
        return(TRUE);


#if RISCOS
    if(!activity_indicator)
        {
        #if defined(DO_VISDELAY_BY_STEAM)
        tracef0("visdelay_begin()\n");
        riscos_visdelay_on();
        #endif
        activity_indicator = TRUE;
        return(TRUE);
        }

#elif MS || ARTHUR

    if(notdrawnyet)
        {
        intl i;

        /* don't bother for first second cos it may not take long */
        if(difftime(time(NULL), lasttime) < (double) 1)
            return(TRUE);

        taglength = strlen(activity_message[message].ptr);

        xsize = 7 + taglength + 5;
        ysize = 5;
        xpos = (pagwid - xsize) / 2;
        ypos = (paghyt - ysize) / 2 + 1;

        save_screen(xpos, ypos, xsize, ysize);
        clip_menu(xpos, ypos, xsize, ysize);
        setcolour(MENU_HOT, MENU_BACK);
        my_rectangle(xpos, ypos, xsize, ysize);

        setcolour(MENU_BACK, MENU_BACK);
        for(i = ypos + 1; i <= ypos + 3; i++)
            {
            at(xpos + 1, i);
            ospca(xsize - 2);
            }
        at(xpos + 3, ypos + 2);
        setcolour(MENU_FORE, MENU_BACK);
        stringout((uchar *) activity_message[message].ptr);

        notdrawnyet = FALSE;
        }
#endif


    /* is there anything to output after the heading */

    if(number != NO_ACTIVITY)
        {
#if RISCOS
        lastpercent = number;

        tracef1("visdelay_percent(%d)\n", number);
        visdelay_percent(number);

#elif MS || ARTHUR

        uchar array[20];

        lastpercent = number;

        at(xpos+taglength+4, ypos+2);
        sprintf((char *) array, "%d%%%  ", number);
        setcolour(MENU_FORE, MENU_BACK);
        stringout(array);
#endif
        }

    sb_show_if_fastdraw();

    return(TRUE);
}


extern void
actind_end(void)
{
    (void) actind(DEACTIVATE, NO_ACTIVITY);
}


/********************************************************************
*                                                                   *
*  this routine does the work for ExchangeNumbersText and Snapshot  *
*                                                                   *
********************************************************************/

static void
bash_slots_about_a_bit(BOOL ent)
{
    SLR   oldpos;
    uchar oldformat;
    uchar dateformat = d_options_DF;
    uchar thousands  = d_options_TH;
    slotp tslot;

    if(!mergebuf_nocheck())
        return;

    oldpos.col = curcol;
    oldpos.row = currow;

    init_marked_block();

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        actind(ACT_BASHING, percent_in_block(DOWN_COLUMNS));

        if(tslot->type == SL_PAGE)
            continue;

        d_options_DF = 'E';
        d_options_TH = TH_BLANK;

        if(!ent)
            switch(tslot->type)
                {
                case SL_TEXT:
                    d_options_DF = dateformat;
                    d_options_TH = thousands;
                    break;

                case SL_DATE:
                case SL_NUMBER:
                case SL_STRVAL:
                case SL_INTSTR:
                    break;

                default:
                    continue;   /* enough to confuse SKS for a while! */
                }

        curcol = in_block.col;
        currow = in_block.row;

        /* decompile into buffer */

        if(ent)
            {
            if(tslot->type == SL_TEXT)
                xf_inexpression = TRUE;

            prccon(linbuf, tslot);
            }
        else
            {
            switch(tslot->type)
                {
                case SL_TEXT:
                    break;

                default:
                    xf_inexpression = TRUE;

                case SL_STRVAL:
                case SL_INTSTR:
                    /* poke the minus/brackets flag to minus */
                    oldformat = tslot->content.number.format;
                    tslot->content.number.format = F_DCPSID | F_DCP;
                    break;
                }

            expand_slot(tslot, in_block.row, linbuf, LIN_BUFSIZ /*pagwid_plus1*/, TRUE, FALSE, FALSE);
            }

        slot_in_buffer = buffer_altered = TRUE;

        /* compile into slot */

        if(xf_inexpression)
            {
            seteex_nodraw();
            merexp();
            endeex_nodraw();

            if(!ent)
                {
                tslot = travel_in_block();
                if(!tslot)
                    break;

                if(tslot->type != SL_TEXT)
                    /* reset format in new slot */
                    tslot->content.number.format = oldformat;
                }
            }
        elif(!mergebuf_nocheck())
            goto EXIT_POINT;
        }

    d_options_DF = dateformat;
    d_options_TH = thousands;

    out_screen = recalc_bit = TRUE;
    curcol = oldpos.col;
    currow = oldpos.row;
    movement = 0;

EXIT_POINT:

    actind_end();
}


/************************************************************************
*                                                                       *
* compile from linbuf into array converting @ fields to slot references *
* return length (including terminating NULL) and set refs_in_this_slot  *
*                                                                       *
************************************************************************/

extern intl
compile_text_slot(char *array /*out*/, const char *from, uchar *slot_refsp /*out*/)
{
    char *to = array;

    tracef0("[compile_text_slot]\n");

    if(slot_refsp)
        *slot_refsp = 0;

    while(*from)
        {
        if((*to++ = *from++) != '@')
            continue;

        /* if block of @s just write them and continue */
        if(*from == '@')
            {
            while(*from == '@')
                {
                *to++ = '@';
                from++;
                }

            continue;
            }

        /* if it's a slot ref deal with it */
        if(slot_ref_here(from))
            {
            colt tcol;
            rowt trow;
            BOOL abscol = FALSE, absrow = FALSE;
            docno doc;

            if(slot_refsp)
                *slot_refsp = SL_REFS;

            *to++ = SLRLDI;

            from += read_docname((uchar *) from, &doc);
            splat(to, (word32) doc, sizeof(docno));
            to += sizeof(docno);

            /* set up getcol and getsbd */
            buff_sofar = (uchar *) from;
            if(*buff_sofar == '$')
                {
                abscol = TRUE;
                buff_sofar++;
                }

            tcol = getcol();

            if(abscol)
                force_abs_col(tcol);

            splat(to, (word32) tcol, sizeof(colt));
            to += sizeof(colt);

            if(*buff_sofar == '$')
                {
                absrow = TRUE;
                buff_sofar++;
                }

            trow = getsbd() - 1;
            if(absrow)
                force_abs_row(trow);

            splat(to, (word32) trow, sizeof(rowt));
            to += sizeof(rowt);

            from = buff_sofar;

            while(*from == '@')
                {
                *to++ = '@';
                from++;
                }

            continue;
            }

        /* check for any other at field */
        #if !RISCOS
        if(strchr("DTPdtp", *from))
        #else
        if(strchr("DTPFGdtpfg", *from))
        #endif
            {
            /* copy across the @ field contents */
            while(*from && *from != '@')
                *to++ = *from++;

            /* if an @ field, write out subsequent @s so they
            don't get confused with anything following */
            while(*from == '@')
                *to++ = *from++;

            continue;
            }
        }

    *to = '\0';
    return((to - array) + 1);
}


/**********************************************************
*                                                         *
* compile a slot into internal form                       *
*                                                         *
* MRJC added hash_to_dollar 21/6/89 - to convert          *
* hashes to dollars when compiling a selection expression *
*                                                         *
**********************************************************/

extern intl
cplent(uchar *oprstb /*out*/, intl hash_to_dollar)
{
    BOOL refs;
    intl len;
    char *i, *o;
    intl instring;

    if(hash_to_dollar)
        {
        /* swap around the hashes and dollars */
        i = o = (char *) linbuf;
        instring = 0;
        do  {
            switch(*i)
                {
                case '\'':
                case '\"':
                    instring ^= 1;
                    *o++ = *i;
                    break;

                case '#':
                    if(instring)
                        *o++ = *i;
                    else
                        *o++ = '$';
                    break;

                case '$':
                    if(instring)
                        *o++ = *i;
                    break;
    
                default:
                    *o++ = *i;
                    break;
                }
            }
        while(*i++);
        }

    len = exp_compile(oprstb, (char *) linbuf, &refs, COMPILE_BUFSIZ);

    refs_in_this_slot = refs ? SL_REFS : 0;

    if(refs_in_this_slot)
        refs_in_this_sheet = SL_REFS;

    return(len);
}


/************************************************
*                                               *
* add an entry to the draw file references list *
*                                               *
************************************************/

#if RISCOS

static intl
draw_add_file_ref(word32 key, dochandle han, colt col, rowt row,
                  double xp, double yp)
{
    LIST *lptr;
    drawfep dfp;
    drawfrp dfrp;
    intl res;

    /* search for a duplicate entry */
    for(lptr = first_in_list(&draw_file_refs);
        lptr;
        lptr = next_in_list(&draw_file_refs))
        {
        dfrp = (drawfrp) lptr->value;

        if((han == dfrp->han)  &&  (col == dfrp->col)  &&  (row == dfrp->row))
            break;
        }

    if(!lptr)
        {
        lptr = add_list_entry(&draw_file_refs, sizeof(struct draw_file_ref), &res);

        if(!lptr)
            return(res);

        lptr->key = 0;
        dfrp = (drawfrp) lptr->value;
        dfrp->draw_file_key = key;
        dfrp->han = han;
        dfrp->col = col;
        dfrp->row = row;
        }

    /* check for range and load factors */
    if((xp > SHRT_MAX)  ||  (xp < 1.))
        xp = 100.;
    if((yp > SHRT_MAX)  ||  (yp < 1.))
        yp = 100.;

    dfrp->xfactor = xp / 100.;
    dfrp->yfactor = yp / 100.;

    /* load pointer after possible alloc */
    dfp = (drawfep) search_list(&draw_file_list, key)->value;

    draw_adjust_file_ref(dfp, dfrp);

    return(1);
}

#endif


/********************************************
*                                           *
* adjust a draw file reference to reflect a *
* new draw file entry                       *
*                                           *
********************************************/

#if RISCOS

static void
draw_adjust_file_ref(drawfep dfp, drawfrp dfrp)
{
    draw_box box;
    window_data *wdp;
    dochandle cdoc;
    coord coff, roff;
    SCRCOL *cptr;
    SCRROW *rptr;
    colt tcol;
    rowt trow;
    drawfep sch_dfp;

    if(!dfp->error)
        {
        /* ensure data pointer valid */
        dfp->diag.data = list_getptr(dfp->memoryh);

        /* get box in OS units */
        draw_queryBox((draw_diag *) &dfp->diag, &box, TRUE);

        /* save size in OS units */
        dfrp->xsize_os = (intl) (((double) (box.x1 - box.x0)) * dfrp->xfactor);
        dfrp->ysize_os = (intl) (((double) (box.y1 - box.y0)) * dfrp->yfactor);

        tracef4("[draw_adjust_file_ref: bbox of file is %d %d %d %d (os)]\n",
                box.x0, box.y0, box.x1, box.y1);
        #if TRACE
        trace_system("memory %p + 180", dfp->diag);
        #endif

        /* scale sizes */
        tracef2("[draw_adjust_file_ref: scaled sizes x: %d, y: %d (os)]\n",
                dfrp->xsize_os, dfrp->ysize_os);
        }

    /* a certain amount of redrawing may be necessary */
    wdp = find_document_using_handle(dfrp->han);
    assert(wdp  &&  (wdp != NO_DOCUMENT));
    if(wdp->Xpict_on_screen)
        {
        cdoc = current_document_handle();

        select_document(wdp);

        rptr = vertvec();

        for(roff = 0; roff < rowsonscreen; roff++, rptr++)
            {
            if(rptr->flags & PICT)
                {
                trow = rptr->rowno;

                cptr = horzvec();

                for(coff = 0; !(cptr->flags & LAST); coff++, cptr++)
                    {
                    tcol = cptr->colno;

                    if(draw_find_file(current_document_handle(), tcol, trow, &sch_dfp, NULL))
                        {
                        tracef3("found picture at col %d, row %d, dfp &%p\n", tcol, trow, sch_dfp);

                        if(dfp == sch_dfp)
                            {
                            wdp->Xxf_interrupted = wdp->Xout_screen = TRUE;
                            break;
                            }
                        }
                    }
                }
            }

        select_document_using_handle(cdoc);
        }
}

#endif


/************************************************
*                                               *
* adjust all draw file references that use this *
* draw file entry                               *
*                                               *
************************************************/

#if RISCOS

static void
draw_adjust_file_refs(drawfep dfp, word32 key)
{
    LIST *lptr;
    drawfrp dfrp;

    /* search for a duplicate entry */
    for(lptr = first_in_list(&draw_file_refs);
        lptr;
        lptr = next_in_list(&draw_file_refs))
        {
        dfrp = (drawfrp) lptr->value;

        if(key == dfrp->draw_file_key)
            draw_adjust_file_ref(dfp, dfrp);
        }
}

#endif


/*****************************************************************
*                                                                *
* given the details of a draw file, make sure it is in the cache *
*                                                                *
* --out--                                                        *
* -1   = error                                                   *
* >= 0 = key number of draw file                                 *
*                                                                *
*****************************************************************/

#if RISCOS

static intl
draw_cache_file(char *name, intl namlen, BOOL adjust_refs,
                double xp, double yp, colt col, rowt row)
{
    LIST *lptr;
    drawfep dfp;
    word32 maxkey, key;
    char namebuf[MAX_FILENAME];
    char *namecopy;
    intl res;

    tracef0("[draw_cache_file]\n");

    lptr = draw_search_cache(name, namlen, &maxkey);

    if(lptr)
        {
        key = lptr->key;
        dfp = (drawfep) lptr->value;
        }
    else
        {
        /* create a list entry, if there is none */
        namecopy = alloc_ptr_using_cache((word32) namlen + 1, &res);

        if(!namecopy)
            return(res ? res : ERR_NOROOM);

        lptr = add_list_entry(&draw_file_list, sizeof(struct draw_file_entry), &res);

        if(!lptr)
            {
            free(namecopy);
            return(res ? res : ERR_NOROOM);
            }

        lptr->key = key = maxkey + 1;
        dfp = (drawfep) lptr->value;

        dfp->name    = namecopy;
        dfp->memoryh = 0;

        *dfp->name   = '\0';
        strncat(dfp->name, name, namlen);
        }


    /* if the file is not loaded, load it */
    if(!dfp->memoryh)
        {
        FILE *fin;
        intl filetype;
        word32 length, lengthfile, spritelength;
        char *drawdp, *readp;

        dfp->error = 0;

        /* generate the name */
        add_prefix_to_name(namebuf, dfp->name, TRUE);

        /* load the file */
        fin = myfopen(namebuf, read_str);

        if(fin)
            {
            char filebuf[256];
            riscos_fileinfo info;

            /* ensure it doesn't blow up in load */
            mysetvbuf(fin, filebuf, sizeof(filebuf));
            lengthfile = length = filelength(fin);

            /* round up size */
            length = ((length - 1) / sizeof(intl) + 1) * sizeof(intl);

            riscos_readfileinfo(&info, namebuf);

            filetype = (info.load >> 8) & 0xFFF;

            switch(filetype)
                {
                case DRAW_FILETYPE:
                    break;

                case SPRITE_FILETYPE:
                    spritelength = length - (sizeof(sprite_area) - 4);
                    length = sizeof(draw_fileheader) +
                             sizeof(draw_objhdr) +
                             spritelength;
                    break;              

                default:
                    length = 0;
                    break;
                }

            if(length)
                {
                mhandle memh;

                tracef2("[draw_cache_file found draw file length: %d, %d]\n",
                        lengthfile, length);
                memh = alloc_handle_using_cache(length);

                /* reload pointer after alloc */
                dfp = (drawfep) search_list(&draw_file_list, key)->value;

                if(memh <= 0)
                    dfp->error = ERR_NOROOM;
                else
                    {
                    readp = drawdp = list_getptr(memh);

                    /* calculate sprite load pos */
                    if(filetype == SPRITE_FILETYPE)
                        readp += sizeof(draw_fileheader) +
                                 sizeof(draw_objhdr) -
                                 (sizeof(sprite_area) - 4);

                    /* load file */
                    if(fread(readp, 1, (size_t) lengthfile, fin) < lengthfile)
                        {
                        list_deallochandle(memh);

                        /* reload pointer after dealloc */
                        dfp = (drawfep) search_list(&draw_file_list, key)->value;
                        dfp->error = ERR_CANNOTREAD;
                        }
                    else
                        {
                        dfp->diag.data   = drawdp;
                        dfp->diag.length = (intl) length;

                        if(filetype == SPRITE_FILETYPE)
                            {
                            draw_box bboxdummy;
                            intl sizedummy;
                            draw_error errordummy;
                            draw_spristr *spritedp;

                            tracef2("[draw_cache_file about to create sprite header &%p -> &%p]\n", &dfp->diag, drawdp);
                            draw_create_diag((draw_diag *) &dfp->diag,
                                             "PipeDream",
                                             bboxdummy);

                            tracef1("[diag.length is: %d]\n",
                                    dfp->diag.length);

                            spritedp = (draw_spristr *)
                                            (drawdp + sizeof(draw_fileheader));

                            spritedp->tag  = draw_OBJSPRITE;
                            spritedp->size = (intl) spritelength +
                                             sizeof(draw_objhdr);

                            plusab(dfp->diag.length, spritedp->size);
                            tracef1("[diag.length is: %d]\n", dfp->diag.length);

                            /* force duff bbox so verifyObject will calculate */
                            spritedp->bbox.x0 = 1;
                            spritedp->bbox.x1 = 0;

                            tracef0("[cache_draw_file about to verify sprite]\n");
                            if(!draw_verifyObject((draw_diag *) &dfp->diag,
                                                  (char *) spritedp - drawdp,
                                                  &sizedummy, &errordummy))
                                tracef1("[failed to verify sprite error: %d]\n",
                                        errordummy.err.draw.code);

                            tracef1("[verifyObject returned sprite size: %d]\n",
                                    sizedummy);
                            }

                        tracef0("[draw_cache_file: file loaded, now verify]\n");
                        if(!draw_verify_diag((draw_diag *) &dfp->diag, NULL))
                            {
                            list_deallochandle(memh);

                            /* reload pointer after dealloc */
                            dfp = (drawfep) search_list(&draw_file_list, key)->value;
                            dfp->error = ERR_BADDRAWFILE;
                            }
                        else
                            {
                            draw_box box;

                            /* always rebind the Draw file */
                            tracef0("[draw_cache_file: file verified, now rebind]\n");
                            draw_rebind_diag((draw_diag *) &dfp->diag);

                            /* get box in Draw units */
                            draw_queryBox((draw_diag *) &dfp->diag,
                                          &box, FALSE);
                            tracef4("[draw_cache_file: diagram bbox %d, %d, %d, %d (Draw)\n",
                                    box.x0, box.y0, box.x1, box.y1);

                            /* move diagram into the corner */
                            tracef2("[draw_cache_file: shift_diag(%d, %d)]\n",
                                            -box.x0, -box.y0);
                            draw_shift_diag((draw_diag *) &dfp->diag,
                                            -box.x0, -box.y0);

                            dfp->memoryh = memh;
                            }
                        }
                    }
                }
            else
                {
                tracef1("[draw_cache_file bad file: %s]\n", name);
                dfp->error = ERR_BADDRAWFILE;
                }

            myfclose(fin);
            }
        else
            {
            tracef1("[draw_cache_file cannot open: %s]\n", name);
            dfp->error = ERR_CANNOTOPEN;
            }
        }

    if(adjust_refs)
        draw_adjust_file_refs(dfp, key);
    elif(!dfp->error)
        {
        res = draw_add_file_ref(key, current_document_handle(), col, row, xp, yp);
        if(res <= 0)
            return(res ? res : ERR_NOROOM);
        }

    return((intl) key);
}

#endif


/************************************************
*                                               *
* when a document is destroyed, this routine    *
* is called to clear up its draw files (if any) *
*                                               *
************************************************/

#if RISCOS

extern void
draw_close_file(dochandle han)
{
    tracef0("[draw_close_file]\n");

    draw_removeblock(han, -1, -1, -1, -1);

    draw_tidy_up();
}

#endif


/*************************************************
*                                                *
* return pointers to information for a draw file *
* given the document and row                     *
*                                                *
*************************************************/

#if RISCOS

extern intl
draw_find_file(dochandle han, colt col, rowt row,
               drawfep *drawfile, drawfrp *drawref)
{
    LIST *lptr, *dflptr;
    drawfrp dfrp;
    drawfep dfp;

    tracef2("[draw_find_file: %d, %d]\n", col, row);

    for(lptr = first_in_list(&draw_file_refs);
        lptr;
        lptr = next_in_list(&draw_file_refs))
        {
        dfrp = (drawfrp) lptr->value;

        tracef3("[draw_find_file found han: %d, col: %d, row: %d]\n",
                dfrp->han, dfrp->col, dfrp->row);

        if( (han == dfrp->han)  &&
            ((col == -1) || (col == dfrp->col))  &&
            (row == dfrp->row))
            {
            dflptr = search_list(&draw_file_list, dfrp->draw_file_key);

            if(dflptr)
                {                    
                dfp = (drawfep) dflptr->value;

                if(dfp->memoryh  &&  !dfp->error)
                    {
                    dfp->diag.data = list_getptr(dfp->memoryh);

                    tracef1("[draw_find_file found key: %d]\n", dflptr->key);

                    if(drawfile)
                        *drawfile = dfp;

                    if(drawref)
                        *drawref = dfrp;

                    return(1);
                    }
                }
            }
        }
    return(0);
}

#endif


/************************************************
*                                               *
* when a graph is changed, the graphics program *
* calls PipeDream to update any draw files it   *
* has of the graph                              *
*                                               *
************************************************/

#if RISCOS

extern void
draw_recache_file(const char *drawfilename)
{
    LIST *lptr;
    drawfep dfp;
    word32 key;
    char buffer[MAX_FILENAME];
    const char *entryname;
    intl res;

    tracef0("[draw_recache_file]\n");

    do  {
        for(lptr = first_in_list(&draw_file_list);
            lptr;
            lptr = next_in_list(&draw_file_list))
            {
            key = lptr->key;
            dfp = (drawfep) lptr->value;

            if(!dfp->processed)
                {
                add_prefix_to_name(buffer, dfp->name, TRUE);
                entryname = buffer;

                /* if any ambiguity in either name, just check leafnames */
                if(!isrooted(entryname)  ||  !isrooted(drawfilename))
                    {
                    entryname    = leafname(entryname);
                    drawfilename = leafname(drawfilename);
                    }

                tracef2("[draw_recache_file: comparing entry %s with %s]\n", entryname, drawfilename);

                if(!stricmp(entryname, drawfilename))
                    {
                    /* stop us from trying to reprocess this after recache */
                    dfp->processed = TRUE;

                    /* throwing away the draw file seems easiest way to recache */
                    list_disposehandle(&dfp->memoryh);

                    /* reload pointer after dealloc */
                    dfp = (drawfep) search_list(&draw_file_list, key)->value;

                    /* most parameters not needed as it's just a recache */
                    res = draw_cache_file(dfp->name, strlen(dfp->name), TRUE, 0., 0., (colt) 0, (rowt) 0);

                    if(res < 0)
                        {
                        reperr_null(res);
                        goto ENDPOINT;
                        }

                    break;
                    }
                }
            }
        }
    while(lptr);

ENDPOINT:

    /* clear out processed flags */
    for(lptr = first_in_list(&draw_file_list);
        lptr;
        lptr = next_in_list(&draw_file_list))
        {
        dfp = (drawfep) lptr->value;

        dfp->processed = FALSE;
        }
}

#endif


/*********************************************
*                                            *
* remove a reference or references to a draw *
* file and do a tidy up                      *
*                                            *
* --in--                                     *
* set col or row to -1 to wildcard           *
*                                            *
*********************************************/

#if RISCOS

extern void
draw_removeblock(dochandle han, colt scol, rowt srow, colt ecol, rowt erow)
{
    LIST *lptr;
    drawfrp dfrp;

    tracef5("[draw_removeblock han: %d, scol: %d, srow: %d, ecol: %d, erow: %d]\n",
            han, scol, srow, ecol, erow);

    /* search references on the list */
    do  {
        for(lptr = first_in_list(&draw_file_refs);
            lptr;
            lptr = next_in_list(&draw_file_refs))
            {
            dfrp = (drawfrp) lptr->value;

            if( ((han == DOCHANDLE_NONE)  ||  (dfrp->han == han))                   &&
                ((scol == -1)  ||  ((dfrp->col >= scol) && (dfrp->col <= ecol)))    &&
                ((srow == -1)  ||  ((dfrp->row >= srow) && (dfrp->row <= erow)))    )
                {
                /* set key so we can find it */
                lptr->key = 1;
                tracef3("[draw_removeblock deleting han: %d, col: %d, row: %d]\n",
                        dfrp->han, dfrp->col, dfrp->row);
                delete_from_list(&draw_file_refs, 1);
                break;
                }
            }
        }
    while(lptr);
}

#endif


/************************************
*                                   *
* remove a draw reference to a slot *
*                                   *
************************************/

#if RISCOS

extern void
draw_removeslot(colt col, rowt row)
{
    draw_removeblock(current_document_handle(), col, row, col, row);
}

#endif


/************************************
*                                   *
* search draw file cache for a file *
*                                   *
************************************/

extern LIST *
draw_search_cache(char *name, intl namlen, word32 *maxkey)
{
    LIST *lptr;
    char namebuf[MAX_FILENAME];
    drawfep dfp;

    *namebuf = '\0';
    strncat(namebuf, name, namlen);

    /* search for the file on the list */
    if(maxkey)
        *maxkey = 0;

    for(lptr = first_in_list(&draw_file_list);
        lptr;
        lptr = next_in_list(&draw_file_list))
        {
        if(maxkey)
            *maxkey = max(lptr->key, *maxkey);

        dfp = (drawfep) lptr->value;

        if(!stricmp(namebuf, dfp->name))
            {
            tracef1("[draw_search_cache found file, key: %d, in list]\n",
                    lptr->key);
            break;
            }
        }

    return(lptr);
}


/******************************************
*                                         *
* search a slot for a draw file reference *
* and if found, add it to the list        *
*                                         *
******************************************/

#if RISCOS

extern intl
draw_str_insertslot(colt col, rowt row)
{
    slotp sl;
    intl found_file = 0;
    double xp, yp;
    intl len, res;
    char *name;
    char tbuf[LIN_BUFSIZ];
    char *c, *to;

    tracef2("[draw_str_insertslot col: %d, row: %d]\n", col, row);

    sl = travel(col, row);
    if(sl)
        {
        c = sl->content.text;

        while(*c)
            {
            if(*c == SLRLDI)
                {
                c += SLRSIZE;
                continue;
                }

            if(*c++ != '@')
                continue;

            /* check for a draw file */
            to = tbuf;
            len = readfxy('G', &c, &to, &name, &xp, &yp);
            if(len)
                {
                tracef0("[draw_str_insertslot found draw file ref]\n");
                if((res = draw_cache_file(name, len, FALSE, xp, xp, col, row)) >= 0)
                    found_file = 1;
                else
                    {
                    reperr_null(res);
                    been_error = FALSE;
                    }
                break;
                }
            }
        }

    return(found_file);
}

#endif


#if RISCOS

extern void
draw_swaprefs(rowt row1, rowt row2,
              colt scol, colt ecol)
{
    LIST *lptr;
    rowt diff1, diff2;
    drawfrp dfrp;

    tracef4("[draw_swaprefs row1: %d, row2: %d, scol: %d, ecol: %d]\n",
            row1, row2, scol, ecol);

    diff1 = row2 - row1;
    diff2 = row1 - row2;

    for(lptr = first_in_list(&draw_file_refs);
        lptr;
        lptr = next_in_list(&draw_file_refs))
        {
        dfrp = (drawfrp) lptr->value;

        if( (dfrp->han == current_document_handle())  &&
            (dfrp->col >= scol)  &&
            (dfrp->col <= ecol))
            {
            if(dfrp->row == row1)
                {
                tracef2("[draw_swaprefs row: %d, becomes: %d]\n",
                        dfrp->row, dfrp->row + diff1);
                plusab(dfrp->row, diff1);
                }
            elif(dfrp->row == row2)
                {
                tracef2("[draw_swaprefs row: %d, becomes: %d]\n",
                        dfrp->row, dfrp->row + diff2);
                plusab(dfrp->row, diff2);
                }
            }
        }
}

#endif

/**********************************************
*                                             *
* tidy up draw files by removing any that are *
* not referenced                              *
*                                             *
**********************************************/

#if RISCOS

extern void
draw_tidy_up(void)
{
    LIST *lptr, *dflptr;
    drawfep dfp;
    drawfrp dfrp;
    word32 draw_file_key;
    mhandle memh;

    /* step thru the actual files, clearing all the flags */
    for(lptr = first_in_list(&draw_file_list);
        lptr;
        lptr = next_in_list(&draw_file_list))
        {
        dfp = (drawfep) lptr->value;
        dfp->flag = 0;
        }

    /* step thru the references, marking files */
    for(lptr = first_in_list(&draw_file_refs);
        lptr;
        lptr = next_in_list(&draw_file_refs))
        {
        dfrp = (drawfrp) lptr->value;

        dflptr = search_list(&draw_file_list, dfrp->draw_file_key);

        if(dflptr)
            {
            tracef3("[draw_tidy_up found ref han: %d, col: %d, row: %d]\n",
                    dfrp->han, dfrp->col, dfrp->row);
            ((drawfep) dflptr->value)->flag = 1;
            }
        }

    /* now step thru the actual files, removing unmarked files */
    do  {
        for(lptr = first_in_list(&draw_file_list);
            lptr;
            lptr = next_in_list(&draw_file_list))
            {
            dfp = (drawfep) lptr->value;

            if(!dfp->flag)
                {
                tracef2("[draw_tidy_up deleting draw file: %s, key: %d]\n",
                        dfp->name, lptr->key);

                draw_file_key = lptr->key;

                /* dfp invalidated on dealloc */
                memh = dfp->memoryh;

                dispose((void **) &dfp->name);

                list_disposehandle(&memh);

                /* must re-start after a delete */
                delete_from_list(&draw_file_list, draw_file_key);
                break;
                }
            }
        }
    while(lptr);
}

#endif


/***************************************************
*                                                  *
* insert a slot from both draw and tree structures *
*                                                  *
***************************************************/

extern intl
draw_tree_str_insertslot(colt col, rowt row, intl sort)
{
    intl res;

    res = tree_str_insertslot(col, row, sort);

    #if RISCOS
    if(draw_str_insertslot(col, row))
        out_rebuildvert = TRUE;
    #endif

    return(res);
}


/****************************************************
*                                                   *
* remove a block from both draw and tree structures *
*                                                   *
****************************************************/

extern void
draw_tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow)
{
    tree_removeblock(scol, srow, ecol, erow);

    #if RISCOS
    draw_removeblock(current_document_handle(), scol, srow, ecol, erow);
    #endif
}


/***************************************************
*                                                  *
* remove a slot from both draw and tree structures *
*                                                  *
***************************************************/

extern void
draw_tree_removeslot(colt col, rowt row)
{
    tree_removeslot(col, row);

    #if RISCOS
    draw_removeslot(col, row);
    #endif
}


/************************************************
*                                               *
* update references in draw file reference list *
*                                               *
************************************************/

#if RISCOS

extern void
draw_updref(colt mrksco, rowt mrksro,
            colt mrkeco, rowt mrkero,
            colt coldiff, rowt rowdiff)
{
    LIST *lptr;
    drawfrp dfrp;

    do  {
        for(lptr = first_in_list(&draw_file_refs);
            lptr;
            lptr = next_in_list(&draw_file_refs))
            {
            dfrp = (drawfrp) lptr->value;

            if( (dfrp->han == current_document_handle())            &&
                (dfrp->col >= mrksco)  &&  (dfrp->col <= mrkeco)    &&
                (dfrp->row >= mrksro)  &&  (dfrp->row <= mrkero)    )
                {
                if(coldiff == BADCOLBIT)
                    {
                    /* set key so we can find it */
                    lptr->key = 1;
                    tracef3("[draw_updref deleting han: %d, col: %d, row: %d]\n",
                            dfrp->han, dfrp->col, dfrp->row);
                    delete_from_list(&draw_file_refs, 1);
                    break;
                    }

                tracef3("[draw_updref updating han: %d, col: %d, row: %d]\n",
                        dfrp->han, dfrp->col, dfrp->row);

                plusab(dfrp->col, coldiff);
                plusab(dfrp->row, rowdiff);
                }
            }
        }
    while(lptr);
}

#endif


extern void
EditExpression_fn(void)
{
    slotp tslot;

    if(!mergebuf() ||  xf_inexpression)
        return;

    *linbuf = '\0';

    tslot = travel_here();

    if(tslot)
        {
        if(tslot->type == SL_PAGE)
            return;

        if(tslot->justify & PROTECTED)
            {
            reperr_null(ERR_PROTECTED);
            return;
            }

        prccon(linbuf, tslot);
        }

    seteex();
    buffer_altered = TRUE;
    mark_row(currowoffset);
}


/****************************
*                           *
* exchange numbers for text *
*                           *
****************************/

extern void
ExchangeNumbersText_fn(void)
{
    bash_slots_about_a_bit(TRUE);
}


/************************
*                       *
* fill buffer from slot *
*                       *
************************/

extern void
filbuf(void)
{
    BOOL zap = FALSE;
    slotp nslot;

    if(slot_in_buffer  ||  buffer_altered  ||  xf_inexpression)
        return;

    nslot = travel_here();

    if(nslot)
        {
        if((nslot->type == SL_TEXT)  &&  !(nslot->justify & PROTECTED))
            {
            prccon(linbuf, nslot);
            slot_in_buffer = TRUE;
            }
        else
            /* ensure linbuf zapped */
            zap = TRUE;
        }

    if(!nslot  ||  zap)
        {
        *linbuf = '\0';
        lescrl = 0;
        slot_in_buffer = !zap;
        }
}


/*
Return column number. Assumes it can read as many alphas as it likes
Deals with leading spaces.

Reads from extern buff_sofar, updating it
*/

extern colt
getcol(void)
{
    colt res = -1;
    uchar ch;

    if(buff_sofar == NULL)
        return(NO_COL);

    while(ch = *buff_sofar, ispunct(ch)  ||  (ch == SPACE))
        buff_sofar++;

    while(ch = *buff_sofar, isalpha(ch))
        {
        buff_sofar++;
        res = (res+1)*26 + toupper(ch) - 'A';
        }

    return((res == -1) ? NO_COL : res);
}


/*****************************************
*                                        *
* add an entry to the graphics link list *
*                                        *
*****************************************/

#if RISCOS

extern intl
graph_add_entry(ghandle ghan, dochandle han, colt col, rowt row,
                intl xsize, intl ysize, const char *leaf, const char *tag,
                intl task)
{
    intl leaflen = strlen(leaf);
    intl taglen  = strlen(tag);
    LIST *lptr;
    graphlinkp glp;
    char *ptr;
    intl res;

    lptr = add_list_entry(&graphics_link_list,
                          sizeof(struct graphics_link_entry) + leaflen + taglen + 1, &res);
                                            /* extra +1 accounted for in sizeof() */    

    if(!lptr)
        return(res);

    lptr->key = (word32) ghan;
    glp = (graphlinkp) (lptr->value);

    glp->han    = han;
    glp->col    = col;
    glp->row    = row;

    glp->task   = task;
    glp->update = FALSE;

    glp->ghan   = ghan;
    glp->xsize  = xsize;
    glp->ysize  = ysize;

    ptr = glp->text;
    strcpy(ptr, leaf);
    ptr += leaflen + 1;
    strcpy(ptr, tag);

    return((intl) ghan);
}

#endif


/****************************************************
*                                                   *
* when a document is destroyed, this routine        *
* is called to clear up its graphics links (if any) *
*                                                   *
****************************************************/

#if RISCOS

extern void
graph_close_file(dochandle han)
{
    tracef0("[graph_close_file]\n");

    graph_removeblock(han, -1, -1, -1, -1);
}

#endif


/*****************************************************
*                                                    *
* remove a slot from graph, draw and tree structures *
*                                                    *
*****************************************************/

extern void
graph_draw_tree_removeslot(colt col, rowt row)
{
    #if RISCOS
    graph_removeslot(col, row);
    #endif

    draw_tree_removeslot(col, row);
}


/******************************************************
*                                                     *
* remove a block from graph, draw and tree structures *
*                                                     *
******************************************************/

extern void
graph_draw_tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow)
{
    #if RISCOS
    graph_removeblock(current_document_handle(), scol, srow, ecol, erow);
    #endif

    draw_tree_removeblock(scol, srow, ecol, erow);
}


/****************************************
*                                       *
* remove a reference or references to a *
* graphics link and do a tidy up        *
*                                       *
* --in--                                *
* set col or row to -1 to wildcard      *
*                                       *
****************************************/

#if RISCOS

extern void
graph_removeblock(dochandle han, colt scol, rowt srow, colt ecol, rowt erow)
{
    LIST *lptr;
    graphlinkp glp;

    tracef5("[graph_removeblock han: %d, scol: %d, srow: %d, ecol: %d, erow: %d]\n",
            han, scol, srow, ecol, erow);

    do  {
        for(lptr = first_in_list(&graphics_link_list);
            lptr;
            lptr = next_in_list(&graphics_link_list))
            {
            glp = (graphlinkp) lptr->value;

            tracef1("considering closing glp &%p\n", glp);

            if( ((han == DOCHANDLE_NONE)  ||  (glp->han == han))                &&
                ((scol == -1)  ||  ((glp->col >= scol) && (glp->col <= ecol)))  &&
                ((srow == -1)  ||  ((glp->row >= srow) && (glp->row <= erow)))  )
                {
                /* tell client goodbye */
                riscos_sendsheetclosed(glp);

                /* set key so we can find it */
                lptr->key = 0;
                tracef3("[graph_removeblock deleting han: %d, col: %d, row: %d]\n",
                        glp->han, glp->col, glp->row);
                delete_from_list(&graphics_link_list, 0);
                break;
                }
            }
        }
    while(lptr);
}

#endif


/*************************************
*                                    *
* remove a graphics link from a slot *
*                                    *
*************************************/

#if RISCOS

extern void
graph_removeslot(colt col, rowt row)
{
    graph_removeblock(current_document_handle(), col, row, col, row);
}

#endif


/****************************************************
*                                                   *
* delete the graphics list entry for this handle    *
*                                                   *
****************************************************/

#if RISCOS

extern void
graph_remove_entry(ghandle han)
{
    delete_from_list(&graphics_link_list, (word32) han);
}

#endif


/************************************************
*                                               *
* find the graphics list entry for this handle  *
*                                               *
************************************************/

#if RISCOS

extern graphlinkp
graph_search_list(ghandle han)
{
    LIST *lptr = search_list(&graphics_link_list, (word32) han);

    return(lptr ? (graphlinkp) (lptr->value) : NULL);
}

#endif


/****************************************
*                                       *
* give warning if graphics links        *
*                                       *
****************************************/

#if RISCOS

extern BOOL
dependent_links_warning(void)
{
    LIST *lptr;
    graphlinkp glp;
    dochandle han = current_document_handle();
    intl nLinks = 0;

    do  {
        for(lptr = first_in_list(&graphics_link_list);
            lptr;
            lptr = next_in_list(&graphics_link_list))
            {
            glp = (graphlinkp) lptr->value;

            if(glp->han == han)
                ++nLinks;
            }
        }
    while(lptr);

    if(nLinks)
        return(riscdialog_query(close_dependent_links_winge_STR) == riscdialogquery_YES);

    return(TRUE);
}

#endif


/*************************************************************************
*                                                                        *
* send a changed block off to client via graphics link if updates wanted *
*                                                                        *
*************************************************************************/

#if RISCOS

/* NB. inclusive, exclusive block, eg. graph_send_block(0, 0, numcol, numrow)
 * but with special case that at least one slot gets sent so
 * we can use scol==ecol or srow==erow if we wish
*/

extern void
graph_send_xblock(colt scol, rowt srow, colt ecol, rowt erow, dochandle han)
{
    LIST *lptr;
    graphlinkp glp;
    intl xoff, yoff;
    colt tcol;
    rowt trow;

    for(lptr = first_in_list(&graphics_link_list);
        lptr;
        lptr = next_in_list(&graphics_link_list))
        {
        glp = (graphlinkp) lptr->value;

        if(glp->update  &&  (glp->han == han))
            {
            tcol = scol;

            do  {
                xoff = (intl) (tcol - glp->col);

                if((xoff >= 0)  &&  (xoff <= glp->xsize))
                    {
                    trow = srow;

                    do  {
                        yoff = (intl) (trow - glp->row);

                        if((yoff >= 0)  &&  (yoff <= glp->ysize))
                            riscos_sendslotcontents(glp, xoff, yoff);
                        }
                    while(++trow < erow);
                    }
                }
            while(++tcol < ecol);
            }
        }
}

extern void
graph_send_block(colt scol, rowt srow, colt ecol, rowt erow)
{
    graph_send_xblock(scol, srow, ecol, erow, current_document_handle());
}

extern void
graph_send_slot(colt col, rowt row)
{
    graph_send_xblock(col, row, col, row, current_document_handle());
}

#endif


/************************************************************************
*                                                                       *
* send a changed slot off to client via graphics link if updates wanted *
*                                                                       *
************************************************************************/

#if RISCOS && FALSE

extern void
graph_send_slot(colt col, rowt row)
{
    LIST *lptr;
    graphlinkp glp;
    dochandle han = current_document_handle();
    intl xoff, yoff;

    for(lptr = first_in_list(&graphics_link_list);
        lptr;
        lptr = next_in_list(&graphics_link_list))
        {
        glp = (graphlinkp) lptr->value;

        if(glp->update  &&  (glp->han == han))
            {
            xoff = (intl) (col - glp->col);

            if((xoff >= 0)  &&  (xoff <= glp->xsize))
                {
                yoff = (intl) (row - glp->row);

                if((yoff >= 0)  &&  (yoff <= glp->ysize))
                    riscos_sendslotcontents(glp, xoff, yoff);
                }
            }
        }
}

#endif


/*******************************************************************
*                                                                  *
* send off any blocks in this document that have been hit by range *
*                                                                  *
*******************************************************************/

extern void
graph_send_split_blocks(colt scol, rowt srow, colt ecol, rowt erow)
{
    LIST *lptr;
    graphlinkp glp;
    dochandle han = current_document_handle();
    colt x0, x1;
    rowt y0, y1;

trace_on();
    tracef4("[graph_send_split_blocks(%d, %d; %d, %d)]\n", scol, srow, ecol, erow);

    for(lptr = first_in_list(&graphics_link_list);
        lptr;
        lptr = next_in_list(&graphics_link_list))
        {
        glp = (graphlinkp) lptr->value;

        if(glp->update  &&  (glp->han == han))
            {
            /* make inclusive,exclusive range */
            x0 = glp->col;
            y0 = glp->row;
            x1 = x0 + glp->xsize + 1;
            y1 = y0 + glp->ysize + 1;

            tracef4("[comparing with block %d, %d; %d, %d]\n", x0, y0, x1, y1);

            if((ecol <= x0)  ||  (erow <= y0)  ||  (x1 <= scol)  ||  (y1 <= srow))
                tracef0("[block not hit]\n");
            else
                riscos_sendallslots(glp);
            }
        }
trace_off();
}


/**************************************
*                                     *
* any active graphs in this document? *
*                                     *
**************************************/

#if RISCOS

extern BOOL
graph_active_present(dochandle han)
{
    LIST *lptr;
    graphlinkp glp;

    for(lptr = first_in_list(&graphics_link_list);
        lptr;
        lptr = next_in_list(&graphics_link_list))
        {
        glp = (graphlinkp) lptr->value;

        if(glp->update  &&  ((han == DOCHANDLE_NONE)  ||  (glp->han == han)))
            return(TRUE);
        }

    return(FALSE);
}

#endif


/******************************************
*                                         *
* update references in graphics link list *
*                                         *
******************************************/

#if RISCOS

extern void
graph_updref(colt mrksco, rowt mrksro,
             colt mrkeco, rowt mrkero,
             colt coldiff, rowt rowdiff)
{
    LIST *lptr;
    graphlinkp glp;

    do  {
        for(lptr = first_in_list(&graphics_link_list);
            lptr;
            lptr = next_in_list(&graphics_link_list))
            {
            glp = (graphlinkp) lptr->value;

            if( (glp->han == current_document_handle())         &&
                (glp->col >= mrksco)  &&  (glp->col <= mrkeco)  &&
                (glp->row >= mrksro)  &&  (glp->row <= mrkero)  )
                {
                if(coldiff == BADCOLBIT)
                    {
                    /* tell client goodbye */
                    riscos_sendsheetclosed(glp);

                    /* set key so we can find it */
                    lptr->key = 0;
                    tracef3("[graph_updref deleting han: %d, col: %d, row: %d]\n",
                            glp->han, glp->col, glp->row);
                    delete_from_list(&graphics_link_list, 0);
                    break;
                    }

                tracef3("[graph_updref updating han: %d, col: %d, row: %d]\n",
                        glp->han, glp->col, glp->row);

                plusab(glp->col, coldiff);
                plusab(glp->row, rowdiff);
                }
            }
        }
    while(lptr);
}

#endif


/******************************************
*                                         *
* say whether a slot contains a draw file *
*                                         *
* --in--                                  *
* set col to -1 to match all columns      *
*                                         *
******************************************/

#if RISCOS

extern intl
is_draw_file_slot(dochandle han, colt col, rowt row)
{
    LIST *lptr;
    drawfrp dfrp;

    /* search references on the list */
    for(lptr = first_in_list(&draw_file_refs);
        lptr;
        lptr = next_in_list(&draw_file_refs))
        {
        dfrp = (drawfrp) lptr->value;

        if( (han == dfrp->han)                      &&
            ((col == -1)  ||  (col == dfrp->col))   &&
            (row == dfrp->row)                      )
            {
            tracef3("[is_draw_file_slot TRUE: %d, %d, key: %d]\n",
                    col, row, dfrp->draw_file_key);
            return((intl) dfrp->draw_file_key);
            }
        }

    return(0);
}

#endif


extern void
mark_slot(slotp tslot)
{
    if(tslot)
        {
        orab(tslot->flags, SL_ALTERED);

        xf_drawsome = TRUE;
        }
}


/*
merexp, merge expression into slot
*/

extern void
merexp(void)
{
    slotp tslot;
    intl clen;
    uchar justifyflags = J_RIGHT;
    uchar formatflags = 0;
    uchar oprstb[COMPILE_BUFSIZ];

    movement |= ABSOLUTE;
    newcol = edtslr_col;
    newrow = edtslr_row;

    if(!buffer_altered)
        {
        slot_in_buffer = FALSE;
        return;
        }

    filealtered(TRUE);

    /* if no text in formula, make it a text slot */
    clen = cplent(oprstb, FALSE);

    if(!clen)
        {
        merst1(curcol, currow);
        return;
        }

    if(clen < 0)
        {
        /* PD 2.n only puts bad formula in as text slot in numbers mode
            RJM changed it 19.5.89 to put text slot in always
        */
    /*  if(d_options_TN == 'N') */
            {
            merst1(curcol, currow);
            return;
            }
        }

    /* save justify flags for numeric slot */
    tslot = travel(newcol, newrow);
    if(tslot)
        switch(tslot->type)
            {
            case SL_NUMBER:
            case SL_ERROR:
            case SL_DATE:
            case SL_INTSTR:
            case SL_STRVAL:
                justifyflags = tslot->justify;
                formatflags  = tslot->content.number.format;
                graph_draw_tree_removeslot(newcol, newrow);
                break;

            default:
                break;
            }

    /* new slot or convert to number */
    tslot = createslot(newcol, newrow, clen, SL_NUMBER);
    if(!tslot)
        {
        buffer_altered = slot_in_buffer = FALSE;
        *linbuf = '\0';
        reperr_null(ERR_NOROOM);
        return;
        }

    memcpy((char *) tslot->content.number.text,
           (char *) oprstb, (unsigned) clen);
    orab(tslot->flags, refs_in_this_slot);
    tslot->justify = justifyflags;
    tslot->content.number.format = formatflags;

    /* update tree (if natural) */
    if(tree_exp_insertslot(newcol, newrow, TRUE) < 0)
        tree_switchoff();

    /* reload tslot */
    tslot = travel(newcol, newrow);

    /* get a sensible value for the current slot */
    evasto(tslot, newcol, newrow);

    graph_send_slot(newcol, newrow);

    buffer_altered = slot_in_buffer = FALSE;

    /* mark for later recalc */
    recalc_bit = TRUE;

    /* mark that file is altered */
    filealtered(TRUE);
}


/*****************
*                *
* merge line buf *
*                *
*****************/

static BOOL
mergebuf_core(BOOL allow_check)
{
    if(xf_inexpression)
        return(TRUE);

    if(!buffer_altered)
        slot_in_buffer = FALSE;

    if(!slot_in_buffer)
        return(TRUE);

    filealtered(TRUE);

    if(!merst1(curcol, currow))
        return(FALSE);

    #if !defined(SPELL_OFF)
    if(allow_check)
        check_word();
    #endif

    return(TRUE);
}


extern BOOL
mergebuf(void)
{
    return(mergebuf_core(TRUE));
}


extern BOOL
mergebuf_nocheck(void)
{
    return(mergebuf_core(FALSE));
}


extern BOOL
merst1(colt tcol, rowt trow)
{
    slotp newslot = travel(tcol, trow);
    intl slen;
    BOOL res = TRUE;
    uchar justifyflag;
    uchar slot_refs;
    uchar array[COMPILE_BUFSIZ];    /* note that SLRs can expand hugely
                                     * on compilation */
    slot_in_buffer = FALSE;

    if(buffer_altered)
        {
        recalc_bit = TRUE;

        if(newslot  &&  (newslot->type == SL_TEXT))
            justifyflag = newslot->justify;
        else
            justifyflag = J_FREE;

        refs_in_this_slot = 0;

        slen = compile_text_slot(array, linbuf, &slot_refs);

        if(slot_refs == SL_REFS)
            refs_in_this_sheet = refs_in_this_slot = slot_refs;

        if(slen == 1)
            {
            if(newslot)
                graph_draw_tree_removeslot(tcol, trow);

            buffer_altered = FALSE;

            res = createhole(tcol, trow);

            if(!res)
                {
                *linbuf = '\0';
                reperr_null(ERR_NOROOM);
                }
            }       
        else
            {
            if(newslot)
                graph_draw_tree_removeslot(tcol, trow);     

            newslot = createslot(tcol, trow, slen, SL_TEXT);
            if(!newslot)
                {
                /* if user is typing in, delete the line, so we don't get caught.
                 * But if loading a file, don't delete the line because rebuffer
                 * may free some memory and merst1 will get called again
                */
                if(!in_load)
                    *linbuf = '\0';

                res = reperr_null(ERR_NOROOM);
                }

            if(res)
                {
                orab(newslot->flags, refs_in_this_slot);
                newslot->justify = justifyflag;
                memcpy((char *) newslot->content.text, (char *) array, (unsigned) slen);

                if(draw_tree_str_insertslot(tcol, trow, TRUE) < 0)
                    {
                    if(!in_load)
                        tree_switchoff();
                    else
                        reperr_null(ERR_NOROOM);

                    res = FALSE;
                    }
                }

            if(res)
                buffer_altered = FALSE;
            }

        if(!in_load)
            /* send slot under most conditions */
            graph_send_slot(tcol, trow);
        }

    return(res);
}



/*
decompile slot, dealing with slot references, @fields etc
decompile from ptr (text field in slot) to linbuf

slot references are stored as  SLR leadin, colt colno, rowt rowno.
Note this may contain '\0' as part of slot reference, but not at end of string.
*/

extern void
prccon(uchar *target, slotp ptr)
{
    uchar *to = target;
    uchar *from;

    if(!ptr  ||  (ptr->type == SL_PAGE))
        {
        *to = '\0';
        return;
        }

    from = (ptr->type == SL_TEXT)
                    ? ptr->content.text
                    : ptr->content.number.text;

    if((ptr->type != SL_TEXT)  &&  (ptr->type != SL_BAD_FORMULA))
        {
        exp_decompile((char *) to, from);
        return;
        }

    if(!(ptr->flags & SL_REFS))
        {
        strcpy((char *) to, (char *) from);
        return;
        }

    /* this copes with slot references in text slots.
     * On compilation, @@s are left in the slot so they can
     * be ignored on decompilation
    */

    while(*from)
        if(*from == SLRLDI)
            {
            colt tcol;
            rowt trow;
            docno doc;

            ++from;
            doc = (docno) talps(from, sizeof(docno));
            from += sizeof(docno);

            to += write_docname(to, doc);

            tcol = (colt) talps(from, sizeof(colt));
            trow = (rowt) talps(from+=sizeof(colt), sizeof(rowt));

            if(bad_reference(tcol, trow))
                *to++ = '%';

            if(abs_col(tcol))
                *to++ = '$';

            to += writecol(to, tcol);

            if(abs_row(trow))
                *to++ = '$';

            to += (int) sprintf((char *) to, "%ld", (long) (1+(trow & ROWNOBITS)));
            from += sizeof(rowt);
            }
        else
            *to++ = *from++;

    *to = '\0';
}


/************************************************
*                                               *
* read an at field in the form @l:f,x,y@ where: *
*   l is the identifier,                        *
*   f is a filename,                            *
*   x and y are optional x and y parameters     *
*                                               *
************************************************/

#if RISCOS

extern intl
readfxy(intl id, char **pfrom, char **pto, char **name, double *xp, double *yp)
{
    uchar *from, *to;
    intl namelen, scanned;

    tracef1("[readfxy from: %s]\n", *pfrom);

    from = *pfrom;

    /* check for a draw file */
    if((toupper(*from) == id)  &&  (*(from + 1) == ':'))
        {
        /* load to pointer and copy l: */
        to = *pto;
        *to++ = *from++;
        *to++ = *from++;

        while(*from == ' ')
            ++from;

        *name = to;
        namelen = scanned = 0;
        *xp = *yp = -1;

        /* copy across the filename */
        while(*from             &&
              (*from != ' ')    &&
              (*from != '@')    &&
              (*from != ',')    )
            {
            *to++ = *from++;
            ++namelen;
            }

        /* must have a useful name */
        if(!namelen)
            return(0);

        while(*from++ == ' ')
            ;

        /* scan following x and y parameters */
        if(*--from == ',')
            {
            sscanf(from, ", %lg %n", xp, &scanned);
            tracef2("[readfxy scanned: %d, from: %s]\n", scanned, from);
            while(scanned--)
                *to++ = *from++;

            scanned = 0;
            sscanf(from, ", %lg %n", yp, &scanned);
            tracef2("[readfxy scanned: %d, from: %s]\n", scanned, from);
            while(scanned--)
                *to++ = *from++;
            }

        /* there must be a following @ */
        if(*from != '@')
            {
            tracef0("[readfxy found no trailing @]\n");
            return(0);
            }

        /* copy second parameter to first if no second */
        if((*xp != -1)  &&  (*yp == -1))
            *yp = *xp;

        /* save new pointers and return name length */
        *pfrom = from;
        *pto = to;

        #if TRACE
        {
        char namebuf[MAX_FILENAME];

        *namebuf = '\0';
        strncat(namebuf, *name, namelen);
        tracef3("[readfxy name: %s, xp: %g, yp: %g]\n", namebuf, *xp, *yp);
        }
        #endif

        return(namelen);
        }
    else
        return(0);
}

#endif


/*************************
*                        *
* set editing expression *
*                        *
*************************/

extern void
seteex_nodraw(void)
{
    edtslr_col = curcol;
    edtslr_row = currow;        /* save current position */

    lecpos = lescrl = 0;
    slot_in_buffer = xf_inexpression = TRUE;
}


extern void
seteex(void)
{
    output_buffer = TRUE;

    seteex_nodraw();
}


/*************************
*                        *
* end expression editing *
*                        *
*************************/

extern void
endeex_nodraw(void)
{
    xf_inexpression = FALSE;
    slot_in_buffer = buffer_altered = FALSE;
    lecpos = lescrl = 0;
}


extern void
endeex(void)
{
    endeex_nodraw();

    /* need to redraw more if inverse cursor moved */
    if((edtslr_col != curcol)  ||  (edtslr_row != currow))
        mark_row(currowoffset);
    output_buffer = out_currslot = TRUE;
    clear_editing_line();
}


/*
is there a slot ref here?
must be sequence of letters followed by digits followed by at least one @
and (rule number 6) NO spaces
*/

static BOOL
slot_ref_here(const char *ptr)
{
    BOOL row_zero = TRUE;
    char ch;

    /* MRJC added this to cope with external references */
    if(*ptr == '[')
        {
        ++ptr;
        while(isalnum(*ptr))
            ++ptr;
        if(*ptr++ != ']')
            return(FALSE);
        }

    if(*ptr == '$')
        ptr++;

    if(!isalpha((int) *ptr))
        return(FALSE);

    do  {
        ch = *++ptr;
        }
    while(isalpha(ch));

    if(ch == '$')
        ch = *++ptr;

    if(!isdigit(ch))
        return(FALSE);

    do  {
        if(ch)
            row_zero = FALSE;
        ch = *++ptr;
        }
    while(isdigit(ch));

    return(!row_zero  &&  (ch == '@'));
}


/*
splat takes a word32 and writes it as size bytes at to, msb first
used for converting slot references to stream of bytes in slot
*/

extern void
splat(uchar *to, word32 num, intl size)
{
    intl i = -1;

    while(++i < size)
        {
        to[i] = (uchar) (num & 0xFF);
        num >>= 8;
        }
}


extern void
Snapshot_fn(void)
{
    /* fudge lead,trail chars and bracket bits */

    uchar minus   = d_options_MB;
/*  uchar leadch  = d_options_LP;*/
/*  uchar trailch = d_options_TP;*/

    d_options_MB = 'M';
/*  d_options_LP = '\0';*/
/*  d_options_TP = '\0';*/

    bash_slots_about_a_bit(FALSE);

    /* restore brackets bit, leading trailing chars */

    d_options_MB = minus;
/*  d_options_LP = leadch;*/
/*  d_options_TP = trailch;*/
}


/*
talps reads size bytes from from, msb first, generates a word32
which it returns
used for converting stream of bytes to slot reference
*/

extern word32
talps(uchar *from, intl size)
{
    word32 res = 0;
    intl i = size;

    while(--i >= 0)
        {
        res <<= 8;
        res  += from[i];
        }

    return(res);
}


/*
expand column into array returning length
*/

#define ONE_LETTER  (26)
#if 1
static colt nth_letter[] =
{
    26,     /* need this entry as loop looks at nlp-1 */
    26 + 26*26,
    26 + 26*26 + 26*26*26,
    26 + 26*26 + 26*26*26 + 26*26*26*26,
    26 + 26*26 + 26*26*26 + 26*26*26*26 + 26*26*26*26*26,
    26 + 26*26 + 26*26*26 + 26*26*26*26 + 26*26*26*26*26 + 26*26*26*26*26*26
};

static colt nth_power[] =
{
    26,         /* dummy entry - never used */
    26*26,
    26*26*26,
    26*26*26*26,
    26*26*26*26*26,
    26*26*26*26*26*26
};
#define last_letter (sizeof(nth_letter)/sizeof(colt) - 1)

#else
#define TWO_LETTER  (26*26 + 26)
#define SQUARE      (26*26)
#define THREE_LETTER    (26*26*26 + 26*26 + 26)
#define CUBE        (26*26*26)
#endif

extern intl
writecol(uchar *array, colt col)
{
    colt tcol = col & COLNOBITS;    /* ignore bad and absolute */
    uchar *ptr = array;
    BOOL force;

    #if 1
    colt *nlp, *npp;
    colt nl, np;

    if(tcol >= ONE_LETTER)
        {
        if(tcol >= nth_letter[1])   /* TWO_LETTER */
            {
            force = FALSE;
            nlp = nth_letter + sizeof(nth_letter)/sizeof(colt) - 1;
            npp = nth_power  + sizeof(nth_power)/sizeof(colt)  - 1;

            do  {
                if(force  ||  (tcol >= *nlp))
                    {
                    nl = *(nlp-1);
                    np = *npp;
                    tcol -= nl;
                    *ptr++ = (uchar) (tcol / np + 'A' - 1);
                    tcol = tcol % np + nl;
                    force = TRUE;
                    }

                --nlp;
                --npp;
                }
            while(nlp > nth_letter);        /* don't ever loop with nth_letter[0] */
            }

        /* nl == 0 */
        *ptr++ = (uchar) (tcol / ONE_LETTER + 'A' - 1);
        tcol = tcol % ONE_LETTER;
        }
    #else
    /* the next bits can be loopified but it will then go slower. It will
        also need a table of all the funny numbers.  But could be extended
        easily.
    */

    if(tcol >= THREE_LETTER)
        {
        *ptr++ = (uchar) ((tcol-TWO_LETTER) / CUBE  + 'A' - 1);
        tcol = (tcol-TWO_LETTER) % CUBE + TWO_LETTER;
        force = TRUE;
        }
    else
        force = FALSE;

    if(force  ||  (tcol >= TWO_LETTER))
        {
        *ptr++ = (uchar) ((tcol-ONE_LETTER) / SQUARE + 'A' - 1);
        tcol = (tcol-ONE_LETTER) % SQUARE + ONE_LETTER;
        force = TRUE;
        }

    if(force  ||  (tcol >= ONE_LETTER))
        {
        *ptr++ = (uchar) ((tcol/ONE_LETTER) + 'A' - 1);
        tcol %= ONE_LETTER;
        }
    #endif

    *ptr++ = (uchar) (tcol + 'A');
    *ptr = '\0';

    return(ptr - array);
}


/*************************************************************************
*                                                                       *
* write a slot reference to ptr, returning number of characters written *
* could be made a bit more efficient by embedding writecol into sprintf *
*                                                                       *
*************************************************************************/

extern intl
writeref(uchar *ptr, docno doc, colt tcol, rowt trow)
{
    intl count = doc ? write_docname(ptr, doc) : 0;
    count += writecol(ptr + count, tcol);
    count += sprintf((char *) (ptr + count), "%ld", (long) (trow+1));

    return(count);
}

/* end of numbers.c */
