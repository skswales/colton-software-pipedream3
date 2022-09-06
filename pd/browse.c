/* browse.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       browse.c - browse through spell dictionary etc.
 * Author:      RJM June 1988
*/

/* standard header files */
#include "flags.h"


#if defined(SPELL_OFF)
/* Entire module not compiled if SPELL_OFF is defined */
#else


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"
#include "dbox.h"
#include "wimp.h"
#include "wimpt.h"
#include "akbd.h"

#include "ext.spell"
#elif MS
#include <dos.h>
#include "spell.ext"
#else
    assert(0);
#endif


#include "datafmt.h"

#if RISCOS
#include "ext.riscos"
#endif


/* external functions */

#if !defined(SPELL_BOUND)
extern BOOL check_spell_installed(void);
#endif
extern void check_word(void);
extern void close_user_dictionaries(void);
extern void del_spellings(void);
extern intl dict_number(const char *name, BOOL create);


typedef struct
{
    #if RISCOS
    dbox d;
    #else
    void *d;
    #endif
    intl dict;
    intl res;
    intl sofar;
    char (*words)[MAX_WORD+1];
    #if MS || ARTHUR
    intl *lastlen;
    #endif
}
merge_dump_strukt;


/* internal functions */

/*static void ana_or_sub_gram(BOOL sub);*/
/*static BOOL a_fillout(A_LETTER *letters, char *array, char *lastword, BOOL subgrams);*/
/*static intl browse(intl dict, char *wild_str);*/
/*static intl close_dict(intl dict);*/
/*static intl compile_wild_string(intl dict, char *to, const char *from);*/
/*static intl do_anagram(intl dict, A_LETTER *letters, BOOL sub);*/
/*static void end_box(merge_dump_strukt *mdsp);*/
static BOOL err_open_master_dict(void);
static intl get_and_display_words(BOOL fillall, BOOL iswild,
            char words[BROWSE_DEPTH][MAX_WORD+1],
            #if MS
            coord lastlen[BROWSE_DEPTH],
            #endif
            char *wild_string, char *template, char *wild_str, intl dict,
            #if RISCOS
            dbox d,
            #endif
            BOOL *was_spell_errp);
/*static BOOL get_next_misspell(char *array *//*out*//*);*/
/*static BOOL get_word_from_file(intl dict, FILE *in, char *array *//*out*//*, BOOL *hasforeignp *//*out*//*);*/
static char *get_word_from_line(intl dict, char *array /*out*/, intl stt_offset, intl *found_offsetp);
#if FALSE
static BOOL init_box(merge_dump_strukt *mdsp,
            #if RISCOS
            const char *dname, const char *title, BOOL statik);
            #else
            coord xsize, coord ysize,
            BOOL blips, const char *instruction);
            #endif
static void fill_box(char words[BROWSE_DEPTH][MAX_WORD+1], const char *first,
            #if RISCOS
            dbox d);
            #else
            coord lastlen[BROWSE_DEPTH], coord xsize, coord ysize,
            const char *instruction);
            #endif
#endif
/*static BOOL insert_most_recent(char **field);*/
/*static BOOL next_word_on_line(void);*/
static BOOL not_in_user_dicts_or_list(const char *word);
/*static intl open_appropriate_dict(const DIALOG *dptr);*/
static intl open_master_dict(void);
/*static BOOL set_check_block(void);*/


/* ----------------------------------------------------------------------- */

#define MERGEDICT_IN_BUFSIZ 256
#define DUMPDICT_OUT_BUFSIZ 1024

#define BROWSE_CENTRE (BROWSE_DEPTH/2 - 1)

#if ARTHUR
#define PREVNEXT_STR    "Press \x8D\x81 for previous/next word"
#elif MS
#define PREVNEXT_STR    "Press \x18\x19 for previous/next word"

static const char SHIFT_STR[] = "Press Shift to pause";
#endif


static merge_dump_strukt anagram_mds    = { NULL, -1, 0, 0, NULL };
static merge_dump_strukt browse_mds     = { NULL, -1, 0, 0, NULL };
static merge_dump_strukt dumpdict_mds   = { NULL, -1, 0, 0, NULL };
static merge_dump_strukt mergedict_mds  = { NULL, -1, 0, 0, NULL };



/* be generous as to what letters one allows into the spelling buffers */
/* - fault foreign words at adding stage */

/* is character valid as the first in a word? */

#if !defined(FOREIGN_SPELL)

static intl
browse_valid_1(intl dict, char ch)
{
    IGNOREPARM(dict);

    return(isalpha(ch)  &&  spell_iswordc(ch));
}

#define spell_valid_1 browse_valid_1

#endif


#if !defined(FOREIGN_SPELL)

static intl
browse_iswordc(intl dict, char ch)
{
    IGNOREPARM(dict);

    return(isalpha(ch)  ||  spell_iswordc(ch));
}

#define spell_iswordc browse_iswordc

#endif


#if !defined(SPELL_BOUND)

/****************************************************************
*                                                               *
*  check spell checking is installed and set up error handler   *
*                                                               *
****************************************************************/

extern BOOL
check_spell_installed(void)
{
    tracef0("[check_spell_installed]\r\n");

    if(!spell_installed)
        {
        tracef0("[spell not installed]\r\n");
        return(reperr_not_installed(ERR_SPELL));
        }

    #if MS
    _harderr(myhandler);

    if(been_error)
        return(FALSE);
    #endif

    return(TRUE);
}

#endif  /* SPELL_BOUND */


/********************************************
*                                           *
* check word at current position in linbuf  *
*                                           *
********************************************/

extern void
check_word(void)
{
    char array[LIN_BUFSIZ];
    char *ptr;
    intl res;
    BOOL tried_no_quotes;

    if( xf_inexpression  ||  in_dialog_box                       ||
        (d_auto[0].option != 'Y')  ||  !check_spell_installed()  )
            return;

    if(err_open_master_dict())
        {
        d_auto[0].option = 'N';
        return;
        }

    if(get_word_from_line(master_dictionary, array, lecpos, NULL))
        {
        tried_no_quotes = FALSE;

    TRY_NO_QUOTE:

        res = spell_checkword(master_dictionary, array);

        if(res == SPELL_ERR_BADWORD)
            res = 0;

        if(res < 0)
            reperr_module(ERR_SPELL, res);
        elif(!res  &&  not_in_user_dicts_or_list(array))
            {
            ptr = array + strlen(array);

            if( !tried_no_quotes  &&
                (   (*--ptr == '\'')  ||
                    ((tolower(*ptr) == 's')  &&  (*--ptr == '\''))))
                {
                *ptr = '\0';
                tried_no_quotes = TRUE;
                goto TRY_NO_QUOTE;
                }
            else
                {
                bleep();
                #if MS
                check_error = TRUE; /* prevent auto-repeat */
                #endif
                }
            }
        }
}


static intl
close_dict(intl dict)
{
    intl res;

    if( (dict != dumpdict_mds.dict)     &&
        (dict != mergedict_mds.dict)    &&
        (dict != anagram_mds.dict))
        {
        res = spell_close(dict);
        delete_from_list(&first_user_dict, (word32) dict);
        }
    else
        res = SPELL_ERR_CANTCLOSE;

    most_recent = -1;

    return(res);
}


/************************************************************
*                                                           *
*  compile the wild string - returning whether it is wild   *
*                                                           *
************************************************************/

static intl
compile_wild_string(intl dict, char *to, const char *from)
{
    const char *ptr = from;
    intl iswild = 0;
    intl ch;

    /* get word template from wild_string */
    if(!str_isblank(ptr))
        while((ch = *ptr++) != '\0')
            {
            if(ch == '^')
                {
                ch = *ptr++;

                switch(ch)
                    {
                    case '\0':
                        --ptr;      /* point at 0 again */
                        break;

                    case '#':
                        ch = SPELL_WILD_MULTIPLE;

                        /* deliberate drop thru */

                    case '?':
                        assert('?' == SPELL_WILD_SINGLE);
                        iswild = TRUE;

                        /* deliberate drop thru */

                    default:
                        *to++ = (char) ch;
                        break;
                    }
                }
            elif(!((ptr == from) ? spell_valid_1 : spell_iswordc)(dict, ch))
                return(SPELL_ERR_BADWORD);
            else
                *to++ = (char) tolower(ch);
            }

    *to = '\0';
    return(iswild);
}


/************************************************
*                                               *
*  delete the words on the mis-spellings list   *
*                                               *
************************************************/

extern void
del_spellings(void)
{
    delete_list(&first_spell);
}


/********************************************************
*                                                       *
* return dictionary number from list of dictionaries    *
* if dictionary not open, open it iff create is TRUE    *
*                                                       *
********************************************************/

extern intl
dict_number(const char *name, BOOL create)
{
    LIST *lptr;
    intl dict, res;
    char *leaf;
    char buffer[MAX_FILENAME];

    tracef1("dict_number(%s)\n", trace_string(name));

    if(str_isblank(name))
        {
        most_recent = -1;
        return(ERR_BAD_NAME);
        }

    leaf = leafname(name);

    /* is it already on list? */

    for(lptr = first_in_list(&first_user_dict);
        lptr;
        lptr = next_in_list(&first_user_dict))
        {
        if(!stricmp(leaf, leafname((char *) lptr->value)))
            return(most_recent = (intl) lptr->key);
        }

    /* not on list so open it possibly */
    if(create)
        {
        if(!add_path(buffer, name, TRUE))
            dict = SPELL_ERR_CANTOPEN;
        else
            {
            if((dict = spell_opendict(buffer)) >= 0)
                {
                res = add_to_list(&first_user_dict, (word32) dict, (uchar *) buffer, &res);
                if(res <= 0)
                    {
                    (void) spell_close(dict);
                    dict = res ? res : ERR_NOROOM;
                    }
                else
                    return(most_recent = dict);
                }
            }
        }
    else
        dict = SPELL_ERR_CANTOPEN;

    most_recent = -1;
    return(dict);
}


/********************************************************
*                                                       *
*  get the word on the line currently, or previous one  *
*                                                       *
********************************************************/

