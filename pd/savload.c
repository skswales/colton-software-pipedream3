/* savload.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       savload.c - module that saves and loads PipeDream files
 * Author:      RJM October 1987
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "wimpt.h"
#include "dbox.h"
#include "xferrecv.h"
#include "xfersend.h"

#ifndef Z88_OFF
#include "ext.z88com"
#endif

#ifndef VIEWSHEET_OFF
#include "ext.vsload"
#endif

#include "ext.pd123"

#define PD_MCTYPE PD_ARCH


#elif MS
#include <direct.h>

#ifndef Z88_OFF
#include "z88com.ext"
#endif

#ifndef VIEWSHEET_OFF
#include "vsload.ext"
#endif

#include "pd123.ext"

#define PD_MCTYPE PD_PC

#else
#   error Unknown target system
#endif


#include "datafmt.h"

#if RISCOS
#include "ext.riscos"
#include "ext.pd"
#include "riscdialog.h"
#endif


#define load_filetype       (d_load[3].option)

#define loading_pd          (load_filetype == PD_CHAR)

#if defined(VIEW_IO)
#include "viewio.h"

#define loading_view        (load_filetype == VIEW_CHAR)
#endif

#if defined(DTP_EXPORT)
#include "dtpsave.h"
#endif

#if defined(DTP_IMPORT)
#define loading_dtp         (load_filetype == DTP_CHAR)
#endif

#if !defined(VIEWSHEET_OFF)
#define loading_viewsheet   (load_filetype == SHEET_CHAR)
#endif

#if defined(NEW_CSV)
#define loading_comma       (load_filetype == CSV_CHAR)
#endif


/* exported functions */

extern void block_updref(colt startcol, colt coffset);
extern BOOL excloa(const char *fname);
extern BOOL excsav(const char *name, char filetype, BOOL initfile);
extern BOOL excsav_core(const char *name, char filetype, BOOL initfile);
extern intl find_file_type(const char *name);
extern BOOL loadfile(char *name, BOOL new_window, BOOL dont_load_as_list);
extern BOOL plain_slot(slotp tslot, colt tcol, rowt trow,
                       char filetype, uchar *buffer);

extern BOOL save_existing(void);
extern void save_opt_to_file(FILE *output, DIALOG *start, intl n);
extern void set_width_and_wrap(colt tcol, coord width, coord wrapwidth);


/* internal functions */

/*static void addcurrentdir(char *name *//*out*//*, const char *fname);*/
static BOOL checkfile(const char *nextname);
/*static BOOL find_data_format(const char *name, BOOL *plaintext, uchar *field_separator);*/


/*static BOOL loadfile_if_not_loaded(char *name);*/
static BOOL loadfile_recurse(char *name, BOOL new_window);
/*static BOOL lukcon(uchar *from, colt *tcol, rowt *trow, uchar *type,
                   uchar *justify, uchar *format, colt firstcol,
                   rowt firstrow, uchar *pageoffset, BOOL *outofmem,
                   rowt *rowinfile, BOOL build_cols, BOOL *first_column);*/

static BOOL save_current(void);
/*static BOOL save_options_to_file(FILE *output, BOOL initfile);*/
/*static BOOL stoslt(colt tcol, rowt trow, uchar type, uchar justify,
                   uchar format, uchar pageoffset);*/

#if !defined(MULTI_OFF)
extern BOOL iniglb(void);
extern intl nxfloa(void);
/*static BOOL process_list_file(const char *name);*/
/*static void resfpp(void);*/
#endif

#if !defined(LOTUS_OFF)
static BOOL do_lotus_save(uchar *name);
#endif

#if !defined(SAVE_OFF)
static BOOL collup(colt tcol, FILE *output);
static BOOL savslt(slotp tslot, FILE *output);
/*static BOOL sav_construct(const char *str, FILE *output);*/
#endif

#if RISCOS
extern int myfread(void *ptr, size_t size, size_t nmemb, FILE *stream);
#else
#define myfread fread
#endif


#if MS
#define same_name_warning(a, b) TRUE
#endif


/* ----------------------------------------------------------------------- */

#define LOAD_BUFSIZ 1024
#define SAVE_BUFSIZ 2048

typedef enum
{
    C_FREE      = 0,    /* must be same as J_FREE */
    C_LEFT      = 1,    /* must be same as J_LEFT */
    C_CENTRE    = 2,    /* must be same as J_CENTRE */
    C_RIGHT     = 3,    /* must be same as J_RIGHT */
    C_JLEFT     = 4,    /* must be same as J_JLEFT */
    C_JRIGHT    = 5,    /* must be same as J_JRIGHT */
    C_LCR       = 6,    /* must be same as J_LCR */
    C_PERCENT   = 7,
    C_COL       = 8,
    C_HIGHL     = 9,
    C_DEC       = 10,
    C_BRAC      = 11,
    C_LEADCH    = 12,
    C_TRAILCH   = 13,
    C_VALUE     = 14,
    C_OPT       = 15,
    C_PAGE      = 16
}
pd_construct_offsets;

static const char *contab[] =
    {
    "F%",   /* free align */
    "L%",   /* left align */
    "C%",   /* centre align */
    "R%",   /* right align */
    "JL%",  /* justify left */
    "JR%",  /* justify right */
    "LCR%", /* lcr align */
    "PC%",  /* '%' character */
    "CO:",  /* column construct */
    "H",    /* highlight n */
    "D",    /* decimal places n */
    "B%",   /* brackets */
    "LC%",  /* leading char */
    "TC%",  /* trailing char */
    "V%",   /* number */
    "OP%",  /* option */
    "P"     /* page eject n */
    };
#define C_ITEMS (sizeof(contab) / (sizeof(char *)))


static intl start_of_construct = 0;     /* used in load, lukcon */
static intl inoption = 0;               /* used in load, lukcon */

static char *loadname = NULL;


/***************************
*                          *
* save options to the file *
*                          *
***************************/

static BOOL
save_options_to_file(FILE *output, BOOL initfile)
{
#if !defined(SAVE_OFF)

    update_all_dialog_from_windvars();

    save_opt_to_file(output, d_options, dialog_head[D_OPTIONS].items);
    save_opt_to_file(output, d_poptions, dialog_head[D_POPTIONS].items);
    save_opt_to_file(output, d_recalc, dialog_head[D_RECALC].items);

    if(!initfile)
        {
        /* save any row & column fixes */
        save_opt_to_file(output, d_fixes, dialog_head[D_FIXES].items);

        save_protected_bits(output);
        }

    #if RISCOS
    save_opt_to_file(output, d_fonts, dialog_head[D_FONTS].items);
    #endif

    /* save print options and colours only to initialization files */
    if(initfile)
        {
        save_opt_to_file(output, d_driver, dialog_head[D_DRIVER].items);
        save_opt_to_file(output, d_colours, dialog_head[D_COLOURS].items);
        save_opt_to_file(output, d_menu, dialog_head[D_MENU].items);

        save_opt_to_file(output, d_save + SAV_LINESEP, 1);

        if(d_deleted[0].option != D_PASTE_SIZE)
            save_opt_to_file(output, d_deleted, 1);

        #if !defined(SPELL_OFF)
        save_opt_to_file(output, d_auto, dialog_head[D_AUTO].items);

        /* each user dictionary has to be saved to file */
        {
        LIST *lptr;

        for(lptr = first_in_list(&first_user_dict);
            lptr;
            lptr = next_in_list(&first_user_dict))
            {
            str_set(&d_user_open[0].textfield, lptr->value);
            save_opt_to_file(output, d_user_open,
                                dialog_head[D_USER_OPEN].items);
            }
        }
        #endif
        }

    update_all_windvars_from_dialog();

#endif

    return(TRUE);
}


#if !defined(SAVE_OFF)

/******************************************************
*                                                     *
* save one option to the file                         *
* does not save the option if it is the default       *
* however, if there is an initialization file and the *
* option is set in there, the option will be saved    *
*                                                     *
******************************************************/

extern void
save_opt_to_file(FILE *output, DIALOG *start, intl n)
{
    DIALOG *dptr;

    for(dptr = start; dptr < start + n; dptr++)
        {
        char *ptr;
        word32 key;
        BOOL not_on_list;

        ptr = linbuf + sprintf(linbuf, "%%OP%%%c%c", dptr->ch1, dptr->ch2);

        key = (word32) dptr->ch1;
        key <<= 8;
        key += (word32) dptr->ch2;
        not_on_list = !search_list(&def_first_option, key);

        switch(dptr->type)
            {
            case F_SPECIAL:
                if(not_on_list  &&  (dptr->option == *dptr->optionlist))
                    continue;

                ptr[0] = dptr->option;
                ptr[1] = '\0';
                break;

            case F_ARRAY:
                if(not_on_list  &&  (dptr->option == 0))
                    continue;

                sprintf(ptr, "%d", (int) dptr->option);
                break;

            case F_LIST:
            case F_NUMBER:
            case F_COLOUR:
                /* is it the default ? */

                if(not_on_list  &&  ((int) dptr->option == atoi(dptr->optionlist)))
                    continue;

                sprintf(ptr, "%d", (int) dptr->option);
                break;

            case F_TEXT:
                /* default options are blank, except leading,trailing characters */
                if( (dptr == d_options + O_LP)  ||
                    (dptr == d_options + O_TP)  )
                    {
                    if(!strcmp(dptr->textfield, dptr->optionlist))
                        continue;
                    }
                elif(   not_on_list  &&
                        (   str_isblank(dptr->textfield)    ||
                            !strcmp(dptr->textfield, dptr->optionlist)))
                    continue;

                if(dptr->textfield)
                    strcat(linbuf, dptr->textfield);
                break;

            default:
            break;
            }

        strcat(linbuf, CR_STR);

        for(ptr = linbuf; *ptr; ptr++)
            {
            uchar ch = *ptr;
            char array[LIN_BUFSIZ];

            if(ishighlight(ch))
                {
                sprintf(array, "%%H%c%%", ch - FIRST_HIGHLIGHT + FIRST_HIGHLIGHT_TEXT);
                away_string(array, output);
                }
            else
                away_byte(ch, output);
            }
    }
}

#endif  /* SAVE_OFF */


extern void
set_width_and_wrap(colt tcol, coord width, coord wrapwidth)
{
    intl *widp, *wwidp;

    readpcolvars(tcol, &widp, &wwidp);
    *widp = width;
    *wwidp = wrapwidth;
}



#if !defined(SAVE_OFF)

static BOOL
sav_construct(const char *str, FILE *output)
{
    char array[LIN_BUFSIZ];
    sprintf(array, "%%%s", str);
    return(away_string(array, output));
}

#endif  /* SAVE_OFF */


/************************************************************
*                                                           *
*  add the current directory to the start of the filename   *
*                                                           *
************************************************************/

static void
addcurrentdir(char *name /*out*/, const char *filename)
{
    /* add current directory prefix (only if global file) */

    if(str_isblank(currentdirectory)  ||  isrooted(filename))
        *name = '\0';
    else
        #if ARTHUR || RISCOS
        strcpy(name, currentdirectory);
        #elif MS
        {
        char *ptr;
        strcpy(name, currentdirectory);
        ptr = strchr(name, '*');
        if(ptr)
            *ptr = '\0';
        }
        #endif

    strcat(name, filename);
}


/************************************************
*                                               *
* ask whether modified file can be overwritten  *
*                                               *
************************************************/

