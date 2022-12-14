/* colourtran.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __colourtran_h
#define __colourtran_h
#ifndef __os_h
#include "os.h"
#endif
#ifndef __wimp_h
#include "wimp.h"
#endif
#ifndef __font_h
#include "font.h"
#endif
os_error *colourtran_select_table (int source_mode, 
 wimp_paletteword *source_palette,
 int dest_mode,
 wimp_paletteword *dest_palette,
 void *buffer);
os_error *colourtran_select_GCOLtable (int source_mode, 
 wimp_paletteword *source_palette,
 int dest_mode,
 wimp_paletteword *dest_palette,
 void *buffer);
os_error *colourtran_returnGCOL (wimp_paletteword entry, int *gcol);
os_error *colourtran_setGCOL (wimp_paletteword entry, int fore_back,
 int gcol_in, int *gcol_out);
 
os_error *colourtran_return_colournumber (wimp_paletteword entry, int *col);
 
os_error *colourtran_return_GCOLformode (wimp_paletteword entry,
 int dest_mode,
 wimp_paletteword *dest_palette,
 int *gcol);
os_error *colourtran_return_colourformode (wimp_paletteword entry,
 int dest_mode,
 wimp_paletteword *dest_palette,
 int *col);
os_error *colourtran_return_OppGCOL (wimp_paletteword entry, int *gcol);
os_error *colourtran_setOppGCOL (wimp_paletteword entry, int fore_back,
 int gcol_in, int *gcol_out);
os_error *colourtran_return_Oppcolournumber (wimp_paletteword entry,
 int *col);
os_error *colourtran_return_OppGCOLformode (wimp_paletteword entry,
 int dest_mode,
 wimp_paletteword *dest_palette,
 int *gcol);
os_error *colourtran_return_Oppcolourformode (wimp_paletteword entry,
 int dest_mode,
 wimp_paletteword *dest_palette,
 int *col);
os_error *colourtran_GCOL_tocolournumber (int gcol, int *col);
os_error *colourtran_colournumbertoGCOL (int col, int *gcol);
os_error *colourtran_returnfontcolours (font *handle, 
 wimp_paletteword *backgnd,
 wimp_paletteword *foregnd,
 int *max_offset);
os_error *colourtran_setfontcolours (font *handle,
 wimp_paletteword *backgnd,
 wimp_paletteword *foregnd,
 int *max_offset);
os_error *colourtran_invalidate_cache (void);
#endif
