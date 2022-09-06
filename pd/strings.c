/* strings.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       strings.c - strings from PipeDream
 * Author:      Stuart K. Swales 24-Feb-1989
*/

/* standard header files */
#include "flags.h"


/* header file */
#include "strings.h"


/* ----------------------------------------------------------------------- */

#if !defined(CODE_FOR_STRINGS)
/* statically defined string variable */
#define string(id, value)   const char id[] = value

#elif !defined(CODE_FOR_STRINGS_NOW)
/* uninitialised string variable */
#define string(id, value)   const char *id

#else
/* string variable initialiser */
#define string(id, value)   id = (const char *) value
#endif


#if defined(CODE_FOR_STRINGS_NOW)

extern void
strings_init(void)
{

#endif

/* what set of countries shall we work with? */

string(DefaultLocale_STR,   "ISO8859-1");
string(Unrecognised_arg_Zs_STR,         "Unrecognised arg -%s");

#if ARTHUR || RISCOS
string(WILD_STR,                        "*");
#elif MS
string(WILD_STR,                        "*.*");
#endif

string(FORESLASH_STR,                   "/");
string(ONE_DOT_STR,                     ".");
string(TWO_DOT_STR,                     "..");

#if MS
string(BACKSLASH_STR,                   "\\");
#endif


#if !defined(Z88_OFF)
string(Z88_STR,                         "Z88");
string(Z88COLON_STR,                    "Z88:");
#endif


/* names of files used by PipeDream */

#if ARTHUR || RISCOS

#if !defined(LOTUS_OFF)
string(temp_file,                       "ltstemp");
#endif
#if !defined(SPELL_OFF)
string(MASTERDICT_STR,                  "dct");
#endif
#if defined(HELP_FILE)
string(HELPFILE_STR,                    "!Help");
#endif
string(INITEXEC_STR,                    "key");
string(INITFILE_STR,                    "ini");
#if RISCOS
string(DUMPFILE_STR,                    "DumpedFile");
string(PD_INITFILE_STR,                 "<PipeDrea3$Dir>.ini");
string(PD_IMAGEFILE_STR,                "<PipeDrea3$Dir>.!RunImage");
string(PD_PATH_STR,                     "PipeDrea3$Path");
string(ZS_MACROFILE_STR,                "%sMacroFile");
string(USERDICT_STR,                    "UserDict");
string(ZS_LINKFILE_ZD_STR,              "%slnk%d");
#elif ARTHUR
string(ZS_LINKFILE_ZD_STR,              "%spiped.lnk%d");
#endif
string(UNTITLED_ZD_STR,                 "Untitled%d");
string(ambiguous_tag_STR,               "$$$AmbiguousTag");

#elif MS

#if !defined(LOTUS_OFF)
string(temp_file,                       "temp.pd");
#endif
#if !defined(SPELL_OFF)
string(MASTERDICT_STR,                  "pd.dct");
#endif
#if defined(HELP_FILE)
string(HELPFILE_STR,                    "pd.hlp");
#endif
string(DUMPFILE_STR,                    "user.dmp");
string(INITEXEC_STR,                    "pd.key");
string(INITFILE_STR,                    "pd.ini");
string(PD_PATH_STR,                     "PATH");
string(ZS_MACROFILE_STR,                "%suser.mac");
string(USERDICT_STR,                    "user.dct");
string(ZS_LINKFILE_ZD_STR,              "%spd%d.lnk");
string(UNTITLED_ZD_STR,                 "NoName%d");
string(serial_port_STR,                 "serial port");

#endif


#if defined(SG_MENU_ITEMS)
string(Ctrl__STR,                       "^");
string(Shift__STR,                      "\x8B"); /* up arrow */
#else
string(Ctrl__STR,                       "Ctrl ");
string(Shift__STR,                      "Shift ");
#endif
string(PRESSANYKEY_STR,                 "Press any key");
string(Initialising_STR,                "Initialising...");


/* for registration numbers & checking */

#if RISCOS
string(program_invalid_STR,             "PipeDream has been corrupted");
#elif MS || ARTHUR
string(run_the_install_program_STR,     "invalid - run the install program");
#endif

#if MS
string(ZsNL_STR,                        "%s\n");
#elif ARTHUR
string(AZsNL_STR,                       "A%s\n");
#endif

#if MS || ARTHUR
string(NLZsNLZsNLNL_STR,                "\n%s\n%s\n\n");
#endif


/* date format strings */

string(Zd_Zs_Zd_STR,                    "%d %s %d");
string(Zd_ZP3s_Zd_STR,                  "%d %.3s %d");

string(FZd_STR,                         "F%d");
string(New_width_STR,                   "New width");
string(New_right_margin_position_STR,   "New right margin position");
string(Zld_Zs_found_STR,                "%ld %s found");
string(Zld_hidden_STR,                  "%ld hidden");
string(file_STR,                        "file");
string(files_STR,                       "files");
string(word_STR,                        "word");
string(words_STR,                       "words");
string(string_STR,                      "string");
string(strings_STR,                     "strings");
string(Zd_unrecognised_Zs_STR,          "%d unrecognised %s");
string(Zd_Zs_added_to_user_dict_STR,    "%d %s added to user dictionary");
string(Zs_of_Zs_STR,                    "%s of %s");
string(Zs_complete_STR,                 "... %s complete ...");

string(Page_Zd_wait_between_pages_STR,  "Page %d.. Press M=Miss page, A=All pages, "                "ENTER=Print this page, ESC=stop print");

#if MS
string(screen_Zd_of_Zd_STR,     " (screen %d of %d)");
#endif

#if RISCOS
string(save_edited_file_Zs_STR,
    "Do you want to save edited file '%s'?");

string(Zd_Zs_edited_are_you_sure_Zs_STR,
    "%d %s edited but not saved in %s: are you sure you want to Quit?");
#endif

string(close_dependent_files_winge_STR,
    "This file has dependent documents. Do you want to close it?");

string(close_dependent_links_winge_STR,
    "This file has dependent links. Do you want to close it?");

string(load_supporting_winge_STR,
    "File '%s' has the same leafname as another"            \
    " supporting document. Do you want to load it?");

string(name_supporting_winge_STR,
    "This file would be given the same leafname as another" \
    " supporting document. Do you want to rename it?");


/* -------------------------- dialog boxes ------------------------------- */

string(DecimalPlaces_Parm_STR,  "23456789F01");


#if MS || ARTHUR
string(NAME_OF_FILE_STR,        "Name of file");
#endif


#if RISCOS

/* Titles to be poked into dialog boxes */

/*  string(Create_user_dictionary_STR,          "Create user dictionary");  */
/*  string(Open_user_dictionary_STR,            "Open user dictionary");    */
/*  string(Close_user_dictionary_STR,           "Close user dictionary");   */
/*  string(Lock_dictionary_STR,                 "Lock dictionary");     */
/*  string(Unlock_dictionary_STR,               "Unlock dictionary");   */
string(Dump_STR,                                "Dump");
string(Dumping_STR,                             "Dumping");
string(Merge_STR,                               "Merge");
string(Merging_STR,                             "Merging");
string(Anagrams_STR,                            "Anagrams");
string(Subgrams_STR,                            "Subgrams");
string(Opened_user_dictionaries_STR,            "Opened user dictionaries");
string(Insert_word_in_user_dictionary_STR,      "Insert word in user dictionary");
string(Delete_word_from_user_dictionary_STR,    "Delete word from user dictionary");
string(Insert_highlights_STR,                   "Insert highlights");


/* Messages to be used for dboxqueries */

string(Overwrite_existing_file_STR,             "Overwrite existing file?");
string(Cannot_store_block_STR,                  "Unable to store block to paste list - continue deletion?");


/* Help system */

/* Help from the icon bar icon */

string(help_iconbar,
    "This is the PipeDrea3 icon.|M"                 \
    "Click SELECT to create a new document.|M"      \
    "Drag a file onto the icon to load the file.|M" \
    "Click MENU to select a particular document or to quit PipeDrea3.");


/* Help from a dialog window */

string(help_dialog_window,
    "This is a PipeDrea3 dialogue box.|M"           \
    "Click OK or press RETURN to complete.|M"       \
    "Use \x8B and \x8A to move between fields.|M"   \
    "Press ESCAPE to cancel.");


/* Help from a PipeDrea3 window */

string(help_main_window,
    "This is a PipeDrea3 document window.|M");

string(help_drag_file_to_insert,
    "Drag a file into the window to insert the file.");

string(help_click_select_to ,
    "Click SELECT to ");

string(help_click_adjust_to ,
    "Click ADJUST to ");

string(help_position_the_caret_in ,
    "position the caret in ");

string(help_insert_a_reference_to,
    "insert a reference to ");

string(help_slot,
    "slot ");

string(help_edit_line,
    "the editing line.|M");

string(help_dot_cr,
    ".|M");

string(help_top_left_corner,
    "Double-click SELECT in the top-left corner of the borders to mark the sheet.|M"  \
    "Double-click ADJUST to clear a marked block.|M");

string(help_col_border,
    "Drag SELECT along the column border to mark all rows.|M"   \
    "Double-click SELECT to mark this column.|M");

string(help_drag_row_border,
    "Drag SELECT along the row border to mark all columns.|M");

string(help_double_row_border,
    "Double-click SELECT to mark this row.|M");

string(help_row_is_page_break, 
    "This row is a page break.|M");

string(help_row_is_hard_page_break ,
    "This row is a hard page break.|M");

string(help_slot_coordinates,
    "These are the current slot coordinates.|M");

string(help_numeric_contents,
    "These are the contents of the current expression slot.|M");


/* Menu strings - keep in step with riscos.c menu offsets */

/* iconbar menu */
/* title is applicationname */
string(iconbar_menu_entries,        ">Info,>Windows,Tidy up,Quit");


/* iconbar 'Windows' submenu */
string(iw_menu_title,               "Windows");
string(iw_menu_entries,             "New window");
string(iw_menu_prefix,              "...");


/* 'Printer font' submenu */
string(pf_menu_title,               "Font");
string(pf_menu_entries,             "System font");


/* 'Font size' submenu */
string(fs_menu_title,               "Size");
string(fs_menu_entries,             "Width,8,10,12,14,nn.nn|Height,8,10,12,14,nn.nn");


/* 'Printer line spacing' submenu */
string(ld_menu_title,               "Spacing");
string(ld_menu_entries,             "8,10,12,14,nn.nn");


/* Printer related */

string(No_RISC_OS_STR,              "No RISC OS");
string(Zs_printer_driver_STR,       "%s printer driver");


string(fp_only_validation_STR,      "a-+.0-9eE");
string(pointsize_STR,               "%.3f");

string(filetype_expand_Zs_ZX_STR,   "%s (%3.3X)");
string(memory_size_Zd_STR,          "%dK");


/* Commands passed to OS_CLI */

string(Create_Zs_STR,               "Create %s");
string(Filer_OpenDir_Zs_STR,        "Filer_OpenDir %s");
string(SetType_Zs_Z3X_STR,          "SetType %s &%3X");
string(PS_Zs_STR,                   "PS %s");

#endif  /* RISCOS */


string(_unable_to_create_new_document_STR,  "- unable to create new document");


/* -------------------------------- Menus ---------------------------------- */

/* Top-level menu headings */

string(Blocks_STR,                          "Blocks");
string(Cursor_STR,                          "Cursor");
string(Edit_STR,                            "Edit");
string(Files_STR,                           "Files");
string(Layout_STR,                          "Layout");
string(Spell_STR,                           "Spell");


/* 'Blocks' submenu */

string(Mark_block_STR,                      "Mark block");
string(Clear_markers_STR,                   "Clear markers");
string(Copy_block_STR,                      "Copy block");
string(Copy_block_to_paste_list_STR,        "Copy block to paste list");
string(Size_of_paste_list_STR,              "Size of paste list");
string(Move_block_STR,                      "Move block");
string(Delete_block_STR,                    "Delete block");
string(Replicate_down_STR,                  "Replicate down");
string(Replicate_right_STR,                 "Replicate right");
string(Replicate_STR,                       "Replicate");
string(Sort_STR,                            "Sort");
string(Search_STR,                          "Search");
string(Next_match_STR,                      "Next match");
string(Previous_match_STR,                  "Previous match");
string(Set_protection_STR,                  "Set protection");
string(Clear_protection_STR,                "Clear protection");
string(Number_X_Text_STR,                   "Number <> Text");
string(Snapshot_STR,                        "Snapshot");
string(Word_count_STR,                      "Word count");


/* 'Cursor' submenu */

string(Format_paragraph_STR,                "Format paragraph");
string(First_column_STR,                    "First column");
string(Last_column_STR,                     "Last column");
string(Next_word_STR,                       "Next word");
string(Previous_word_STR,                   "Previous word");
#if RISCOS
string(Centre_window_STR,                   "Centre window");
#elif MS || ARTHUR
string(Centre_window_STR,                   "Centre screen");
#endif
string(Save_position_STR,                   "Save position");
string(Restore_position_STR,                "Restore position");
#if RISCOS
string(Swap_position_and_caret_STR,         "Swap position and caret");
#elif MS || ARTHUR
string(Swap_position_and_caret_STR,         "Swap position and cursor");
#endif
string(Go_to_slot_STR,                      "Go to slot");
string(Define_key_STR,                      "Define key");
string(Define_function_key_STR,             "Define function key");
string(Define_command_STR,                  "Define command");
string(Do_macro_file_STR,                   "Do macro file");
string(Record_macro_file_STR,               "Record macro file");


/* 'Edit' submenu */

string(Delete_character_STR,                "Delete character");
string(Insert_space_STR,                    "Insert space");
string(Insert_character_STR,                "Insert character");
string(Delete_word_STR,                     "Delete word");
string(Delete_to_end_of_slot_STR,           "Delete to end of slot");
string(Delete_row_STR,                      "Delete row");
string(Insert_row_STR,                      "Insert row");
string(Paste_STR,                           "Paste");
#if RISCOS
string(Insert_STR,                          "Insert");
#elif MS || ARTHUR
string(Insert_STR,                          "Insert/Overtype");
#endif
string(Swap_case_STR,                       "Swap case");
string(Edit_expression_STR,                 "Edit expression");
string(Insert_reference_STR,                "Insert reference");
string(Split_line_STR,                      "Split line");
string(Join_lines_STR,                      "Join lines");
string(Insert_row_in_column_STR,            "Insert row in column");
string(Delete_row_in_column_STR,            "Delete row in column");
string(Insert_column_STR,                   "Insert column");
string(Delete_column_STR,                   "Delete column");
string(Add_column_STR,                      "Add column");
string(Insert_page_STR,                     "Insert page");


/* 'Files' submenu */

string(Load_STR,                            "Load");
string(Save_STR,                            "Save");
string(Save_initialisation_file_STR,        "Save initialisation file");
string(Name_STR,                            "Name");
string(New_window_STR,                      "New window");
string(Short_menus_STR,                     "Short menus");
string(Options_STR,                         "Options");
string(Colours_STR,                         "Colours");
string(Next_file_STR,                       "Next file");
string(Previous_file_STR,                   "Previous file");
string(Top_file_STR,                        "Top file");
string(Bottom_file_STR,                     "Bottom file");
string(Create_linking_file_STR,             "Create linking file");
string(Recalculate_STR,                     "Recalculate");
string(Recalculation_options_STR,           "Recalculation options");
string(Help_STR,                            "Help");
string(About_STR,                           "About");
string(Exit_STR,                            "Quit");
#if ARTHUR
string(OSCommand_STR,                       "* commands");
#elif MS
string(OSCommand_STR,                       "DOS");
string(Deep_screen_STR,                     "Deep screen");
#endif


/* 'Layout' submenu */

string(Set_column_width_STR,                "Set column width");
string(Set_right_margin_STR,                "Set right margin");
string(Fix_row_STR,                         "Fix row");
string(Fix_column_STR,                      "Fix column");
string(Move_margin_right_STR,               "Move margin right");
string(Move_margin_left_STR,                "Move margin left");
string(Left_align_STR,                      "Left align");
string(Centre_align_STR,                    "Centre align");
string(Right_align_STR,                     "Right align");
string(LCR_align_STR,                       "LCR align");
string(Free_align_STR,                      "Free align");
string(Decimal_places_STR,                  "Decimal places");
string(Sign_brackets_STR,                   "Sign brackets");
string(Sign_minus_STR,                      "Sign minus");
string(Leading_characters_STR,              "Leading characters");
string(Trailing_characters_STR,             "Trailing characters");
string(Default_format_STR,                  "Default format");


/* 'Print' submenu */

string(Print_STR,                           "Print");
string(Set_parameter_STR,                   "Set parameter");
string(Page_layout_STR,                     "Page layout");
string(Printer_configuration_STR,           "Printer configuration");
string(Microspace_pitch_STR,                "Microspace pitch");
string(Printer_font_STR,                    "Printer font");
string(Insert_font_STR,                     "Insert font");
string(Printer_line_spacing_STR,            "Printer line spacing");
string(Underline_STR,                       "Underline");
string(Bold_STR,                            "Bold");
string(Extended_sequence_STR,               "Extended sequence");
string(Italic_STR,                          "Italic");
string(Subscript_STR,                       "Subscript");
string(Superscript_STR,                     "Superscript");
string(Alternate_font_STR,                  "Alternate font");
string(User_defined_STR,                    "User defined");
string(Remove_highlights_STR,               "Remove highlights");
string(Highlight_block_STR,                 "Highlight block");


/* 'Spell' submenu */

string(Auto_check_STR,                      "Auto check");
string(Check_document_STR,                  "Check document");
string(Delete_word_from_dictionary_STR,     "Delete word from dictionary");
string(Insert_word_in_dictionary_STR,       "Insert word in dictionary");
string(Display_user_dictionaries_STR,       "Display user dictionaries");
string(Browse_STR,                          "Browse");
string(Dump_dictionary_STR,                 "Dump dictionary");
string(Merge_file_with_dictionary_STR,      "Merge file with dictionary");
string(Create_user_dictionary_STR,          "Create user dictionary");
string(Open_user_dictionary_STR,            "Open user dictionary");
string(Close_user_dictionary_STR,           "Close user dictionary");
string(Pack_user_dictionary_STR,            "Pack user dictionary");
string(Lock_dictionary_STR,                 "Lock dictionary");
string(Unlock_dictionary_STR,               "Unlock dictionary");


/* 'Random' submenu */

string(Cursor_up_STR,                       "Cursor up");
string(Cursor_down_STR,                     "Cursor down");
string(Cursor_left_STR,                     "Cursor left");
string(Cursor_right_STR,                    "Cursor right");
string(Top_of_column_STR,                   "Top of column");
string(Bottom_of_column_STR,                "Bottom of column");
string(Start_of_slot_STR,                   "Start of slot");
string(End_of_slot_STR,                     "End of slot");
string(Scroll_up_STR,                       "Scroll up");
string(Scroll_down_STR,                     "Scroll down");
string(Scroll_left_STR,                     "Scroll left");
string(Scroll_right_STR,                    "Scroll right");
string(Page_up_STR,                         "Page up");
string(Page_down_STR,                       "Page down");
string(Enter_STR,                           "Enter");
string(Rubout_STR,                          "Rubout");
string(Next_column_STR,                     "Next column");
string(Previous_column_STR,                 "Previous column");
string(Escape_STR,                          "Escape");
string(Pause_STR,                           "Pause");
string(Replace_STR,                         "Replace");
string(Next_window_STR,                     "Next window");
string(Close_window_STR,                    "Close window");
string(Tidy_up_STR,                         "Tidy up");
string(Mark_sheet_STR,                      "Mark sheet");


/* ------------------------------ Dialog boxes ------------------------------*/

/* thousands separator options */

string(thousands_none_STR,                  "none");
string(thousands_comma_STR,                 ",");
string(thousands_dot_STR,                   ".");
string(thousands_space_STR,                 "space");


/* printer type options */

string(printertype_RISC_OS_STR,             "RISC OS");
string(printertype_Parallel_STR,            "Parallel");
string(printertype_Serial_STR,              "Serial");
string(printertype_Network_STR,             "Network");
string(printertype_User_STR,                "User");


/* function key names */

string(F1_STR,                              "F1");
string(F2_STR,                              "F2");
string(F3_STR,                              "F3");
string(F4_STR,                              "F4");
string(F5_STR,                              "F5");
string(F6_STR,                              "F6");
string(F7_STR,                              "F7");
string(F8_STR,                              "F8");
string(F9_STR,                              "F9");
string(F10_STR,                             "F10");
string(F11_STR,                             "F11");
string(F12_STR,                             "F12");

string(Shift_F1_STR,                        "Shift F1");
string(Shift_F2_STR,                        "Shift F2");
string(Shift_F3_STR,                        "Shift F3");
string(Shift_F4_STR,                        "Shift F4");
string(Shift_F5_STR,                        "Shift F5");
string(Shift_F6_STR,                        "Shift F6");
string(Shift_F7_STR,                        "Shift F7");
string(Shift_F8_STR,                        "Shift F8");
string(Shift_F9_STR,                        "Shift F9");
string(Shift_F10_STR,                       "Shift F10");
string(Shift_F11_STR,                       "Shift F11");
string(Shift_F12_STR,                       "Shift F12");

string(Ctrl_F1_STR,                         "Ctrl F1");
string(Ctrl_F2_STR,                         "Ctrl F2");
string(Ctrl_F3_STR,                         "Ctrl F3");
string(Ctrl_F4_STR,                         "Ctrl F4");
string(Ctrl_F5_STR,                         "Ctrl F5");
string(Ctrl_F6_STR,                         "Ctrl F6");
string(Ctrl_F7_STR,                         "Ctrl F7");
string(Ctrl_F8_STR,                         "Ctrl F8");
string(Ctrl_F9_STR,                         "Ctrl F9");
string(Ctrl_F10_STR,                        "Ctrl F10");
string(Ctrl_F11_STR,                        "Ctrl F11");
string(Ctrl_F12_STR,                        "Ctrl F12");

string(Ctrl_Shift_F1_STR,                   "Ctrl-Shift F1");
string(Ctrl_Shift_F2_STR,                   "Ctrl-Shift F2");
string(Ctrl_Shift_F3_STR,                   "Ctrl-Shift F3");
string(Ctrl_Shift_F4_STR,                   "Ctrl-Shift F4");
string(Ctrl_Shift_F5_STR,                   "Ctrl-Shift F5");
string(Ctrl_Shift_F6_STR,                   "Ctrl-Shift F6");
string(Ctrl_Shift_F7_STR,                   "Ctrl-Shift F7");
string(Ctrl_Shift_F8_STR,                   "Ctrl-Shift F8");
string(Ctrl_Shift_F9_STR,                   "Ctrl-Shift F9");
string(Ctrl_Shift_F10_STR,                  "Ctrl-Shift F10");
string(Ctrl_Shift_F11_STR,                  "Ctrl-Shift F11");
string(Ctrl_Shift_F12_STR,                  "Ctrl-Shift F12");


/* Month names */

string(month_January_STR,                   "January");
string(month_February_STR,                  "February");
string(month_March_STR,                     "March");
string(month_April_STR,                     "April");
string(month_May_STR,                       "May");
string(month_June_STR,                      "June");
string(month_July_STR,                      "July");
string(month_August_STR,                    "August");
string(month_September_STR,                 "September");
string(month_October_STR,                   "October");
string(month_November_STR,                  "November");
string(month_December_STR,                  "December");


#if defined(CODE_FOR_STRINGS_NOW)

}   /* end of init proc */

#endif

/* end of strings.c */