static char *
get_word_from_line(intl dict, char *array /*out*/, intl stt_offset, intl *found_offsetp)
{
    char *to = array;
    intl ch, len;
    const uchar *src = linbuf;
    const uchar *ptr;

    tracef2("[get_word_from_line(): linbuf '%s', stt_offset = %d]\n", linbuf, stt_offset);

    if(!xf_inexpression  &&  slot_in_buffer)
        {
        len = strlen(src);
        ptr = src + min(len, stt_offset);

        /* goto start of this or last word */

        /* skip back until a word is hit */
        while((ptr > src)  &&  !spell_iswordc(dict, *ptr))
            --ptr;

        /* skip back until a valid word start is hit */
        while((ptr > src)  &&  spell_iswordc(dict, *(ptr-1)))
            --ptr;

        /* words must start with a letter form the current character set
         * as we don't know which dictionary will be used
        */
        while(spell_iswordc(dict, *ptr)  &&  !spell_valid_1(dict, *ptr))
            ++ptr;

        if(spell_valid_1(dict, *ptr))
            {
            if(found_offsetp)
                *found_offsetp = ptr - src;

            while(spell_iswordc(dict, *ptr)  &&  (to - array) < (MAX_WORD))
                {
                ch = (intl) *ptr++;

                *to++ = (char) (isalpha(ch) ? tolower(ch) : ch);
                }
            }
        }

    *to = '\0';

    tracef1("[get_word_from_line returns '%s']\n", trace_string((*array != '\0') ? array : NULL));

    return((*array != '\0') ? array : NULL);
}


/********************************************************************
*                                                                   *
* put dictionary name of most recently used dictionary in the field *
*                                                                   *
********************************************************************/

static BOOL
insert_most_recent(char **field)
{
    if(!check_spell_installed())
        return(FALSE);

    if(most_recent >= 0)
        return(str_setc(field, leafname(search_list(&first_user_dict, (word32) most_recent)->value)));

    #if RISCOS
    return(str_setc(field, USERDICT_STR));
    #else
    return(TRUE);
    #endif
}


static intl
open_appropriate_dict(const DIALOG *dptr)
{
    return( ((dptr->option == 'N')  ||  str_isblank(dptr->textfield))
                    ? open_master_dict()
                    : dict_number(dptr->textfield, TRUE)
            );
}


/****************************
*                           *
*  open master dictionary   *
*                           *
****************************/

static BOOL
err_open_master_dict(void)
{
    intl res = open_master_dict();

    if(res < 0)
        return(!reperr_module(ERR_SPELL, res));

    return(FALSE);
}


static intl
open_master_dict(void)
{
    char buffer[MAX_FILENAME];
    const char *name;

    if(master_dictionary < 0)
        {
        name = MASTERDICT_STR;

        if(!add_path(buffer, name, FALSE))
            {
            reperr(ERR_CANNOTOPEN, name);
            return(ERR_CANNOTOPEN);
            }

        master_dictionary = spell_opendict(buffer);
        }

    return(master_dictionary);
}


/****************************
*                           *
*  create user dictionary   *
*                           *
****************************/

extern void
overlay_CreateUserDict_fn(void)
{
    char buffer[MAX_FILENAME];
    char *name;
    intl res;

    if(!check_spell_installed())
        return;

    if(!init_dialog_box(D_USER_CREATE))
        return;

    while(dialog_box(D_USER_CREATE))
        {
        name = d_user_create[0].textfield;

        if(str_isblank(name))
            {
            reperr_null(ERR_BAD_NAME);
            (void) str_setc(&d_user_create[0].textfield, USERDICT_STR);
            continue;
            }

        name = add_prefix_to_name(buffer, name, TRUE);

        if(TRUE /*checkoverwrite(name)*/)
            {
            res = spell_createdict(name);

            if(res >= 0)
                res = dict_number(name, TRUE);

            if(res < 0)
                {
                reperr_module(ERR_SPELL, res);
                continue;
                }
            }

        if(dialog_box_ended())
            break;
        }
}


/************************
*                       *
* open user dictionary  *
*                       *
************************/

extern void
overlay_OpenUserDict_fn(void)
{
    char *name;
    intl res;

    if(!check_spell_installed())
        return;

    if(!init_dialog_box(D_USER_OPEN))
        return;

    while(dialog_box(D_USER_OPEN))
        {
        name = d_user_open[0].textfield;

        if(str_isblank(name))
            {
            reperr_null(ERR_BAD_NAME);
            (void) str_setc(&d_user_open[0].textfield, USERDICT_STR);
            continue;
            }

        /* open it, if not already open */
        res = dict_number(name, TRUE);

        if(res < 0)
            {
            reperr_module(ERR_SPELL, res);
            continue;
            }

        if(dialog_box_ended())
            break;
        }
}


/************************
*                       *
* close user dictionary *
*                       *
************************/

extern void
overlay_CloseUserDict_fn(void)
{
    char *name;
    intl res;

    if(!insert_most_recent(&d_user_close[0].textfield))
        return;

    while(dialog_box(D_USER_CLOSE))
        {
        name = d_user_close[0].textfield;

        if(str_isblank(name))
            {
            reperr_null(ERR_BAD_NAME);
            (void) str_setc(&d_user_close[0].textfield, USERDICT_STR);
            continue;
            }

        /* check it is open already, but not allowing it to be opened */
        res = dict_number(name, FALSE);

        if(res >= 0)
            res = close_dict(res);
        else
            res = 0;    /* ignore error from dict_number */

        if(res < 0)
            {
            reperr_module(ERR_SPELL, res);
            continue;
            }

        if(dialog_box_ended())
            break;
        }
}


/********************************
*                               *
*  delete word from dictionary  *
*                               *
********************************/

extern void
overlay_DeleteWordFromDict_fn(void)
{
    intl res;
    char array[LIN_BUFSIZ];

    if( !insert_most_recent(&d_user_delete[1].textfield)                                     ||
        err_open_master_dict()                                                               ||
        !str_setc(&d_user_delete[0].textfield, get_word_from_line(-1, array, lecpos, NULL))  )
            return;

    while(dialog_box(D_USER_DELETE))
        {
        res = dict_number(d_user_delete[1].textfield, TRUE);

        if(res >= 0)
            res = spell_deleteword(res, d_user_delete[0].textfield);

        if(res < 0)
            {
            reperr_module(ERR_SPELL, res);
            continue;
            }

        if(dialog_box_ended())
            break;
        }
}


/****************************
*                           *
* insert word in dictionary *
*                           *
****************************/

extern void
overlay_InsertWordInDict_fn(void)
{
    intl res;
    char array[LIN_BUFSIZ];

    if( !insert_most_recent(&d_user_insert[1].textfield)                                                    ||
        err_open_master_dict()                                                                              ||
        !str_setc(&d_user_insert[0].textfield, get_word_from_line(master_dictionary, array, lecpos, NULL))  )
            return;

    while(dialog_box(D_USER_INSERT))
        {
        res = dict_number(d_user_insert[1].textfield, TRUE);

        if(res >= 0)
            {
            res = spell_addword(res, d_user_insert[0].textfield);

            if(res == 0)
                res = ERR_WORDEXISTS;
            }

        if(res < 0)
            {
            reperr_module(ERR_SPELL, res);
            continue;
            }

        if(dialog_box_ended())
            break;
        }
}


/****************************************
*                                       *
*  close all of the user dictionaries   *
*                                       *
****************************************/

extern void
close_user_dictionaries(void)
{
    LIST *lptr;

    while((lptr = first_in_list(&first_user_dict)) != NULL)
        close_dict((intl) lptr->key);
}


#if RISCOS

/* all functions that use init,fill,end _box must have these icons */

#define browsing_Template   1
#define browsing_WordBox    2
#define browsing_ScrollUp   3   /* F2 */
#define browsing_ScrollDown 4   /* F3 */
#define browsing_PageUp     5   /* F4 */
#define browsing_PageDown   6   /* F5 */
#define browsing_FirstWord  7

#elif MS

#define dbox_OK             CR
#define dbox_CLOSE          ESCAPE

#define browsing_ScrollUp   UPCURSOR
#define browsing_ScrollDown DOWNCURSOR
#define browsing_PageUp     SUPCURSOR
#define browsing_PageDown   SDOWNCURSOR

#endif  /* RISCOS */


/********************************************
*                                           *
*  create/draw box to put browsed words in  *
*                                           *
********************************************/

static BOOL
init_box(merge_dump_strukt *mdsp,

#if RISCOS

         const char *dname, const char *title, BOOL statik)
{
    intl y, res;
    dbox d;
    void *core;
    char *errorp;

    tracef3("init_box(&%p, %s, static = %s):", mdsp, dname, trace_boolstring(statik));

    mdsp->res   = 0;
    mdsp->sofar = -1;

    /* get some memory for the words array */

    core = alloc_ptr_using_cache(sizeof(char [BROWSE_DEPTH][MAX_WORD+1]), &res);
    if(res < 0)
        return(reperr_null(res));
    if(!core)
        return(FALSE);

    mdsp->words = core;

    /* poke dbox with correct title */

    if(title)
        dbox_setinittitle(dname, title);

    /* create a dialog box */

    d = dbox_new(dname, &errorp);

    if(!d)
        {
        if(errorp)
            rep_fserr(errorp);
        free(core);
        return(FALSE);
        }

    mdsp->d = d;

    dbox_setfield(d, browsing_Template, NULLSTR);

    for(y = 0; y < BROWSE_DEPTH; ++y)
        {
        mdsp->words[y][0] = '\0';
        dbox_setfield(d, browsing_FirstWord + y, mdsp->words[y]);
        }

    if(statik)
        dbox_showstatic(d);
    else
        dbox_show(d);

    return(TRUE);
}

#elif MS || ARTHUR

         coord xsize, coord ysize, BOOL blips, const char *instruction)
{
    intl y, res;
    void *core;
    coord xpos, ypos, yboxsize;

    mdsp->res   = 0;
    mdsp->sofar = -1;

    /* get some memory for the words array */

    core = alloc_ptr_using_cache(sizeof(char [BROWSE_DEPTH][MAX_WORD+1]) + sizeof(intl [BROWSE_DEPTH]), &res);
    if(res < 0)
        return(reperr_null(res));
    if(!core)
        return(FALSE);

    mdsp->words   = core;
    mdsp->lastlen = core + sizeof(char [BROWSE_DEPTH][MAX_WORD+1]);


    xsize += 4;
/*  ysize += 6; */
    yboxsize = ysize + 6;

    xpos = (pagwid - xsize) / 2;
    ypos = (paghyt - yboxsize) / 2 + 1;

    /* draw box */
    save_screen(xpos, ypos, xsize, yboxsize);
    clip_menu(xpos, ypos,xsize, yboxsize);
    setcolour(MENU_HOT, MENU_BACK);
    my_rectangle(xpos, ypos, xsize, yboxsize);
    if(blips)
        draw_bar(xpos, ypos+3+BROWSE_CENTRE, xsize);

    /* clear box */
    for(y = ypos + 1; y < ypos + yboxsize - 1; ++y)
        {
        at(xpos + 1, y);
        ospca(xsize - 2);
        }

    for(y = 0; y < ysize; ++y)
        {
        mdsp->lastlen[y] = xsize - 4;
        *mdsp->words[y]  = '\0';
        }

    draw_bar(xpos, ypos + 2,            xsize);
    draw_bar(xpos, ypos + yboxsize - 3, xsize);

    setcolour(MENU_FORE, MENU_BACK);

    at(xpos + 2, ypos + yboxsize - 2);
    stringout(instruction);

    return(TRUE);
}

