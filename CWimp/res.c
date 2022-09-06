/* Title:   -> c.res
 * Purpose: Improved access to resources
 * Author:  WStoye,SSwales
 * History:
 *   20-Jan-88 WRS,SKS: created
 *   21-Jan-88 SKS fixed tracing
 *   18-Feb-88 WRS
 *               problems with debuggers
 *               problems with Arthur-2, without !Run.
 *               use of new trace facilities
 *   17-Mar-88 WRS sort out BOOL mess.
 *   28-Mar-88 WRS radically simplified by wimprun stuff.
 *   07-Mar-89 SKS quick hack
*/

#define BOOL int
#define TRUE 1
#define FALSE 0


#include <stdio.h>
#include <string.h>


#include "res.h" /* Ensure consistent interface */


static const char *res__programname;


extern BOOL
res_findname(const char *leafname, char *buf /*out*/)
{
    sprintf(buf, "<%s$Dir>.%s", res__programname, leafname);
    return(TRUE);
}


extern void
res_init(const char *name) /* Started up with the program name */
{
    res__programname = name;
}


extern FILE *
res_openfile(const char *leafname, const char *mode)
{
    char tempname[256];
    res_findname(leafname, tempname);
    return(fopen(tempname, mode));
}


/* end of res.c */
