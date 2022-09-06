/* markers.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       markers.h - exported objects from markers.c
 * Author:      Stuart K. Swales 12-May-1989
*/

#ifndef __pd__markers_h
#define __pd__markers_h

/* exported functions */

extern void alter_marked_block(colt tcol, rowt trow);
extern void clear_markers(void);
extern void force_next_col(void);
extern BOOL inblock(colt tcol, rowt trow);      /* is this slot in marked block? */
extern void init_block(const SLR *bs, const SLR *be);   /* initializes block to be given block or current slot */
extern void init_marked_block(void);            /* initializes block to be marked block or current slot */
extern void init_doc_as_block(void);            /* initializes block to be whole document */
extern void make_single_mark_into_block(void);
#define DOWN_COLUMNS    TRUE
#define ACROSS_ROWS     FALSE
extern BOOL next_in_block(BOOL direction);
extern intl percent_in_block(BOOL direction);
extern void set_marked_block(colt scol, rowt srow, colt ecol, rowt erow, BOOL new);
extern void set_marker(colt tcol, rowt trow);
extern void start_marking_block(colt scol, rowt srow, colt ecol, rowt erow, intl drag, BOOL allow_continue);

#if RISCOS
extern void application_button_click(gcoord x, gcoord y, intl buttonstate);
extern void application_drag(gcoord x, gcoord y, BOOL ended);
#endif

#define MARKER_SOMEWHERE()      (blkstart.col != NO_COL)

#define MARKER_DEFINED()        ((blkdochandle == current_document_handle())  &&  \
                                MARKER_SOMEWHERE())

#define MARKED_BLOCK_DEFINED()  (MARKER_DEFINED()  &&  (blkend.col != NO_COL))

#define IN_COLUMN_HEADING(roff) ((roff == -1)  &&  (borderheight != 0))
#define IN_ROW_BORDER(coff)     ((coff == -1)  &&  (borderwidth  != 0))

/* 'Active' corner of a marked block */
#define ACTIVE_COL  ((blkanchor.col == blkstart.col) ? blkend.col : blkstart.col)
#define ACTIVE_ROW  ((blkanchor.row == blkstart.row) ? blkend.row : blkstart.row)

#endif  /* __pd__markers_h */

/* end of markers.h */
