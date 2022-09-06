/* commlin.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       commlin.c - module that edits and calls commands
 * Author:      RJM 1987
 * History:
 *  0.01 xx-mmm-87 RJM created
 *  0.02 20-Feb-89 SKS moved display_heading into scdraw
 *  0.03 01-Mar-89 SKS RISC OS menus spring into life!
 *  0.04 07-Mar-89 SKS different tokens for menu title items, remove crud
 *  0.05 08-Mar-89 SKS more tokens, fixed arrow+F12 displays, octal tokens
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"

#elif MS
#include <graph.h>
#include "version.ext"

#else
    assert(0);
#endif


#include "datafmt.h"

#if RISCOS
#include "ext.riscos"
#include "riscmenu.h"

#if defined(SG_MENU_ITEMS)
#include "cs-wimptx.h" /* for wimptx_os_version_query */
#endif
#endif


/* exported functions */

extern intl command_edit(intl ch);
extern void display_heading(intl idx);
extern BOOL do_command(intl key, BOOL spool);
extern void do_execfile(const char *name);
extern void draw_bar(coord xpos, coord ypos, coord length);
extern intl fiddle_with_menus(intl c, BOOL alt_detected);
extern BOOL get_menu_item(MENU_HEAD *header, MENU *mptr, char *array /*out*/);
extern word32 getsbd(void);
extern void headline_initialise_once(void);
extern intl inpchr(BOOL curs);
extern void output_char_to_macro_file(uchar ch);
extern void out_comm_start_to_macro_file(MENU *mptr);
extern void out_comm_parms_to_macro_file(DIALOG *dptr, intl size, BOOL ok);
extern void out_comm_end_to_macro_file(MENU *mptr);
extern void resize_menus(BOOL short_m);
extern BOOL schkvl(intl c);


/* exported variables */

coord this_heading_length = 0;


/* internal functions */

#if TRUE
/*static uchar alt_letter(intl c);*/
static intl lukucm(BOOL allow_redef);               /* look up command */
static intl part_command(BOOL allow_redef);

#if RISCOS
static intl fiddle_alt_riscos(intl ch);
#elif MS || ARTHUR
static intl do_menu(void);
static void draw_item(coord xpos, coord ypos, MENU_HEAD *header, MENU *mptr,
                      BOOL inverse, coord menuwidth);
#endif
#endif


/* ----------------------------------------------------------------------- */

#if RISCOS

/****************************************************
*                                                   *
* variant of inpchr for RISC OS                     *
*                                                   *
* --in--                                            *
*   16-bit character (as from rdch())               *
*                                                   *
* --out--                                           *
*   +ve     char to be inserted                     *
*   0       do nothing as there's been an error     *
*   -ve     function number to execute              *
*                                                   *
****************************************************/

extern intl
inpchr_riscos(intl keyfromriscos)
{
    intl c;

    tracef1("inpchr_riscos(&%3.3X)\n", keyfromriscos);

    tracef2("[inpchr_riscos cbuff_offset: %d, keyidx: %d]\n",
            cbuff_offset, keyidx);

    do  {
        do  {
            /* if in expansion get next char */
            if(keyidx)
                {
                if((c = *keyidx++) == '\0')
                    keyidx = NULL;      /* this expansion has ended */
                }
            else
                /* no expansion char so try 'keyboard' */
                {
                if(keyfromriscos == 0)
                    {
                    tracef0("read key for risc os already: returns 0\n");
                    return(0);
                    }

                c = keyfromriscos;
                keyfromriscos = 0;
                }
            }
        /* expand char if not in edit line and not already expanding */
        while(!alt_array[0]  &&  !cbuff_offset  &&  c  &&  !keyidx  &&  schkvl(c));

        tracef2("[inpchr_riscos cbuff_offset: %d, keyidx: %d]\n",
                cbuff_offset, keyidx);

        if(!*keyidx)
            {
            /* expansion now completed after reading last char */
            keyidx = NULL;

            if(cbuff_offset  &&  (c != CR))
                {
                /* half a command entered on function key or something */
                cbuff_offset = 0;
                reperr_null(ERR_BAD_PARM);
                tracef0("error in key expansion: returns 0\n");
                return(0);          /* an uninteresting result */
                }
            }

        /* see if we're doing an Alt-sequence */
        c = fiddle_alt_riscos(c);

        /* starting, or continuing a command from a key expansion? */
        if((c == CMDLDI)  ||  cbuff_offset)
            c = command_edit(c);
        }
    while(!c  &&  (cbuff_offset > 0));  /* incomplete command? */

    tracef1("inpchr_riscos returns %d\n", c);
    return(c);
}

#elif MS || ARTHUR

/********************************************************
*                                                       *
* get a character and return it if it is to be inserted *
* Otherwise deal with command editing and execution     *
*                                                       *
********************************************************/

extern intl
inpchr(BOOL curs)
{
    intl c = 0;
    BOOL from_keyboard = TRUE;
    time_t entered = time(NULL);

    do  {
        BOOL alt_detected = FALSE;

        for(;;)
            {
            if(keyidx)
                {
                /* get next char from expansion */
                from_keyboard = FALSE;
                /* need to trap CTRL Y == CMDLDI, when typed */
                c = *keyidx++;
                if(!c)
                    keyidx = NULL;
                }
            else
                /* no expansion char so get from keyboard */
                {
                BOOL entered_alt = depressed(ALT);

                if(curs)
                    curon();

                while(c == '\0'  &&  !alt_detected)
                    {
#if MS
                    ack_esc();
#endif

                    if(!depressed(ALT))
                        entered_alt = FALSE;

                    while(!entered_alt && depressed(ALT))
                        {
                        alt_detected = TRUE;
                        if(keyinbuffer())
                            {
                            alt_detected = FALSE;
                            c = rdch(TRUE, TRUE);
                            break;
                            }
                        }
                    if(keyinbuffer())
                        c = rdch(curs, TRUE);
                    /*
                    if nothing happening at keyboard for 5 seconds go
                    back up to top-level for resumption of calshe
                    */

                    if(!entered_alt && c==0 && 
                            difftime(time(NULL), entered) > (double) 2)         
                        return(0);
                    }

                curoff();
                }

            if(!alt_detected  &&  cbuff_offset == 0  &&  c != '\0'  &&
                                    keyidx == NULL  &&  schkvl(c))
                continue;

            /* expand char if not expanding and not in edit line */

            break;
            }

        if(*keyidx == '\0')
            {
            keyidx = NULL;
            if((cbuff_offset > 0)  &&  (c != CR))
                {
                /* half a command entered on function key or something */
                c = cbuff_offset = 0;
                reperr_null(ERR_BAD_PARM);
                break;
                }
            }

        /* starting, or continuing a command from a key expansion? */
        if((c == CMDLDI  &&  !from_keyboard)  ||  (cbuff_offset > 0))
            c = command_edit(c);
        else
            c = fiddle_with_menus(c, alt_detected);

        if(xf_drawmenuheadline  &&  in_dialog_box)
            display_heading(-1);

        } while((c == 0)  &&  (cbuff_offset > 0));

    return(c);
}

#endif


/********************************************************************
*                                                                   *
* exec the file, cannot assume it exists or that it is correct type *
*                                                                   *
********************************************************************/

extern void
do_execfile(const char *in_name)
{
    intl res;
    int c;
    FILE *input;
    uchar name[MAX_FILENAME];
    uchar array[LIN_BUFSIZ];
    uchar buffer[LIN_BUFSIZ];
    uchar *ptr;

    res = add_path(name, in_name, TRUE)
                ? find_file_type(name)
                : '\0';

    if(res != 'T')
        {
        reperr((res == '\0') ? ERR_NOTFOUND : ERR_NOTTABFILE, in_name);
        return;
        }

    input = myfopen(name, read_str);
    if(!input)
        {
        reperr(ERR_CANNOTOPEN, in_name);
        return;
        }
 
    mysetvbuf(input, buffer, sizeof(buffer));

    filbuf();

    in_execfile = TRUE;

    do  {
        c = getfield(input, array, FALSE);  /* get a line or something */

        prccml(array);      /* convert funny characters */

        ptr = array;

        while(*ptr)
            {
            /* if it's a command execute it, else if it's a key, expand it */

            if(been_error)
                {
                myfclose(input);

                in_execfile = FALSE;
                return;
                }

            if(*ptr == CMDLDI)
                {
                intl comm;
                uchar *oldpos = ptr;

                exec_ptr = NULL;
                keyidx   = cbuff;

                /* copy command to small array for execution */

                for(;;)
                    {
                    switch(*ptr)
                        {
                        case TAB:
                        case CR:
                        /* exec_ptr points to the start of a possible parameter string
                         * for dialog box.  If there is a dialog box then after act_on_c()
                         * exec_ptr will point to the carriage-return
                        */
                            exec_ptr = ptr++;
                            break;

                        case '\0':
                            /* unmatched / */
                            reperr_null(ERR_BAD_PARM);
                            goto BREAKOUT;

                        default:
                            *keyidx++ = *ptr++;
                            continue;
                        }
                    break;
                    }

                *keyidx = '\0';
                keyidx = cbuff;
                comm = lukucm(FALSE);
                if(comm == NO_COMM)
                    {
                    /* can't find command so convert CMDLDI back to backslash */
                    ptr = oldpos;
                    *ptr = '\\';
                    }
                else
                    {
                    act_on_c(comm);
                    ptr = exec_ptr+1;

                    /* having done a command, read to end of line */
                    while(*ptr != '\0' && *ptr != CR && *ptr != LF)
                        ptr++;
                    }

                keyidx = NULL;
                }
            else
                /* it's not a CMDLDI */
                {
                cbuff[0] = *ptr;
                cbuff[1] = '\0';
                keyidx = cbuff;

                /* reset keyidx if it's a command, eg CR, TAB */
                schkvl(*ptr++);

                if(keyidx)
                    /* 'input' the characters */
                    #if RISCOS
                    act_on_c(inpchr_riscos(0));
                    #else
                    act_on_c(inpchr(FALSE));
                    #endif
                }

            if(is_current_document())
                draw_screen();
            }
        }
    while (c != EOF);

BREAKOUT:
    myfclose(input);

    exec_ptr    = NULL;
    in_execfile = FALSE;
}


