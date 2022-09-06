/* riscdialog.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       riscdialog.h - exported objects from riscdialog.c
 * Author:      Stuart K. Swales 14 Mar 1989
*/

#if RISCOS
/* Only export objects if RISCOS */

#ifndef __pd__riscdialog_h
#define __pd__riscdialog_h

/* exported functions */

extern intl currentfiletype(optiontype filetype);
extern void pausing_null(void);
#if FREEBIE
#define riscos_install_pipedream() do {/*EMPTY*/} while(FALSE)
#else
extern void riscos_install_pipedream(void);
#endif
extern void riscdialog_dispose(void);
extern void riscdialog_dopause(intl nseconds);
extern intl riscdialog_ended(void);
extern intl riscdialog_execute(dialog_proc proc, const char *dname, DIALOG *dptr, intl boxnumber);
extern void riscdialog_getfontsize(void);
extern void riscdialog_initialise_once(void);
extern intl riscdialog_query(const char *mess);
extern intl riscdialog_queryexit(const char *mess);
extern intl riscdialog_replace_dbox(const char *mess1, const char *mess2);
extern void riscdialog_replace_dbox_end(void);
extern intl riscdialog_save_existing(void);

/* shared dprocs */
extern void dproc_anasubgram(DIALOG *dptr);
extern void dproc_numtext(DIALOG *dptr);
extern void dproc_onecomponoff(DIALOG *dptr);
extern void dproc_onenumeric(DIALOG *dptr);
extern void dproc_onespecial(DIALOG *dptr);
extern void dproc_onetext(DIALOG *dptr);
extern void dproc_twotext(DIALOG *dptr);

/* unique dprocs */
extern void dproc_aboutfile(DIALOG *dptr);
extern void dproc_aboutprog(DIALOG *dptr);
extern void dproc_browse(DIALOG *dptr);
extern void dproc_checkdoc(DIALOG *dptr);
extern void dproc_checked(DIALOG *dptr);
extern void dproc_checking(DIALOG *dptr);
extern void dproc_colours(DIALOG *dptr);
extern void dproc_createlinking(DIALOG *dptr);
extern void dproc_decimal(DIALOG *dptr);
extern void dproc_defkey(DIALOG *dptr);
extern void dproc_deffnkey(DIALOG *dptr);
extern void dproc_dumpdict(DIALOG *dptr);
extern void dproc_insertchar(DIALOG *dptr);
extern void dproc_loadfile(DIALOG *dptr);
extern void dproc_microspace(DIALOG *dptr);
extern void dproc_options(DIALOG *dptr);
extern void dproc_overwrite(DIALOG *dptr);
extern void dproc_pagelayout(DIALOG *dptr);
extern void dproc_print(DIALOG *dptr);
extern void dproc_printconfig(DIALOG *dptr);
extern void dproc_recalc(DIALOG *dptr);
extern void dproc_savefile(DIALOG *dptr);
extern void dproc_save_deleted(DIALOG *dptr);
extern void dproc_save_existing(DIALOG *dptr);
extern void dproc_search(DIALOG *dptr);
extern void dproc_sort(DIALOG *dptr);


typedef enum
{
    riscdialogquery_YES     = 1,
    riscdialogquery_NO      = 2,
    riscdialogquery_CANCEL  = 3

} riscdialogquery_REPLY;

#endif  /* __pd__riscdialog_h */

#endif  /* RISCOS */

/* end of riscdialog.h */