extern BOOL
save_existing(void)
{
#if !defined(SAVE_OFF)
    BOOL first_time = TRUE;

    if(!xf_filealtered)
        return(TRUE);

    do  {
        if(!first_time)
            bleep();
        else
            first_time = FALSE;

        (void) init_dialog_box(D_SAVE_EXISTING);

        /* get current filename */
        if(!str_setc(&d_save_existing[1].textfield, fndfcl()))
            return(FALSE);

        if(!dialog_box(D_SAVE_EXISTING))
            return(FALSE);

        dialog_box_end();

        } while((d_save_existing[0].option == 'Y')  &&
                str_isblank(d_save_existing[1].textfield));

    if(d_save_existing[0].option == 'Y')
        {
        BOOL res = excsav_core(d_save_existing[1].textfield, d_save_FORMAT, FALSE);

        if(whole_file_saved_ok)
            filealtered(FALSE);

        return(res);
        }

    filealtered(FALSE);

#endif

    return(TRUE);
}


/************************************************
*                                               *
* name the current text, losing tag file status *
*                                               *
************************************************/

extern void
overlay_NameFile_fn(void)
{
    if(!str_setc(&d_name[0].textfield, fndfcl()))
        return;

    while(dialog_box(D_NAME))
        {
        if(same_name_warning(d_name[0].textfield, name_supporting_winge_STR))
            {
            if(d_name[0].textfield)
                {
                (void) str_setc(&currentfilename, d_name[0].textfield);
                #if RISCOS
                riscos_settitlebar(currentfilename);
                #endif
                }
            else
                (void) set_untitled_document();

            exp_rename_file();

            delfch();               /* delete file chain, clears glbbit */
            }

        if(dialog_box_ended())
            break;
        }
}


#if !defined(MULTI_OFF)

/****************************************************************************
*                                                                           *
*                           global file operations                          *
*                                                                           *
****************************************************************************/

struct multifilestr
{
    intl Sfilpof;
    intl Sfilpnm;
    unsigned char Sinfo_ok;
    char Sname[1];
}
multifilestr;


#if !RISCOS
#define xfersend_file_is_safe() TRUE
#endif

/****************************************************
*                                                   *
* read list file, building list of filenames        *
*                                                   *
* --out--                                           *
*   FALSE   list file processing not done           *
*           because ~Auto, inserting, Z88, ~Lsuffix *
*                                                   *
*   TRUE    list file read in, currentdirectory set *
*                                                   *
****************************************************/

static BOOL
process_list_file(const char *name)
{
    const char *cptr;
    char filebuffer[256];
    char array[MAX_FILENAME];
    char newdir[MAX_FILENAME];
    char *ptr;
    FILE *input;
    int ch;
    BOOL lsuffix;
    intl type, res;
    BOOL ok;
    dochandle doc = current_document_handle();
    intl filenum = 0;
    LIST *lptr;
    struct multifilestr *mfp;
    BOOL file_is_safe = in_execfile || xferrecv_file_is_safe();

    /* if inserting or not auto format or if scrap or if z88 return FALSE */
    if( inserting                       ||
        (load_filetype != AUTO_CHAR)    ||
        !file_is_safe                   ||
        looks_like_z88(name)            )
            return(FALSE);

    /* strip leading spaces */
    while(*name++ == SPACE)
        ;
    --name;


    #if ARTHUR || RISCOS
    cptr = leafname(name);

    lsuffix =  ((cptr > name + 3)           &&
                (toupper(cptr[-2]) == 'L')  &&
                ((cptr[-3] == '.')  ||  (cptr[-3] == COLON)));

    if(lsuffix)
        {
        /* copy over the new prefix */
        *newdir = '\0';
        strncat(newdir, name, cptr-name-2);
        tracef1("new prefix is \"%s\"\n", newdir);
        }

    #elif MS
    /* point to the char after the last good one */
    cptr = name;
    while(((ch = *cptr++) != SPACE)  ||  (ch != '\0'))
        ;
    cptr--;

    lsuffix = ((toupper(*(cptr-1)) == 'L')  &&  (*(cptr-2) == '.'));

    *newdir = '\0';
    #endif

    /* if no suffix, add one and see if it's there */
    if(!lsuffix)
        {
        /* return FALSE if not suffixed and file exists */
        if(filereadable(name))
            return(FALSE);

        #if ARTHUR || RISCOS
        *array = '\0';

        if(cptr != name)
            strncat(array, name, cptr-name);

        strcat(array, "L.");
        strcat(array, cptr);

        #elif MS
        strcat(array, ".L");
        #endif
        }
    else
        strcpy(array, name);

    type = find_file_type(array);

    if(type != TAB_CHAR)
        {
        if(type != '\0')
            reperr(ERR_NOTTABFILE, array);

        return(FALSE);
        }

    if(!create_new_document())
        return(FALSE);

    input = myfopen(array, read_str);

    ok = (input != NULL);

    if(ok)
        {
        mysetvbuf(input, filebuffer, sizeof(filebuffer));

        do  {
            ptr = array;

            do { ch = myfgetc(input); } while(ch == SPACE);

            while((ch != EOF)  &&  (ch != CR)  &&  (ch != LF))
                {
                *ptr++ = (uchar) ch;
                ch = myfgetc(input);
                }
            *ptr = '\0';

            if(strlen(array))
                {
                if(!isrooted(array))
                    {
                    /* add new dir on start of filename */
                    intl len = strlen(newdir);
                    memmove(array + len, array, strlen(array) + 1);
                    memcpy(array, newdir, len);
                    }

                tracef1("adding filename %s to global file list\n", array);
                trace_pause();

                lptr = add_list_entry(&first_file,
                                      sizeof(struct multifilestr) - 1 + strlen(array) + 1, &res);

                if(lptr)
                    {
                    lptr->key = (word32) filenum++;
                    mfp = (struct multifilestr *) lptr->value;
                    tracef1("mfp = &%p\n", mfp);
                    trace_pause();
                    mfp->Sinfo_ok = FALSE;
                    strcpy(mfp->Sname, array);
                    }
                else
                    {
                    reperr_null(res ? res : ERR_NOROOM);
                    ok = FALSE;
                    }
                }
            }
        while(ok  &&  (ch != EOF));
        }

    if(input)
        myfclose(input);

    if(ok)
        {
        #if ARTHUR || RISCOS
        if(lsuffix)
            str_setc(&currentdirectory, newdir);
        #endif

        curfil = -1;
        glbbit = TRUE;
        }
    else
        {
        destroy_current_document();

        select_document_using_handle(doc);
        }

    return(ok);
}


/***************************************
*                                      *
* restore filpof and filpnm from chain *
*                                      *
***************************************/

static void
resfpp(void)
{
    LIST *lptr = search_list(&first_file, curfil);

    if(lptr)
        {
        struct multifilestr *mfp = (struct multifilestr *) lptr->value;

        if(mfp->Sinfo_ok)
            {
            filpof = mfp->Sfilpof;
            filpnm = mfp->Sfilpnm;
            tracef2("[resfpp(): valid page number info: %d %d]\n", filpnm, filpof);
            }
        else
            tracef2("[resfpp(): no valid page number info: %d %d]\n", filpnm, filpof);
        }
    else
        tracef2("[resfpp(): not on list: %d %d]\n", filpnm, filpof);
}


/************************************************************
*                                                           *
* initialize for global file operations and load first file *
*                                                           *
************************************************************/

extern BOOL
iniglb(void)
{
    BOOL res;
    char *filename;
    intl old_curfil = curfil;

    if(!glbbit)
        return(FALSE);

    if(!curfil)
        return(TRUE);

    if((curfil > 0)  &&  !save_current())
        return(FALSE);

    curfil = 0;

    /* must make copy of filename cos destroy_current_doc in loadfile_recurse
     * moves the world around
    */
    filename = NULL;
    str_set(&filename, fndfcl());

    if(!checkfile(filename))
        {
        reperr(ERR_NOTFOUND, filename);
        res = FALSE;
        }
    else
        {
        #if RISCOS
        riscos_savecurrentwindowpos();
        #endif

        /* may fail, possibly destroying current document */
        res = loadfile_recurse(filename, EXISTING_WINDOW);

        if(res)
            {
            resfpp();
            #if RISCOS
            riscos_restorecurrentwindowpos();
            #endif
            }
        }

    if(!res  &&  is_current_document())
        curfil = old_curfil;

    str_clr(&filename);

    return(res);
}


/************************************
*                                   *
* next file in multi-file document  *
*                                   *
************************************/

extern intl
nxfloa(void)
{
    char *nextname;
    intl res;
    intl old_encpln;
    intl old_curfil = curfil;
    LIST *lptr = search_list(&first_file, curfil);

    /* save filpof, filpnm of current file on chain */
    if(d_poptions_PL == 0)
        filpof = filpnm = 1;

    if(lptr)
        {
        struct multifilestr *mfp = (struct multifilestr *) lptr->value;
        tracef3("[nxfloa(): saving filpnm %d filpof %d at mfp &%p]\n", filpnm, filpof, mfp);
        mfp->Sfilpof    = filpof;
        mfp->Sfilpnm    = filpnm;
        mfp->Sinfo_ok   = TRUE;
        }
    else
        tracef0("[nxfloa(): *** couldn't save positions as no entry found on chain!]\n");

    curfil = old_curfil + 1;

    nextname = NULL;
    str_set(&nextname, fndfcl());

    curfil = old_curfil;    /* for save_current() to work! */

    if(!nextname)
        res = ERR_ENDOFLIST;

    elif(!save_current())
        res = ERR_CANNOTWRITE;

    elif(!checkfile(nextname))
        {
        res = ERR_NOTFOUND;
        reperr(res, nextname);          /* more helpful here */
        }

    else
        {
        /* fiddle with pages */
        pagoff = filpof;                /* pagtfl() */
        pagnum = filpnm;

        fixpage((rowt) 0, numrow);      /* go from top of file to bottom */

        filpof = pagoff;
        filpnm = pagnum;

        tracef2("[nxfloa(): setting filpnm %d filpof %d]\n", filpnm, filpof);

        curfil = old_curfil + 1;

        /* strange situation where current file has page length 0 and
         * next one not. force next file to start at pagoff 0
        */
        old_encpln = encpln;

        /* may fail, possibly destroying current document */
        res = loadfile_recurse(nextname, EXISTING_WINDOW)
                            ? 0
                            : ERR_CANNOTREAD;

        if(!res)
            {
            if(!old_encpln  &&  encpln)
                {
                tracef0("[nxfloa(): old file had encpln 0 and new file has encpln ~0 - reset pag/filoff]\n");
                filpof = pagoff = 1;
                }
            }
        elif(is_current_document())
            curfil = old_curfil;
        }

    str_clr(&nextname);

    return(res);
}


/****************************************
*                                       *
* check the file exists and can be read *
*                                       *
****************************************/

static BOOL
checkfile(const char *nextname)
{
    char name[MAX_FILENAME];

    addcurrentdir(name, nextname);

    if(looks_like_z88(nextname))
        return(TRUE);

    return(filereadable(name));
}


static BOOL
mergebuf_n_glbbit(void)
{
    if(!mergebuf())
        return(FALSE);

    if(!glbbit)
        return(reperr_null(ERR_NOLISTFILE));

    return(TRUE);
}


/************************************
*                                   *
* first file in multi-file document *
*                                   *
************************************/

extern void
overlay_FirstFile_fn(void)
{
    if(!mergebuf_n_glbbit())
        return;

    /* may fail, possibly destroying current document */
    (void) iniglb();
}


/********************************************************
*                                                       *
* last file - go through every file acquiring page info *
*                                                       *
********************************************************/

extern void
overlay_LastFile_fn(void)
{
    intl res;

    if(!mergebuf_n_glbbit())
        return;

    #if RISCOS
    riscos_savecurrentwindowpos();
    #endif

    /* may fail, possibly destroying current document */
    do { res = nxfloa(); } while(!res);

    if(res != ERR_ENDOFLIST)
        reperr_null(res);

    #if RISCOS
    riscos_restorecurrentwindowpos();
    #endif
}


/************************************
*                                   *
* next file in multi-file document  *
*                                   *
************************************/

