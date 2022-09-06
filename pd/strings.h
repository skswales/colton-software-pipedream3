/* strings.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       strings.h - header file for strings.c
 * Author:      Stuart K. Swales 24-Feb-1989
*/

#ifndef __pd__strings_h
#define __pd__strings_h

/* Macro definitions */

/* Both NorCroft and Microsoft give better code for shorter inline strings
 * as opposed to externals
*/

#define NULLSTR         ""
#define UNULLSTR        ((uchar *) NULLSTR)
#define CR_STR          "\x0D"
#define SPACE_STR       " "

#define YES_STR         "Yes"
#define NO_STR          "No"

#if ARTHUR || RISCOS
#define POUND_STR       "\xA3"
#elif MS
#define POUND_STR       "\x9C"
#endif

#define read_str        "rb"
#define write_str       "wb"
#define update_str      "r+b"


/* exported variables */

#if !defined(CODE_FOR_STRINGS)
#define stringdef(id)   extern const char id[]

#else
#define stringdef(id)   extern const char *id
#endif

stringdef(DefaultLocale_STR);
stringdef(Unrecognised_arg_Zs_STR);

stringdef(WILD_STR);

stringdef(FORESLASH_STR);
stringdef(ONE_DOT_STR);
stringdef(TWO_DOT_STR);

#if MS
stringdef(BACKSLASH_STR);
#endif

/* names of files used by PipeDream */

#if !defined(LOTUS_OFF)
stringdef(temp_file);
#endif
#if !defined(SPELL_OFF)
stringdef(MASTERDICT_STR);
#endif
#if defined(HELP_FILE)
stringdef(HELPFILE_STR);
#endif
stringdef(DUMPFILE_STR);
stringdef(INITEXEC_STR);
stringdef(INITFILE_STR);
stringdef(ZS_LINKFILE_ZD_STR);
stringdef(ZS_MACROFILE_STR);
stringdef(UNTITLED_ZD_STR);
stringdef(PD_PATH_STR);
#if RISCOS
stringdef(PD_INITFILE_STR);
stringdef(PD_IMAGEFILE_STR);
stringdef(ambiguous_tag_STR);
#else
#define PD_INITFILE_STR     INITFILE_STR
stringdef(serial_port_STR);
#endif
stringdef(USERDICT_STR);

#if !defined(Z88_OFF)
stringdef(Z88_STR);
stringdef(Z88COLON_STR);
#endif



stringdef(Ctrl__STR);
stringdef(Shift__STR);
stringdef(PRESSANYKEY_STR);
stringdef(Initialising_STR);


/* for registration numbers & checking */

#if RISCOS
stringdef(program_invalid_STR);
#elif MS || ARTHUR
stringdef(run_the_install_program_STR);
#endif

#if MS
stringdef(ZsNL_STR);
#elif ARTHUR
stringdef(AZsNL_STR);
#endif

#if MS || ARTHUR
stringdef(NLZsNLZsNLNL_STR);
#endif


/* date format strings */

stringdef(Zd_Zs_Zd_STR);
stringdef(Zd_ZP3s_Zd_STR);

stringdef(FZd_STR);
stringdef(New_width_STR);
stringdef(New_right_margin_position_STR);
stringdef(Zld_Zs_found_STR);
stringdef(Zld_hidden_STR);
stringdef(file_STR);
stringdef(files_STR);
stringdef(word_STR);
stringdef(words_STR);
stringdef(string_STR);
stringdef(strings_STR);
stringdef(Zd_unrecognised_Zs_STR);
stringdef(Zd_Zs_added_to_user_dict_STR);
stringdef(Zs_of_Zs_STR);
stringdef(Zs_complete_STR);
stringdef(Page_Zd_wait_between_pages_STR);

#if MS
stringdef(screen_Zd_of_Zd_STR);
#endif

#if RISCOS
stringdef(save_edited_file_Zs_STR);
stringdef(Zd_Zs_edited_are_you_sure_Zs_STR);
#endif