#endif  /* RISCOS */


/********************************************************************
*                                                                   *
* fill box with words                                               *
* first is a string to be put the top                               *
* words is an array of [BROWSE_DEPTH][MAX_WORD+1] of uchar words    *
* lastlen is the corresponding latest lengths of the words          *
*                                                                   *
********************************************************************/

static void
fill_box(char words[BROWSE_DEPTH][MAX_WORD+1], const char *first,

#if RISCOS

         dbox d)
{
    intl y;

    if(first)
        dbox_setfield(d, browsing_Template, first);

    for(y = 0; y < BROWSE_DEPTH; ++y)
        dbox_setfield(d, browsing_FirstWord + y, words[y]);
}

#elif MS || ARTHUR

         coord lastlen[BROWSE_DEPTH], coord xsize, coord ysize,
         const char *instruction)
{
    intl y;
    coord xpos, ypos;

    xsize += 4;
    ysize += 6;

    xpos = (pagwid - xsize) / 2;
    ypos = (paghyt - ysize) / 2 + 1;

    /* draw words */
    at(xpos + 2, ypos + 1);
    ospca(xsize - 4 - stringout(first));

    for(y = 0; y < BROWSE_DEPTH; ++y)
        {
        coord len;

        at(xpos + 2, ypos + 3 + y);
        len = stringout(words[y]);
        ospca(lastlen[y]-len);
        lastlen[y] = len;
        }

    if(!str_isblank(instruction))
        {
        at(xpos + 2, ypos + ysize - 2);
        ospca(xsize-4-stringout(instruction));
        }

    sb_show_if_fastdraw();
}

#endif  /* RISCOS */


static void
end_box(merge_dump_strukt *mdsp)
{
    mdsp->dict = -1;

    dispose((void **) &mdsp->words);

    #if RISCOS
    dbox_dispose(&mdsp->d);
    #elif MS
    dispose((void **) &mdsp->lastlen);
    #endif
}


#if RISCOS

static dbox_field
browse_fillin(dbox d)
{
    dochandle doc = current_document_handle();
    dbox_field f;

    f = dbox_fillin(d);

    select_document_using_handle(doc);

    return(f);
}

#endif  /* RISCOS */


static void
scroll_words_up(char words[BROWSE_DEPTH][MAX_WORD+1], intl depth)
{
    intl y;

    for(y = 0; y < depth-1; ++y)
        strcpy(words[y], words[y+1]);
}


static void
scroll_words_down(char words[BROWSE_DEPTH][MAX_WORD+1], intl depth)
{
    intl y;

    for(y = depth-1; y >= 1; --y)
        strcpy(words[y], words[y-1]);
}


#if RISCOS

static char *browse_wild_str;
static char *browse_template;
static char *browse_wild_string;
static BOOL  browse_iswild;
static BOOL  browse_was_spell_err;

/* has writeable template changed? - checked on null events */

extern void
browse_null(void)
{
    merge_dump_strukt *mdsp = &browse_mds;
    dbox d = mdsp->d;
    char *str = (browse_iswild) ? browse_wild_str : browse_template;
    char array[MAX_WORD+1];

    tracef0("browse_null()\n");

    dbox_getfield(d, browsing_Template, array, sizeof(array));

    if(stricmp(array, str))
        {
        if(browse_iswild)
            {
            bleep();
            dbox_setfield(d, browsing_Template, str);
            }
        else
            {
            tracef2("template changed from %s to %s\n", str, array);
            if(array[0])
                {
                strcpy(str, array);
                mdsp->res = get_and_display_words(TRUE, browse_iswild, mdsp->words,
                                                  browse_wild_string, browse_template,
                                                  browse_wild_str, mdsp->dict,
                                                  d, &browse_was_spell_err);
                if(mdsp->res < 0)
                    dbox_sendclose(d);
                /* which will cause a CLOSE to be returned to fillin */
                }
            else
                tracef0("string has been emptied");
            }
        }
    else
        tracef0("no change in string\n");
}

#endif  /* RISCOS */


/* get words from dictionary */

#if !RISCOS
static BOOL first;
#endif

static intl
get_and_display_words(BOOL fillall, BOOL iswild,
                      char words[BROWSE_DEPTH][MAX_WORD+1],
                      #if MS
                      coord lastlen[BROWSE_DEPTH],
                      #endif
                      char *wild_string, char *template,
                      char *wild_str, intl dict,
                      #if RISCOS
                      dbox d,
                      #endif
                      BOOL *was_spell_errp)
{
    intl res = 1;

    if(fillall)
        {
        intl y;

        strcpy(words[BROWSE_CENTRE], iswild ? NULLSTR : template);

        tracef3("fillall: iswild = %s, wild = '%s', template = '%s'\n",
                trace_boolstring(iswild), wild_string, template);

        if(iswild  ||  ((res = spell_checkword(dict, template)) == 0))
            if((res = spell_nextword(dict, words[BROWSE_CENTRE], words[BROWSE_CENTRE], wild_string, &ctrlflag)) == 0)
                res = spell_prevword(dict, words[BROWSE_CENTRE], template, wild_string, &ctrlflag);


        for(y = BROWSE_CENTRE + 1;
            !ctrlflag  &&  (res >= 0)  &&  (y < BROWSE_DEPTH);
            ++y)
            if(!*words[y-1]  ||  ((res = spell_nextword(dict, words[y], words[y-1], wild_string, &ctrlflag)) <= 0))
                words[y][0] = '\0';
            elif(iswild)
                #if RISCOS
                fill_box(words, wild_str, d);
                #else
                fill_box(words, wild_str,
                         lastlen, MAX_WORD, BROWSE_DEPTH, NULLSTR);
                #endif

        for(y = BROWSE_CENTRE-1; !ctrlflag  &&  (y >= 0) &&  (res >= 0); --y)
            if(!*words[y+1]  ||  ((res = spell_prevword(dict, words[y], words[y+1], wild_string, &ctrlflag)) <= 0))
                words[y][0] = '\0';

        if(ctrlflag)
            {
            ack_esc();
            return(ERR_ESCAPE);
            }

        if(res < 0)
            {
            *was_spell_errp = TRUE;
            return(res);
            }
        }

    #if RISCOS
    fill_box(words, iswild ? wild_str : template, d);
    #else
    fill_box(words, iswild ? wild_str : template,
             lastlen, MAX_WORD, BROWSE_DEPTH, first ? PREVNEXT_STR : NULLSTR);

    if(first)
        first = FALSE;
    #endif

    return(res);
}


#if RISCOS

static void
adjust(dbox_field *fp, dbox_field a, dbox_field b, BOOL adjustclicked)
{
    if(adjustclicked  &&  ((*fp == a)  ||  (*fp == b)))
        *fp = *fp ^ a ^ b;
}


/************************************************
*                                               *
* catch raw events sent to the browse dbox and  *
* turn (Page)Up/Down events into button hits    *
*                                               *
************************************************/

static BOOL
browse_raw_eventhandler(dbox d, void *event, void *handle)
{
    wimp_eventstr *e = (wimp_eventstr *) event;
    intl hiticon;

    IGNOREPARM(d);
    IGNOREPARM(handle);

    tracef1("raw_event_browse got %s\n", trace_wimp_event(e));

    switch(e->e)
        {
        case wimp_EKEY:
            switch(e->data.key.chcode)
                {
                case akbd_UpK:
                case akbd_DownK:
                    hiticon = (e->data.key.chcode == akbd_UpK)
                                        ? browsing_ScrollUp
                                        : browsing_ScrollDown;
                    break;

                case akbd_PageUpK:
                case akbd_PageDownK:
                    hiticon = (e->data.key.chcode == akbd_PageUpK)
                                        ? browsing_PageUp
                                        : browsing_PageDown;
                    break;

                default:
                    hiticon = -1;
                    break;
                }

            if(hiticon != -1)
                {
                e->e                = wimp_EBUT;
                e->data.but.m.i     = hiticon;
                e->data.but.m.bbits = wimp_BLEFT;

                clearkeyboardbuffer();
                }
            break;

        default:
            break;
        }

    return(FALSE);
}

#endif  /* RISCOS */