extern void
overlay_NextFile_fn(void)
{
    intl res;

    if(!mergebuf_n_glbbit())
        return;

    #if RISCOS
    riscos_savecurrentwindowpos();
    #endif

    res = nxfloa();     /* may fail, possibly destroying current document */

    if(res)
        reperr_null(res);
    #if RISCOS
    else
        riscos_restorecurrentwindowpos();
    #endif
}


/****************************************
*                                       *
* previous file in multi-file document  *
*                                       *
****************************************/

extern void
overlay_PrevFile_fn(void)
{
    BOOL res;
    char *prevname;
    intl old_curfil = curfil;

    if(!mergebuf_n_glbbit())
        return;

    if(old_curfil <= 0)
        {
        reperr_null(ERR_ENDOFLIST);
        return;
        }

    if(!save_current())
        return;

    curfil = old_curfil - 1;

    prevname = NULL;
    str_set(&prevname, fndfcl());

    res = checkfile(prevname);

    if(!res)
        reperr(ERR_NOTFOUND, prevname);
    else
        {
        #if RISCOS
        riscos_savecurrentwindowpos();
        #endif

        /* may fail, possibly destroying current document */
        res = loadfile_recurse(prevname, EXISTING_WINDOW);

        if(res)
            {
            resfpp();
            #if RISCOS
            riscos_restorecurrentwindowpos();
            #endif
            }
        }

    if(!res  &&  is_current_document())
        curfil = old_curfil;

    str_clr(&prevname);
}

#endif  /* MULTI_OFF */


/****************************
*                           *
* save initialisation file  *
*                           *
****************************/

extern void
overlay_SaveInitFile_fn(void)
{
    #if defined(SAVE_OFF)
    reperr_not_installed(ERR_GENFAIL);
    #else

    while(dialog_box(D_SAVEINIT))
        {
        if(str_isblank(d_saveinit[0].textfield))
            reperr_null(ERR_BAD_NAME);
        else
            (void) excsav_core(d_saveinit[0].textfield, PD_CHAR, TRUE);

        if(dialog_box_ended())
            break;
        }

    #endif  /* SAVE_OFF */
}


/************
*           *
* save file *
*           *
************/

extern void
overlay_SaveFile_fn(void)
{
#if defined(SAVE_OFF)
    reperr_not_installed(ERR_GENFAIL);
#else
    char name[MAX_FILENAME];
    optiontype tcrlf    = d_save[SAV_LINESEP].option;
    optiontype tformat  = d_save_FORMAT;

    if(xf_inexpression)
        {
        reperr_null(ERR_EDITINGEXP);
        return;
        }

    if(glbbit)
        {
        #if defined(MANY_DOCUMENTS)
        addcurrentdir(name, currentfilename);
        #else
        addcurrentdir(name, d_save[SAV_NAME].textfield);
        #endif

        if(!str_setc(&d_save[SAV_NAME].textfield, name))
            return;
        }
    #if defined(MANY_DOCUMENTS)
    else
        if(!str_setc(&d_save[SAV_NAME].textfield, currentfilename))
            return;
    #endif

    /* rest of options preserved from last time round */

    while(dialog_box(D_SAVE))
        {
        if(str_isblank(d_save[SAV_NAME].textfield))
            {
            reperr_null(ERR_BAD_NAME);
            continue;
            }

        (void) excsav(d_save[SAV_NAME].textfield, d_save_FORMAT, FALSE);

        if(!saving_part_file())
            {
            /* remember options iff whole file save */
            tcrlf   = d_save[SAV_LINESEP].option;
            tformat = d_save_FORMAT;
            }

        if(dialog_box_ended())
            break;
        }

    /* must reset marked block, column range, row selection flags so
     * that excsav calls from other routines eg. save_existing, next file
     * etc. always use default (full file) settings
    */
    d_save[SAV_COLRANGE].option = d_save[SAV_ROWCOND].option = d_save[SAV_BLOCK].option = 'N';

    d_save[SAV_LINESEP].option  = tcrlf;
    d_save_FORMAT               = tformat;

#endif  /* SAVE_OFF */
}


extern void
overlay_SaveFileAsIs_fn(void)
{
#if defined(SAVE_OFF)
    reperr_not_installed(ERR_GENFAIL);
#else
    char namebuf[MAX_FILENAME];
    char *name;

    if(glbbit)
        {
        addcurrentdir(namebuf, currentfilename);
        name = namebuf;
        }
    else
        name = currentfilename;

    /* should save process winge as filename not certain? */
    if(leafname(name) == name)
        {
        SaveFile_fn();
        return;
        }

    if(xf_inexpression)
        {
        reperr_null(ERR_EDITINGEXP);
        return;
        }

    (void) excsav_core(name, d_save_FORMAT, FALSE);

    if(whole_file_saved_ok)
        filealtered(FALSE);

#endif  /* SAVE_OFF */
}


/*************************************************
*                                                *
* output a slot in plain text mode, saving       *
* results of numbers rather than contents        *
* note that fonts are OFF so no juicy formatting *
*                                                *
*************************************************/

extern BOOL
plain_slot(slotp tslot, colt tcol, rowt trow, char filetype, uchar *buffer /*out*/)
{
    uchar oldformat, thousands;
    intl len;
    intl colwid;
    intl fwidth = LIN_BUFSIZ;

    IGNOREPARM(tcol);

    switch(tslot->type)
        {
        case SL_TEXT:
        case SL_PAGE:
            expand_slot(tslot, trow, buffer, fwidth, TRUE, FALSE, FALSE);
            return(FALSE);


        default:
            oldformat = tslot->content.number.format;
            thousands = d_options_TH;

            if(filetype == CSV_CHAR)
                {
                /* poke slot to minus format */
                tslot->content.number.format = F_DCPSID | F_DCP;
                d_options_TH = TH_BLANK;
                }
            elif(filetype == DTP_CHAR)
                {
                /* format slots on output to the same as we'd print them */
                colwid = colwidth(tcol);
                fwidth = chkolp(tslot, tcol, trow);
                fwidth = min(fwidth, colwid);
                fwidth = min(fwidth, LIN_BUFSIZ);
                }

            tracef6("[plain_slot(&%p, %d, %d, %c, &%p): fwidth %d]\n", tslot, tcol, trow, filetype, buffer, fwidth);

            expand_slot(tslot, trow, buffer, fwidth, TRUE, FALSE, FALSE);

            tslot->content.number.format = oldformat;
            d_options_TH = thousands;

            /* remove padding space */
            len = strlen(buffer);

            while(len--)
                if(buffer[len] == FUNNYSPACE)
                    {
                    buffer[len] = '\0';
                    break;
                    }

            tracef1("[plain_slot returns '%s']\n", buffer);
            return(TRUE);
        }
}


/********************************
*                               *
* save current file if altered  *
* return FALSE if problem       *
*                               *
********************************/

static BOOL
save_current(void)
{
#if defined(SAVE_OFF)
    return(TRUE);
#else
    docno doc, curdoc;
    intl index;
    char name[LIN_BUFSIZ];
    BOOL res;

    if(!xf_filealtered)
        return(TRUE);

    init_dependent_docs(&curdoc, &index);

    while((doc = next_dependent_doc(&curdoc, &index)) != 0)
        if(doc != curdoc)
            return(TRUE);

    addcurrentdir(name, fndfcl());

    res = excsav_core(name, d_save_FORMAT, FALSE);

    if(whole_file_saved_ok)
        filealtered(FALSE);

    return(res);
#endif
}


#if !defined(LOTUS_OFF)

/****************************************************
*                                                   *
* save file as a lotus file                         *
* first saves the file in PD format to temp.pd      *
* then calls writelotus to convert temp.pd to name  *
* then reloads temp.pd                              *
*                                                   *
****************************************************/

static BOOL
do_lotus_save(uchar *name)
{
    FILE *oldpipe;
    FILE *newlotus;
    intl res;
    list_block *multi_ptr;
    BOOL tglbbit = FALSE;
    intl tcurfil;
    char *tname = NULL;
    BOOL tfilealtered = xf_filealtered;

    if(!excsav_core(temp_file, PD_CHAR, FALSE))
        return(reperr(ERR_CANNOTWRITE, (uchar *) temp_file));


    /* dummy loop, so there is a single exit point for errors */
    for(;;)
        {
        /* save multi-file status */
        if((tglbbit = glbbit) != FALSE)
            {
            multi_ptr = first_file;
            first_file = NULL;
            tcurfil = curfil;
            }
        else
            if(!str_setc(&tname, currentfilename))
                return(FALSE);

        newfil();

        oldpipe = myfopen(temp_file, read_str);
        if(!oldpipe)
            {
            reperr(ERR_NOTFOUND, temp_file);
            break;
            }

        newlotus = myfopen(name, write_str);
        if(!newlotus)
            {
            myfclose(oldpipe);
            reperr(ERR_CANNOTOPEN, name);
            break;
            }

        res = writelotus(oldpipe, newlotus, PD_MCTYPE, NULL);

        myfclose(oldpipe);
        myfclose(newlotus);

        if(!excloa((uchar *) temp_file))
            break;

        if((glbbit = tglbbit) != FALSE)
            {
            first_file = multi_ptr;
            curfil = tcurfil;
            }
        else
            {
            str_setc(&currentfilename, tname);
            str_set(&d_save[SAV_NAME].textfield, fndfcl());
            str_clr(&tname);
            }

        xf_filealtered = tfilealtered;

        d_save_FORMAT = LOTUS_CHAR;

        if(res != 0)
            {
            reperr_module(ERR_LOTUS, res);
            been_error = FALSE;
            check_not_blank_sheet();
            return(FALSE);
            }

        return(TRUE);
        }

    /* something failed - clean up - delete file list */
    if(tglbbit)
        {
        first_file = multi_ptr;
        delfch();
        }
    else
        str_clr(&tname);

    return(FALSE);
}

#endif


/****************
*               *
* save the file *
*               *
****************/

#if !defined(SAVE_OFF)

extern BOOL
excsav(const char *name, char filetype, BOOL initfile)
{
    BOOL file_is_safe = in_execfile || xfersend_file_is_safe();
    BOOL res;

    res = excsav_core(name, filetype, initfile);

    /* rename file if whole file saved ok to safe place and not multi-file */
    if(whole_file_saved_ok  &&  file_is_safe)
        {
        filealtered(FALSE);

        if(!glbbit  &&  same_name_warning(name, name_supporting_winge_STR))
            {
            (void) str_setc(&currentfilename, name);
            #if RISCOS
            riscos_settitlebar(currentfilename);
            #endif
            exp_rename_file();
            }
        }

    return(res);
}