/****************************************************************
*                                                               *
* command_edit is called by inpchr when a key is being expanded *
*                                                               *
****************************************************************/

extern intl
command_edit(intl ch)
{
    intl i;

    switch((int) ch)
        {
        case TAB:
            /* have got a command parameter
             * set exec_ptr to the TAB position
             * set keyidx to first character after return
            */
            exec_ptr = keyidx - 1;
        /*  in_execfile = TRUE; */
            command_expansion = TRUE;

            do { ch = *keyidx++; } while((ch != CR)  &&  (ch != '\0'));

            /* deliberate fall thru... */

        case CR:
            cbuff[cbuff_offset] = '\0';
            i = lukucm(FALSE);
        /*  if(i != NO_COMM) */
                cbuff_offset = 0;
            return(i);

        default:
            if((ch != CMDLDI)  &&  ((ch < SPACE)  ||  (ch > 127)))
                break;

            /* add character to end of command buff */
            if(cbuff_offset >= CMD_BUFSIZ-1)
                {
                bleep();
                return(0);
                }

            cbuff[cbuff_offset++] = (uchar) ch;
            break;
        }

    return(0);           /* nothing to add to text buffer */
}


/********************************************************************
*                                                                   *
* get and return decimal, updating pointer                          *
* assumed capable of reading row numbers, therefore must read longs *
* note usage of buff_sofar                                          *
* note NO_VAL assumed negative by things calling getsbd             *
*                                                                   *
********************************************************************/

extern word32
getsbd(void)
{
    intl count;
    word32 res;
    uchar ch;

    if(!buff_sofar)
        return(NO_VAL);

    do { ch = *buff_sofar++; } while(ch == SPACE);
    --buff_sofar;

    if(!isdigit(*buff_sofar))
        {
        /* for define key need to return chars, return as negative */
        ch = *buff_sofar++;
        if(ch == '\0')
            return(NO_VAL);

        return((coord) (0 - (coord) ch));
        }

#if ARTHUR || RISCOS || MS
    sscanf((char *) buff_sofar, "%ld%n", &res, &count);
    if(!count)
        return(NO_VAL);

    #if MS
    /* MS scanf can read past terminating NULL */
    if(buff_sofar[count-1] == '\0')
        count--;
    #endif
#else
    assert(0);  /* check %n in sscanf works on this implementation */
#endif

    buff_sofar += count;

    return(res);
}


#if defined(OVERLAYS)

#if defined(HELP_OVERLAY)
extern void Help_fn(void)               { overlay_Help_fn(); }
#endif

#if defined(SPELL_OVERLAY)
extern void AutoSpellCheck_fn(void)     { overlay_AutoSpellCheck_fn(); }
extern void CheckDocument_fn(void)      { overlay_CheckDocument_fn(); }
extern void BrowseDictionary_fn(void)   { overlay_BrowseDictionary_fn(); }
extern void Anagrams_fn(void)           { overlay_Anagrams_fn(); }
extern void Subgrams_fn(void)           { overlay_Subgrams_fn(); }
extern void DumpDictionary_fn(void)     { overlay_DumpDictionary_fn(); }
extern void MergeFileWithDict_fn(void)  { overlay_MergeFileWithDict_fn(); }
extern void InsertWordInDict_fn(void)   { overlay_InsertWordInDict_fn(); }
extern void DeleteWordFromDict_fn(void) { overlay_DeleteWordFromDict_fn(); }
extern void LockDictionary_fn(void)     { overlay_LockDictionary_fn(); }
extern void UnlockDictionary_fn(void)   { overlay_UnlockDictionary_fn(); }
extern void CreateUserDict_fn(void)     { overlay_CreateUserDict_fn(); }
extern void OpenUserDict_fn(void)       { overlay_OpenUserDict_fn(); }
extern void CloseUserDict_fn(void)      { overlay_CloseUserDict_fn(); }
extern void DisplayOpenDicts_fn(void)   { overlay_DisplayOpenDicts_fn(); }
extern void PackUserDict_fn(void)       { overlay_PackUserDict_fn(); }
#endif

#if defined(PRINT_OVERLAY)
extern void Print_fn(void)              { overlay_Print_fn(); }
extern void PageLayout_fn(void)         { overlay_PageLayout_fn(); }
extern void PrinterConfig_fn(void)      { overlay_PrinterConfig_fn(); }
extern void MicrospacePitch_fn(void)    { overlay_MicrospacePitch_fn(); }
extern void SetMacroParm_fn(void)       { overlay_SetMacroParm_fn(); }
extern void PrintStatus_fn(void)        { overlay_PrintStatus_fn(); }
#endif

#if defined(SEARCH_OVERLAY)
extern void Search_fn(void)             { overlay_Search_fn(); }
extern void NextMatch_fn(void)          { overlay_NextMatch_fn(); }
extern void PrevMatch_fn(void)          { overlay_PrevMatch_fn(); }
#endif

#if defined(FILES_OVERLAY)
extern void LoadFile_fn(void)           { overlay_LoadFile_fn(); }
extern void NameFile_fn(void)           { overlay_NameFile_fn(); }
extern void SaveFile_fn(void)           { overlay_SaveFile_fn(); }
extern void SaveInitFile_fn(void)       { overlay_SaveInitFile_fn(); }
extern void FirstFile_fn(void)          { overlay_FirstFile_fn(); }
extern void NextFile_fn(void)           { overlay_NextFile_fn(); }
extern void PrevFile_fn(void)           { overlay_PrevFile_fn(); }
extern void LastFile_fn(void)           { overlay_LastFile_fn(); }
#endif

#endif /* OVERLAYS */


/************************************************************************
*                                                                       *
*                               Menus                                   *
*                                                                       *
************************************************************************/

/********************
*                   *
* menu definitions  *
*                   *
********************/


/* flags, text, command, help key, default key, function address, number */

#if RISCOS
#   define  menu__item(m, t, c, h, k, f, n)     { t, c, k, h, m, n, f }
#elif MS || ARTHUR
#   define  menu__item(m, t, c, h, k, f, n)     { t, c, k, h, m, n, f }
#endif


#if defined(DEMO) || defined(MULTI_OFF)

/* a function to replace functions taken out for various purposes */

static void
not_installed_func(void)
{
    reperr_not_installed(ERR_GENFAIL);
}

#endif


#if defined(DEMO)
#   define  demofunc(f)                     not_installed_func
#   define  demo__item(m, t, c, h, k, f, n) menu__item((m) | GREYDEMO, t, c, h, k, f, n)
#else
#   define  demofunc(f)                     f
#   define  demo__item                      menu__item
#endif


#if defined(MULTI_OFF)
#define multifunc(f) not_installed_func
#else
#define multifunc(f) f
#endif


/* special menu item: draw line across menu at this point */

#define menu__item_line_short   menu__item( SHORT,  NULL, NULL, 0, 0, NULL, 0 )
#define menu__item_line_long    menu__item( LONG,   NULL, NULL, 0, 0, NULL, 0 )


static MENU files_ptr[] =
{
    menu__item( SHORT | HAS_DIALOG, Load_STR,                       "Fl",   0,      0,              LoadFile_fn,                    N_LoadFile          ),
    demo__item( SHORT | HAS_DIALOG, Save_STR,                       "Fs",   0,      0,              demofunc(SaveFile_fn),          N_SaveFile          ),
    demo__item( LONG  | HAS_DIALOG, Save_initialisation_file_STR,   "Fi",   0,      0,              demofunc(SaveInitFile_fn),      N_SaveInitFile      ),
    menu__item( SHORT | HAS_DIALOG, Name_STR,                       "Fc",   0,      0,              NameFile_fn,                    N_NameFile          ),
    menu__item_line_long,
    menu__item( LONG,               New_window_STR,                 "Bnew", 0,      0,              NewWindow_fn,                   N_NewWindow         ),
#if !RISCOS
    menu__item( LONG,               Next_window_STR,                "Fw",   0,      0,              NextWindow_fn,                  N_NextWindow        ),
    menu__item( LONG,               Close_window_STR,               "Fq",   0,      0,              CloseWindow_fn,                 N_CloseWindow       ),
#endif
    menu__item_line_short,
/* need address of tickable entry */
#if RISCOS
#define menu_option_flagp (&(files_ptr + 7)->flags)
#else
#define menu_option_flagp (&(files_ptr + 9)->flags)
#endif
    menu__item( SHORT | TICKABLE,   Short_menus_STR,                "M",    0,      0,              MenuSize_fn,                    N_MenuSize          ),
    menu__item( SHORT | HAS_DIALOG, Options_STR,                    "O",    0,      0,              Options_fn,                     N_Options           ),
    menu__item( SHORT | HAS_DIALOG, Colours_STR,                    "Fr",   0,      0,              Colours_fn,                     N_Colours           ),
#if MS
    menu__item( LONG,               Deep_screen_STR,                "Fe",   0,      0,              DeepScreen_fn,                  N_DeepScreen        ),
#endif
#if !defined(MULTI_OFF)
    menu__item_line_long,
    menu__item( LONG,               Next_file_STR,                  "Fn",   0x16,   NEXTFILE,       multifunc(NextFile_fn),         N_NextFile          ),
    menu__item( LONG,               Previous_file_STR,              "Fp",   0x15,   PREVFILE,       multifunc(PrevFile_fn),         N_PrevFile          ),
    menu__item( LONG,               Top_file_STR,                   "Ft",   0,      0,              multifunc(FirstFile_fn),        N_FirstFile         ),
    menu__item( LONG,               Bottom_file_STR,                "Fb",   0,      0,              multifunc(LastFile_fn),         N_LastFile          ),
#endif
#if !defined(SMALLPD)
    menu__item_line_long,
    demo__item( LONG  | HAS_DIALOG, Create_linking_file_STR,        "Ff",   0,      0,              demofunc(CreateLinkingFile_fn), N_CreateLinkingFile ),
#endif
    menu__item_line_short,
#if MS || ARTHUR
    menu__item( LONG,               OSCommand_STR,                  "Fd",   0,      0,              OSCommand_fn,                   N_OSCommand         ),
#endif
    menu__item( SHORT,              Recalculate_STR,                "A",    0,      0,              Recalculate_fn,                 N_Recalculate       ),
    menu__item( LONG  | HAS_DIALOG, Recalculation_options_STR,      "Fo",   0,      0,              RecalcOptions_fn,               N_RecalcOptions     ),
#if !defined(HELP_OFF)
    menu__item( SHORT,              Help_STR,                       "Fh",   0x01,   HELP,           Help_fn,                        N_Help              ),
    menu__item( SHORT | HAS_DIALOG, About_STR,                      "Fa",   0,      0,              About_fn,                       N_About             )
#endif
#if !RISCOS
,   menu__item( SHORT,              Exit_STR,                       "Fx",   0,      0,              Quit_fn,                        N_Quit              )
#endif
};


