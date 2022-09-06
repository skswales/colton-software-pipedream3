/* > c.visdelay
 *
 * Title: visdelay.c
 * Purpose: Visual indication of some delay
 * Author: W. Stoye, A.Thompson
 * Requires:
 *   h.os
 * History:
 *   07-Oct-87: WRS: started, in AEM-2
 *   23-Nov-87: WRS: moved to C.
 *    5-May-88: APT: filled in the bodies with Hourglass SWIs
 *   19-Feb-90: SKS: added beginafter()
*/

#include "os.h"

#include "visdelay.h"

#define Hourglass_On         (0x406C0 + os_X)
#define Hourglass_Off        (0x406C1 + os_X)
#define Hourglass_Smash      (0x406C2 + os_X)
#define Hourglass_Start      (0x406C3 + os_X)
#define Hourglass_Percentage (0x406C4 + os_X)
#define Hourglass_LEDs       (0x406C5 + os_X)


extern void
visdelay_begin(void)
{
    os_swi0(Hourglass_On);
}
/* Indicate to the user that there will be a short, computation-intensive
delay. The cursor changes to an hourglass. */


extern void
visdelay_beginafter(int delay)
{
    os_swi1(Hourglass_Start, delay);
}
/* Indicate to the user that there will be a short, computation-intensive
delay. The cursor changes to an hourglass. */


extern void
visdelay_percent(int p)
{
    os_swi1(Hourglass_Percentage, p);
}
/* Indicate to the user that the delay is p/100 complete */


extern void
visdelay_end(void)
{
    os_swi0(Hourglass_Off);
}
/* End the delay indication. */


/* end of visdelay.c */
