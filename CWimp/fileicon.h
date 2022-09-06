/*
 * Title: fileicon.h
 * Purpose: general display of a file icon in a dialog box.
 * Status: Arthur-specific, part of ArcEdit.
 * History:
 *   15-Mar-88: WRS: carved off from xfersend.
 */

void fileicon(wimp_w, wimp_i, int filetype);

/* Delete and recreate the given icon of the given window, so that
it becomes an icon for the given filetype. */