static MENU edit_ptr[] =
{
    menu__item( SHORT,              Delete_character_STR,           "G",    0x0A,   DELETECHAR,     DeleteCharacterRight_fn,        N_DeleteCharacterRight  ),
    menu__item( LONG,               Insert_space_STR,               "U",    0x09,   INSERTCHAR,     InsertSpace_fn,                 N_InsertSpace           ),
    menu__item( LONG  | HAS_DIALOG, Insert_character_STR,           "Ec",   0,      0,              InsertCharacter_fn,             N_InsertCharacter       ),
    menu__item( SHORT,              Delete_word_STR,                "T",    0x14,   DELETEWORD,     DeleteWord_fn,                  N_DeleteWord            ),
    menu__item( SHORT,              Delete_to_end_of_slot_STR,      "D",    0x04,   DELEOL,         DeleteToEndOfSlot_fn,           N_DeleteToEndOfSlot     ),
    menu__item( SHORT,              Paste_STR,                      "I",    0,      0,              Paste_fn,                       N_Paste                 ),
    menu__item( SHORT,              Delete_row_STR,                 "Y",    0x08,   DELETEROW,      DeleteRow_fn,                   N_DeleteRow             ),
    menu__item( SHORT,              Insert_row_STR,                 "N",    0x07,   INSERTROW,      InsertRow_fn,                   N_InsertRow             ),
    menu__item_line_short,
/* need address of tickable entry */
#define insert_option_flagp (&(edit_ptr + 9)->flags)
    menu__item( LONG  | TICKABLE,   Insert_STR,                     "V",    0x25,   INSOVER,        InsertOvertype_fn,              N_InsertOvertype        ),
#if !defined(SPELL_OFF)
    menu__item( SHORT,              Swap_case_STR,                  "ss",   0x12,   SWAPCASE,       SwapCase_fn,                    N_SwapCase              ),
#else
    menu__item( SHORT,              Swap_case_STR,                  "S",    0x12,   SWAPCASE,       SwapCase_fn,                    N_SwapCase              ),
#endif
    menu__item( SHORT,              Edit_expression_STR,            "X",    0x02,   EXPRESSION,     EditExpression_fn,              N_EditExpression        ),
    menu__item( LONG,               Insert_reference_STR,           "K",    0x26,   INSREF,         InsertReference_fn,             N_InsertReference       ),
    menu__item_line_short,
    menu__item( SHORT,              Split_line_STR,                 "Esl",  0x27,   SPLITLINE,      SplitLine_fn,                   N_SplitLine             ),
    menu__item( SHORT,              Join_lines_STR,                 "Ejl",  0x28,   JOINLINES,      JoinLines_fn,                   N_JoinLines             ),
    menu__item( LONG,               Insert_row_in_column_STR,       "Eirc", 0x17,   INSROWINCOL,    InsertRowInColumn_fn,           N_InsertRowInColumn     ),
    menu__item( LONG,               Delete_row_in_column_STR,       "Edrc", 0x18,   DELROWINCOL,    DeleteRowInColumn_fn,           N_DeleteRowInColumn     ),
    menu__item( SHORT,              Insert_column_STR,              "Eic",  0x23,   INSERTCOL,      InsertColumn_fn,                N_InsertColumn          ),
    menu__item( SHORT,              Delete_column_STR,              "Edc",  0,      0,              DeleteColumn_fn,                N_DeleteColumn          ),
    menu__item( SHORT,              Add_column_STR,                 "Eac",  0x29,   ADDCOL,         AddColumn_fn,                   N_AddColumn             ),
    menu__item( SHORT | HAS_DIALOG, Insert_page_STR,                "Eip",  0x24,   INSERTPAGE,     InsertPageBreak_fn,             N_InsertPageBreak       )
};


static MENU layout_ptr[] =
{
    menu__item( SHORT | HAS_DIALOG, Set_column_width_STR,           "W",    0,      0,              ColumnWidth_fn,                 N_ColumnWidth           ),
    menu__item( LONG  | HAS_DIALOG, Set_right_margin_STR,           "H",    0,      0,              RightMargin_fn,                 N_RightMargin           ),
    menu__item( SHORT,              Move_margin_left_STR,           "Lml",  0x19,   WRAPLEFT,       MoveMarginLeft_fn,              N_MoveMarginLeft        ),
    menu__item( SHORT,              Move_margin_right_STR,          "Lmr",  0x1A,   WRAPRIGHT,      MoveMarginRight_fn,             N_MoveMarginRight       ),
    menu__item( SHORT,              Fix_row_STR,                    "Lfr",  0,      0,              FixRows_fn,                     N_FixRows               ),
    menu__item( SHORT,              Fix_column_STR,                 "Lfc",  0,      0,              FixColumns_fn,                  N_FixColumns            ),
    menu__item_line_short,
    menu__item( SHORT,              Left_align_STR,                 "Lal",  0,      0,              LeftAlign_fn,                   N_LeftAlign             ),
    menu__item( SHORT,              Centre_align_STR,               "Lac",  0,      0,              CentreAlign_fn,                 N_CentreAlign           ),
    menu__item( SHORT,              Right_align_STR,                "Lar",  0,      0,              RightAlign_fn,                  N_RightAlign            ),
    menu__item( LONG,               LCR_align_STR,                  "Llcr", 0,      0,              LCRAlign_fn,                    N_LCRAlign              ),
    menu__item( SHORT,              Free_align_STR,                 "Laf",  0,      0,              FreeAlign_fn,                   N_FreeAlign             ),
    menu__item_line_short,
    menu__item( SHORT | HAS_DIALOG, Decimal_places_STR,             "Ldp",  0,      0,              DecimalPlaces_fn,               N_DecimalPlaces         ),
    menu__item( LONG,               Sign_brackets_STR,              "Lsb",  0,      0,              SignBrackets_fn,                N_SignBrackets          ),
    menu__item( LONG,               Sign_minus_STR,                 "Lsm",  0,      0,              SignMinus_fn,                   N_SignMinus             ),
    menu__item( SHORT,              Leading_characters_STR,         "Lcl",  0,      0,              LeadingCharacters_fn,           N_LeadingCharacters     ),
    menu__item( SHORT,              Trailing_characters_STR,        "Lct",  0,      0,              TrailingCharacters_fn,          N_TrailingCharacters    ),
    menu__item( LONG,               Default_format_STR,             "Ldf",  0,      0,              DefaultFormat_fn,               N_DefaultFormat         )
};


static MENU print_ptr[] =
{
    demo__item( SHORT | HAS_DIALOG, Print_STR,                      "Po",   0,      PRINTKEY,       demofunc(Print_fn),             N_Print                 ),
    menu__item( SHORT | HAS_DIALOG, Page_layout_STR,                "Py",   0,      0,              PageLayout_fn,                  N_PageLayout            ),
    demo__item( SHORT | HAS_DIALOG, Printer_configuration_STR,      "Pd",   0,      0,              demofunc(PrinterConfig_fn),     N_PrinterConfig         ),
    demo__item( LONG  | HAS_DIALOG, Microspace_pitch_STR,           "Pm",   0,      0,              demofunc(MicrospacePitch_fn),   N_MicrospacePitch       ),
    demo__item( LONG  | HAS_DIALOG, Set_parameter_STR,              "Pp",   0,      0,              demofunc(SetMacroParm_fn),      N_SetMacroParm          ),
#if RISCOS
    menu__item_line_short,
    menu__item( SHORT | HAS_DIALOG, Printer_font_STR,               "",     0,      0,              NULL,                           N_PRINTERFONT           ),
    menu__item( SHORT | HAS_DIALOG, Insert_font_STR,                "",     0,      0,              NULL,                           N_INSERTFONT            ),
    menu__item( SHORT | HAS_DIALOG, Printer_line_spacing_STR,       "",     0,      0,              NULL,                           N_PRINTERLINESPACE      ),
#endif
    menu__item_line_short,
    menu__item( SHORT,              Underline_STR,                  "Pu",   0,      0,              Underline_fn,                   N_Underline             ),
    menu__item( SHORT,              Bold_STR,                       "Pb",   0,      0,              Bold_fn,                        N_Bold                  ),
    menu__item( LONG,               Extended_sequence_STR,          "Px",   0,      0,              ExtendedSequence_fn,            N_ExtendedSequence      ),
    menu__item( SHORT,              Italic_STR,                     "Pi",   0,      0,              Italic_fn,                      N_Italic                ),
    menu__item( SHORT,              Subscript_STR,                  "Pl",   0,      0,              Subscript_fn,                   N_Subscript             ),
    menu__item( SHORT,              Superscript_STR,                "Pr",   0,      0,              Superscript_fn,                 N_Superscript           ),
    menu__item( LONG,               Alternate_font_STR,             "Pa",   0,      0,              AlternateFont_fn,               N_AlternateFont         ),
    menu__item( LONG,               User_defined_STR,               "Pe",   0,      0,              UserDefinedHigh_fn,             N_UserDefinedHigh       ),
    menu__item_line_long,
    menu__item( LONG  | HAS_DIALOG, Highlight_block_STR,            "Phb",  0,      0,              HighlightBlock_fn,              N_HighlightBlock        ),
    menu__item( LONG  | HAS_DIALOG, Remove_highlights_STR,          "Phr",  0,      0,              RemoveHighlights_fn,            N_RemoveHighlights)
};