extern BOOL
excsav_core(const char *name, char filetype, BOOL initfile)
{
    BOOL goingdown = TRUE;
    FILE *output;
    BOOL all_ok = TRUE;
    rowt timer = 0;
    char save_buffer[SAVE_BUFSIZ];
    colt tcol, prevcol;
    rowt trow, prevrow;
    slotp tslot;
    SLR first, last;
    BOOL marked_block;
    uchar rowselection[LIN_BUFSIZ];
    char array[32];
    char field_separator = TAB;             /* unless CSV */
    BOOL res;
    #if defined(VIEW_IO) || defined(DTP_EXPORT)
    optiontype tcrlf = d_save[SAV_LINESEP].option;
    #endif
    #if RISCOS
    BOOL triscos_fonts;
    #endif
    #if defined(VIEW_IO)
    coord v_chars_sofar = 0;
    intl splitlines = 0;
    coord rightmargin = 0;
    #endif
    BOOL saving_Z88;
    BOOL file_is_safe = in_execfile || xfersend_file_is_safe();

    whole_file_saved_ok = FALSE;

    if(!mergebuf())
        return(FALSE);

    if(!name)
        return(reperr_null(ERR_BAD_NAME));

    #if MS || ARTHUR
    ack_esc();
    #endif

    saving_Z88 = looks_like_z88(name);

    /* if saving part file to a non-scrap file,
     * check that existing file shouldn't be overwritten
    */
    tracef3("saving_part_(): %s\nfile_is_safe:   %s\nfilereadable(): %s\n",
            trace_boolstring(saving_part_file()), trace_boolstring(file_is_safe), trace_boolstring(filereadable(name)));
    if(!saving_Z88  &&  saving_part_file()  &&  file_is_safe)
        if(!checkoverwrite(name))
            return(FALSE);


    /* sort out format to be saved */

    switch(filetype)
        {
        case LOTUS_CHAR:
            #if defined(LOTUS_OFF)
            return(reperr_not_installed(ERR_LOTUS));
            #else
            if(looks_like_z88(name))
                return(reperr_null(ERR_BAD_OPTION));

            res = (BOOL) do_lotus_save((char *) name);
            #if RISCOS
            if(res == FALSE)
                stampfile(name, LOTUS123_FILETYPE);
            #endif
            remove(temp_file);
            return(res);
            #endif

        case PD_CHAR:
            goingdown = TRUE;
            break;

        case CSV_CHAR:
            field_separator = COMMA;

            /* deliberate drop thru */

        case TAB_CHAR:
        #if defined(VIEW_IO)
        case VIEW_CHAR:
        #endif
        #if defined(DTP_EXPORT)
        case DTP_CHAR:
        #endif
        default:
            goingdown = FALSE;
            break;
        }


    /* save marked block? */

    marked_block = (d_save[SAV_BLOCK].option == 'Y');

    if(marked_block)
        {
        if(!MARKER_DEFINED())
            return(reperr_null(MARKER_SOMEWHERE() ? ERR_NOBLOCKINDOC : ERR_NOBLOCK));

        first = blkstart;
        last  = (blkend.col == NO_COL) ? blkstart : blkend;
        }
    else
        {
        first.col = (colt) 0;
        first.row = (rowt) 0;
        last.col  = numcol - 1;
        last.row  = numrow - 1;
        }


    /* save range of columns? */

    if(d_save[SAV_COLRANGE].option == 'Y')
        {
        /* disallow marked block and column range */
        if(marked_block)
            return(reperr_null(ERR_BAD_OPTION));

        buff_sofar = (uchar *) d_save[SAV_COLRANGE].textfield;

        first.col = getcol();
        last.col  = getcol();

        if( last.col == NO_COL)
            last.col = first.col;

        if((first.col == NO_COL)  ||  (last.col < first.col))
            return(reperr_null(ERR_BAD_RANGE));
        }


    prevcol = goingdown ? NO_COL : first.col;
    prevrow = first.row;


    /* open file and buffer it */

    escape_enable();

    actind(ACT_SAV, 0);

    output = myfopen(name, write_str);
    if(!output)
        {
        actind_end();
        escape_disable();
        return(reperr_null(ERR_CANNOTOPEN));
        }

    if(!using_Z88)
        setvbuf(output, save_buffer, saving_Z88 ? _IONBF : _IOFBF, sizeof(save_buffer));


    #if RISCOS
    /* set up font dialog box for saving */

    update_dialog_from_fontinfo();

    triscos_fonts = font_stack(riscos_fonts);
    riscos_fonts = FALSE;
    #endif


    /* row and column fixes have dummy dialog box - set it up */

    (void) init_dialog_box(D_FIXES);

    if(n_rowfixes)
        {
        sprintf(array, "%ld,%d", row_number(0), n_rowfixes);
        (void) str_set(&d_fixes[0].textfield, array);
        }

    if(n_colfixes)
        {
        sprintf(array, "%d,%d", col_number(0), n_colfixes);
        (void) str_set(&d_fixes[1].textfield, array);
        }



    /* file successfully opened at this point - start outputting preamble */

    if(goingdown  &&  !saving_part_file()  &&  !save_options_to_file(output, initfile))
        all_ok = FALSE;


    #if defined(VIEW_IO)
    /* save VIEW preamble here */

    if(filetype == VIEW_CHAR)
        {
        assert(VIEW_LINESEP == CR);
        d_save[SAV_LINESEP].option = SAV_LSEP_CR;

        if(!view_save_ruler_options(&rightmargin, output))
            all_ok = FALSE;
        }
    #endif

    #if defined(DTP_EXPORT)
    /* save DTP preamble here */

    if(filetype == DTP_CHAR)
        {
        assert(DTP_LINESEP == LF);
        d_save[SAV_LINESEP].option = SAV_LSEP_LF;

        if(!dtp_save_preamble(output))
            all_ok = FALSE;
        }
    #endif


    /* print all slots in block, for plain text going
     * row by row, otherwise column by column.
    */
    init_block(&first, &last);

    /* if just an initialization file, only print column constructs */

    if(initfile)
        {
        tcol = 0;

        while(!ctrlflag  &&  all_ok  &&  (tcol < numcol))
            if(!collup(tcol++, output))
                all_ok = FALSE;
        }
    else
        {
        /* ensure consistently bad page numbers are saved */
        curpnm = 0;

        /* save all slots */
        while(!ctrlflag  &&  all_ok  &&  next_in_block(goingdown))
            {
            tcol = in_block.col;
            trow = in_block.row;

            /* row selection - if it's a new column re-initialize row selection */

            if((trow == first.row)  &&  (goingdown  ||  (tcol == first.col)))
                {
                if((d_save[SAV_ROWCOND].option == 'Y')  &&  !str_isblank(d_save[SAV_ROWCOND].textfield))
                    {
                    rowt trow;
                    intl len;
                    uchar oprstb[COMPILE_BUFSIZ];

                    /* set up row selection */
                    strcpy((char *) linbuf, d_save[SAV_ROWCOND].textfield);

                    /* compile expression, and copy result to rowselection */
                    xf_inexpression = TRUE;
                    len = cplent(oprstb, TRUE);
                    xf_inexpression = FALSE;

                    if(len < 0)
                        {
                        escape_disable();
                        myfclose(output);
                        return(reperr_null(ERR_BAD_SELECTION));
                        }

                    memcpy(rowselection, oprstb, len);

                    /* make sure slot references start at beginning of marked block */
                    for(trow = 0; trow < first.row; trow++)
                        inc_rows(rowselection);
                    }
                else
                    *rowselection = '\0';
                }


            /* now check row selection for this row */

            if(!str_isblank(rowselection))
                {
                BOOL increment_row = TRUE;

                /* when going across only increment the row on the last column */
                if(!goingdown  &&  (tcol != last.col))
                    increment_row = FALSE;

                if(!selrow(rowselection, trow, increment_row))
                    continue;
                }

            if(!goingdown  &&  (prevrow != trow))
                {
                prevrow = trow;

                if(!away_eol(output))
                    {
                    all_ok = FALSE;
                    break;
                    }

                #if defined(VIEW_IO)
                /* reset VIEW line count */
                v_chars_sofar = 0;
                #endif
                }


            tslot = travel(tcol, trow);

            /* for a new column, either a column heading construct needs to be
             * be written or (when in plain text mode) tabs up to
             * the current column
            */
            if(prevcol != tcol)
                {
                if(!goingdown)
                    {
                    /* in plain text (or commarated) mode don't bother writing
                     * TABs if there are no more slots in this row
                    */
                    if( prevcol > tcol)
                        prevcol = tcol;  /* might be new row */

                    /* if it's a blank slot don't bother outputting field separators
                     * watch out for numeric slots masquerading as blanks
                    */
                    if( !isslotblank(tslot)  ||
                        (tslot  &&  (tslot->type != SL_TEXT)))
                        {
                        while(prevcol < tcol)
                            {
                            if(!away_byte(field_separator, output))
                                {
                                all_ok = FALSE;
                                break;
                                }

                            prevcol++;
                            }
                        }
                    }
                else
                    {
                    /* write new column construct info */
                    prevcol = tcol;

                    if(!collup(tcol, output))
                        all_ok = FALSE;
                    }
                }

            if(!tslot)
                {
                if(atend(tcol, trow))
                    {
                    if(goingdown)
                        force_next_col();
                    }
                elif(goingdown  &&  !away_eol(output))
                    {
                    all_ok = FALSE;
                    break;
                    }
                }
            else
                {
                /* slot needs writing out */
                if(timer++ > TIMER_DELAY)
                    {
                    timer = 0;
                    actind(ACT_SAV, percent_in_block(goingdown));
                    #if MS || ARTHUR
                    ack_esc();
                    #endif
                    }
                elif(saving_Z88)
                    timer += TIMER_DELAY >> 4;

                if(goingdown)
                    {
                    if(!savslt(tslot, output)  ||  !away_eol(output))
                        all_ok = FALSE;
                    }
                else
                    {
                    switch(filetype)
                        {
                        #if defined(VIEW_IO)
                        case VIEW_CHAR:
                            /* check for page-break */
                            if(chkrpb(trow))
                                {
                                if(tcol == 0)
                                    {
                                    uchar condval = travel(0, trow) ->content.page.condval;
                                    char array[10];

                                    strcpy(array, "PE");
                                    if(condval > 0)
                                        sprintf(array+2, "%d", (int) condval);

                                    if(!view_save_stored_command(array, output))
                                        all_ok = FALSE;
                                    }

                                /* don't output anything on page-break line */
                                continue;
                                }

                            if(tcol == 0)
                                {
                                switch((tslot->justify) & J_BITS)
                                    {
                                    case J_LEFT:    
                                        if(!view_save_stored_command("LJ", output))
                                            all_ok = FALSE;
                                        break;

                                    case J_CENTRE:  
                                        if(!view_save_stored_command("CE", output))
                                            all_ok = FALSE;
                                        break;

                                    case J_RIGHT:   
                                        if(!view_save_stored_command("RJ", output))
                                            all_ok = FALSE;
                                        break;

                                    default:
                                        break;
                                    }
                                }

                            if( all_ok  &&
                                tslot   &&  !view_save_slot(tslot, tcol, trow, output,
                                                            &v_chars_sofar,
                                                            &splitlines, rightmargin))
                                all_ok = FALSE;

                            break;
                        #endif  /* VIEW_IO */


                        #if defined(DTP_EXPORT)
                        case DTP_CHAR:
                            /* check for page-break */
                            if(chkrpb(trow))
                                {
                                if(!tcol)
                                    {
                                    uchar condval = travel(0, trow) ->content.page.condval;

                                    /* don't bother with conditional breaks - they're too stupid */

                                    if(!condval  &&  !away_byte(DTP_FORMFEED, output))
                                        all_ok = FALSE;
                                    }

                                /* don't output anything on page break line */
                                continue;
                                }

                            if(tslot  &&  !dtp_save_slot(tslot, tcol, trow, output))
                                all_ok = FALSE;

                            break;
                        #endif  /* DTP_EXPORT */


                        default:
                            {
                            /* write just text or formula part of slot out */
                            BOOL csv_quotes = !plain_slot(tslot, tcol, trow, filetype, linbuf)
                                              &&  (filetype == CSV_CHAR);
                            uchar *lptr = linbuf;
                            uchar ch;

                            /* output contents, not outputting highlight chars */
                            if(csv_quotes  &&  !away_byte(QUOTES, output))
                                all_ok = FALSE;                     

                            while(all_ok  &&  ((ch = *lptr++) != '\0'))
                                {
                                if( all_ok  &&  (filetype == CSV_CHAR)  &&  (ch == QUOTES)  &&
                                    !away_byte(QUOTES, output))
                                        all_ok = FALSE;                     

                                if( all_ok  &&  !ishighlight(ch)  &&
                                    !away_byte(ch, output))
                                        all_ok = FALSE;
                                }

                            if( all_ok  &&  csv_quotes  &&  !away_byte(QUOTES, output))
                                all_ok = FALSE;                     
                            }

                            break;
                        }
                    }
                }
            }
        }

    /* terminate with a carriage return if not PD file */
    if(!goingdown  &&  !away_eol(output))
        all_ok = FALSE;

    if((!using_Z88  &&  fflush(output))  ||  !all_ok)
        {
        reperr_null(ERR_CANNOTWRITE);
        all_ok = FALSE;
        }

    if(myfclose(output))
        {
        reperr_null(ERR_CANNOTCLOSE);
        all_ok = FALSE;
        }
    #if ARTHUR || RISCOS
    elif(!saving_Z88)
        {
        #if ARTHUR
        stampfile(name, rft_from_option(filetype));
        #elif RISCOS
        /* set correct file type */
        riscos_settype(&currentfileinfo, currentfiletype(filetype));
        /* use new time in appropriate circumstances */
        if(initfile  ||  xf_filealtered  ||  saving_part_file())
            riscos_readtime(&currentfileinfo);
        riscos_writefileinfo(&currentfileinfo, name);
        #endif
        }
    #endif

    #if defined(VIEW_IO)
    if(all_ok  &&  (splitlines > 0))
        {
        char array[20];
        sprintf(array, "%d", splitlines);
        reperr(ERR_LINES_SPLIT, array);
        }
    #endif


    #if defined(VIEW_IO) && defined(DTP_EXPORT)
    /* reset old carriage return status */
    d_save[SAV_LINESEP].option = tcrlf;
    #endif

    #if RISCOS
    riscos_fonts = font_unstack(triscos_fonts);
    #endif

    escape_disable();

    actind_end();

    slot_in_buffer = FALSE;

    whole_file_saved_ok = all_ok  &&  !saving_part_file()  &&  !initfile;

    return(all_ok);
}


