/* dtpsave.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       dtpsave.h - header for dtpsave.c
 * Author:      RJM July 1989
*/

#if !defined(H_DTPSAVE_FLAG)
#define H_DTPSAVE_FLAG

#define DTP_IMPORT


#define DTP_LINESEP         LF
#define DTP_LINELENGTH      ((uchar) 159)
#define DTP_RMARGIN         ((uchar) ']')
#define DTP_TABSTOP         ((uchar) '\x7F')
#define DTP_PICA            ((uchar) '0')
#define DTP_SPACE           ((uchar) '\x1E')
#define DTP_NOHIGHLIGHTS    ((uchar) 0x80)
#define DTP_FORMFEED        ((uchar) '\x0C')
#define DTP_FORMAT_LINE     ((uchar) '\x1F')

#define DTP_PAGE_LAYOUT     '0'
#define DTP_HEADER          '1'
#define DTP_FOOTER          '2'
#define DTP_RULER           '9'

#define DTP_DELIMITER '/'
#define DTP_ESCAPE_CHAR     ((uchar) '\x1B')
#define DTP_STRETCH_SPACE   ((uchar) '\x1C')
#define DTP_INDENT_SPACE    ((uchar) '\x1D')
#define DTP_SOFT_SPACE      ((uchar) '\x1E')

#define DTP_HARD_PAGE       ((uchar) '\x0C')
#define DTP_SOFT_PAGE       ((uchar) '\x0B')


/* exported functions */

extern void dtp_change_highlights(uchar new_byte, uchar old_byte);
extern intl dtp_convert(intl c, FILE *loadinput, uchar *field_separator, 
            uchar *justify, uchar *h_byte, uchar *pageoffset, uchar *type);
extern BOOL dtp_isdtpfile(const char *bytes);
extern void dtp_load_preinit(FILE *loadinput);
extern void dtp_load_postinit(void);
extern BOOL dtp_save_preamble(FILE *output);
extern BOOL dtp_save_slot(slotp tslot, colt tcol, rowt trow, FILE *output);

#endif  /* H_DTPSAVE_FLAG */

/* end of dtpsave.h */