static MENU blocks_ptr[] =
{
    menu__item( SHORT,              Mark_block_STR,                 "Z",    0x03,   MARKSLOT,       MarkSlot_fn,                    N_MarkSlot              ),
    menu__item( SHORT,              Clear_markers_STR,              "Q",    0x13,   CLEARMARKS,     ClearMarkedBlock_fn,            N_ClearMarkedBlock      ),
    menu__item_line_short,
    menu__item( LONG,               Copy_block_STR,                 "Bc",   0,      0,              CopyBlock_fn,                   N_CopyBlock             ),
    menu__item( SHORT,              Copy_block_to_paste_list_STR,   "Bf",   0,      0,              CopyBlockToPasteList_fn,        N_CopyBlockToPasteList  ),
    menu__item( LONG  | HAS_DIALOG, Size_of_paste_list_STR,         "Bpd",  0,      0,              PasteListDepth_fn,              N_PasteListDepth        ),
    menu__item( LONG,               Move_block_STR,                 "Bm",   0x11,   MOVEBLOCK,      MoveBlock_fn,                   N_MoveBlock             ),
    menu__item( SHORT,              Delete_block_STR,               "Bd",   0x21,   DELETEBLOCK,    DeleteBlock_fn,                 N_DeleteBlock           ),
    menu__item_line_short,
    menu__item( SHORT,              Replicate_down_STR,             "Brd",  0,      0,              ReplicateDown_fn,               N_ReplicateDown         ),
    menu__item( SHORT,              Replicate_right_STR,            "Brr",  0,      0,              ReplicateRight_fn,              N_ReplicateRight        ),
    menu__item( LONG  | HAS_DIALOG, Replicate_STR,                  "Bre",  0,      0,              Replicate_fn,                   N_Replicate             ),
    menu__item( SHORT | HAS_DIALOG, Sort_STR,                       "Bso",  0,      0,              SortBlock_fn,                   N_SortBlock             ),
    menu__item( SHORT | HAS_DIALOG, Search_STR,                     "Bse",  0,      0,              Search_fn,                      N_Search                ),
    menu__item( SHORT,              Next_match_STR,                 "Bnm",  0x22,   NEXTMATCH,      NextMatch_fn,                   N_NextMatch             ),
    menu__item( SHORT,              Previous_match_STR,             "Bpm",  0,      0,              PrevMatch_fn,                   N_PrevMatch             ),
    menu__item_line_long,
    menu__item( LONG,               Set_protection_STR,             "Bps",  0,      0,              SetProtectedBlock_fn,           N_SetProtectedBlock     ),
    menu__item( LONG,               Clear_protection_STR,           "Bpc",  0,      0,              ClearProtectedBlock_fn,         N_ClearProtectedBlock   ),
    menu__item_line_short,
    menu__item( LONG,               Number_X_Text_STR,              "Ent",  0,      0,              ExchangeNumbersText_fn,         N_ExchangeNumbersText   ),
    menu__item( LONG,               Snapshot_STR,                   "Bss",  0,      0,              Snapshot_fn,                    N_Snapshot              ),
    menu__item( SHORT,              Word_count_STR,                 "Bwc",  0,      0,              WordCount_fn,                   N_WordCount             ),
};


static MENU cursor_ptr[] =
{
    menu__item( SHORT,              Format_paragraph_STR,           "R",    0x2A,   FORMAT,         FormatParagraph_fn,             N_FormatParagraph   ),
    menu__item_line_short,
    menu__item( SHORT,              First_column_STR,               "Cfc",  0x05,   FIRSTCOL,       FirstColumn_fn,                 N_FirstColumn       ),
    menu__item( SHORT,              Last_column_STR,                "Clc",  0x06,   LASTCOL,        LastColumn_fn,                  N_LastColumn        ),
    menu__item( SHORT,              Next_word_STR,                  "Cnw",  0x1E,   SRIGHTCURSOR,   NextWord_fn,                    N_NextWord          ),
    menu__item( SHORT,              Previous_word_STR,              "Cpw",  0x1D,   SLEFTCURSOR,    PrevWord_fn,                    N_PrevWord          ),
    menu__item( SHORT,              Centre_window_STR,              "Cwi",  0x1C,   CENTRE_SCREEN,  CentreWindow_fn,                N_CentreWindow      ),
    menu__item_line_long,
#if !defined(SMALLPD)
    menu__item( LONG,               Save_position_STR,              "Csp",  0x1B,   SAVE_POS,       SavePosition_fn,                N_SavePosition      ),
    menu__item( LONG,               Restore_position_STR,           "Crp",  0x2B,   RESTORE_POS,    RestorePosition_fn,             N_RestorePosition   ),
    menu__item( LONG,               Swap_position_and_caret_STR,    "Cwc",  0x0B,   SWAP_POS,       SwapPosition_fn,                N_SwapPosition      ),
    menu__item( LONG  | HAS_DIALOG, Go_to_slot_STR,                 "Cgs",  0,      0,              GotoSlot_fn,                    N_GotoSlot          ),
    menu__item_line_long,
#endif
    menu__item( LONG  | HAS_DIALOG, Define_key_STR,                 "Cdk",  0,      0,              DefineKey_fn,                   N_DefineKey         ),
    menu__item( LONG  | HAS_DIALOG, Define_function_key_STR,        "Cdf",  0,      0,              DefineFunctionKey_fn,           N_DefineFunctionKey ),
    menu__item( LONG  | HAS_DIALOG, Define_command_STR,             "Cdc",  0,      0,              DefineCommand_fn,               N_DefineCommand     ),
/* need address of tickable entry */
#if !defined(SMALLPD)
#define record_option_flagp (&(cursor_ptr + 16)->flags)
#else
#define record_option_flagp (&(cursor_ptr + 11)->flags)
#endif
    menu__item( LONG  | TICKABLE,   Record_macro_file_STR,          "Fy",   0,      0,              RecordMacroFile_fn,             N_RecordMacroFile   ),
    menu__item( LONG  | HAS_DIALOG, Do_macro_file_STR,              "Fz",   0,      0,              DoMacroFile_fn,                 N_DoMacroFile       )
};


#if !defined(SPELL_OFF) || defined(DEMO)
static MENU spell_ptr[] =
{
/* need address of tickable entry */
#define check_option_flagp (&(spell_ptr + 0)->flags)
    demo__item( SHORT | TICKABLE,   Auto_check_STR,                 "Sa",   0,      0,              demofunc(AutoSpellCheck_fn),    N_AutoSpellCheck    ),
    demo__item( SHORT | HAS_DIALOG, Check_document_STR,             "Sc",   0,      0,              demofunc(CheckDocument_fn),     N_CheckDocument     ),
    demo__item( LONG  | HAS_DIALOG, Delete_word_from_dictionary_STR,"Sd",   0,      0,              demofunc(DeleteWordFromDict_fn),N_DeleteWordFromDict),
    demo__item( LONG  | HAS_DIALOG, Insert_word_in_dictionary_STR,  "Si",   0,      0,              demofunc(InsertWordInDict_fn),  N_InsertWordInDict  ),
    demo__item( LONG  | HAS_DIALOG, Display_user_dictionaries_STR,  "Se",   0,      0,              demofunc(DisplayOpenDicts_fn),  N_DisplayOpenDicts  ),
    menu__item_line_short,
    demo__item( SHORT | HAS_DIALOG, Browse_STR,                     "Sb",   0,      0,              demofunc(BrowseDictionary_fn),  N_BrowseDictionary  ),
    demo__item( LONG  | HAS_DIALOG, Dump_dictionary_STR,            "Su",   0,      0,              demofunc(DumpDictionary_fn),    N_DumpDictionary    ),
    demo__item( LONG  | HAS_DIALOG, Merge_file_with_dictionary_STR, "Sm",   0,      0,              demofunc(MergeFileWithDict_fn), N_MergeFileWithDict ),
    demo__item( SHORT | HAS_DIALOG, Anagrams_STR,                   "Sg",   0,      0,              demofunc(Anagrams_fn),          N_Anagrams          ),
    demo__item( SHORT | HAS_DIALOG, Subgrams_STR,                   "Sh",   0,      0,              demofunc(Subgrams_fn),          N_Subgrams          ),
    menu__item_line_long,
    demo__item( LONG  | HAS_DIALOG, Create_user_dictionary_STR,     "Sn",   0,      0,              demofunc(CreateUserDict_fn),    N_CreateUserDict    ),
    demo__item( LONG  | HAS_DIALOG, Open_user_dictionary_STR,       "So",   0,      0,              demofunc(OpenUserDict_fn),      N_OpenUserDict      ),
    demo__item( LONG  | HAS_DIALOG, Close_user_dictionary_STR,      "Sz",   0,      0,              demofunc(CloseUserDict_fn),     N_CloseUserDict     ),
    demo__item( LONG  | HAS_DIALOG, Pack_user_dictionary_STR,       "Sp",   0,      0,              demofunc(PackUserDict_fn),      N_PackUserDict      ),
    demo__item( LONG  | HAS_DIALOG, Lock_dictionary_STR,            "Sl",   0,      0,              demofunc(LockDictionary_fn),    N_LockDictionary    ),
    demo__item( LONG  | HAS_DIALOG, Unlock_dictionary_STR,          "Sk",   0,      0,              demofunc(UnlockDictionary_fn),  N_UnlockDictionary  )
};
#endif  /* SPELL_OFF */