stringdef(close_dependent_files_winge_STR);
stringdef(close_dependent_links_winge_STR);
stringdef(load_supporting_winge_STR);
stringdef(name_supporting_winge_STR);


stringdef(DecimalPlaces_Parm_STR);

#if MS || ARTHUR
stringdef(NAME_OF_FILE_STR);
#endif


#if RISCOS

/*  stringdef(Create_user_dictionary_STR);  */
/*  stringdef(Open_user_dictionary_STR);    */
/*  stringdef(Close_user_dictionary_STR);   */
/*  stringdef(Anagrams_STR);                */
/*  stringdef(Subgrams_STR);                */
/*  stringdef(Lock_dictionary_STR);         */
/*  stringdef(Unlock_dictionary_STR);       */
stringdef(Dump_STR);
stringdef(Dumping_STR);
stringdef(Merge_STR);
stringdef(Merging_STR);
stringdef(Anagrams_STR);
stringdef(Subgrams_STR);
stringdef(Opened_user_dictionaries_STR);
stringdef(Insert_word_in_user_dictionary_STR);
stringdef(Delete_word_from_user_dictionary_STR);
stringdef(Insert_highlights_STR);

stringdef(Overwrite_existing_file_STR);
stringdef(Cannot_store_block_STR);

stringdef(help_iconbar);
stringdef(help_dialog_window);
stringdef(help_main_window);
stringdef(help_drag_file_to_insert);
stringdef(help_click_select_to);
stringdef(help_click_adjust_to);
stringdef(help_position_the_caret_in);
stringdef(help_insert_a_reference_to);
stringdef(help_slot);
stringdef(help_edit_line);
stringdef(help_dot_cr);
stringdef(help_top_left_corner);
stringdef(help_col_border);
stringdef(help_drag_row_border);
stringdef(help_double_row_border);
stringdef(help_row_is_page_break);
stringdef(help_row_is_hard_page_break);
stringdef(help_slot_coordinates);
stringdef(help_numeric_contents);
stringdef(iconbar_menu_entries);
stringdef(iw_menu_title);
stringdef(iw_menu_entries);
stringdef(iw_menu_prefix);
stringdef(pf_menu_title);
stringdef(pf_menu_entries);
stringdef(fs_menu_title);
stringdef(fs_menu_entries);
stringdef(ld_menu_title);
stringdef(ld_menu_entries);
stringdef(No_RISC_OS_STR);
stringdef(Zs_printer_driver_STR);

stringdef(fp_only_validation_STR);
stringdef(pointsize_STR);

stringdef(filetype_expand_Zs_ZX_STR);
stringdef(memory_size_Zd_STR);

stringdef(Create_Zs_STR);
stringdef(Filer_OpenDir_Zs_STR);
stringdef(PS_Zs_STR);
stringdef(SetType_Zs_Z3X_STR);

#endif  /* RISCOS */

stringdef(_unable_to_create_new_document_STR);


stringdef(Blocks_STR);
stringdef(Cursor_STR);
stringdef(Edit_STR);
stringdef(Files_STR);
stringdef(Layout_STR);
stringdef(Spell_STR);

stringdef(Mark_block_STR);
stringdef(Clear_markers_STR);
stringdef(Copy_block_STR);
stringdef(Copy_block_to_paste_list_STR);
stringdef(Size_of_paste_list_STR);
stringdef(Move_block_STR);
stringdef(Delete_block_STR);
stringdef(Replicate_down_STR);
stringdef(Replicate_right_STR);
stringdef(Replicate_STR);
stringdef(Sort_STR);
stringdef(Search_STR);
stringdef(Next_match_STR);
stringdef(Previous_match_STR);
stringdef(Set_protection_STR);
stringdef(Clear_protection_STR);
stringdef(Number_X_Text_STR);
stringdef(Snapshot_STR);
stringdef(Word_count_STR);

