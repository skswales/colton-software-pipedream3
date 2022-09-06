/* msgs.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __msgs_h
#define __msgs_h
#define msgs_TAG_MAX 10
#define msgs_MSG_MAX 255
void msgs_init(void);
char *msgs_lookup(char *tag_and_default);
#endif