/**************************************************************
*                                                             *
* the rest are commands which don't go on menus               *
* they are stored for the function and cursor key definitions *
*                                                             *
**************************************************************/

static MENU random_ptr[] =
{
    menu__item( NOMENU,             Cursor_up_STR,                  "Ccu",  0,      UPCURSOR,       CursorUp_fn,                    N_CursorUp              ),
    menu__item( NOMENU,             Cursor_down_STR,                "Ccd",  0,      DOWNCURSOR,     CursorDown_fn,                  N_CursorDown            ),
    menu__item( NOMENU,             Cursor_left_STR,                "Ccl",  0,      LEFTCURSOR,     CursorLeft_fn,                  N_CursorLeft            ),
    menu__item( NOMENU,             Cursor_right_STR,               "Ccr",  0,      RIGHTCURSOR,    CursorRight_fn,                 N_CursorRight           ),
    menu__item( NOMENU,             Start_of_slot_STR,              "Cbs",  0,      CLEFTCURSOR,    StartOfSlot_fn,                 N_StartOfSlot           ),
    menu__item( NOMENU,             End_of_slot_STR,                "Ces",  0,      CRIGHTCURSOR,   EndOfSlot_fn,                   N_EndOfSlot             ),
    menu__item( NOMENU,             Scroll_up_STR,                  "Csu",  0,      CSUPCURSOR,     ScrollUp_fn,                    N_ScrollUp              ),
    menu__item( NOMENU,             Scroll_down_STR,                "Csd",  0,      CSDOWNCURSOR,   ScrollDown_fn,                  N_ScrollDown            ),
    menu__item( NOMENU,             Scroll_left_STR,                "Csl",  0,      CSLEFTCURSOR,   ScrollLeft_fn,                  N_ScrollLeft            ),
    menu__item( NOMENU,             Scroll_right_STR,               "Csr",  0,      CSRIGHTCURSOR,  ScrollRight_fn,                 N_ScrollRight           ),
    menu__item( NOMENU,             Page_up_STR,                    "Cpu",  0,      SUPCURSOR,      PageUp_fn,                      N_PageUp                ),
    menu__item( NOMENU,             Page_down_STR,                  "Cpd",  0,      SDOWNCURSOR,    PageDown_fn,                    N_PageDown              ),
    menu__item( NOMENU,             Top_of_column_STR,              "Ctc",  0,      CUPCURSOR,      TopOfColumn_fn,                 N_TopOfColumn           ),
    menu__item( NOMENU,             Bottom_of_column_STR,           "Cbc",  0,      CDOWNCURSOR,    BottomOfColumn_fn,              N_BottomOfColumn        ),
    menu__item( NOMENU,             Previous_column_STR,            "Cpc",  0,      SHIFT_TAB,      PrevColumn_fn,                  N_PrevColumn            ),
    menu__item( NOMENU,             Next_column_STR,                "Cnc",  0,      TAB_KEY,        NextColumn_fn,                  N_NextColumn            ),
    menu__item( NOMENU,             Enter_STR,                      "Cen",  0,      CR,             Return_fn,                      N_Return                ),
    menu__item( NOMENU,             Escape_STR,                     "Cx",   0,      ESCAPE,         Escape_fn,                      N_Escape                ),
    menu__item( NOMENU,             Rubout_STR,                     "Crb",  0,      RUBOUT,         DeleteCharacterLeft_fn,         N_DeleteCharacterLeft   ),
    menu__item( NOMENU,             Pause_STR,                      "Ccp",  0,      0,              Pause_fn,                       N_Pause                 ),
    menu__item( NOMENU,             Replace_STR,                    "Brp",  0,      0,              Search_fn,                      N_Search                )
#if RISCOS
,   menu__item( NOMENU,             Next_window_STR,                "Fw",   0,      0,              NextWindow_fn,                  N_NextWindow            )
,   menu__item( NOMENU,             Close_window_STR,               "Fq",   0,      0,              CloseWindow_fn,                 N_CloseWindow           )
,   menu__item( NOMENU,             Tidy_up_STR,                    "Fgc",  0,      0,              TidyUp_fn,                      N_TidyUp                )
,   menu__item( NOMENU,             Exit_STR,                       "Fx",   0,      0,              Quit_fn,                        N_Quit                  )
,   demo__item( NOMENU,             Save_STR,                       "Fgs",  0,      0,              demofunc(SaveFileAsIs_fn),      N_SaveFileAsIs          )
,   menu__item( NOMENU,             Mark_sheet_STR,                 "Fgz",  0,      0,              MarkSheet_fn,                   N_MarkSheet             )
#endif
#if defined(DEBUG)
,   menu__item( NOMENU,             "Debug",                        "Fgd",  0,      0,              Debug_fn,                       N_Debug                 )
#endif
};


/************************************
*                                   *
* define the menu heading structure *
*                                   *
************************************/

#if RISCOS
#   define menu__head(name, l3, tail) \
        {   name, tail, TRUE, sizeof(tail)/sizeof(MENU), 0, 0,           NULL }
#elif MS || ARTHUR
#   define menu__head(name, l3, tail) \
        {   name, tail, TRUE, sizeof(tail)/sizeof(MENU), 0, 0, l3, FALSE      }
#endif


extern MENU_HEAD headline[] =
{
    /* menu head title,   key len,  submenu */
    menu__head( Files_STR,  9,      files_ptr   ),
    menu__head( Edit_STR,   9,      edit_ptr    ),
#define EDIT_MENU 1
    menu__head( Layout_STR, 10,     layout_ptr  ),
    menu__head( Print_STR,  1,      print_ptr   ),
    menu__head( Blocks_STR, 9,      blocks_ptr  ),
    menu__head( Cursor_STR, 8,      cursor_ptr  ),
#if !defined(SPELL_OFF) || defined(DEMO)
    menu__head( Spell_STR,  1,      spell_ptr   ),
#define SPELL_MENU 6
#endif
    menu__head( NULL,       0,      random_ptr  )
};
#define HEAD_SIZE ((intl) (sizeof(headline) / sizeof(MENU_HEAD) - 1))

intl head_size = HEAD_SIZE;


/************************************
*                                   *
*  size the menus (titlelen only)   *
*                                   *
************************************/

extern void
resize_menus(BOOL short_m)
{
    MENU_HEAD *firstmhptr = headline;
    MENU_HEAD *mhptr      = firstmhptr + HEAD_SIZE;
    MENU *mptr, *lastmptr;
    intl titlelen, commandlen;
    intl maxtitlelen, maxcommandlen;
    const char *ptr;
    #if RISCOS
    intl offset;

    /* not yet found */
    printer_font_mhptr = NULL;
    #endif

    do  {
        if(mhptr->installed)
            {
            mptr            = mhptr->tail;
            lastmptr        = mptr + mhptr->items;
            maxtitlelen     = 0;
            maxcommandlen   = 0;
            #if RISCOS
            offset = 1;     /* menu offsets start at 1 */
            #endif

            do  {
                ptr = mptr->title;

                /* ignore lines between menus and missing items */
                if(ptr  &&  (!short_m  ||  (mptr->flags & SHORT)))
                    {
                    titlelen    = strlen(ptr);
                    commandlen  = strlen(mptr->command);

                    #if RISCOS
                    if(mptr->funcnum == N_PRINTERFONT)
                        {
                        printer_font_mhptr  = mhptr;
                        printer_font_offset = offset;

                        assert((mptr+1)->funcnum == N_INSERTFONT);
                        assert((mptr+2)->funcnum == N_PRINTERLINESPACE);
                        }
                    #endif

                    maxtitlelen   = max(maxtitlelen, titlelen);
                    maxcommandlen = max(maxcommandlen, commandlen);

                    #if RISCOS
                    offset++;
                    #endif
                    }
                }
            while(++mptr < lastmptr);

            #if RISCOS
            mhptr->titlelen   = maxtitlelen;
            mhptr->commandlen = maxcommandlen + 1;
            #elif MS || ARTHUR
            mhptr->titlelen   = (char) (maxtitlelen + 3);   /* menu fudge */
            mhptr->commandlen = (char) maxcommandlen + 1;
            #endif
            tracef2("set titlelen for menu %s to %d\n",
                                                mhptr->name, mhptr->titlelen);
            }
        }
    while(--mhptr >= firstmhptr);
}


/********************************************************
*                                                       *
* initialise menu header line. only called at startup   *
* Determines whether SPELL menu allowed                 *
* Sizes menus dynamically too                           *
*                                                       *
********************************************************/

extern void
headline_initialise_once(void)
{
    tracef0("headline_initialise_once()\n");

    #if !defined(SPELL_OFF)  &&  !defined(SPELL_BOUND)
    headline[SPELL_MENU].installed = spell_installed;
    #endif

    resize_menus(short_menus());
}


/* *********************************************************************** */

/* read the beginning of the command buffer, find and return function number.
 * If there is a function return its number (offset in function table) otherwise
 * generate an error and return a flag indicating error
 *
 * If the lookup is successful, cbuff_offset is set to point at the first
 * parameter position.
*/

extern intl
lukucm(BOOL allow_redef)
{
    intl i = 0;

    do  {
        alt_array[i] = (uchar) toupper((int) cbuff[i+1]);
        }
    while(++i < MAX_COMM_SIZE);

    i = part_command(allow_redef);

    return((i < 0) ? NO_COMM : 0 - i);
}


/****************************************************************************
*                                                                           *
* search for an expansion of the key value in the key expansion list.       *
* If not there, and the key is possibly a function key or control char look *
* in the original definitions.  If there, copy into an array, adding \,\n   *
*                                                                           *
****************************************************************************/

