/*
 * Title:   -> h.res
 * Purpose: Improved access to resources
 * Author:  WStoye,SSwales
 * History:
 *   20-Jan-88 WRS,SKS: created
 *   24-Feb-88 WRS: requirements clause added.
 * Requires:
 *   <stdio.h>
*/

extern void  res_init(const char *progname);
extern int   res_findname(const char *leafname, char *buf /*out*/);
extern FILE *res_openfile(const char *leafname, const char *mode);


/* end of res.h */