/*****************************
*                            *
* output column heading info *
*                            *
*****************************/


static BOOL
collup(colt tcol, FILE *output)
{
    uchar colnoarray[20];
    char array[LIN_BUFSIZ];
    intl *widp, *wwidp;

    writecol(colnoarray, tcol);

    /* note that to get the wrapwidth, we cannot call colwrapwidth
    here because it would not return a zero value */

    readpcolvars(tcol, &widp, &wwidp);

    sprintf(array,
            "%%CO:%s,%d,%d%%",
            colnoarray,
            colwidth(tcol),
            *wwidp);

    return(away_string(array, output));
}



/************
*           *
* save slot *
*           *
************/

static BOOL
savslt(slotp tslot, FILE *output)
{
    uchar *lptr, ch;
    BOOL numerictype = FALSE;
    uchar justify = tslot->justify & J_BITS;
    char array[20];

    /* save slot type, followed by justification, followed by formats */

    switch((int) tslot->type)
        {
        case SL_PAGE:
            sprintf(array, "P%d%%", tslot->content.page.condval);
            if(!sav_construct(array, output))
                return(FALSE);
            break;

        case SL_ERROR:
        case SL_NUMBER:
        case SL_STRVAL:
        case SL_INTSTR:
        case SL_DATE:
        case SL_BAD_FORMULA:
            numerictype = TRUE;
            if(!sav_construct(contab[C_VALUE], output))
                return(FALSE);
            break;

        default:
            break;
        }


    if((justify > 0)  &&  (justify <= 6))
        if(!sav_construct(contab[justify], output))
            return(FALSE);

    if(numerictype)
        {
        uchar format;

        format = tslot->content.number.format;
        if(format & F_LDS)
            if(!sav_construct(contab[C_LEADCH], output))
                return(FALSE);

        if(format & F_TRS)
            if(!sav_construct(contab[C_TRAILCH], output))
                return(FALSE);

        /* if DCP set, save minus and decimal places */

        if(format & F_DCP)
            {
            uchar decimals = 0;

            /* sign minus, don't bother saving it. load() will set the DCP
                bit by implication as the decimal places field is saved
            */

            if(format & F_BRAC)
                if(!sav_construct(contab[C_BRAC], output))
                    return(FALSE);

            decimals = format & F_DCPSID;       /* mask out non-decimal bits */
            decimals = (uchar) ((decimals == 0xF) ? 'F' : (decimals + '0'));
            sprintf(array, "D%c%%", decimals);
            if(!sav_construct(array, output))
                return(FALSE);
            }
        }

    prccon(linbuf, tslot);

    /* output contents, dealing with highlight chars and % */

    lptr = linbuf;

    while((ch = *lptr++) != '\0')
        {
        if(ch == '%')
            {
            if(!sav_construct(contab[C_PERCENT], output))
                return(FALSE);
            }
        elif(ishighlight(ch))
            {
            char array[32];
            sprintf(array, "%%H%c%%", (int) (ch - FIRST_HIGHLIGHT + FIRST_HIGHLIGHT_TEXT));
            if(!away_string(array, output))
                return(FALSE);
            }
        elif(!away_byte(ch, output))
            return(FALSE);
        }

    return(TRUE);
}

#endif


/****************************************************
*                                                   *
* look up construct in table.                       *
* If valid, do whatever is needed                   *
* It is passed the addresses of type,justify,format *
* so that they can be updated                       *
*                                                   *
****************************************************/

static BOOL
lukcon(uchar *from, colt *tcol, rowt *trow, uchar *type, uchar *justify,
        uchar *format, colt firstcol, rowt firstrow, uchar *pageoffset,
        BOOL *outofmem, rowt *rowinfile, BOOL build_cols, BOOL *first_column)
{
    uchar *tptr;
    intl i;
    intl len;

    static colt column_offset;


    /* check another % in range */

    for(tptr = from+1; ; tptr++)
        if(*tptr == '%')
            break;
        elif(tptr >= from+15  ||  !*tptr)
            return(FALSE);

    /* check starts with alpha */

    if(!isalpha(from[1]))
        return(FALSE);

    /* itsa construct.  if can't find construct in table ignore it */

    for(i = 0; ; i++)
        {
        if(i >= C_ITEMS)
            return(TRUE);

        if(!memcmp(contab[i], from + 1, (unsigned) (len = strlen(contab[i]))))
            break;
        }

    switch((int) i)
        {
        coord wid, wrapwid;
        colt luk_newcol;
        uchar or_in;
        uchar from2;
        intl poffset;
        uchar *coloff;

        case C_VALUE:
            *type = SL_NUMBER;
            break;

        case C_FREE:
        case C_LEFT:
        case C_CENTRE:
        case C_RIGHT:
        case C_JLEFT:
        case C_JRIGHT:
        case C_LCR:
            *justify = (uchar) i;
            break;

        case C_PERCENT:
            /* just adjust lecpos to point to the first % */
            start_of_construct++;
            break;

        case C_COL:
            coloff = buff_sofar = from+len+1;
            /* luk_newcol is number of new column */
            luk_newcol = getcol() + firstcol;

            if(build_cols && !createcol(luk_newcol))
                {
                *outofmem = TRUE;
                return(reperr_null(ERR_NOROOM));
                }

            if(*buff_sofar++ != ',')
                return(FALSE);

            /* get the width - a valid response is a +number
             * or a -ve ',' meaning none specified
            */
            wid = (coord) getsbd();
            if(wid >= 0)
                {
                if(*buff_sofar++ != ',')
                    return(FALSE);
                }
            elif(0-wid != ',')
                return(FALSE);

            /* get the wrapwidth - a valid response is a +ve number
             * or a -ve ',' meaning none specified
            */
            wrapwid = (coord) getsbd();
            if(wrapwid >= 0)
                {
                if(*buff_sofar != '%')
                    return(FALSE);
                }
            elif(0-wrapwid != '%')
                return(FALSE);

            if(*first_column)
                {
                column_offset = inserting ? luk_newcol : 0;
                *first_column = FALSE;
                }

            *tcol = firstcol + luk_newcol - column_offset;
            *trow = firstrow;
            *rowinfile = 0;

            if(build_cols)
                set_width_and_wrap(*tcol, wid, wrapwid);

            break;


        /* for option, just remember position: gets dealt with at end of slot */

        case C_OPT:
            inoption = start_of_construct;
            return(FALSE);

        case C_HIGHL:
            from2 = from[2];
            if((from2 < FIRST_HIGHLIGHT_TEXT)  ||  (from2 > LAST_HIGHLIGHT_TEXT)  ||  (from[3] != '%'))
                return(FALSE);
            *from = FIRST_HIGHLIGHT + from2 - FIRST_HIGHLIGHT_TEXT;
            start_of_construct++;
            break;


        case C_DEC:
            /* decimal places - set F_DCP bit
             * if %DF%, float format else
             * or in amount in last four bits
            */
            from2 = from[2];
            if(((from2 < '0'  ||  from2 > '9') && from2 != 'F')  ||  from[3] != '%')
                return(FALSE);
            or_in = (uchar) ((from2 == 'F')     ? (F_DCP | 0xF)
                                                : (F_DCP | (from2-'0')));
            *format |= or_in;
            break;

        case C_LEADCH:
            *format |= F_LDS;
            break;

        case C_TRAILCH:
            *format |= F_TRS;
            break;

        case C_BRAC:
            *format |= (F_BRAC | F_DCP);
            break;

        case C_PAGE:
            *type = SL_PAGE;
            buff_sofar = from+2;
            poffset = (intl) getsbd();
            if(poffset < 0  ||  *buff_sofar++ != '%')
                return(FALSE);
            *pageoffset = (uchar) poffset;
            break;

        default:
            return(FALSE);
        }

    return(TRUE);
}


/****************************
*                           *
* store from linbuf in slot *
*                           *
****************************/

static BOOL
stoslt(colt tcol, rowt trow, uchar type, uchar justify, uchar format, uchar pageoffset)
{
    intl len;
    slotp tslot;
    uchar flags = 0;
    uchar oprstb[COMPILE_BUFSIZ];

    switch(type)
        {
        case SL_NUMBER:
            xf_inexpression = TRUE;
            len = cplent(oprstb, FALSE);
            xf_inexpression = FALSE;

#if 1
            if(len > 0)
                {
                flags |= refs_in_this_slot;
                break;
                }

            justify = J_FREE;

            /* deliberate drop thru */
#else
            if(len < 0)
                return(store_bad_formula(linbuf, tcol, trow, len));

            flags |= refs_in_this_slot;
            break;
#endif


        case SL_TEXT:
            /* compile directly into slot */
            /* merst1 does a tree_str_insertslot */
            buffer_altered = TRUE;
            if(!merst1(tcol, trow))
                {
                buffer_altered = FALSE; /* so filbuf succeeds */
                tracef0("[stoslt returning FALSE]\n");
                return(FALSE);
                }

            tslot = travel(tcol, trow);
            if(tslot)
                tslot->justify = justify;
            return(TRUE);


        case SL_PAGE:
            len = 1;
            break;


        default:
            len = 0;
            break;
        }

    /* type is not text */
    tslot = createslot(tcol, trow, len, type);
    if(!tslot)
        {
        slot_in_buffer = buffer_altered = FALSE;
        tracef0("[stoslt * returning FALSE]\n");
        return(reperr_null(ERR_NOROOM));
        }

    tslot->type = type;
    tslot->flags = flags;
    tslot->justify = justify;

    switch(type)
        {
        case SL_NUMBER:
            memcpy((char *) tslot->content.number.text, (char *) oprstb,
                   (unsigned) len);
            tslot->content.number.format = format;

            /* insert slot into tree */
            if(tree_exp_insertslot(tcol, trow, FALSE) < 0)
                {
                tracef0("[stoslt ** returning FALSE]\n");
                return(reperr_null(ERR_NOROOM));
                }
            break;


        case SL_PAGE:
            tslot->content.page.condval = pageoffset;
            break;


        default:
            break;
        }

    return(TRUE);
}


/************
*           *
* load file *
*           *
************/