extern BOOL
schkvl(intl c)
{
    LIST *ptr = search_list(&first_key, (word32) c);
    MENU_HEAD *firstmhptr, *mhptr;
    MENU *firstmptr, *mptr;

    if(ptr)
        keyidx = ptr->value;
    elif((c != 0)  &&  ((c < (intl) SPACE)  ||  (c >= (intl) 127)))
        {
        firstmhptr  = headline;
        mhptr       = firstmhptr + HEAD_SIZE + 1;   /* check random_ptr too */

        tracef0("[schkvl searching menu definitions]\n");

        while(--mhptr >= firstmhptr)
            if(mhptr->installed)
                {
                firstmptr   = mhptr->tail;
                mptr        = firstmptr + mhptr->items;

                while(--mptr >= firstmptr)
                    if(mptr->key == c)
                        {
                        /* expand original definition into buffer
                         * adding \ and \n
                        */
                        keyidx = expanded_key_buff;
                        strcat(strcpy(keyidx + 1, mptr->command), CR_STR);
                        tracef1("[schkvl found: %s]\n", trace_string(keyidx));
                        return(TRUE);
                        }
                }
        }

    if(!*keyidx)
        keyidx = NULL;

    #if TRACE
    if(keyidx)
        tracef1("[schkvl found: %s]\n", trace_string(keyidx));
    #endif

    return(keyidx != NULL);
}


/*******************************************************************
*                                                                  *
* scan all the menus to see if what we have is a part of a command *
* if it is return TRUE                                             *
* if it is a whole command, do it and return FALSE                 *
* if it is an invalid sequence, return FALSE                       *
*                                                                  *
*******************************************************************/

static intl
part_command_core(void)
{
    MENU_HEAD *firstmhptr   = headline;
    MENU_HEAD *mhptr        = firstmhptr + HEAD_SIZE + 1;   /* check random_ptr too */
    MENU *firstmptr, *mptr;
    const char *aptr, *bptr;
    char a, b;

    tracef1("look up alt_array %s in menu structures\n", alt_array);

    while(--mhptr >= firstmhptr)
        if(mhptr->installed)
            {
            firstmptr   = mhptr->tail;
            mptr        = firstmptr + mhptr->items;

            while(--mptr >= firstmptr)
                {
                aptr = alt_array;
                bptr = mptr->command;

                if(bptr)
                    do  {
                        a = *aptr++;
                        b = *bptr++;

                        if(!a  ||  (a == SPACE))
                            {
                            if(!b)
                                {
                                tracef0("found whole command\n");
                                *alt_array = '\0';
                                return(mptr->funcnum);
                                }

                            tracef0("found partial command\n");
                            return(IS_SUBSET);
                            }
                        }
                    while(a == toupper(b));
                }
            }

    tracef0("invalid sequence - discarding\n");
    *alt_array = '\0';
    #if !defined(HEADLINE_OFF)
    xf_drawmenuheadline = TRUE;
    #endif
    bleep();

    return(IS_ERROR);
}


static intl
part_command(BOOL allow_redef)
{
    const char *aptr, *bptr;
    char a, b;
    LIST *lptr;

    if(allow_redef)
        {
        tracef1("look up alt_array %s on redefinition list\n", alt_array);

        for(lptr = first_in_list(&first_command_redef);
            lptr;
            lptr = next_in_list(&first_command_redef))
            {
            aptr = alt_array;
            bptr = lptr->value;

            tracef2("comparing alt_array %s with redef %s\n", aptr, bptr);

            if(bptr)
                do  {
                    a = *aptr++;    /* alt_array always UC */
                    b = *bptr++;    /* def'n & redef'n always UC */

                    if(!a  ||  (a == SPACE))
                        {
                        if(b == '_')
                            {
                            tracef0("found full redefined command - look it up\n");
                            strcpy(alt_array, bptr);
                            return(part_command_core());
                            }

                        tracef0("found partial redefined command\n");
                        return(IS_SUBSET);
                        }
                    }
                while(a == b);
            }
        }

    return(part_command_core());
}


extern BOOL
do_command(intl key, BOOL spool)
{
    MENU_HEAD *firstmhptr = headline;
    MENU_HEAD *mhptr      = firstmhptr + HEAD_SIZE + 1;
    MENU *firstmptr, *mptr;
    BOOL to_comm_file;

    tracef1("do_command(&%p)\n", key);

    if(key == 0)
        return(FALSE);

    while(--mhptr >= firstmhptr)
        if(mhptr->installed)
            {
            firstmptr   = mhptr->tail;
            mptr        = firstmptr + mhptr->items;

            while(--mptr >= firstmptr)
                {
                vtracef1(FALSE, "mptr->funcnum = &%p\n", mptr->funcnum);

                if(mptr->funcnum == key)
                    {
                    to_comm_file = 
                        (spool && macro_recorder_on && (key != N_RecordMacroFile));

                    if(to_comm_file)
                        out_comm_start_to_macro_file(mptr);

                    (*(mptr->cmdfunc))();

                    /* only output command if not Ctrl-FY */
                    if(to_comm_file)
                        out_comm_end_to_macro_file(mptr);

                    return(TRUE);
                    }
                }
            }

    return(FALSE);
}


/********************************************************************
*                                                                   *
* alt_letter takes a 16-bit character (eg from rdch()) and if it is *
* an Alt-letter returns the upper case letter. Otherwise returns 0  *
*                                                                   *
********************************************************************/

#if RISCOS

static uchar
alt_letter(intl c)
{
    if((c <= 0)  ||  ((c & ALT_ADDED) == 0))
        return('\0');
    else
        return((uchar) (c & 0xFF));
}


#elif ARTHUR

#define ALTA -0xC0
#define ALTJ -0xC9
#define ALTK -0xCE
#define ALTV -0xD9
#define ALTW -0xDE
#define ALTX -0xDF
#define ALTY -0xEE
#define ALTZ -0xEF

