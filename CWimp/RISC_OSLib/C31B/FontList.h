/* FontList.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __fontlist_h
#define __fontlist_h
typedef struct fontlist_node
{
 char name[40];
 struct fontlist_node *son;
 struct fontlist_node *brother;
 int flag;
}fontlist_node;
extern fontlist_node *font__tree;
fontlist_node *fontlist_list_all_fonts( BOOL system ); 
void fontlist_free_font_tree( fontlist_node *font_tree );
#endif