extern void
overlay_LoadFile_fn(void)
{
    #if MS || ARTHUR
    char array[LIN_BUFSIZ];
    #endif

    if(xf_inexpression)
        {
        reperr_null(ERR_EDITINGEXP);
        return;
        }

    d_load[3].option = *(d_load[3].optionlist);     /* Init to Auto */

    while(dialog_box(D_LOAD))
        {
        #if RISCOS
        /* Don't do these checks */
        #elif MS || ARTHUR
        if(str_isblank(d_load[0].textfield))
            {
            if(!str_setc(&d_load[0].textfield, WILD_STR))
                return;
            }
        elif(strlen(d_load[0].textfield) > MAX_FILENAME)
            {
            reperr_null(ERR_BAD_NAME);
            continue;
            }

        if( isdirectory(UNULLSTR, (uchar *) d_load[0].textfield)
            ||  strchr(d_load[0].textfield, COLON)
            ||  strchr(d_load[0].textfield, '*')    )
            {
            strcpy(array, d_load[0].textfield);

            if(been_error  ||  !getfilename(array))     /* from slector */
                {
                #if !defined(Z88_OFF)
                using_Z88 = FALSE;
                #endif
                return;
                }

            str_setc(&d_load[0].textfield, array);
            }
        #endif  /* RISCOS */


        #if !defined(SPELL_OFF) && !defined(MANY_DOCUMENTS)
            del_spellings();
        #endif

        if(!been_error)
            #if RISCOS
            (void) loadfile(d_load[0].textfield, NEW_WINDOW, FALSE);
            #else
            if( loadfile(d_load[0].textfield, NEW_WINDOW, FALSE)  &&
                !str_isblank(currentdirectory))
                str_setc(&d_load[0].textfield, currentdirectory);
            #endif

        if(dialog_box_ended())
            break;
        }
}


/************************************************************
*                                                           *
* loadfile receives a name and loads a file                 *
* gets its options from d_load                              *
* it is called by FLfunc and on startup (if *PD filename)   *
*                                                           *
************************************************************/

extern BOOL
loadfile(char *name, BOOL new_window, BOOL dont_load_as_list)
{
    BOOL res;

    #if !defined(MULTI_OFF)
    /* if loading list file get the first one */
    if(!dont_load_as_list  &&  process_list_file(name))
        {
        FirstFile_fn();

        res = is_current_document()
                    ? glbbit
                    : FALSE;
        }
    elif(been_error)
        res = FALSE;
    else
    #endif
        res = loadfile_recurse(name, new_window);

    #if RISCOS
    if(res)
        xf_acquirecaret = TRUE;
    #endif

    return(res);
}



/*
load the file if it isn't loaded
*/
/*
static BOOL
loadfile_if_not_loaded(char *name)
{
    dochandle han;

    if((han = find_document_using_leafname(name)) != DOCHANDLE_NONE)
        {
        select_document_using_handle(han);
        return(TRUE);
        }
    else
        return(loadfile_recurse(name, NEW_WINDOW));
}
*/


/************************************************************************
*                                                                       *
* open a window, maybe, and load the file from disc                     *
* if there are any unresolved external references, get those files too  *
*                                                                       *
************************************************************************/

static BOOL
loadfile_recurse(char *name, BOOL new_window)
{
    BOOL old_was_searching;
    list_block *old_first_file;
    intl old_curfil, old_pagoff, old_pagnum, old_filpof, old_filpnm, old_real_pagcnt;
    char *old_currentdirectory;
    char name_array[MAX_FILENAME];
    BOOL res;
    dochandle doc;

    tracef1("[loadfile_recurse %s]\n", name);

    /* insertion is harmless wrt. load recursion etc. */
    if(inserting)
        return(excloa(name));

    if(!same_name_warning(name, load_supporting_winge_STR))
        return(FALSE);

    res = TRUE;
    doc = current_document_handle();

    if(!new_window) 
        {           
        /* in multi-file document so blow away
         * current window keeping salient features for new.
         * Add prefix here too before it gets destroyed
        */
        addcurrentdir(name_array, name);
        name = name_array;

        old_was_searching = (schdochandle == doc);
        old_pagoff = pagoff;
        old_pagnum = pagnum;
        old_filpof = filpof;
        old_filpnm = filpnm;
        old_real_pagcnt = real_pagcnt;
        old_curfil = curfil;

        old_currentdirectory = currentdirectory;
        currentdirectory = NULL;

        old_first_file = first_file;
        first_file = NULL;

        destroy_current_document();
        }

    res = create_new_document();

    if(res)
        {
        #if RISCOS
        if(!new_window)
            riscos_resetwindowpos();
        #endif

        res = excloa(name);

        if(res)
            {
            if(!new_window)
                {
                /* set glbbit, page stuff for new window */
                tracef2("[loadfile_recurse(): restoring filpnm %d filpof %d]\n", old_filpnm, old_filpof);

                if(old_was_searching)
                    schdochandle = current_document_handle();

                pagoff = old_pagoff;
                pagnum = old_pagnum;
                filpof = old_filpof;
                filpnm = old_filpnm;
                real_pagcnt = old_real_pagcnt;
                curfil = old_curfil;

                currentdirectory = old_currentdirectory;

                first_file = old_first_file;
                glbbit = TRUE;

                reset_filpnm();         /* faff about with start page etc. */

                tracef2("[loadfile_recurse(): ending with filpnm %d filpof %d]\n", filpnm, filpof);
                }
            else
                draw_screen();

            if(!been_error)
                {           
                char *leafp;
                docno doc, ndoc;
                intl index;
                dochandle dhandle, curhan;
  
                do  {
                    curhan = current_document_handle();
                    init_supporting_files(&doc, &index);

                    while(!been_error &&
                          ((ndoc = next_supporting_file(&doc, &index,
                                                        &dhandle, &leafp)) != 0))
                            {
                            char nambuf[MAX_FILENAME];
                            char *ptr = leafname(currentfilename);
                            intl c = ptr - currentfilename;

                            memcpy(nambuf, currentfilename, c);
                            strcpy(nambuf + c, leafp);

                            tracef3("[loadfile_recurse name: %s, next_supp: %s, han: %d]\n",
                                    name, nambuf, dhandle);

                            if(dhandle == DOCHANDLE_NONE)
                                {
                                loadfile_recurse(nambuf, NEW_WINDOW);
                                select_document_using_handle(curhan);
                                break;
                                }
                            }

                    }
                while(!been_error && ndoc);
                }

            if(new_window)
                riscos_frontmainwindow(FALSE);
            }
        else
            destroy_current_document();
        }

    if(!res)
        {
        /* maybe old document destroyed! */
        if(find_document_using_handle(doc))
            select_document_using_handle(doc);

        if(!new_window)
            {
            /* must destroy all that we hold from destroyed document */
            str_clr(&old_currentdirectory);
            delete_list(&old_first_file);
            }
        }

    return(res);
}


/************************************************************
*                                                           *
* open the load file and have a peek to see what type it is *
*                                                           *
************************************************************/

static uchar lotus_leadin[4] = { '\x00', '\x00', '\x02', '\x00' };

/* we know it's a real file, name not null */

#define BYTES_TO_SEARCH 512

extern intl
find_file_type(const char *name)
{
    FILE *input;
    char array[BYTES_TO_SEARCH];
    char buff[LOAD_BUFSIZ];
    intl res, size;
    char *ptr, *start_of_line;
    char ch;
    intl comma_values;
    BOOL in_number, in_string;
    colt dcol;
    rowt drow;
    uchar dch;
    BOOL dbool;

    if(using_Z88  ||  looks_like_z88(name))
        {
        tracef0("looks like Z88, so pretend it's PD\n");
        return(PD_CHAR);
        }

    #if MS || ARTHUR
    if(isdirectory((uchar *) name, (uchar *) UNULLSTR))
        return('\0');
    #endif

    input = myfopen(name, read_str);
    if(!input)
        return('\0');

    mysetvbuf(input, buff, sizeof(buff));


    size = myfread(array, 1, BYTES_TO_SEARCH, input);
    #if RISCOS
    /* error in reading? */
    if(size == -1)
        return('\0');
    #endif
tracef2("[read %d bytes from file, BYTES_TO_SEARCH = %d]\n", size, BYTES_TO_SEARCH);

    /* ensure terminated at a suitable place */
    if(size == BYTES_TO_SEARCH)
        --size;
    array[size] = '\0';


    #if !defined(VIEWSHEET_OFF)
    {
    /* isvsfile screws file position */
    res = isvsfile(input);

    myfclose(input);

    if(res)
        {
        if(res < 0)
            {
            tracef1("error %d from isvsfile\n", res);
            reperr_module(ERR_SHEET, res);
            return('\0');
            }

        tracef0("ViewSheet file\n");
        return(SHEET_CHAR);
        }
    }
    #else
    myfclose(input);
    #endif  /* VIEWSHEET_OFF */


    /* look to see if it's a LOTUS file, yuk */

    if( !memcmp(array, lotus_leadin, 4)         &&
        (array[4] >= 4)  &&  (array[4] <= 6)    &&
        (array[5] == 4))
        {
        tracef0("LOTUS FILE DETECTED!!!\n");
        #if defined(LOTUS_OFF)
        reperr_not_installed(ERR_LOTUS);
        return('\0');
        #else
        return(LOTUS_CHAR);
        #endif
        }


    /* not lotus */

    #if defined(DTP_IMPORT)
    /* test for 1WP file */ 
    if(dtp_isdtpfile(array))
        {
        tracef0("DTP/1wp file\n");
        return(DTP_CHAR);
        }
    #endif


    ptr = array;
    start_of_line = array;
    comma_values = 0;
    in_number = FALSE;

    do  {
        ch = *ptr++;

        tracef2("read char %c (%d)\n", ch, ch);

        switch(ch)
            {
            #if MS
            /* end of file? */
            case CTRLZ:
            #endif
            case TAB:
                {
                tracef0("Tab file found\n");
                return(TAB_CHAR);
                }


            case CR:
            case LF:
                /* if we complete a line full of only comma type options, ok */
                if(ptr - start_of_line > 1)
                    if(comma_values > 1)
                        {
                        tracef0("Line of valid comma options: CSV\n");
                        return(CSV_CHAR);
                        }

                comma_values = 0;
                start_of_line = ptr;
                break;


            case '%':
                /* if it's a PD construct return PD */
                inoption = -1;

                if( lukcon(ptr - 1, &dcol, &drow,
                           &dch, &dch, &dch, dcol, drow,
                           &dch, &dbool, &drow, FALSE, &dbool)  ||
                    (inoption > -1))
                        {
                        tracef0("PipeDream file found\n");
                        return(PD_CHAR);
                        }

                break;


            case SPACE:
                if(in_number)
                    {
                    /* numbers followed by one or more spaces then comma tend to be CSV */
                    do  {
                        ch = *ptr++;
                        tracef2("read char %c in skipspaces (%d)\n", ch, ch);
                        }
                    while(ch == SPACE);
                    --ptr;

                    if(ch != COMMA)
                        in_number = FALSE;
                    }

                break;


            case COMMA:
                if(in_number)
                    {
                    /* if we hit a comma whilst in a number, score one more for the CSV's */  
                    in_number = FALSE;
                    ++comma_values;
                    }
                elif(*ptr == COMMA)
                    /* adjacent commas are a good sign of CSV format */
                    ++comma_values;

                break;


            case QUOTES:
                in_number = FALSE;
                in_string = TRUE;

                do  {
                    ch = *ptr++;

                    tracef2("read char %c in string read (%d)\n", ch, ch);

                    switch(ch)
                        {
                        case CR:
                        case LF:
                        case '\0':
                            /* faulty quoted string */
                            tracef0("Tab file as quoted string was faulty\n");
                            return(TAB_CHAR);

                        case QUOTES:
                            if(*ptr != QUOTES)
                                {
                                /* quoted string ended - one more kosher CSV element */
                                in_string = FALSE;
                                ++comma_values;
                                }
                            else
                                tracef0("escaped quote\n");
                            break;

                        default:
                            break;
                        }
                    }
                while(in_string);

                break;


            #if defined(VIEW_IO)

            /* VIEW stored command? */
            case VIEW_STORED_COMMAND:
                in_number = FALSE;
                comma_values = 0;

                if(isupper(*ptr)  &&  isupper(*(ptr+1)))
                    {
                    tracef0("VIEW stored command\n");
                    return(VIEW_CHAR);
                    }

                break;


            /* VIEW ruler? */
            case VIEW_RULER:
                in_number = FALSE;
                comma_values = 0;

                if((*ptr == '.')  &&  (*(ptr+1) == '.'))
                    {
                    tracef0("VIEW ruler\n");
                    return(VIEW_CHAR);
                    }

                break;

            #endif  /* VIEW_IO */


            default:
                if(isdigit(ch)  ||  (ch == '.')  ||  (ch == '+')  ||  (ch == '-')  ||  (tolower(ch) == 'e'))
                    in_number = TRUE;
                else
                    {
                    in_number = FALSE;
                    comma_values = 0;
                    }

                break;
            }
        }
    while(ch);

    /* nothing recognized */

    tracef0("End of BYTES_TO_SEARCH/EOF - so call it Tab\n");
    return(TAB_CHAR);
}


