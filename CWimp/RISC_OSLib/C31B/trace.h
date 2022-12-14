/* trace.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __trace_h
#define __trace_h
#ifndef TRACE
#define TRACE 0
#endif
#if TRACE
 
 extern void tracef (char *, ...);
 #define tracef0 tracef
 #define tracef1 tracef
 #define tracef2 tracef
 #define tracef3 tracef
 #define tracef4 tracef
 
 extern int trace_is_on (void); 
 extern void trace_on (void); 
 extern void trace_off (void); 
#else
 
 
 extern void tracef (char *, ...);
 #define tracef0(a) ((void) 0)
 #define tracef1(a,b) ((void) 0)
 #define tracef2(a,b,c) ((void) 0)
 #define tracef3(a,b,c,d) ((void) 0)
 #define tracef4(a,b,c,d,e) ((void) 0)
 #define trace_is_on() 0
 #define trace_on() ((void) 0)
 #define trace_off() ((void) 0)
#endif
#endif
