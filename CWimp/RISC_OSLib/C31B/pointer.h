/* pointer.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __pointer_h
#define __pointer_h
#ifndef __sprite_h
#include "sprite.h"
#endif
#ifndef __os_h
#include "os.h"
#endif
#define wimp_spritearea ((sprite_area *) -1)
os_error *pointer_set_shape(sprite_area *, sprite_id *, int, int) ;
void pointer_reset_shape(void) ;
#endif