stringdef(Format_paragraph_STR);
stringdef(First_column_STR);
stringdef(Last_column_STR);
stringdef(Next_word_STR);
stringdef(Previous_word_STR);
stringdef(Centre_window_STR);
stringdef(Save_position_STR);
stringdef(Restore_position_STR);
stringdef(Swap_position_and_caret_STR);
stringdef(Go_to_slot_STR);
stringdef(Define_key_STR);
stringdef(Define_function_key_STR);
stringdef(Record_macro_file_STR);
stringdef(Do_macro_file_STR);
stringdef(Define_command_STR);

stringdef(Delete_character_STR);
stringdef(Insert_space_STR);
stringdef(Insert_character_STR);
stringdef(Delete_word_STR);
stringdef(Delete_to_end_of_slot_STR);
stringdef(Delete_row_STR);
stringdef(Insert_row_STR);
stringdef(Paste_STR);
stringdef(Insert_STR);
stringdef(Swap_case_STR);
stringdef(Edit_expression_STR);
stringdef(Insert_reference_STR);
stringdef(Split_line_STR);
stringdef(Join_lines_STR);
stringdef(Insert_row_in_column_STR);
stringdef(Delete_row_in_column_STR);
stringdef(Insert_column_STR);
stringdef(Delete_column_STR);
stringdef(Add_column_STR);
stringdef(Insert_page_STR);

stringdef(Load_STR);
stringdef(Save_STR);
stringdef(Save_initialisation_file_STR);
stringdef(Name_STR);
stringdef(New_window_STR);
stringdef(Short_menus_STR);
stringdef(Options_STR);
stringdef(Colours_STR);
stringdef(Next_file_STR);
stringdef(Previous_file_STR);
stringdef(Top_file_STR);
stringdef(Bottom_file_STR);
stringdef(Create_linking_file_STR);
stringdef(Recalculate_STR);
stringdef(Recalculation_options_STR);
stringdef(Help_STR);
stringdef(About_STR);
stringdef(Exit_STR);
#if MS || ARTHUR
stringdef(OSCommand_STR);
#endif
#if MS
stringdef(Deep_screen_STR);
#endif

stringdef(Set_column_width_STR);
stringdef(Set_right_margin_STR);
stringdef(Fix_row_STR);
stringdef(Fix_column_STR);
stringdef(Move_margin_right_STR);
stringdef(Move_margin_left_STR);
stringdef(Left_align_STR);
stringdef(Centre_align_STR);
stringdef(Right_align_STR);
stringdef(LCR_align_STR);
stringdef(Free_align_STR);
stringdef(Decimal_places_STR);
stringdef(Sign_brackets_STR);
stringdef(Sign_minus_STR);
stringdef(Leading_characters_STR);
stringdef(Trailing_characters_STR);
stringdef(Default_format_STR);

stringdef(Print_STR);
stringdef(Set_parameter_STR);
stringdef(Page_layout_STR);
stringdef(Printer_configuration_STR);
stringdef(Microspace_pitch_STR);
stringdef(Printer_font_STR);
stringdef(Insert_font_STR);
stringdef(Printer_line_spacing_STR);
stringdef(Underline_STR);
stringdef(Bold_STR);
stringdef(Extended_sequence_STR);
stringdef(Italic_STR);
stringdef(Subscript_STR);
stringdef(Superscript_STR);
stringdef(Alternate_font_STR);
stringdef(User_defined_STR);
stringdef(Remove_highlights_STR);
stringdef(Highlight_block_STR);

stringdef(Auto_check_STR);
stringdef(Check_document_STR);
stringdef(Delete_word_from_dictionary_STR);
stringdef(Insert_word_in_dictionary_STR);
stringdef(Display_user_dictionaries_STR);
stringdef(Browse_STR);
stringdef(Dump_dictionary_STR);
stringdef(Merge_file_with_dictionary_STR);
stringdef(Create_user_dictionary_STR);
stringdef(Open_user_dictionary_STR);
stringdef(Close_user_dictionary_STR);
stringdef(Pack_user_dictionary_STR);
stringdef(Lock_dictionary_STR);
stringdef(Unlock_dictionary_STR);

