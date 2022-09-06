/* > h.baricon */

/* Title:   baricon.h
 * Purpose: Support placing of an icon on the icon bar
 * History:
 *   21-Mar-88 WRS extracted from ArcEdit
 *   02-Mar-88 SKS radical rehack to make it useful
 */


#ifndef __wimplib__baricon_h
#define __wimplib__baricon_h


typedef struct baricon__str *baricon; /* Abstraction */


extern baricon baricon_new(const char *spritename, const char *str);

#if defined(BARICON_EXTRAS)
extern void baricon_dispose(baricon *bip);
extern void baricon_settext(baricon bi, const char *str);
extern int  baricon_syshandle(baricon bi);
#endif


#endif


/* end of baricon.h */
