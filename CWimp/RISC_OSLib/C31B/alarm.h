/* alarm.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __alarm_h
#define __alarm_h
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef void (*alarm_handler)(int called_at, void *handle);
void alarm_init(void);
int alarm_timenow(void);
int alarm_timedifference(int t1, int t2);
void alarm_set(int at, alarm_handler proc, void *handle);
void alarm_remove(int at, void *handle);
void alarm_removeall(void *handle);
BOOL alarm_anypending(void *handle);
BOOL alarm_next(int *result);
void alarm_callnext(void);
#endif
