/****************************************************************************
 * This source file was written by Acorn Computers Limited. It is part of   *
 * the RISCOS library for writing applications in C for RISC OS. It may be  *
 * used freely in the creation of programs for Archimedes. It should be     *
 * used with Acorn's C Compiler Release 3 or later.                         *
 *                                                                          *
 ***************************************************************************/

/*
 * Title:  colourtran.h
 * Purpose: C interface to the ColourTrans SWIs
 *
 */

#ifndef __colourtran_h
#define __colourtran_h


/* ----------------------- colourtran_select_table -------------------------
 * Description:   Sets up a translation table in a buffer, given a source
 *                mode and palette, and a destination mode and palette.
 *
 * Parameters:    int source_mode -- source mode
 *                wimp_paletteword *source_palette -- source palette
 *                int dest_mode -- destination mode
 *                wimp_paletteword *dest_palette -- destination palette
 *                void *buffer -- pointer to store for the table.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_select_table (int source_mode, 
                                   wimp_paletteword *source_palette,
                                   int dest_mode,
                                   wimp_paletteword *dest_palette,
                                   void *buffer);


/* ---------------------- colourtran_select_GCOLtable ----------------------
 * Description:   Sets up a list of GCOLs in a buffer, given a source
 *                mode and palette, and a destination mode and palette.
 *
 * Parameters:    int source_mode -- source mode
 *                wimp_paletteword *source_palette -- source palette
 *                int dest_mode -- destination mode
 *                wimp_paletteword *dest_palette -- destination palette
 *                void *buffer -- pointer to store for the list of GCOLs.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_select_GCOLtable (int source_mode, 
                                       wimp_paletteword *source_palette,
                                       int dest_mode,
                                       wimp_paletteword *dest_palette,
                                       void *buffer);


/* ------------------------- colourtran_returnGCOL -------------------------
 * Description:   Informs caller of the closest GCOL in the current mode
 *                to a given palette entry.
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int *gcol -- returned GCOL value.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_returnGCOL (wimp_paletteword entry, int *gcol);


/* ------------------------ colourtran_setGCOL -----------------------------
 * Description:   Informs caller of the closest GCOL in the current mode
 *                to a given palette entry, and also sets the GCOL.
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int fore_back -- set to 0 for foreground, 
 *                                 set to 128 for background
 *                int gcol_in -- GCOL action
 *                int *gcol_out -- returned closest GCOL.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_setGCOL (wimp_paletteword entry, int fore_back,
                              int gcol_in, int *gcol_out);


/* ---------------------- colourtran_return_colournumber -------------------
 * Description:   Informs caller of the closeset colour number to a given
 *                palette entry, in the current mode and palette
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int *col -- returned colour number.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */
 
os_error *colourtran_return_colournumber (wimp_paletteword entry, int *col);


/* --------------------- colourtran_return_GCOLformode ---------------------
 * Description:   Informs caller of the closest GCOL to a given palette
 *                entry, destination mode and destination palette.
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int dest_mode -- destination mode
 *                wimp_paletteword *dest_palette -- destination palette
 *                int *gcol -- returned closest GCOL.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */
 
os_error *colourtran_return_GCOLformode (wimp_paletteword entry,
                                         int dest_mode,
                                         wimp_paletteword *dest_palette,
                                         int *gcol);


/* ------------------ colourtran_return_colourformode ----------------------
 * Description:   Informs caller of the closest colour number to a given
 *                palette entry, destination mode and destination palette.
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int dest_mode -- destination mode
 *                wimp_paletteword *dest_palette -- destination palette
 *                int *col -- returned closest colour number.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_return_colourformode (wimp_paletteword entry,
                                           int dest_mode,
                                           wimp_paletteword *dest_palette,
                                           int *col);


/* ----------------------- colourtran_return_OppGCOL -----------------------
 * Description:   Informs caller of the furthest GCOL in the current mode
 *                from a given palette entry.
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int *gcol -- returned GCOL value.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_return_OppGCOL (wimp_paletteword entry, int *gcol);


/* ------------------------ colourtran_setOppGCOL --------------------------
 * Description:   Informs caller of the furthest GCOL in the current mode
 *                from a given palette entry, and also sets the GCOL.
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int fore_back -- set to 0 for foreground,
 *                                 set to 128 for background
 *                int gcol_in -- GCOL action
 *                int *gcol_out -- returned furthest GCOL.
 * Returns:       possible error condition.
 * Other Info:    none. 
 *
 */