/********************
*                   *
* find the filetype *
*                   *
********************/

static BOOL
find_data_format(const char *name, BOOL *plaintext, uchar *field_separator)
{
#if defined(LOTUS_OFF)
    IGNOREPARM(name);
#else
    FILE *filein;
    FILE *fileout;
    intl res;
#endif

    *plaintext = FALSE;

    if(!inserting)
        d_save_FORMAT = load_filetype;

    /* find the data format */
    switch((int) load_filetype)
        {
        case PD_CHAR:
            break;


        case CSV_CHAR:
            *field_separator = COMMA;

        #if defined(VIEW_IO)
        case VIEW_CHAR:
        #endif
        #if defined(DTP_IMPORT)
        case DTP_CHAR:
        #endif
        case TAB_CHAR:
            *plaintext = TRUE;
            break;


        #if !defined(VIEWSHEET_OFF)
        case SHEET_CHAR:
            *plaintext = TRUE;
            if(!inserting)
                d_save_FORMAT = PD_CHAR;
            break;
        #endif


        case '\0':
        case LOTUS_CHAR:
#if defined(LOTUS_OFF)
            return(reperr_not_installed(ERR_LOTUS));
#else
            /* clear the text */
            if(inserting)
                return(reperr_null(ERR_CANNOTINSERT));

            newfil();

            filein = myfopen(name, read_str);
            if(!filein)
                return(reperr(ERR_NOTFOUND, name));

            fileout = myfopen(temp_file, write_str);
            if(!fileout)
                {
                reperr_null(ERR_CANNOTWRITE);
                myfclose(filein);
                return(FALSE);
                }

            res = readlotus(filein, fileout, PD_MCTYPE, NULL);

            myfclose(filein);
            myfclose(fileout);

            if(res)
                {
                reperr_module(ERR_LOTUS, res);
                been_error = FALSE;
                check_not_blank_sheet();
                }

            loadname = (char *) temp_file;
        /*  inserting = FALSE; */
            break;
#endif  /* LOTUS_OFF */

        default:
            break;
        }

    return(TRUE);
}


#if RISCOS

/********************************
*                               *
*  return the RISC OS filetype  *
*  from our letter options      *
*                               *
********************************/

extern intl
rft_from_option(int option)
{
    switch(option)
    {
        case TAB_CHAR:
            return(TEXT_FILETYPE);

        case CSV_CHAR:
            return(CSV_FILETYPE);

        #if !defined(LOTUS_OFF)
        case LOTUS_CHAR:
            return(LOTUS123_FILETYPE);
        #endif

        #if defined(VIEW_IO)
        case VIEW_CHAR:
            return(VIEW_FILETYPE);
        #endif

        #if defined(DTP_IMPORT)
        case DTP_CHAR:
            return(DTP_FILETYPE);
        #endif

        default:
            return(PIPEDREAM_FILETYPE);
        }
}

#endif


/* 
block_updref is like upfred except that it takes only a column number, 
and operates on the block of that column and all columns to the right

Adds on coffset to the column part of slot references

Used by:    insert on load
            save_block_on_stack
*/

extern void
block_updref(colt startcol, colt coffset)
{
    slotp tslot;
    SLR topleft;
    SLR botright;

    topleft.col = startcol;
    topleft.row = (rowt) 0;
    botright.col = numcol-1;
    botright.row = numrow-1;

    init_block(&topleft, &botright);

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        if(tslot->flags & SL_REFS)
            {
            uchar *rptr;

            rptr = (tslot->type == SL_TEXT) ? tslot->content.text
                                            : tslot->content.number.text;

            my_init_ref(tslot->type);
            while((rptr = my_next_ref(rptr, tslot->type)) != NULL)
                {
                /* for each slot reference */
                colt cref;
                docno dref;

                /* when inserting block, check for an invalid
                    document number and zot to zero if invalid */
                if(coffset > 0)
                    {
                    dref = (docno) talps(rptr, sizeof(docno));

                    if(check_docvalid(dref) < 0)
                        splat(rptr, (word32) 0, sizeof(docno));
                    }
                rptr += sizeof(docno);

                /* all column references to be updated */
                cref = (colt) talps(rptr, sizeof(colt));

                splat(rptr, (word32) cref + (word32) coffset, sizeof(colt));

                mark_slot(tslot);
                rptr += sizeof(colt) + sizeof(rowt);
                }
            }

        /* put on tree if inserting block into sheet */
        if(coffset > 0)
            {
            if(tslot->type == SL_TEXT)
                {
                if(draw_tree_str_insertslot(in_block.col,
                                            in_block.row, FALSE) < 0)
                    tree_switchoff();                                                       
                }
            else
                {
                if(tree_exp_insertslot(in_block.col, in_block.row, FALSE) < 0)
                    tree_switchoff();
                }
            }
        }
}



#if defined(NEW_CSV)

/* 
munges input character for CSV loading
returns 0 if dealt with character completely, else new char
*/

static intl
comma_convert(intl c, FILE *loadinput, BOOL *in_quotes, uchar *lastch, uchar *typep, uchar *justifyp)
{
    switch(c)
        {
        case QUOTES:
            *typep = SL_TEXT;
            *justifyp = J_FREE;

            if(lecpos == 0)
                {
                *in_quotes = TRUE;
                return(0);
                }

            if(!*in_quotes)
                {
                *lastch = linbuf[lecpos++] = QUOTES;
                return(0);
                }

            if((c = myfgetc(loadinput)) < 0)
                return(c);

            /* continue with new character */
            break;


        case COMMA:
            if(*in_quotes)
                {
                *lastch = linbuf[lecpos++] = (uchar) c;
                return(0);
                }

            break;


        default:
            break;
        }

    return(c);
}

#endif  /* NEW_CSV */


/****************
*               *
* get the file  *
*               *
****************/