static uchar
alt_letter(intl c)
{
    if((c <= ALTA)  &&  (c >= ALTJ))
        return((uchar) ((ALTA - c) + 'A')); /* A..J */

    if((c <= ALTK)  &&  (c >= ALTV()
        return((uchar) ((ALTK - c) + 'K')); /* K..V */

    switch(c)
        {
        case ALTW:
            return('W');

        case ALTX:
            return('X');

        case ALTY:
            return('Y');

        case ALTZ:
            return('Z');

        default:
            return('\0');
        }
}


#elif MS

static const uchar first_line[]  = "POIUYTREWQ";    /* qwertyuiop sdrawkcab */
static const uchar second_line[] = "LKJHGFDSA";
static const uchar third_line[]  = "MNBVCXZ";

#define ALTQ -16
#define ALTP -25
#define ALTA -30
#define ALTL -38
#define ALTZ -44
#define ALTM -50

static uchar
alt_letter(intl c)
{
    if((c <= ALTQ)  &&  (c >= ALTP))
        return(first_line[c - ALTP]);

    if((c <= ALTA)  &&  (c >= ALTL))
        return(second_line[c - ALTL]);

    if((c <= ALTZ)  &&  (c >= ALTM))
        return(third_line[c - ALTM]);

    return('\0');
}

#endif


#if RISCOS

/************************************
*                                   *
* get a menu item printed into core *
* returns TRUE if submenu possible  *
*                                   *
************************************/

extern BOOL
get_menu_item(MENU_HEAD *header, MENU *mptr, char *array /*out*/)
{
    char *out = array;
    const char *ptr;
    char ch;

    /* copy item title to array */
    strcpy(out, mptr->title);
    out += strlen(out);

    /* pad title field with spaces */
    tracef2("out - array = %d, header->titlelen = %d\n",
                out - array, header->titlelen);
#if defined(SG_MENU_ITEMS)
    if(wimptx_os_version_query() < RISC_OS_3_5)
        while((out - array) < header->titlelen) /* was <= but below code forces another space */
            *out++ = ' ';
#else
    while((out - array) <= header->titlelen)
        *out++ = ' ';
#endif

    /* copy item command sequence to array */
#if defined(SG_MENU_ITEMS)
    if(mptr->command[0])
    {
        *out++ = ' ';
        *out++ = '^';
    }
#endif

    ptr = mptr->command;
    while((ch = *ptr++) != '\0')
        *out++ = toupper(ch);

    if(short_menus()  ||  ((ch = mptr->func_key) == 0))
        *out = '\0';
    else
        {
        /* pad command field with spaces */
        tracef3("F%3X, out - array = %d, header->lengths = %d\n",
                    ch, out - array, header->titlelen + header->commandlen);
#if defined(SG_MENU_ITEMS)
        if(wimptx_os_version_query() < RISC_OS_3_5)
        {
            while((out - array) <= (header->titlelen + header->commandlen))
                *out++ = ' ';
        }
        else
        {
            *out++ = '\xA0'; /* SKS NBSP to glue them together at the RHS */
            *out++ = '\xA0';
        }
#else
        while((out - array) <= (header->titlelen + header->commandlen))
            *out++ = ' ';
#endif

        /* copy function key used to array */
        if(ch & 0x20)
            {
            strcpy(out, Ctrl__STR);
            out += strlen(out);
            }

        if(ch & 0x10)
            {
            strcpy(out, Shift__STR);
            out += strlen(out);
            }

        ch = ch & 0x0F;

        switch(ch)
            {
            case 0x0D:  /* left arrow: NB. not related to real values */
            case 0x0E:  /* right arrow */
                *out++ = 0x88 + ch - 0x0D;
                *out   = '\0';
                break;

            default:
                sprintf(out, FZd_STR, (intl) ch);
                break;
            }
        }

    return((BOOL) mptr->flags & HAS_DIALOG);
}


#elif MS || ARTHUR

/********************************
*                               *
* draw a single item on a menu  *
*                               *
********************************/

static void
draw_item(coord xpos, coord ypos, MENU_HEAD *header, MENU *mptr,
          BOOL inverse, coord menuwidth)
{
    const char *ptr;
    char ch;

    setcolour(MENU_HOT, MENU_BACK);
    at(xpos, ypos);
    wrch_funny(VERTBAR);

    if(!mptr->title)
        {
        draw_bar(xpos, ypos, menuwidth);
        return;
        }

    setcolour(  inverse ? FORE     : MENU_FORE,
                inverse ? MENU_HOT : MENU_BACK );

    wrchrep(SPACE, menuwidth-2);


    if(mptr->flags & TICK_STATUS)
        {
        at(xpos+1, ypos);
#define TICK 251
        wrch(TICK);
        }

    at(xpos+2, ypos);

    /* write out the prompt */

    for(ptr = mptr->title; *ptr; ptr++)
        wrch(*ptr);

    at(xpos + header->titlelen, ypos); /* Note the +2 is in the data */

    for(ptr = mptr->command; *ptr; ptr++)
        if(islower(*ptr))
            {
            if(!inverse)
                setcolour(MENU_HOT, MENU_BACK);
            wrch(toupper(*ptr));
            if(!inverse)
                setcolour(MENU_FORE, MENU_BACK);
            }
        else
            wrch(*ptr);

    /* func keys might be Shift or Ctrl */

    if((ch = mptr->func_key) > 0)
        {
        at(xpos + header->titlelen + header->commandlen, ypos);  /* Ditto */

        if(ch & 0x20)
            stringout(Ctrl__STR);

        if(ch & 0x10)
            stringout(Shift__STR);

        ch = ch & 0x0F;

        if(ch <= 10)
            {
            char array[16];
            sprintf(array, FZd_STR, (intl) ch);
            stringout(array);
            }
        else
/* cursor key icons are ordered up, down, left, right from CURSOR_KEY_BASE */
#if MS
#define CURSOR_KEY_BASE 24
#elif ARTHUR
#define CURSOR_KEY_BASE 128
#endif
            wrch((uchar) (CURSOR_KEY_BASE - 11) + ch);
        }

    setcolour(MENU_HOT, MENU_BACK);
    at(xpos + header->titlelen + header->commandlen + header->keylen - 1, ypos);
    wrch_funny(VERTBAR);
}


static BOOL
menu_item_available(MENU *mptr)
{
    if(short_menus()  &&  (mptr->flags & LONG))
        return(FALSE);

    return(TRUE);
}



static coord
count_items_in_menu(MENU *mptr, coord max)
{
    coord i, count;

    for(i = 0, count = 0; i < max; i++, mptr++)
        if(short_menus())
            {
            if(mptr->flags & SHORT)
                count++;
            }
        else
            count++;

    return(count);
}


/********************************************
*                                           *
* activate a menu                           *
* array has part of a command in it         *
* which we are going to build up or reject  *
*                                           *
********************************************/

static intl
do_menu(void)
{
    coord xpos;
    intl ch = 0;
    MENU *cmenu;
    intl idx;
    intl first = 0;

    init_screen_array();

    for(idx = 0; idx < HEAD_SIZE; idx++)
        if(headline[idx].installed)
            {
            headline[idx].beenhere = FALSE;
            if(*alt_array == *headline[idx].name)
                first = idx;
            }

    for(idx = first;;)
        {
        coord i;
        coord current_field = 0;
        coord items_on_screen;
        coord items_in_full_menu;
        coord this_item_pos;
        coord menuwidth;
        MENU *thismenu;
        MENU_HEAD *header;

        /* find the menu */

        if(idx >= HEAD_SIZE)
            {
            idx = 0;
            while(!headline[idx].installed)
                idx++;
            }
        elif(idx < 0)
            {
            idx = HEAD_SIZE-1;
            while(!headline[idx].installed)
                idx--;
            }

        /* find the offset of the menu */

        for(i = 0, xpos = 0; i < idx; i++)
            xpos += 2 + strlen(headline[i].name);

        /* display the heading to pick out the menu */

        display_heading(idx);

        /* display the menu itself */

        header = headline + idx;
        menuwidth = (uchar) (header->titlelen + header->commandlen + header->keylen);
        thismenu = header->tail;
        items_in_full_menu = header->items;
        items_on_screen = count_items_in_menu(thismenu, items_in_full_menu);
        this_item_pos = 0;

        /* draw the menu */
        {
        MENU *mptr;

#if MS
        if(fastdraw)
            clip_menu(xpos, 1, menuwidth, 2+items_on_screen);
        else
            if(!header->beenhere)
                save_screen(xpos, 1, menuwidth, 2+items_on_screen);
#elif ARTHUR
        clip_menu(xpos, 1, menuwidth, 2+items_on_screen);
#endif

        setcolour(MENU_HOT, MENU_BACK);
        topline(xpos, 1, menuwidth);

        at(xpos, 1);
        wrch_funny(DROP_LEFT);

        at(xpos + this_heading_length + 1, 1);
        wrch_funny(DROP_MIDDLE);

        at(xpos,0);
        wrch_funny(VERTBAR);

        at(xpos + this_heading_length + 1, 0);
        wrch(VERTBAR);          /* ^^^ already defined */

        for(mptr=thismenu, i = 0; i < items_on_screen; i++)
            {
            while(!menu_item_available(mptr))
                mptr++;

            draw_item(xpos, i+2, header, mptr++, (i==current_field), menuwidth);
            }

        bottomline(xpos, i+2, menuwidth);
        }

        header->beenhere = TRUE;

        /* clip screen around box */
#if MS
        if(!fastdraw)
            clip_menu(xpos, 1, menuwidth, 2 + items_on_screen);
#endif

        clearkeyboardbuffer();


        /* move around until RETURN or ESCAPE */

        while(ch != ESCAPE)
            {
            coord oldfield = current_field;
            coord old_item_pos = this_item_pos;
            intl oldidx = idx;
            intl retval = IS_SUBSET;
            BOOL alt_pressed = FALSE;

            cmenu = thismenu + current_field;

            sb_show_if_fastdraw();

            /* look for Alt appearing and disappearing */
            while(!keyinbuffer())
                {
                BOOL isdown = depressed(ALT);

                if(isdown)
                    alt_pressed = TRUE;

                if(ctrlflag || (alt_pressed && !isdown))
                    {
                    ack_esc();
                    ch = ESCAPE;
                    break;
                    }
                }

            if(ch != ESCAPE)
                ch = rdch(FALSE, TRUE);

            switch(ch)
                {
                intl oldch, array_len;

                case UPCURSOR:
                    for(;;)
                        {
                        if(--current_field < 0)
                            {
                            this_item_pos = items_on_screen;
                            current_field = items_in_full_menu-1;                           
                            }

                        if(!thismenu[current_field].title)
                            {
                            this_item_pos--;
                            continue;
                            }

                        if(menu_item_available(thismenu+current_field))
                            break;
                        }

                    this_item_pos--;
                    break;

                case DOWNCURSOR:
                    for(;;)
                        {
                        if(++current_field >= items_in_full_menu)
                            {
                            this_item_pos = -1;
                            current_field = 0;                          
                            }

                        if(!thismenu[current_field].title)
                            {
                            this_item_pos++;
                            continue;
                            }

                        if(menu_item_available(thismenu+current_field))
                            break;
                        }

                    this_item_pos++;
                    break;

                case LEFTCURSOR:
                    while(!headline[--idx].installed  &&  (idx >= 0))
                        ;
                    break;

                case RIGHTCURSOR:
                    while(!headline[++idx].installed  &&  (idx <= HEAD_SIZE))
                        ;
                    break;

                case CR:
                    retval = cmenu->funcnum;
                    break;

                case ESCAPE:
                    clip_menu(0,1,0,0);
                    break;

                default:
                    oldch = ch;
                    if((ch = alt_letter(ch)) == 0)
                        ch = oldch;
                    ch = toupper(ch);
                    array_len = strlen((char *) alt_array);
                    alt_array[array_len++] = (uchar) ch;
                    alt_array[array_len] = '\0';
                    xf_drawmenuheadline = TRUE;
                    retval = part_command(FALSE);
                    break;
                }

            if(retval != IS_SUBSET || ch == ESCAPE)
                {
                clip_menu(0,1,0,0);
                setcolour(FORE, BACK);
                if(retval == IS_ERROR  || ch == ESCAPE)
                    retval = 0;

                *alt_array = '\0';
                display_heading(-1);
                return(0-retval);
                }

            display_heading(idx);

            /* is it new menu */

            if(idx != oldidx)
                break;

            if(oldfield != current_field)
                {
                draw_item(xpos, old_item_pos+2,      header, header->tail+oldfield,      FALSE, menuwidth);
                draw_item(xpos, this_item_pos+2, header, header->tail+current_field, TRUE,  menuwidth);
                }
            }
        }
}

#endif


#if RISCOS

/************************************************
*                                               *
* --in--                                        *
*   +ve     char, maybe Alt                     *
*   0       to be ignored                       *
*   -ve     command number                      *
*                                               *
* --out--                                       *
*   +ve     char                                *
*   0       to be ignored, or error happened    *
*                                               *
************************************************/

static intl
fiddle_alt_riscos(intl ch)
{
    intl retval;
    uchar *str  = alt_array + strlen((char *) alt_array);
    uchar kh = alt_letter(ch);

    tracef2("[fiddle_alt_riscos alt_letter: %d, %c]\n",
            kh, kh);

    if(!kh)
        {
        if(str != alt_array)
            {
            bleep();
            *alt_array = '\0';
            return('\0');
            }
        else
            return(ch);
        }

    *str++  = kh;
    *str    = '\0';
    tracef1("alt_array := \"%s\"\n", alt_array);

    retval = part_command(TRUE);

    return((retval < 0) ? 0 : 0 - retval);  /* ERROR or SUBSET */
}


#elif MS || ARTHUR

/********************************************
*                                           *
* fiddle_with_menus gets called from inpchr *
* to deal with possible ALT keys            *
*                                           *
********************************************/

extern intl
fiddle_with_menus(intl c, BOOL alt_detected)
{
    intl index = 0;
    BOOL first_time = TRUE;

    if(!alt_detected  ||  in_dialog_box)
        do  {
            intl retval;

            /* if in a dialog box throw away any ALT letters */

            if(in_dialog_box)
                xf_drawmenuheadline = TRUE;

            if((alt_array[index] = alt_letter(c)) == '\0')
                {
                if(first_time)
                    return(c);

                *alt_array = '\0';
                display_heading(-1);
                bleep();
                return('\0');
                }

            alt_array[++index] = '\0';
            first_time = FALSE;

            /* test if it is
             * (a) a command
             * (b) a part of a command or
             * (c) an invalid sequence
            */
            if((retval = part_command(TRUE)) == IS_ERROR)
                return(0);

            display_heading(-1);

            if(retval != IS_SUBSET)
                {
                /* we have a whole command. Check it is allowed */
                if(in_dialog_box && !allowed_in_dialog(retval))
                    {
                    bleep();
                    return(0);
                    }

                return(0-retval);
                }

            /* if menus not allowed (cos in dialog box) continue round loop
             * until command found or error, and don't call do_menu()
            */
            while(depressed(ALT) || in_dialog_box)
                if(keyinbuffer())
                    {
                    c = rdch(FALSE, TRUE);
                    break;
                    }

            }   while(depressed(ALT) || in_dialog_box);

    return(do_menu());
}


/****************************************
*                                       *
*  draw a bar in a menu or help screen  *
*                                       *
****************************************/

extern void
draw_bar(coord xpos, coord ypos, coord length)
{
    at(xpos, ypos);
    wrch_funny(DROP_LEFT);

    wrch_definefunny(HORIZBAR);
    wrchrep(HORIZBAR, length-2);

    wrch_funny(DROP_RIGHT);
}

#endif



#if !defined(SPELL_OFF)

/************************************
*                                   *   
* switch auto check tick on or off  *
*                                   *
************************************/

extern void
check_state_changed(void)
{
    optiontype opt = *check_option_flagp;
    tracef1("check_option_flagp = &%p\n", check_option_flagp);
    *check_option_flagp = (d_auto[0].option == 'Y')
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;
}

#endif  /* SPELL_OFF */


/****************************************
*                                       *
* switch insert/overtype tick on or off *
*                                       *
****************************************/

extern void
insert_state_changed(void)
{
    optiontype opt = *insert_option_flagp;
    tracef1("insert_option_flagp = &%p\n", insert_option_flagp);
    *insert_option_flagp = xf_iovbit
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;
}


/****************************************
*                                       *
* switch macro recorder tick on or off  *
*                                       *
****************************************/

static void
record_state_changed(void)
{
    optiontype opt = *record_option_flagp;
    tracef1("record_option_flagp = &%p\n", record_option_flagp);
    *record_option_flagp = macro_recorder_on
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;
}


/************************************
*                                   *
* switch short menu tick on or off  *
*                                   *
************************************/

extern void
menu_state_changed(void)
{
    BOOL short_m;
    optiontype opt = *menu_option_flagp;
    tracef1("menu_option_flagp = &%p\n", menu_option_flagp);
    *menu_option_flagp = d_menu[0].option
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;

    short_m = short_menus();
    resize_menus(short_m);
    #if RISCOS
    riscmenu_buildmenutree(short_m);
    #endif
}


extern void
MenuSize_fn(void)
{
    d_menu[0].option = (optiontype) !d_menu[0].option;

    menu_state_changed();

    #if RISCOS
    riscmenu_clearmenutree();
    #endif
}


static FILE       *macro_recorder_file = NULL;
static void       *macro_buffer = NULL;
static BOOL        macro_needs_lf = FALSE;
static const char *macro_command_name = NULL;
static BOOL        macro_fillin_failed = FALSE;

/* output string to macro file */

static void
output_string_to_macro_file(char *str)
{
    if(!away_string(str, macro_recorder_file))
        {
        reperr(ERR_CANNOTWRITE, d_macro_file[0].textfield);
        myfclose(macro_recorder_file);
        macro_recorder_file = NULL;
        macro_recorder_on = FALSE;
        }
}


extern void
output_char_to_macro_file(uchar ch)
{
    uchar array[3];
    uchar *ptr = array;

    if((ch == QUOTES)  ||  (ch == '|'))
        *ptr++ = '|';

    *ptr++ = ch;
    *ptr = '\0';

    output_string_to_macro_file(array);

    macro_needs_lf = TRUE;
}


/* output the command string to the macro file */

extern void
out_comm_start_to_macro_file(MENU *mptr)
{
    macro_command_name = mptr->command;

    macro_fillin_failed = FALSE;
}


static intl
really_out_comm_start(void)
{
    uchar array[LIN_BUFSIZ];

    if(macro_needs_lf)
        {
        macro_needs_lf = FALSE;
        output_string_to_macro_file(CR_STR);
        }

    if(macro_command_name)
        {
        sprintf(array, "\\%s", macro_command_name);
        macro_command_name = NULL;
        output_string_to_macro_file(array);
        return(strlen(array));
        }

    return(0);
}


/* output the command string to the macro file */

extern void
out_comm_end_to_macro_file(MENU *mptr)
{
    if(!macro_fillin_failed)
        {
        uchar array[LIN_BUFSIZ];
        intl sofar = really_out_comm_start();
                    /* 0123456789 */
        strcpy(array, "|m        ");
        strcpy(array + 10 - sofar, mptr->title);
        strcat(array, CR_STR);
    
        output_string_to_macro_file(array);

        macro_needs_lf = FALSE;
        }
}


/* output the dialog box parameters to the macro file */

extern void
out_comm_parms_to_macro_file(DIALOG *dptr, intl size, BOOL ok)
{
    uchar array[LIN_BUFSIZ];
    DIALOG *dptri;
    intl i;

    if(!ok)
        {
        macro_fillin_failed = TRUE;
        macro_needs_lf = FALSE;
        return;
        }

    really_out_comm_start();
    
    for(i = 0, dptri = dptr; i < size; i++, dptri++)    
        {
        uchar *ptr = array;
        uchar *textptr = dptri->textfield;
        BOOL charout = FALSE;
        BOOL numout = FALSE;
        BOOL textout = FALSE;
        BOOL arrayout = FALSE;

        switch(dptr[i].type)
            {
            case F_ERRORTYPE:
                return;

            case F_COMPOSITE:
                charout = textout = TRUE;
                break;

            case F_ARRAY:
                arrayout = TRUE;                    
                break;

            case F_LIST:
            case F_NUMBER:
            case F_COLOUR:
                numout = TRUE;
                break;

            case F_SPECIAL:
            case F_CHAR:
                charout = TRUE;
                break;

            case F_TEXT:
                textout = TRUE;
                break;

            default:
#ifdef ROB
                        printf("error in out_comm_parms...()");
                        bleep();
                        exit(5);
#endif
                        break;                      
            }

        if(charout || numout)
            ptr += sprintf(ptr, charout ? "|i %c " : "|i %d ", (int) dptri->option);

        if(textout || arrayout)
            {
            /* expand string in, putting | before " */
            uchar *fptr;

            strcpy(ptr, "|i \"");
            ptr += strlen(ptr);
            if(textout)
                fptr = !textptr ? UNULLSTR : textptr;
            else
                fptr = *((char **) dptri->optionlist + dptri->option);

            while(*fptr)
                {
                if(*fptr == QUOTES)
                    *ptr++ = '|';
                *ptr++ = *fptr++;
                }

            strcpy(ptr, "\" ");
            }

        output_string_to_macro_file(array);
        }
}


/*
switch macro recording on and off
when switching on, get filename from dialog box and stick tick on menu item
when switching off remove tick from menu item
*/

static char *macro_file_name = NULL;

extern void
RecordMacroFile_fn(void)
{
    char buffer[MAX_FILENAME];
    char *name;
    intl res;

    macro_needs_lf = FALSE;

    if(!macro_recorder_on)
        {
        while(dialog_box(D_MACRO_FILE))
            {
            name = d_macro_file[0].textfield;

            if(str_isblank(name))
                {
                reperr_null(ERR_BAD_NAME);
                been_error = FALSE;
                (void) init_dialog_box(D_MACRO_FILE);
                continue;
                }

            (void) add_prefix_to_name(buffer, name, TRUE);

            if(!str_set(&macro_file_name, buffer))
                continue;

            macro_recorder_file = myfopen(buffer, write_str);
            if(!macro_recorder_file)
                {
                reperr(ERR_CANNOTOPEN, name);
                continue;
                }

#define MACROFILE_BUFSIZ 256

            macro_buffer = alloc_ptr_using_cache(MACROFILE_BUFSIZ, &res);

            if(res < 0)
                {
                myfclose(macro_recorder_file);
                macro_recorder_file = NULL;
                reperr_null(res);
                continue;
                }

            mysetvbuf(macro_recorder_file, macro_buffer, MACROFILE_BUFSIZ);

            dialog_box_end();

            macro_recorder_on = TRUE;

            if(dialog_box_ended())
                break;
            }
        }
    else
        {
        name = macro_file_name;

        if(myfclose(macro_recorder_file))
            {
            str_clr(&d_execfile[0].textfield);
            reperr(ERR_CANNOTCLOSE, name);
            }
        else
            {
            (void) str_set(&d_execfile[0].textfield, d_macro_file[0].textfield);
            #if RISCOS
            stampfile(name, PDMACRO_FILETYPE); 
            #endif
            }

        str_clr(&macro_file_name);

        macro_recorder_file = NULL;

        dispose((void **) &macro_buffer);

        macro_recorder_on   = FALSE;
        }

    record_state_changed();
}

/* end of commlin.c */
