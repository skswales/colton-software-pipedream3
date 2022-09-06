/*
 * Title: visdelay.h
 * Purpose: Visual indication of some delay
 * Requires:
 *   h.os
 * Author: W. Stoye
 * History:
 *   07-Oct-87: WRS: started, in AEM-2
 *   23-Nov-87: WRS: moved to C.
*/

void visdelay_begin(void);
/* Indicate to the user that there will be a short, computation-intensive
delay. Tyically the cursor might change to some suitable image. */

void visdelay_beginafter(int delay);
/* Start hourglass after delay/100 seconds */

void visdelay_percent(int p);
/* Indicate to the user that the delay is p/100 complete */

void visdelay_end(void);
/* End the delay indication. */

/* end of visdelay.h */
