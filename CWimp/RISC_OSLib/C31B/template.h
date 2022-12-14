/* template.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __template_h
#define __template_h
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef struct template__str 
{
 struct template__str *next;
 char * workspace;
 int workspacesize;
 char *font;
 char name[12];
 wimp_wind window;
}template;
template *template_copy (template *from);
BOOL template_readfile (char *name);
 
template *template_find(char *name);
BOOL template_loaded(void);
void template_use_fancyfonts(void);
 
void template_init(void);
wimp_wind *template_syshandle(char *name);
 
#endif