os_error *colourtran_setOppGCOL (wimp_paletteword entry, int fore_back,
                                int gcol_in, int *gcol_out);


/* ---------------------- colourtran_return_Oppcolournumber ----------------
 * Description:   Informs caller of the furthest colour number from a given
 *                palette entry, in the current mode and palette
 *
 * Parameters:    wimp_paletteword -- the palette entry
 *                int *col -- returned colour number.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_return_Oppcolournumber (wimp_paletteword entry,
                                             int *col);


/* --------------------- colourtran_return_OppGCOLformode ------------------
 * Description:   Informs caller of the furthest GCOL from a given palette
 *                entry, destination mode and destination palette.
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int dest_mode -- destination mode
 *                wimp_paletteword *dest_palette -- destination palette
 *                int *gcol -- returned furthest GCOL.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_return_OppGCOLformode (wimp_paletteword entry,
                                            int dest_mode,
                                            wimp_paletteword *dest_palette,
                                            int *gcol);


/* ------------------ colourtran_return_Oppcolourformode -------------------
 * Description:   Informs caller of the furthest colour number from a given
 *                palette entry, destination mode and destination palette.
 *
 * Parameters:    wimp_paletteword entry -- the palette entry
 *                int dest_mode -- destination mode
 *                wimp_paletteword *dest_palette -- destination palette
 *                int *col -- returned furthest colour number.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_return_Oppcolourformode (wimp_paletteword entry,
                                              int dest_mode,
                                              wimp_paletteword *dest_palette,
                                              int *col);


/* ---------------------- colourtran_GCOL_tocolournumber -------------------
 * Description:   Translates a GCOL to a colournumber (assuming 256-colour
 *                mode).
 *
 * Parameters:    int gcol -- the GCOL
 *                int *col -- returned colour number.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_GCOL_tocolournumber (int gcol, int *col);


/* ----------------- colourtran_colournumbertoGCOL -------------------------
 * Description:   Translates a colour number to a GCOL (assuming 256-colour
 *                mode).
 *
 * Parameters:    int col -- the colour number
 *                int *gcol -- the returned GCOL.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_colournumbertoGCOL (int col, int *gcol);


/* ------------------ colourtran_returnfontcolours -------------------------
 * Description:   Informs caller of font colours to match given colours.
 *
 * Parameters:    font *handle -- the font's handle
 *                wimp_paletteword *backgnd -- background palette entry
 *                wimp_paletteword *foregnd -- foreground palette entry
 *                int *max_offset
 * Returns:       possible error condition.
 * Other Info:    Closest approximations to fore/background colours will be
 *                set, and as many intermediate colours as possible (up to
 *                a maximum of *max_offset). Values are returned through
 *                the parameters.
 *
 */

os_error *colourtran_returnfontcolours (font *handle, 
                                        wimp_paletteword *backgnd,
                                        wimp_paletteword *foregnd,
                                        int *max_offset);


/* -------------------- colourtran_setfontcolours --------------------------
 * Description:   Informs caller of font colours to match given colours, and
 *                calls font_setfontcolour() to set them.
 *
 * Parameters:    font *handle -- the font's handle
 *                wimp_paletteword *backgnd -- background palette entry
 *                wimp_paletteword *foregnd -- foreground palette entry
 *                int *max_offset
 * Returns:       possible error condition.
 * Other Info:    Closest approximations to fore/background colours will be
 *                set, and as many intermediate colours as possible (up to
 *                a maximum of *max_offset). Values are returned through
 *                the parameters. Font_setfontcolours() is then called with
 *                these as parameters.
 *
 */
os_error *colourtran_setfontcolours (font *handle,
                                     wimp_paletteword *backgnd,
                                     wimp_paletteword *foregnd,
                                     int *max_offset);


/* ----------------------- colourtran_invalidate_cache ---------------------
 * Description:   To be called when the palette has changed since a call was
 *                last made to a function in this module.
 *
 * Parameters:    void.
 * Returns:       possible error condition.
 * Other Info:    none.
 *
 */

os_error *colourtran_invalidate_cache (void);

#endif

/* end colourtran.h */