static void
browse_process(void)
{
    merge_dump_strukt *mdsp = &browse_mds;
    #if RISCOS
    dbox d = mdsp->d;
    dbox_field f;
    #elif MS
    intl f;
    #endif
    intl dict = mdsp->dict;
    char (*words)[MAX_WORD+1] = mdsp->words;
    intl i;
    intl which = -1;        /* which word was clicked on */
    BOOL fillall;
    char array[MAX_WORD+1];

    if(!*browse_template)
        {
        /* start off at 'a' if nothing specified */
        *browse_template   = 'a';
        browse_template[1] = '\0';
        }

    escape_enable();

    mdsp->res = get_and_display_words(TRUE, browse_iswild, words,
                                      #if MS
                                      mdsp->lastlen,
                                      #endif
                                      browse_wild_string, browse_template,
                                      browse_wild_str, dict,
                                      #if RISCOS
                                      d,
                                      #endif
                                      &browse_was_spell_err);

    if(mdsp->res < 0)
        {
        if(browse_was_spell_err)
            reperr_module(ERR_SPELL, mdsp->res);
        else
            reperr_null(mdsp->res);

        escape_disable();
        return;
        }

    if(escape_disable())
        return;


    #if RISCOS

    browsing_doc = current_document_handle();

    dbox_raw_eventhandler(d, browse_raw_eventhandler, NULL);

    while(((f = browse_fillin(d)) != dbox_CLOSE)  &&
          (f != dbox_OK))
        {
        BOOL adjustclicked = riscos_adjustclicked();

        adjust(&f, browsing_ScrollUp,   browsing_ScrollDown,    adjustclicked);
        adjust(&f, browsing_PageUp,     browsing_PageDown,      adjustclicked);

    #elif MS

    while(TRUE)
        {
        intl len;

        if(!*browse_template)
            {
            *browse_template   = 'a';
            browse_template[1] = '\0';
            }

        /* make sure cursor not off end of word */
        len = strlen(browse_template);

        if( cursor > len)
            cursor = len;

        /* position cursor */
        at(xpos + 2 + cursor, ypos + 1);

        /* let user move little cursor around */
        sb_show_if_fastdraw();

        #if MS
        if(!(auto_repeat  &&  depressed_shift()))
        #endif
            f = rdch(TRUE, TRUE);

        #if MS
        auto_repeat = FALSE;
        #endif

        clearkeyboardbuffer();

    #endif  /* RISCOS */


        fillall = FALSE;

        escape_enable();

        switch(f)
            {
            case browsing_ScrollUp:
                if(*words[BROWSE_CENTRE-1])
                    {
                    scroll_words_down(words, BROWSE_DEPTH);

                    if(!browse_iswild)
                        strcpy(browse_template, words[BROWSE_CENTRE]);

                    if(*words[0])
                        mdsp->res = spell_prevword(dict,
                                                   words[0],
                                                   words[0],
                                                   browse_wild_string,
                                                   &ctrlflag);

                    #if MS
                    auto_repeat = TRUE;
                    #endif
                    }
                break;


            case browsing_ScrollDown:
                if(*words[BROWSE_CENTRE+1])
                    {
                    scroll_words_up(words, BROWSE_DEPTH);

                    if(!browse_iswild)
                        strcpy(browse_template, words[BROWSE_CENTRE]);

                    if(*mdsp->words[BROWSE_DEPTH-1])
                        mdsp->res = spell_nextword(dict,
                                                   words[BROWSE_DEPTH-1],
                                                   words[BROWSE_DEPTH-1],
                                                   browse_wild_string,
                                                   &ctrlflag);

                    #if MS
                    auto_repeat = TRUE;
                    #endif
                    }
                break;


            case browsing_PageUp:
                if(browse_iswild)
                    bleep();
                else
                    {
                    mdsp->res = 1;

                    for(i = 0; (mdsp->res > 0)  &&  (i < BROWSE_DEPTH); ++i)
                        {
                        strcpy(array, browse_template);

                        mdsp->res = spell_prevword(dict,
                                                   browse_template,
                                                   browse_template,
                                                   browse_wild_string,
                                                   &ctrlflag);

                        if(!mdsp->res)
                            {
                            /* restore template if no word found */
                            strcpy(browse_template, array);
                            break;
                            }
                        }

                    fillall = TRUE;

                    #if MS
                    auto_repeat = TRUE;
                    #endif
                    }
                break;


            case browsing_PageDown:
                if(browse_iswild)
                    bleep();
                else
                    {
                    mdsp->res = 1;

                    for(i = 0; (mdsp->res > 0)  &&  (i < BROWSE_DEPTH); ++i)
                        {
                        strcpy(array, browse_template);

                        mdsp->res = spell_nextword(dict,
                                                   browse_template,
                                                   browse_template,
                                                   browse_wild_string,
                                                   &ctrlflag);

                        if(!mdsp->res)
                            {
                            /* restore template if no word found */
                            strcpy(browse_template, array);
                            break;
                            }
                        }

                    fillall = TRUE;

                    #if MS
                    auto_repeat = TRUE;
                    #endif
                    }
                break;


            #if RISCOS

            default:
                /* clicks on words enter them */
                which = f - browsing_FirstWord;
                if( (which >= 0)  &&  (which < BROWSE_DEPTH)  &&
                    *words[which])
                        goto LOOP_EXIT;
                else
                    which = -1;
                break;

            #elif MS || ARTHUR

            default:
                if(browse_iswild)
                    bleep();
                elif(spell_iswordc(dict, c)  &&  (c > 0)  &&  (c < 0x80)  &&  (cursor < MAX_WORD))
                    {
                    fillall = TRUE;
                    browse_template[cursor++] = (uchar) c;
                    browse_template[cursor]   = '\0';
                    }
                elif(c == SPACE)
                    {
                    fillall = TRUE;
                    browse_template[cursor] = '\0';
                    }
                else
                    bleep();

                break;


            case RIGHTCURSOR:
                if(browse_iswild)
                    bleep();
                elif(cursor <= strlen(browse_template))
                    ++cursor;
                break;


            case LEFTCURSOR:
                if(browse_iswild)
                    bleep();
                elif(cursor > 0)
                    --cursor;
                break;


            #if ARTHUR
            case LEFTDELETE:
            #endif
            case RUBOUT:
                if(browse_iswild)
                    bleep();
                elif(cursor > 0)
                    {
                    if(--cursor > 0)
                        browse_template[cursor] = '\0';

                    fillall = TRUE;
                    }
                break;

            #endif  /* RISCOS */

            }

        mdsp->res = get_and_display_words(fillall, browse_iswild, words,
                                          #if MS
                                          mdsp->lastlen,
                                          #endif
                                          browse_wild_string, browse_template,
                                          browse_wild_str, dict,
                                          #if RISCOS
                                          d,
                                          #endif
                                          &browse_was_spell_err);

        if(mdsp->res < 0)
            {
            escape_disable();
            break;
            }


    LOOP_EXIT:

        if(escape_disable())
            {
            mdsp->res = ERR_ESCAPE;
            break;
            }

        if(which >= 0)
            {
            f = dbox_OK;
            break;
            }
        }


    /* click/type-ahead may have resulted in string emptying
     * without us getting to see it
    */
    if((mdsp->res >= 0)  &&  (f == dbox_OK))
        {
        if(which >= 0)
            strcpy(word_to_insert, words[which]);
        else
            {
            #if RISCOS
            dbox_getfield(d, browsing_Template, browse_template, LIN_BUFSIZ);
            #endif

            if(!browse_template[0])
                {
                word_to_insert[0] = 'a';
                word_to_insert[1] = browse_template[0];
                }
            else
                {
                if(!browse_iswild)
                    strcpy(word_to_insert, browse_template);
                else
                    strcpy(word_to_insert, words[BROWSE_CENTRE]);
                }
            }
        }

    tracef1("returning word_to_insert \"%s\"\n", word_to_insert);

    #if RISCOS
    browsing_doc = DOCHANDLE_NONE;
    #endif
}


/********************************
*                               *
*  browse through a dictionary  *
*                               *
********************************/

static intl
browse(intl dict, char *wild_str)
{
    char template[LIN_BUFSIZ];
    char wild_string[LIN_BUFSIZ];
    merge_dump_strukt *mdsp = &browse_mds;
    #if MS
    intl browse_iswild = FALSE;
    coord xpos = (pagwid - (4 + MAX_WORD)) / 2;
    intl res = 1;
    coord ypos = (paghyt - (6 + BROWSE_DEPTH)) / 2 + 1;
    intl c, y;
    coord cursor = 0;
    BOOL fillall = TRUE;
    BOOL first = TRUE;
    BOOL auto_repeat = FALSE;
    #endif

    /* compile the wild string */
    *template = '\0';

    if(!wild_str)
        wild_str = NULLSTR;

    browse_iswild = compile_wild_string(dict, wild_string, wild_str);
    if(browse_iswild  < 0)
        return(browse_iswild);

    if(!browse_iswild)
        {
        *wild_string = '\0';
        strcpy(template, wild_str);
        }

    *word_to_insert = template[MAX_WORD] = '\0';

#if RISCOS

    mdsp->dict = dict;

    browse_wild_str     = wild_str;
    browse_template     = template;
    browse_wild_string  = wild_string;

    if(init_box(mdsp, "browsing", NULL, FALSE))
#elif MS
    if(init_box(mdsp, MAX_WORD, BROWSE_DEPTH, TRUE, NULLSTR))
#endif  /* RISCOS */

        {
        browse_process();

        end_box(mdsp);
        }

    return(mdsp->res);
}


/************************************************************
*                                                           *
*  open dictionary and browse through it reporting errors   *
*                                                           *
************************************************************/

extern void
overlay_BrowseDictionary_fn(void)
{
    intl res;
    char array[LIN_BUFSIZ];
    intl found_offset;

    d_user_browse[1].option = 'N';

    if( !insert_most_recent(&d_user_browse[1].textfield)                                                             ||
        err_open_master_dict()                                                                                       ||
        !str_setc(&d_user_browse[0].textfield, get_word_from_line(master_dictionary, array, lecpos, &found_offset))  )
            return;

    if(!dialog_box(D_USER_BROWSE))
        return;

    dialog_box_end();

    browse_was_spell_err = FALSE;

    res = open_appropriate_dict(&d_user_browse[1]);

    if(res >= 0)
        res = browse(res, d_user_browse[0].textfield);

    if(res < 0)
        {
        if(browse_was_spell_err)
            reperr_module(ERR_SPELL, res);
        else
            reperr_null(res);
        }
    elif(*word_to_insert)
        {
        tracef1("[SBRfunc * word_to_insert: %s]\n", trace_string(word_to_insert));

        /* delete word in text, copying original (unfolded) to array & then replace it */
        delete_bit_of_line(found_offset, strlen(array), TRUE);

        #if !defined(SEARCH_OFF)
        sch_pos_end.col = sch_pos_stt.col = curcol;
        sch_pos_end.row = sch_pos_stt.row = currow;
        sch_end_offset  = sch_stt_offset;

        if(!do_replace(word_to_insert))
            bleep();
        #endif
        }
}


/*
 * get all anagrams of string
 * return FALSE if error or escape
 * TRUE otherwise
*/

/*
 * copy the matching letters from lastword into array
 * fill out array with smallest word
*/

static BOOL
a_fillout(A_LETTER *letters, char *array, char *lastword, BOOL subgrams)
{
    A_LETTER *aptr = letters;
    uchar *from = lastword;
    uchar *to = array;
    intl length = 0;
    BOOL dontfillout = FALSE;

    /* set all letters unused */
    while(aptr->letter)
        {
        (aptr++)->used = FALSE;
        length++;
        }

    /* get the common letters */
    for( ; *from; from++)
        for(aptr = letters; ; aptr++)
            {
            if(!aptr->letter)
                {
                /* need to step back letter */
                dontfillout = TRUE;

                for(;;)
                    {
                    uchar letter_to_free;
                    A_LETTER *smallest_bigger = NULL;

                    /* maybe no letters to step back */
                    if(--to < array)
                        return(FALSE);

                    /* free the letter that is here and look for new letter */
                    letter_to_free = *to;

                    /* search backwards, picking up the smallest bigger letter */
                    for(aptr = letters + length; ; --aptr)
                        {
                        if(aptr->used == FALSE)
                            {
                            if(aptr->letter > letter_to_free)
                                {
                                smallest_bigger = aptr;
                                continue;
                                }
                            }
                        elif(aptr->letter == letter_to_free)
                            {
                            aptr->used = FALSE;

                            if(smallest_bigger != NULL)
                                {
                                smallest_bigger->used = TRUE;
                                *to++ = smallest_bigger->letter;
                                goto DO_FILL;
                                }
                            /* go back another letter */
                            break;
                            }
                        }
                    }
                }

            if(aptr->used)
                continue;

            if(*from < aptr->letter)
                {
                /* no match so take next biggest letter */
                aptr->used = TRUE;
                *to++ = aptr->letter;
                dontfillout = TRUE;
                goto DO_FILL;
                }

            if(*from == aptr->letter)
                {
                /* this letter matches, flag it and go round again */
                aptr->used = TRUE;
                *to++ = *from;
                break;
                }
            }

DO_FILL:

    /* now fill out with remains */
    if(!subgrams  ||  (subgrams  &&  !dontfillout  &&  (to == array)))
        for(aptr = letters; aptr->letter; aptr++)
            if(aptr->used == FALSE)
                {
                *to++ = aptr->letter;
                if(subgrams)
                    break;
                }

    *to = '\0';
    return(TRUE);
}


#if RISCOS

static void
add_word_to_box(merge_dump_strukt *mdsp, const char *str)
{
    tracef1("adding word %s to box\n", str);

    if(++(mdsp->sofar) >= BROWSE_DEPTH)
        {
        scroll_words_up(mdsp->words, BROWSE_DEPTH);
        mdsp->sofar = BROWSE_DEPTH-1;
        }

    strcpy((mdsp->words)[mdsp->sofar], str);

    fill_box(mdsp->words, NULL, mdsp->d);
}


static void
complete_box(merge_dump_strukt *mdsp, const char *str)
{
    char array[64];

    sprintf(array, Zs_complete_STR, str);

    add_word_to_box(mdsp, NULLSTR);
    add_word_to_box(mdsp, array);
}


static char     *anagram_last_found;
static A_LETTER *anagram_letters;
static BOOL      anagram_sub;
static BOOL      anagram_stopped;

#define anagram_Stop        0
#define anagram_Pause       3
#define anagram_Continue    4

extern void
anagram_null(void)
{
    merge_dump_strukt *mdsp = &anagram_mds;
    char newword[MAX_WORD+1];

    tracef0("anagram_null()\n");

    if(a_fillout(anagram_letters, newword, anagram_last_found, anagram_sub))
        {
        mdsp->res = spell_checkword(mdsp->dict, newword);

        if(mdsp->res < 0)
            {
            dbox_sendclose(mdsp->d);
            return;
            }

        /* don't like most single letter words */
        if( mdsp->res  &&
            ((newword[1])  || (*newword == 'i') ||  (*newword == 'a'))
            )
            add_word_to_box(mdsp, newword);

        escape_enable();

        if(!spell_valid_1(mdsp->dict, *newword))
            *newword = '\0';

        mdsp->res = spell_nextword(mdsp->dict, anagram_last_found,
                                        newword, NULL, &ctrlflag);

        if(escape_disable()  ||  (mdsp->res < 0))
            {
            dbox_sendclose(mdsp->d);
            return;
            }
        }
    else
        {
        mdsp->res = 0;
        anagram_stopped = TRUE;
        }

    if(!mdsp->res)
        {
        complete_box(mdsp, anagram_sub ? Subgrams_STR : Anagrams_STR);

        /* force punter to do explicit end */
        anagram_doc = DOCHANDLE_NONE;
        return;
        }
}


static void
anagram_process(void)
{
    merge_dump_strukt *mdsp = &anagram_mds;
    dbox_field f;

    anagram_doc = current_document_handle();

    /* all filling done on null events */

    while(((f = browse_fillin(mdsp->d)) != dbox_CLOSE)  &&
          (f != dbox_OK))
        {
        switch(f)
            {
            case anagram_Continue:
                if(!anagram_stopped)
                    anagram_doc = current_document_handle();
                break;

            case anagram_Pause:
                anagram_doc = DOCHANDLE_NONE;
                /* stops null events to anagram processor */
                break;

            default:
                break;
            }
        }

    anagram_doc = DOCHANDLE_NONE;
}

#endif  /* RISCOS */


/*
 * find anagrams in the specified dictionary of this collection of letters
 * assumes letters sorted into order
 * works like pair of co-routines
 *      a_fillout gets asked to supply next possible string
 *      spell_next gets asked for next word in dictionary
*/

static intl
do_anagram(intl dict, A_LETTER *letters, BOOL sub)
{
    char last_found[MAX_WORD+1];
    uchar newword[MAX_WORD+1];
    merge_dump_strukt *mdsp = &anagram_mds;
    #if !RISCOS
    char header_string[32+MAX_WORD*2];
    #endif

    *last_found = *newword = '\0';

    #if RISCOS

    mdsp->dict = dict;

    anagram_last_found  = last_found;
    anagram_letters     = letters;
    anagram_sub         = sub;
    anagram_stopped     = FALSE;

    if(init_box(mdsp, "anagram", sub ? Subgrams_STR : Anagrams_STR, FALSE))
        {
        dbox_setfield(mdsp->d, browsing_Template, d_user_anag[0].textfield);

        anagram_process();

        end_box(mdsp);
        }

    #else   /* RISCOS */

    sprintf(header_string, Zs_of_Zs_STR,
            sub ? Subgrams_STR : Anagrams_STR,
            d_user_anag[0].textfield);

    init_box(mdsp, MAX_WORD, BROWSE_DEPTH, FALSE, SHIFT_STR);

    for(;;)
        {
        if(!a_fillout(letters, newword, last_found, sub))
            break;

        mdsp->res = spell_checkword(dict, (char *) newword);

        if(mdsp->res <= 0)
            break;

        /* don't like most single letter words */
        if(newword[1] || *newword == 'i' || *newword == 'a')
            if(mdsp->res > 0)
                {
                if(++(mdsp->sofar) > BROWSE_DEPTH-1)
                    {
                    scroll_words_up(mdsp->words, BROWSE_DEPTH);
                    mdsp->sofar = BROWSE_DEPTH-1;
                    }

                strcpy(mdsp->words[mdsp->sofar], (char *) newword);

                fill_box(mdsp->words, header_string,
                         lastlen, MAX_WORD, BROWSE_DEPTH, NULLSTR);

                while(depressed_shift())
                    ;

                if(ctrlflag)
                    break;
                }

        if(!isalpha(*newword))
            *newword = '\0';

        mdsp->res = spell_nextword(dict, (char *) last_found, (char *) newword, NULL, &ctrlflag);

        if(mdsp->res <= 0)
            break;
        }

    if(mdsp->res >= 0)
        {
        fill_box(mdsp->words, header_string,
                 lastlen, MAX_WORD, BROWSE_DEPTH, (char *) PRESSANYKEY_STR);

        rdch(FALSE, TRUE);

        ack_esc();

        mdsp->res = 0;
        }

    end_box(mdsp);

    #endif  /* RISCOS */

    return(mdsp->res);
}


/*
top-level routine for anagrams and subgrams
*/

static void
ana_or_sub_gram(BOOL sub)
{
    intl dict, res;
    char *word;
    char array[MAX_WORD+1];
    A_LETTER letters[MAX_WORD+1];
    A_LETTER *aptr;
    uchar *from, *to;
    char ch;

    #if RISCOS
    if(anagram_mds.d)
        {
        reperr_null(anagram_sub ? ERR_ALREADYSUBGRAMS : ERR_ALREADYANAGRAMS);
        return;
        }
    #endif

    if(!insert_most_recent(&d_user_anag[1].textfield))
        return;

    while(dialog_box(sub ? D_USER_SUBGRAM : D_USER_ANAG))
        {
        word = d_user_anag[0].textfield;

        if(str_isblank(word)  ||  (strlen(word) > MAX_WORD))
            {
            reperr_null(SPELL_ERR_BADWORD);
            continue;
            }

        dialog_box_end();

        dict = open_appropriate_dict(&d_user_anag[1]);

        res = (dict < 0)
                ? dict
                : compile_wild_string(dict, array, word);

        if(res >= 0)
            {
            /* sort letters into order */
            from = word;
            *array = '\0';
            while((ch = *from++) != '\0')
                {
                ch = tolower(ch);

                for(to = array; ; to++)
                    if(!*to  ||  (*to > ch))
                        {
                        memmove(to + 1, to, strlen((char *) to) + 1);
                        *to = ch;
                        break;
                        }
                }

            tracef1("array=_%s_\n", array);

            /* copy letters into letters array */
            aptr = letters;
            from = array;
            do  {
                ch = *from++;
                aptr++->letter = ch;
                }
            while(ch);

            res = do_anagram(dict, letters, sub);
            }

        if((res < 0)  &&  !ctrlflag)
            reperr_module(ERR_SPELL, res);

        if(ctrlflag)
            {
            ack_esc();
            reperr_null(ERR_ESCAPE);
            }

        if(dialog_box_ended())
            break;
        }
}


/***********
*          *
* anagrams *
*          *
***********/

extern void
overlay_Anagrams_fn(void)
{
    ana_or_sub_gram(FALSE);
}


/***********
*          *
* subgrams *
*          *
***********/

extern void
overlay_Subgrams_fn(void)
{
    ana_or_sub_gram(TRUE);
}


/************************************
*                                   *
*  switch auto checking on or off   *
*                                   *
************************************/

extern void
overlay_AutoSpellCheck_fn(void)
{
    if(!check_spell_installed())
        return;

    #if RISCOS

    d_auto[0].option = (optiontype) (d_auto[0].option ^ ('Y' ^ 'N'));
    check_state_changed();

    #else

    while(dialog_box(D_AUTO))
        {
        check_state_changed();

        if(dialog_box_ended())
            break;
        }

    #endif
}


/************************************************
*                                               *
*  check word is not in the user dictionaries   *
*                                               *
************************************************/

static BOOL
not_in_user_dicts_or_list(const char *word)
{
    LIST *lptr;
    intl res;

    for(lptr = first_in_list(&first_spell);
        lptr;
        lptr = next_in_list(&first_spell))
            if(!strcmp((char *) lptr->value, word))
                return(FALSE);

    for(lptr = first_in_list(&first_user_dict);
        lptr;
        lptr = next_in_list(&first_user_dict))
            if((res = spell_checkword((intl) lptr->key, (char *) word)) > 0)
                return(FALSE);
            elif((res < 0)  &&  (res != SPELL_ERR_BADWORD))
                reperr_module(ERR_SPELL, res);

    return(TRUE);
}


#define C_BLOCK 0
#define C_ALL 1

/*
 * set up the block to check
*/

static BOOL
set_check_block(void)
{
    /* set up the check block */

    if(d_checkon[C_BLOCK].option == 'Y')
        {
        /* marked block selected - check exists */
        if(!MARKER_DEFINED())
            return(reperr_null((blkstart.col != NO_COL)
                                        ? ERR_NOBLOCKINDOC
                                        : ERR_NOBLOCK));

        sch_stt = blkstart;
        sch_end = (blkend.col != NO_COL) ? blkend : blkstart;
        }
    else
        {
        /* all rows */

        sch_stt.row = 0;
        sch_end.row = numrow-1;

        sch_stt.col = 0;
        sch_end.col = numcol-1;
        }

    return(TRUE);
}


/*
 * readjust sch_stt_offset at start of next word on line
 * assumes line in buffer
*/

static BOOL
next_word_on_line(void)
{
    intl len = strlen((char *) linbuf);
    uchar *ptr;

    if(sch_stt_offset >= len)
        return(FALSE);

    /* if pointing to a word, skip it */
    ptr = linbuf + sch_stt_offset;

    if(spell_valid_1(master_dictionary, *ptr++))
        do { ; } while(spell_iswordc(master_dictionary, *ptr++));

    sch_stt_offset = (--ptr) - linbuf;

    while(!spell_valid_1(master_dictionary, *ptr)  &&  (sch_stt_offset < len))
        {
        ++ptr;
        ++sch_stt_offset;
        }

    return(sch_stt_offset < len);
}


/*
 * find the next mis-spelt word starting with the current word
*/

static BOOL
get_next_misspell(char *array /*out*/)
{
    intl res;
    slotp tslot;
    char *ptr;
    BOOL tried_no_quote;

    tracef1("[get_next_misspell(): sch_stt_offset = %d\n]", sch_stt_offset);

    for(;;)
        {
        if(sch_stt_offset == -1)
            {
            /* find another slot to scan for misspells (NB. set sch_pos_stt.col = sch_stt.col - 1 to start) */

            do  {
                if(sch_pos_stt.col < sch_end.col)
                    {
                    ++sch_pos_stt.col;
                    tracef1("[get_next_misspell stepped to column %d]\n", sch_pos_stt.col);
                    }
                elif(sch_pos_stt.row < sch_end.row)
                    {
                    ++sch_pos_stt.row;
                    sch_pos_stt.col = sch_stt.col;
                    tracef2("[get_next_misspell stepped to row %d, reset to column %d]\n", sch_pos_stt.row, sch_pos_stt.col);
                    if(!actind(ACT_CHECK, (intl) ((100 * sch_pos_stt.row - sch_stt.row) / (sch_end.row - sch_stt.row + 1))))
                        return(FALSE);
                }
                else
                    {
                    tracef0("[get_next_misspell ran out of slots]\n");
                    return(FALSE);
                    }

                tslot = travel(sch_pos_stt.col, sch_pos_stt.row);
                }
            while(!tslot  ||  (tslot->type != SL_TEXT)  ||  str_isblank(tslot->content.text));

            prccon(linbuf, tslot);

            sch_stt_offset = 0;
            }

        /* if current word not ok set variables */

        if(spell_valid_1(master_dictionary, linbuf[sch_stt_offset]))
            {
            tried_no_quote = FALSE;

            slot_in_buffer = TRUE;
            (void) get_word_from_line(master_dictionary, array, sch_stt_offset, NULL);
            slot_in_buffer = FALSE;

        TRY_WITH_NO_QUOTE:

            res = spell_checkword(master_dictionary, array);

            if(res == SPELL_ERR_BADWORD)
                res = 0;

            if(res < 0)
                return(reperr_module(ERR_SPELL, res));

            if(!res  &&  (not_in_user_dicts_or_list(array)))
                {
                /* can't find it anywhere: try stripping off trailing ' or 's */
                ptr = array + strlen(array);

                if( !tried_no_quote  &&
                    (   (*--ptr == '\'')  ||
                        ((tolower(*ptr) == 's')  &&  (*--ptr == '\''))))
                    {
                    tried_no_quote = TRUE;
                    *ptr = '\0';
                    goto TRY_WITH_NO_QUOTE;
                    }

                /* not in any dictionary, with or without quote: move there! */
                chknlr(sch_pos_stt.col, sch_pos_stt.row);
                return(TRUE);
                }
            }

        /* go to next word if available, else reload from another slot */
        if(!next_word_on_line())
            sch_stt_offset = -1;
        }
}


#define C_CHANGE 0
#define C_BROWSE 1
#define C_ADD    2

/************************
*                       *
*  check current file   *
*                       *
************************/

extern void
overlay_CheckDocument_fn(void)
{
    SLR  oldpos;
    intl tlecpos;
    optiontype tcase;
    intl mis_spells = 0, words_added = 0, res;
    BOOL checking_all;
    char array[MAX_WORD];
    char original[LIN_BUFSIZ];
    char array1[64];
    char array2[64];

    if(!check_spell_installed())
        return;

    (void) init_dialog_box(D_CHECKON);

    if(!dialog_box(D_CHECKON))
        return;

    dialog_box_end();

    if( !mergebuf_nocheck()     ||
        !set_check_block()      ||
        err_open_master_dict()  )
            return;

    #if RISCOS
    /* must be at least one line we can get to */
    if(n_rowfixes >= rowsonscreen)
    #elif MS || ARTHUR
    /* yuk - word mustn't be behind dialog box */
    if(n_rowfixes > 0)
    #endif
        FixRows_fn();

    if( glbbit  &&
        (d_checkon[C_ALL].option == 'Y')  &&
        (d_checkon[C_BLOCK].option != 'Y'))
        {
        checking_all = TRUE;

        if(!iniglb())
            return;
        }
    else
        checking_all = FALSE;

    /* save current position */
    oldpos.col = curcol;
    oldpos.row = currow;
    tlecpos    = lecpos;

    sch_pos_stt.col = sch_stt.col - 1;
    sch_pos_stt.row = sch_stt.row;
    sch_stt_offset  = -1;

    escape_enable();

    while(!been_error)
        {
        /* find next mis-spelled word */
        for(; get_next_misspell(array) && !been_error && !ctrlflag; ++mis_spells)
            {
            actind(DEACTIVATE, NO_ACTIVITY);

            /* NB. lowercase version! */
            if(!str_set(&d_check[C_CHANGE].textfield, array))
                break;

            d_check[C_CHANGE].option = d_check[C_BROWSE].option = 'N';

            /* take a copy of the misplet worm */
            *original = '\0';
            strncat(original, linbuf + sch_stt_offset, strlen(array));
            tracef1("[CheckDocument_fn: misplet word is '%s']\n", original);

            word_to_invert = original;
            lecpos = sch_stt_offset;

            draw_screen();

    DOG_BOX:
            (void) insert_most_recent(&d_check[C_ADD].textfield);

            clearkeyboardbuffer();

            if(!dialog_box(D_CHECK))
                {
                been_error = TRUE;
                break;
                }

            dialog_box_end();

            /* add to user dictionary? */
            if(d_check[C_ADD].option == 'Y')
                {
                intl dict = dict_number(d_check[C_ADD].textfield, TRUE);

                if(dict >= 0)
                    {
                    if((res = spell_addword(dict,
                                            d_check[C_CHANGE].textfield))
                                            < 0)
                        dict = res;
                    elif(res > 0)
                        ++words_added;
                    }

                if(dict < 0)
                    {
                    reperr_module(ERR_SPELL, dict);
                    break;
                    }
                }

            /* browse? */
            if(d_check[C_BROWSE].option == 'Y')
                {
                res = browse(master_dictionary, d_check[C_CHANGE].textfield);

                tracef1("word_to_insert = _%s_", word_to_insert);

                if(res < 0)
                    {
                    reperr_module(ERR_SPELL, res);
                    break;
                    }
                elif(*word_to_insert)
                    {
                    str_set(&d_check[C_CHANGE].textfield, word_to_insert);
                    d_check[C_CHANGE].option = 'Y';
                    }

                d_check[C_BROWSE].option = 'N';
                goto DOG_BOX;
                }

            /* redraw the row sometime */
            word_to_invert = NULL;
            mark_row(currowoffset);

            /* replace word? */
            if(d_check[C_CHANGE].option == 'Y')
                {
                sch_pos_end.col = sch_pos_stt.col;
                sch_pos_end.row = sch_pos_stt.row;
                sch_end_offset  = sch_stt_offset + strlen((char *) array);

                filbuf();

                #if !defined(SEARCH_OFF)
                tcase = d_search[SCH_CASE].option;
                d_search[SCH_CASE].option = 'Y';
                res = do_replace((uchar *) d_check[C_CHANGE].textfield);
                d_search[SCH_CASE].option = tcase;

                if(!res)
                    {
                    bleep();
                    break;
                    }

                if(!mergebuf_nocheck())
                    break;
                #endif
                }
            /* if all set to no, add to temporary spell list */
            elif((d_check[C_BROWSE].option == 'N')  &&  (d_check[C_ADD].option == 'N'))
                {
                res = add_to_list(&first_spell, (word32) 0, array, &res);
                if(res <= 0)
                    {
                    reperr_null(res ? res : ERR_NOROOM);
                    break;
                    }
                }

/*          filbuf();

            while(spell_iswordc(master_dictionary, linbuf[lecpos]))
                ++lecpos;
*/          }

        if(!checking_all  ||  nxfloa())
            break;
        }

    word_to_invert = NULL;

    (void) escape_disable();

    actind(DEACTIVATE, NO_ACTIVITY);


    /* restore old position */
    (void) mergebuf_nocheck();

    if(!checking_all)
        {
        chknlr(curcol = oldpos.col, currow = oldpos.row);
        lecpos = tlecpos;
        }

    out_screen = xf_drawcolumnheadings = TRUE;

    if(!in_execfile)
        {
        /* put up message box saying what happened */
        sprintf(array1, Zd_unrecognised_Zs_STR,
                mis_spells, (mis_spells == 1) ? word_STR : words_STR);
        sprintf(array2, Zd_Zs_added_to_user_dict_STR,
                words_added, (words_added == 1) ? word_STR : words_STR);

        #if RISCOS
        d_check_mess[0].textfield = array1;
        d_check_mess[1].textfield = array2;
        #elif MS || ARTHUR
        d_check_mess[0].tag = array1;
        d_check_mess[1].tag = array2;
        #endif

        if(dialog_box(D_CHECK_MESS))
            dialog_box_end();
        }
}


/****************************************************************************
*                                                                           *
*                       Merge file with dictionary                          *
*                                                                           *
****************************************************************************/

/************************************
*                                   *
*  get the next word from the file  *
*                                   *
************************************/

static BOOL
get_word_from_file(intl dict, FILE *in, char *array /*out*/, BOOL *hasforeignp /*out*/)
{
    intl c;
    char *ptr;

    if(hasforeignp)
        *hasforeignp = FALSE;

    for(;;)
        {
        ptr = array;

        for(;;)
            {
            if((c = myfgetc(in)) == EOF)
                return(FALSE);

            if(spell_valid_1(dict, c))
                break;
            }

        /* c is first letter of word */

        for(;;)
            {
            if(ptr - array > MAX_WORD)
                break;

            *ptr++ = (char) c;

            c = myfgetc(in);

            if((c == EOF)  ||  !spell_iswordc(dict, c))
                {
                *ptr = '\0';
                return(TRUE);
                }
            }
        }
}


static FILE *mergedict_in        = NULL;
static char *mergedict_in_buffer = NULL;
static char  mergedict_array[MAX_WORD+1];

static void
mergedict_close(void)
{
    if(mergedict_in)
        {
        myfclose(mergedict_in);
        mergedict_in = NULL;
        }

    dispose((void **) &mergedict_in_buffer);
}


/* mergedict complete - tidy up */

static void
mergedict_end(merge_dump_strukt *mdsp)
{
    #if RISCOS
    mergedict_wants_nulls = FALSE;
    #endif

    #if !RISCOS
    if(ctrlflag)
        {
        ack_esc();
        reperr_null(ERR_ESCAPE);
        }
    else
    #endif

    if(mdsp->res < 0)
        reperr_module(ERR_SPELL, mdsp->res);

    mergedict_close();

    end_box(mdsp);
}


#if RISCOS

#define mergedict_Stop      0
#define mergedict_Pause     3
#define mergedict_Continue  4

/* mergedict upcall processor */

static void
mergedict_eventhandler(dbox d, void *handle)
{
    merge_dump_strukt *mdsp = (merge_dump_strukt *) handle;
    dbox_field f = dbox_get(d);

    tracef2("mergedict_eventhandler got button %d: mergedict_in = &%p\n", f, mergedict_in);

    switch(f)
        {
        case mergedict_Continue:
            /* only resume if paused with file still open */
            if(mergedict_in)
                mergedict_wants_nulls = TRUE;
            break;

        case mergedict_Pause:
            /* stop null events to mergedict null processor */
            mergedict_wants_nulls = FALSE;
            break;

        default:
            mergedict_end(mdsp);
            break;
            }
}


/* mergedict null processor */

extern void
mergedict_null(void)
{
    merge_dump_strukt *mdsp = &mergedict_mds;
    BOOL hasforeign;

    tracef0("mergedict_null()\n");

    if(!get_word_from_file(mdsp->dict, mergedict_in, mergedict_array, &hasforeign))
        {
        complete_box(mdsp, Merge_STR);

        mergedict_close();

        /* force punter to do explicit end */
        mergedict_wants_nulls = FALSE;
        return;
        }

    /* don't add foreign words till spell can! */
    if(hasforeign)
        return;

    mdsp->res = spell_addword(mdsp->dict, mergedict_array);

    if(mdsp->res < 0)
        {
        dbox_sendclose(mdsp->d);
        return;
        }

    if(mdsp->res)
        add_word_to_box(&mergedict_mds, mergedict_array);
}


static void
mergedict_start(void)
{
    merge_dump_strukt *mdsp = &mergedict_mds;

    mergedict_wants_nulls = TRUE;

    dbox_eventhandler(mdsp->d, mergedict_eventhandler, mdsp);

    /* all merging done on null events, button processing on upcalls */
}

#endif  /* RISCOS */


/************************************
*                                   *
*  merge file with user dictionary  *
*                                   *
************************************/

extern void
overlay_MergeFileWithDict_fn(void)
{
    char buffer[MAX_FILENAME];
    char *name;
    merge_dump_strukt *mdsp = &mergedict_mds;
    intl res;

    #if RISCOS
    if(mdsp->d)
        {
        reperr_null(ERR_ALREADYMERGING);
        return;
        }
    #endif

    /* suggest a file to merge with */
    if( !insert_most_recent(&d_user_merge[1].textfield)  ||
        (str_isblank(d_user_merge[0].textfield)  &&  !str_setc(&d_user_merge[0].textfield, DUMPFILE_STR)))
            return;

    if(!dialog_box(D_USER_MERGE))
        return;

    dialog_box_end();

    /* open file */
    name = d_user_merge[0].textfield;

    if(str_isblank(name))
        {
        reperr_null(ERR_BAD_NAME);
        return;
        }

    if(!add_path(buffer, name, TRUE))
        {
        reperr(ERR_NOTFOUND, name);
        return;
        }

    mergedict_in = myfopen(buffer, read_str);
    if(!mergedict_in)
        {
        reperr(ERR_CANNOTOPEN, name);
        return;
        }

    mergedict_in_buffer = alloc_ptr_using_cache(MERGEDICT_IN_BUFSIZ, &res);
    if(!mergedict_in_buffer)
        {
        reperr_null(res ? res : ERR_NOROOM);
        mergedict_close();
        return;
        }

    mysetvbuf(mergedict_in, mergedict_in_buffer, MERGEDICT_IN_BUFSIZ);

    /* open user dictionary */
    mdsp->dict = dict_number(d_user_merge[1].textfield, TRUE);

    if(mdsp->dict >= 0)
        {
        /* read words from file adding to user dictionary */

        #if RISCOS

        if(init_box(mdsp, "merging", Merging_STR, TRUE))
            {
            mergedict_start();
            return;
            }

        #else   /* RISCOS */

        init_box(mdsp, MAX_WORD, BROWSE_DEPTH, FALSE, SHIFT_STR);

        while(!ctrlflag  &&  get_word_from_file(mdsp->dict, mergedict_in, mergedict_array, NULL))
            {
            mdsp->res = spell_addword(mdsp->dict, mergedict_array);

            if(mdsp->res < 0)
                break;

            if(mdsp->res)
                {
                if(++(mdsp->sofar) >= BROWSE_DEPTH)
                    {
                    scroll_words_up(mdsp->words, BROWSE_DEPTH);
                    mdsp->sofar = BROWSE_DEPTH-1;

                    while(depressed_shift())
                        ;
                    }

                strcpy(mdsp->words[mdsp->sofar], mergedict_array);

                fill_box(mdsp->words, Merge_STR,
                         lastlen, MAX_WORD, BROWSE_DEPTH, NULLSTR);
                }
            }

        fill_box(mdsp->words, Merge_STR,
                 lastlen, MAX_WORD, BROWSE_DEPTH, (char *) PRESSANYKEY_STR);

        rdch(FALSE, TRUE);

        end_box(mdsp);

        #endif  /* RISCOS */
        }
    else
        mdsp->res = mdsp->dict;

    mergedict_end(mdsp);
}


/****************************************************************************
*                                                                           *
*                           Dump dictionary to file                         *
*                                                                           *
****************************************************************************/

static char *dumpdict_name       = NULL;
static FILE *dumpdict_out        = NULL;
static char *dumpdict_out_buffer = NULL;
static BOOL  dumpdict_file_error;
static char  dumpdict_template[MAX_WORD+1];
static char  dumpdict_wild_string[MAX_WORD+1];


static void
dumpdict_close(void)
{
    if(dumpdict_out)
        {
        myfclose(dumpdict_out);
        dumpdict_out = NULL;

        dispose((void **) &dumpdict_out_buffer);

        been_error = been_error || dumpdict_file_error;

        #if RISCOS
        if(!been_error)
            stampfile(dumpdict_name, TEXT_FILETYPE);

        str_clr(&dumpdict_name);
        #endif

        if(!been_error)
            /* suggest as the file to merge with */
            (void) str_set(&d_user_merge[0].textfield, d_user_dump[1].textfield);
        }
}


/* dumpdict complete - tidy up */

static void
dumpdict_end(merge_dump_strukt *mdsp)
{
    #if RISCOS
    dumpdict_wants_nulls = FALSE;
    #endif

    #if !RISCOS
    if(ctrlflag)
        {
        ack_esc();
        reperr_null(ERR_ESCAPE);
        been_error = FALSE;
        }
    else
    #endif

    if(dumpdict_file_error)
        reperr(ERR_CANNOTWRITE, d_user_dump[1].textfield);
    elif(mdsp->res < 0)
        reperr_module(ERR_SPELL, mdsp->res);

    dumpdict_close();

    end_box(mdsp);
}


#if RISCOS

#define dumpdict_Stop       0
#define dumpdict_Pause      3
#define dumpdict_Continue   4

/* dumpdict upcall processor */

static void
dumpdict_eventhandler(dbox d, void *handle)
{
    merge_dump_strukt *mdsp = (merge_dump_strukt *) handle;
    dbox_field f = dbox_get(d);

    tracef2("dumpdict_eventhandler got button %d: dumpdict_out = &%p\n", f, dumpdict_out);

    switch(f)
        {
        case dumpdict_Continue:
            /* only resume if paused with file still open */
            if(dumpdict_out)
                dumpdict_wants_nulls = TRUE;
            break;

        case dumpdict_Pause:
            /* stop null events to dumpdict null processor */
            dumpdict_wants_nulls = FALSE;
            break;

        default:
            dumpdict_end(mdsp);
            break;
        }
}


/* dumpdict null processor */

extern void
dumpdict_null(void)
{
    merge_dump_strukt *mdsp = &dumpdict_mds;
    BOOL was_ctrlflag;

    tracef0("dumpdict_null\n");

    escape_enable();

    mdsp->res = spell_nextword(mdsp->dict, dumpdict_template,
                               dumpdict_template, dumpdict_wild_string,
                               &ctrlflag);

    was_ctrlflag = escape_disable();

    if(!mdsp->res)
        {
        complete_box(&dumpdict_mds, Dump_STR);

        dumpdict_close();

        /* force punter to do explicit end */
        dumpdict_wants_nulls = FALSE;
        return;
        }

    if(was_ctrlflag  &&  (mdsp->res > 0))
        mdsp->res = ERR_ESCAPE;

    if(mdsp->res < 0)
        {
        dbox_sendclose(mdsp->d);
        return;
        }

    add_word_to_box(&dumpdict_mds, dumpdict_template);

    /* write to file */
    if( !away_string(dumpdict_template, dumpdict_out)  ||
        !away_byte(CR, dumpdict_out))
        {
        dumpdict_file_error = TRUE;
        dbox_sendclose(mdsp->d);
        }
}


static void
dumpdict_start(void)
{
    merge_dump_strukt *mdsp = &dumpdict_mds;

    dumpdict_wants_nulls = TRUE;

    dbox_eventhandler(mdsp->d, dumpdict_eventhandler, mdsp);

    /* all dumping done on null events, button processing on upcalls */
}

#endif  /* RISCOS */


/********************
*                   *
*  dump dictionary  *
*                   *
********************/

extern void
overlay_DumpDictionary_fn(void)
{
#if !defined(SAVE_OFF)
    char  buffer[MAX_FILENAME];
    char *name;
    merge_dump_strukt *mdsp = &dumpdict_mds;
    intl res;

    #if RISCOS
    if(mdsp->d)
        {
        reperr_null(ERR_ALREADYDUMPING);
        return;
        }
    #endif

    /* leave last word template alone */

    /* suggest a file to dump to */
    if( !insert_most_recent(&d_user_dump[2].textfield)  ||
        (str_isblank(d_user_dump[1].textfield)  &&  !str_setc(&d_user_dump[1].textfield, DUMPFILE_STR)))
            return;

    if(!dialog_box(D_USER_DUMP))
        return;

    dialog_box_end();

    dumpdict_file_error = FALSE;

    mdsp->dict = open_appropriate_dict(&d_user_dump[2]);

    mdsp->res = (mdsp->dict < 0)
                    ? mdsp->dict
                    : compile_wild_string(mdsp->dict, dumpdict_wild_string, d_user_dump[0].textfield);

    if(mdsp->res < 0)
        {
        reperr_module(ERR_SPELL, mdsp->res);
        return;
        }

    name = d_user_dump[1].textfield;

    if(str_isblank(name))
        {
        reperr_null(ERR_BAD_NAME);
        return;
        }

    (void) add_prefix_to_name(buffer, name, TRUE);

    if(!str_set(&dumpdict_name, buffer))
        return;

    dumpdict_out = myfopen(buffer, write_str);
    if(!dumpdict_out)
        {
        reperr(ERR_CANNOTOPEN, name);
        return;
        }

    dumpdict_out_buffer = alloc_ptr_using_cache(DUMPDICT_OUT_BUFSIZ, &res);
    if(!dumpdict_out_buffer)
        {
        reperr_null(res ? res : ERR_NOROOM);
        dumpdict_close();
        return;
        }

    mysetvbuf(dumpdict_out, dumpdict_out_buffer, DUMPDICT_OUT_BUFSIZ);

    if(mdsp->dict >= 0)
        {
        *dumpdict_template = '\0';

        #if RISCOS

        if(init_box(mdsp, "merging", Dumping_STR, TRUE))
            {
            dumpdict_start();
            return;
            }

        #else   /* RISCOS */

        init_box(mdsp, MAX_WORD, BROWSE_DEPTH, FALSE, SHIFT_STR);

        while(!ctrlflag)
            {
            mdsp->res = spell_nextword(mdsp->dict, dumpdict_template,
                                        dumpdict_template, dumpdict_wild_string,
                                        &ctrlflag);

            if(mdsp->res <= 0)
                break;

            if(++(mdsp->sofar) >= BROWSE_DEPTH)
                {
                scroll_words_up(mdsp->words, BROWSE_DEPTH);
                mdsp->sofar = BROWSE_DEPTH-1;

                while(depressed_shift())
                    ;
                }

            strcpy(mdsp->words[mdsp->sofar], dumpdict_template);

            fill_box(mdsp->words, Dump_STR,
                     lastlen, MAX_WORD, BROWSE_DEPTH, NULLSTR);

            /* write to file */
            if( !away_string(dumpdict_template, dumpdict_out)  ||
                !away_byte(CR, dumpdict_out))
                {
                dumpdict_file_error = TRUE;
                break;
                }
            }

        fill_box(mdsp->words, Dump_STR,
                 lastlen, MAX_WORD, BROWSE_DEPTH, (char *) PRESSANYKEY_STR);

        rdch(FALSE, TRUE);

        end_box(mdsp);

        #endif  /* RISCOS */
        }
    else
        mdsp->res = mdsp->dict;

    dumpdict_end(mdsp);

#endif  /* SAVE_OFF */
}


/********************
*                   *
*  lock dictionary  *
*                   *
********************/

extern void
overlay_LockDictionary_fn(void)
{
    intl dict, res;

    if(!insert_most_recent(&d_user_lock[0].textfield))
        return;

    while(dialog_box(D_USER_LOCK))
        {
        res = dict = open_appropriate_dict(&d_user_lock[0]);

        if(res >= 0)
            {
            res = spell_load(dict);
            if(res < 0)
                spell_unlock(dict);
            }

        if(res < 0)
            {
            reperr_module(ERR_SPELL, res);
            continue;
            }

        if(dialog_box_ended())
            break;
        }
}


/********************
*                   *
* unlock dictionary *
*                   *
********************/

extern void
overlay_UnlockDictionary_fn(void)
{
    intl res;

    if(!insert_most_recent(&d_user_unlock[0].textfield))
        return;

    while(dialog_box(D_USER_UNLOCK))
        {
        res = open_appropriate_dict(&d_user_unlock[0]);

        if(res >= 0)
            res = spell_unlock(res);

        if(res < 0)
            {
            reperr_module(ERR_SPELL, res);
            continue;
            }

        if(dialog_box_ended())
            break;
        }
}


/********************************************
*                                           *
* display list of opened user dictionaries  *
*                                           *
********************************************/

#if RISCOS
#define dispdict_Stop       0
#define dispdict_Pause      3
#define dispdict_Continue   4
#endif

extern void
overlay_DisplayOpenDicts_fn(void)
{
    merge_dump_strukt *mdsp = &browse_mds;
    #if RISCOS
    dbox_field f;
    #endif
    intl i;
    LIST *lptr;

    if(!check_spell_installed())
        return;

    #if RISCOS
    if(init_box(mdsp, "merging", Opened_user_dictionaries_STR, FALSE))
    #else
    if(init_box(mdsp, MAX_WORD, BROWSE_DEPTH, FALSE, PRESSANYKEY_STR))
    #endif
        {
        i = 1;  /* put one line of space at top */
        lptr = first_in_list(&first_user_dict);

        do  {
            if(lptr)
                {
                strcpy(mdsp->words[i], leafname((char *) (lptr->value)));
                tracef1("got user dict %s\n", mdsp->words[i]);
                lptr = next_in_list(&first_user_dict);
                }
            }
        while(++i < BROWSE_DEPTH);


        #if RISCOS

        fill_box(mdsp->words, NULL, mdsp->d);

        /* rather simple process: no nulls required! */

        while(((f = browse_fillin(mdsp->d)) != dbox_CLOSE)  &&
              (f != dbox_OK))
            ;

        #else   /* RISCOS */

        fill_box(mdsp->words, Opened_user_dictionaries_STR,
                 lastlen, MAX_WORD, BROWSE_DEPTH, NULLSTR);

        rdch(FALSE, TRUE);
        
        #endif  /* RISCOS */


        end_box(mdsp);
        }
}


/********************
*                   *
*  pack dictionary  *
*                   *
********************/

extern void
overlay_PackUserDict_fn(void)
{
    intl res, dict0 = 0, dict1 = 0; /*dataflower*/
    char array0[MAX_FILENAME];
    char array1[MAX_FILENAME];
    char *name0, *name1, *leaf;

    if(!insert_most_recent(&d_user_pack[0].textfield))
        return;

    while(dialog_box(D_USER_PACK))
        {
        name0 = d_user_pack[0].textfield;
        name1 = d_user_pack[1].textfield;

        if(str_isblank(name0)  ||  str_isblank(name1))
            {
            reperr_null(ERR_BAD_NAME);
            continue;
            }

        /* a better way would be to have a 'read pathname of dict' fn
         * for the case that dict0 is already open as we'd like to create
         * dict1 relative to it. But that's too much hard work so we
         * take the easy way out and always look the pathname up by hand.
        */
        close_user_dictionaries();

        if(!add_path(array0, name0, TRUE))
            {
            reperr(ERR_NOTFOUND, name0);
            continue;
            }

        name0 = array0;

        if(!isrooted(name1))
            {
            /* use pathname of name0 for name1 if not rooted */
            leaf = leafname(name0);
            memcpy(array1, name0, leaf - name0);
            strcpy(array1 + (leaf - name0), name1);
            name1 = array1;
            }

        /* open first dictionary */
        if((res = dict_number(name0, TRUE)) >= 0)
            dict0 = res;

        /* create second dictionary */
        if(res >= 0)
            res = spell_createdict(name1);

        if(res >= 0)
            if((dict1 = dict_number(name1, TRUE)) < 0)
                res = dict1;

        if(res < 0)
            {
            reperr_module(ERR_SPELL, res);
            continue;
            }

        if(res >= 0)
            res = spell_pack(dict0, dict1);

        if(res >= 0)
            {
            close_dict(dict0);
            close_dict(dict1);
            }
        else
            {
            reperr_module(ERR_SPELL, res);
            continue;
            }

        if(dialog_box_ended())
            break;
        }
}

#endif /* SPELL_OFF */

/* end of browse.c */