extern BOOL
excloa(const char *fname)
{
    char *name = (char *) fname;
    intl c = 0;
    colt tcol;
    rowt trow, timer = 0, rowinfile = 0;
    char lastch = 0;
    BOOL rowrange;
    SLR first;
    BOOL something_on_this_line = FALSE;
    BOOL breakout = FALSE;
    BOOL ctrl_chars = FALSE;
    BOOL first_column = TRUE;
    rowt row_range_start;   /* start of row range */
    rowt row_range_end;     /* end   of row range */
    BOOL plaintext;
    uchar field_separator = TAB;
    uword32 flength;
    uword32 chars_read = 0;
    char load_buffer[LOAD_BUFSIZ];
    FILE *loadinput;
    BOOL loading_Z88;
    #if defined(VIEW_IO)
    BOOL been_ruler = FALSE;
    #endif
    #if !defined(VIEWSHEET_OFF)
    intl vsrows;
    #endif
    #if defined(DTP_IMPORT)
    /* highlights have to be reinstated at the start of each slot */
    uchar h_byte = DTP_NOHIGHLIGHTS;
    #endif
    colt insert_col;
    rowt insert_row;
    rowt insert_numrow = -1;

    if(!name)
        return(reperr_null(ERR_BAD_NAME));

    /* check file there before deleting existing text */

    loading_Z88 = looks_like_z88(name);

    if(!loading_Z88  &&  !filereadable(name))
        return(reperr(ERR_NOTFOUND, name));

    /* are we loading a range of rows? */
    rowrange = (BOOL) (d_load[2].option == 'Y');

    if(rowrange)
        {
        buff_sofar = (uchar *) d_load[2].textfield;
        row_range_start = getsbd()-1;
        row_range_end   = getsbd()-1;

        if( bad_row(row_range_start)    ||
            bad_row(row_range_end)      ||
            (row_range_end < row_range_start))
            return(reperr_null(ERR_BAD_RANGE));
        }

    if( load_filetype == AUTO_CHAR)
        load_filetype = find_file_type(name);

    tracef1("determined file type to be '%c'\n", load_filetype);


    if(!find_data_format(name, &plaintext, &field_separator))
        return(FALSE);


    loadname = name;

    /* are we inserting at slot? */
    if(inserting)
        {
        /* insert off to the right, don't update references during load,
         * bodge references of inserted bit afterwards, and move in to where
         * it's supposed to be.
        */
        first.col = numcol;
        first.row = 0;

        if(str_isblank(d_load[1].textfield))
            {
            insert_col = curcol;
            insert_row = currow;
            }
        else
            {
            buff_sofar = (uchar *) d_load[1].textfield;
            insert_col = getcol();
            insert_row = getsbd()-1;
            }

        if(bad_reference(insert_col, insert_row))
            return(reperr_null(ERR_BAD_SLOT));
        }
    else  /* overwriting existing(?) file */
        {
        first.col = (colt) 0;
        first.row = (rowt) 0;

        if(!save_existing())
            return(FALSE);

        newfil();

        if(plaintext)
            dftcol();           /* >>>>>> may now fail */
        else
            killcoltab();

        #if RISCOS && FALSE
        if(name == FILE_IN_RAM_BUFFER)
            {
            if(!set_untitled_document())
                return(FALSE);
            }
        else
        #endif
            {
            if(!str_setc(&currentfilename, name))
                return(FALSE);

            #if RISCOS
            riscos_settitlebar(currentfilename);

            riscos_readfileinfo(&currentfileinfo, name);
            tracef2("fileinfo: load = %8.8X exec = %8.8X\n",
                        currentfileinfo.load, currentfileinfo.exec);

            if(load_filetype != TAB_CHAR)
                riscos_settype(&currentfileinfo, rft_from_option(load_filetype));

            initial_filetype = load_filetype;
            #endif
            }

        exp_rename_file();
        }

    tcol = first.col;
    trow = first.row;

    /* open the file and buffer it */

    loadinput = myfopen(loadname, read_str);

    if(!loadinput)
        {
        if(using_Z88)
            return(FALSE);

        return(reperr(ERR_CANNOTOPEN, loadname));
        }

    if(!using_Z88)
        setvbuf(loadinput, load_buffer, loading_Z88 ? _IONBF : _IOFBF, sizeof(load_buffer));

    if(!loading_Z88)
        {
        #if MS
        flength = filelength(fileno(loadinput));
        #else
        flength = filelength(loadinput);
        #endif /* MS */
        /* we're going to divide by this */
        if(!flength)
            flength = 1;
        }

    #if MS || ARTHUR
    ack_esc();
    #endif


    #if defined(VIEW_IO)
    if(loading_view)
        view_load_preinit();
    #endif

    #if defined(DTP_IMPORT)
    if(loading_dtp)
        dtp_load_preinit(loadinput);
    #endif


    #if !defined(VIEWSHEET_OFF)
    if(loading_viewsheet)
        {
        if(isvsfile(loadinput) <= 0) 
            vsrows = VSLOAD_ERR_CANTREAD;
        else
            vsrows = loadvsfile(loadinput);

        if(vsrows < 0)
            {
            myfclose(loadinput);
            return(reperr_module(ERR_SHEET, vsrows));
            }
        }
    #endif
    
    (void) init_dialog_box(D_FIXES);

    #if RISCOS
    (void) init_dialog_box(D_FONTS);
    #endif

    in_load = TRUE;

    escape_enable();

    /* read each slot from the file */

    #if !defined(VIEWSHEET_OFF)
    if(loading_viewsheet)
        {
        intl col, row;

        tracef0("[loading ViewSheet]\n");

        for(col = 0; col < 255; col++)
            for(row = 0; row < vsrows; row++)
                {
                intl vstype, vsdecp, vsrjust, vsminus;
                char *vslot;
                uchar type, justify, format;

                if(!actind(ACT_LOAD, col * 100 / 255))
                    {
                    breakout = TRUE;
                    goto ENDSHEET;
                    }

                vslot = vstravel(col, row, &vstype, &vsdecp,
                                 &vsrjust, &vsminus);

                if(vslot)
                    {
                    justify = vsrjust ? J_RIGHT : J_LEFT;

                    if(vstype == VS_TEXT)
                        type = SL_TEXT;
                    else
                        {
                        type = SL_NUMBER;

                        if(vsminus < 0)
                            {
                            justify = J_RIGHT;
                            format = 0;
                            }
                        else
                            {
                            format = (uchar) (vsminus ? 0 : F_BRAC);
                            format |= F_DCP | ((vsdecp < 0) ? 0xF : vsdecp);
                            }
                        }

                    if(inserting)
                        {
                        if(!(insertslotat((colt) col, (rowt) row)))
                            goto ENDSHEET;

                        /* need to know how deep inserted bit is */
                        if( insert_numrow < (rowt) row + 1)
                            insert_numrow = (rowt) row + 1;
                        }

                    /* if something to put in slot, put it in */
                    strcpy(linbuf, vslot);
                    if(!stoslt((colt) col, (rowt) row,
                               type, justify, format, 0))
                        {
                        breakout = TRUE;
                        goto ENDSHEET;
                        }
                    }
                }

        ENDSHEET:
            ;
        }
    else
    #endif
    
    /* each slot */
    while(c >= 0)
        {
        BOOL inconstruct = FALSE;
        uchar type = loading_comma ? SL_NUMBER : SL_TEXT;
        uchar justify = loading_comma ? J_RIGHT : J_FREE;
        char pageoffset = 0;
        uchar format = 0;
        BOOL outofmem = FALSE;
        #if defined(NEW_CSV)
        BOOL in_quotes = FALSE;
        #endif

        inoption = -1;
        start_of_construct = 0;

        if(ctrlflag  ||  breakout  ||  (timer++ > TIMER_DELAY))
            {
            if( ctrlflag  ||  breakout  ||
                !actind(ACT_LOAD, loading_Z88
                                    ? NO_ACTIVITY
                                    : (intl) ((100 * chars_read) / flength)))
                {
                breakout = TRUE;
                break;
                }

            timer = 0;
            }
        /* on slow devices or inserting, poll actind more frequently */
        elif(loading_Z88  ||  inserting)
            timer += TIMER_DELAY >> 4;


        /* each char in slot */
        lecpos = 0;

        #if defined(DTP_IMPORT)
        dtp_change_highlights(h_byte, DTP_NOHIGHLIGHTS);
        #endif

        do  {
            uchar ch;

            if((c = myfgetc(loadinput)) < 0)
                break;

            chars_read++;

            ch = (uchar) c;

            /* ignore CR or LF if previous char was LF or CR */
            if((ch== CR)  ||  (ch == LF))
                if((ch + lastch) == (CR + LF))
                    {
                    lastch = 0;
                    continue;
                    }


            switch(load_filetype)
                {
                #if defined(VIEW_IO)
                case VIEW_CHAR:
                    c = view_convert(c, loadinput, &lastch, &been_ruler,
                                     &justify, &type, &pageoffset);
                    break;
                #endif

                #if defined(NEW_CSV)
                case CSV_CHAR:
                    c = comma_convert(c, loadinput, &in_quotes, &lastch, &type, &justify);
                    break;
                #endif

                #if defined(DTP_IMPORT) 
                case DTP_CHAR:
                    c = dtp_convert(c, loadinput, &field_separator, 
                                    &justify, &h_byte, &pageoffset, &type);
                    break;
                #endif

                default:
                    break;  
                }

            if(c <= 0)
                continue;


            /* check for control chars */
            switch(c)
                {
                #if MS
                case CTRLZ:
                    /* don't give ctrl chars error message for CTRLZ
                     * 'cos MS-DOS creates these on copy
                    */
                    continue;
                #endif

                case SLRLDI:
                case FUNNYSPACE:
                    ctrl_chars = TRUE;
                    continue;

                default:
                    break;
                }

            lastch = ch = (uchar) c;

            /* end of slot? */
            if((ch == CR)  ||  (ch == LF))
                break;

            if((ch == field_separator)  &&  plaintext)
                break;


            /* add char to buffer */
            linbuf[lecpos++] = ch;

            if((ch == '%')  &&  !plaintext  &&  (inoption < 0))
                {
                if(!inconstruct)
                    {
                    inconstruct = TRUE;
                    start_of_construct = lecpos-1; /* points at first % */
                    }
                else
                    {
                    linbuf[lecpos] = '\0';

                    if(lukcon(linbuf+start_of_construct, &tcol, &trow, &type,
                                &justify, &format, first.col, first.row,
                                &pageoffset, &outofmem, &rowinfile, TRUE, &first_column))
                        {
                        lecpos = start_of_construct;
                        inconstruct = FALSE;
                        }
                    elif(outofmem)
                        break;
                    else
                        start_of_construct = lecpos;
                    }
                }
            }
        while((c >= 0)  &&  (lecpos < MAXFLD));


        if(outofmem)
            break;

        linbuf[lecpos] = '\0';

        if(inoption > -1)
            {
            if(!inserting)
                getoption(linbuf+inoption);
            }
        else
            {
            if(!rowrange || ((rowinfile >= row_range_start)  &&  (rowinfile <= row_range_end)))
                {
                something_on_this_line = TRUE;

                /* if inserting, insert slot here */
                if(inserting  &&  ((c >= 0)  ||  (lecpos > 0)))
                    {
                    if(!(insertslotat(tcol, trow)))
                        break;

                    /* need to know how deep inserted bit is */
                    if( insert_numrow < trow+1)
                        insert_numrow = trow+1;
                    }

                /* if something to put in slot, put it in */
                if( ((lecpos > 0)  ||  (type == SL_PAGE))  &&
                    !stoslt(tcol, trow, type, justify, format, pageoffset))
                    {
                    breakout = TRUE;
                    break;
                    }
                }

            /* plain text mode, move right */
            if(lastch == field_separator)
                tcol++;
            else
                {
                rowinfile++;    /* move down */

                if(something_on_this_line)
                    {
                    something_on_this_line = FALSE;
                    trow++;
                    }

                if(plaintext)
                    tcol = first.col;
                }
            } 
        }


    #if !defined(Z88_OFF)
    if(using_Z88  &&  (c != EOF))
        {
        breakout = TRUE;
        newfil();
        reperr_module(ERR_Z88, z88_geterr());
        }
    #endif

    #if defined(VIEW_IO)
    if(loading_view)
        view_load_postinit();
    #endif

    #if !defined(VIEWSHEET_OFF)
    if(loading_viewsheet)
        vsfileend();
    #endif

    if(breakout)
        glbbit = FALSE;

    myfclose(loadinput);

    in_load = FALSE;

    if(!check_not_blank_sheet())
        breakout = TRUE;

    #if !defined(HEADLINE_OFF)
    xf_drawmenuheadline = 
    #endif
    out_screen = out_rebuildvert = out_rebuildhorz = TRUE;

    curcol = first.col;
    currow = first.row;

    if(inserting)
        {
        SLR o_blkstart;
        SLR o_blkend;
        dochandle o_blkdochandle;
        colt width_of_new;

        o_blkdochandle  = blkdochandle;
        o_blkstart      = blkstart;
        o_blkend        = blkend;

        /* move block to insert_col, insert_row, insert_numrow rows deep */
        blkdochandle = current_document_handle();
        blkstart.col = first.col;
        blkstart.row = 0;
        blkend.col   = numcol-1;
        blkend.row   = insert_numrow-1;

        curcol = insert_col;
        currow = insert_row;

        block_updref(first.col, first.col);

        /* if ran out of memory, throw away what's been loaded */
        if(breakout)
            do_delete_block(FALSE);
        else
            MoveBlock_fn();

        blkdochandle = o_blkdochandle;
        blkstart     = o_blkstart;
        blkend       = o_blkend;

        /* determine whether new bit makes sheet wider */
        width_of_new = numcol - first.col;
        if(width_of_new + curcol >= first.col)
            first.col = width_of_new + curcol;

        /* delete all the columns at end */
        delcolentry(first.col, numcol - first.col);
        }
    else
        {
        FirstColumn_fn();
        TopOfColumn_fn();

        #if RISCOS 
        update_fontinfo_from_dialog();
        #endif

        /* get the fixed rows and cols info and do something with it */
            
        if(!str_isblank(d_fixes[0].textfield))
            {
            /* rows */
            long firstrow = 0, nrows = 0;

            sscanf(d_fixes[0].textfield, "%ld,%ld", &firstrow, &nrows);

            if(nrows > 0)
                {
                filvert((rowt) firstrow, (rowt) 0, FALSE);
                /* fix rows */
                currow = firstrow + nrows - 1;
                FixRows_fn();
                currow = 0;
                }
            }

        if(!str_isblank(d_fixes[1].textfield))
            {
            /* cols */
            int firstcol = 0, ncols = 0;

            sscanf(d_fixes[1].textfield, "%d,%d", &firstcol, &ncols);

            if(ncols > 0)
                {
                filhorz((colt) firstcol, (colt) 0);
                /* fix cols */
                curcoloffset = ncols - 1;
                FixColumns_fn();
                curcol = 0;
                }
            }
        }

    clear_protect_list();   /* updating slots - must do after possible insertion discard */

    setlogcolours();

    actind_end();

    escape_disable();

    if(ctrl_chars)
        reperr_null(ERR_CTRL_CHARS);

    if(!breakout)
        {
        #if RISCOS
        /* Init all but filename */
        char *name = NULL;
        str_swap(&name, &d_load[0].textfield);      
        (void) init_dialog_box(D_LOAD);
        str_swap(&name, &d_load[0].textfield);      

        #else

        #if !defined(MANY_DOCUMENTS)
        if(!str_setc(&d_save[SAV_NAME].textfield, fndfcl()))
            breakout = TRUE;
        #endif
        (void) init_dialog_box(D_LOAD);
        #endif
        }
    #if defined(MANY_DOCUMENTS) && !RISCOS
    else
        str_clr(&d_save[SAV_NAME].textfield);
    #endif

    update_variables();
    xf_fileloaded = TRUE;

    filealtered(inserting);

    if(!breakout)
        global_recalc = recalc_forced = recalc_bit = TRUE;

    #if !defined(LOTUS_OFF)
    if(d_save_FORMAT == LOTUS_CHAR)
        remove(temp_file);
    #endif

    return(!breakout);
}


/************************************************
*                                               *
*  return pointer to file name of current file  *
*                                               *
************************************************/

extern char *
fndfcl(void)
{
    #if defined(MULTI_OFF)
    return(currentfilename);
    #else
    LIST *lptr;

    if(!glbbit)
        return(currentfilename);

    lptr = search_list(&first_file, (word32) curfil);

    return(lptr ? ((struct multifilestr *) lptr->value)->Sname : NULL);
    #endif
}

/* end of savload.c */