stringdef(Cursor_up_STR);
stringdef(Cursor_down_STR);
stringdef(Cursor_left_STR);
stringdef(Cursor_right_STR);
stringdef(Top_of_column_STR);
stringdef(Bottom_of_column_STR);
stringdef(Start_of_slot_STR);
stringdef(End_of_slot_STR);
stringdef(Scroll_up_STR);
stringdef(Scroll_down_STR);
stringdef(Scroll_left_STR);
stringdef(Scroll_right_STR);
stringdef(Page_up_STR);
stringdef(Page_down_STR);
stringdef(Enter_STR);
stringdef(Rubout_STR);
stringdef(Next_column_STR);
stringdef(Previous_column_STR);
stringdef(Escape_STR);
stringdef(Pause_STR);
stringdef(Replace_STR);
stringdef(Next_window_STR);
stringdef(Close_window_STR);
stringdef(Tidy_up_STR);
stringdef(Mark_sheet_STR);


/* thousands separator options */

stringdef(thousands_none_STR);
stringdef(thousands_comma_STR);
stringdef(thousands_dot_STR);
stringdef(thousands_space_STR);


/* printer type options */

stringdef(printertype_RISC_OS_STR);
stringdef(printertype_Parallel_STR);
stringdef(printertype_Serial_STR);
stringdef(printertype_Network_STR);
stringdef(printertype_User_STR);


/* function key names */

stringdef(F1_STR);
stringdef(F2_STR);
stringdef(F3_STR);
stringdef(F4_STR);
stringdef(F5_STR);
stringdef(F6_STR);
stringdef(F7_STR);
stringdef(F8_STR);
stringdef(F9_STR);
stringdef(F10_STR);
stringdef(F11_STR);
stringdef(F12_STR);

stringdef(Shift_F1_STR);
stringdef(Shift_F2_STR);
stringdef(Shift_F3_STR);
stringdef(Shift_F4_STR);
stringdef(Shift_F5_STR);
stringdef(Shift_F6_STR);
stringdef(Shift_F7_STR);
stringdef(Shift_F8_STR);
stringdef(Shift_F9_STR);
stringdef(Shift_F10_STR);
stringdef(Shift_F11_STR);
stringdef(Shift_F12_STR);

stringdef(Ctrl_F1_STR);
stringdef(Ctrl_F2_STR);
stringdef(Ctrl_F3_STR);
stringdef(Ctrl_F4_STR);
stringdef(Ctrl_F5_STR);
stringdef(Ctrl_F6_STR);
stringdef(Ctrl_F7_STR);
stringdef(Ctrl_F8_STR);
stringdef(Ctrl_F9_STR);
stringdef(Ctrl_F10_STR);
stringdef(Ctrl_F11_STR);
stringdef(Ctrl_F12_STR);

stringdef(Ctrl_Shift_F1_STR);
stringdef(Ctrl_Shift_F2_STR);
stringdef(Ctrl_Shift_F3_STR);
stringdef(Ctrl_Shift_F4_STR);
stringdef(Ctrl_Shift_F5_STR);
stringdef(Ctrl_Shift_F6_STR);
stringdef(Ctrl_Shift_F7_STR);
stringdef(Ctrl_Shift_F8_STR);
stringdef(Ctrl_Shift_F9_STR);
stringdef(Ctrl_Shift_F10_STR);
stringdef(Ctrl_Shift_F11_STR);
stringdef(Ctrl_Shift_F12_STR);


/* month names */

stringdef(month_January_STR);
stringdef(month_February_STR);
stringdef(month_March_STR);
stringdef(month_April_STR);
stringdef(month_May_STR);
stringdef(month_June_STR);
stringdef(month_July_STR);
stringdef(month_August_STR);
stringdef(month_September_STR);
stringdef(month_October_STR);
stringdef(month_November_STR);
stringdef(month_December_STR);

#endif  /* __pd__strings_h */

/* end of strings.h */
