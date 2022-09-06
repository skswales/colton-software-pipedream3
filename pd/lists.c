/* lists.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*********************************************************
*                                                        *
* module that handles lists                              *
* RJM August 1987                                        *
* SKS 23-Jan-89 Split into lists and dialog box handling *
* MRJC March 1989 updated for SPARSE matrix              *
*                                                        *
*********************************************************/

#include "datafmt.h"


/*
function declarations
*/

extern LIST * add_list_entry(list_block **list, intl size, intl *resp);
extern intl   add_to_list(list_block **list, word32 key, const uchar *str, intl *resp);
extern BOOL   delete_from_list(list_block **list, word32 key);
extern void   delete_list(list_block **);
extern intl   duplicate_list(list_block **dst, /*const*/ list_block **src);
extern LIST * first_in_list(list_block **);
extern LIST * next_in_list(list_block **);
extern LIST * search_list(list_block **first_one, word32 target);


/*
internal functions
*/

static list_itemno searchkey(list_block **list, word32 key, list_itemp *itp);


/**************************
*                         *
* add definition to list  *
*                         *
* --out--                 *
*   -ve, NULL: error      *
*     0, NULL: no room    *
*   +ve, xxxx: added      *
*                         *
**************************/

extern LIST *
add_list_entry(list_block **list, intl size, intl *resp)
{
    list_itemp it;
    intl res = 0;

    tracef3("add_list_entry(&%p, %d, &%p)\n", list, size, resp);

    if(resp)
        *resp = res;

    if(!size)
        return(NULL);

    /* allocate new list if pointer is null */
    if(!*list)
        {
        *list = alloc_ptr_using_cache(sizeof(list_block), resp);
        if(!*list)
            return(NULL);

        list_init(*list, 400, 2000);

        list_register(*list);
        }

    do  {
        it = list_createitem(*list,
                             list_numitem(*list),
                             sizeof(LIST) - 1 + size,
                             FALSE);

        if(it)
            {
            if(resp)
                *resp = 1;

            return((LIST *) it->i.inside);
            }

        #if !defined(SPELL_OFF)
        if(resp)
            res = spell_freemem();
        #endif
        }
    while(res > 0);

    if(resp)
        *resp = res;

    return(NULL);
}


/****************************************************
*                                                   *
* add definition to list; the body must be a string *
*                                                   *
* --out--                                           *
*   -ve: error                                      *
*     0: no room                                    *
*   +ve: added (or string empty)                    *
*                                                   *
****************************************************/

extern intl
add_to_list(list_block **list, word32 key, const uchar *str, intl *resp)
{
    LIST *lpt;
    intl res = 1;

    tracef4("add_to_list(&%p, %d, %s, &%p)\n", list, key, trace_string(str), resp);

    if(!str_isblank(str))
        {
        lpt = add_list_entry(list, strlen((char *) str) + 1, resp);

        if(lpt)
            {
            lpt->key = key;
            strcpy((char *) lpt->value, (char *) str);
            }
        elif(!resp)
            res = 0;
        }

    if(resp)
        *resp = res;

    return(res);
}


/**************************************************************************
*                                                                         *
* if there is an entry in the list for the given key, delete it.          *
* It looks through the list until it finds the entry, holding the pointer *
* to the current list element. On finding the entry it makes the pointer  *
* point to the following element and deletes the entry.                   *
*                                                                         *
**************************************************************************/

extern BOOL
delete_from_list(list_block **list, word32 key)
{
    list_itemno item;

    if(*list)
        {
        item = searchkey(list, key, NULL);

        if(item >= 0)
            {
            list_deleteitems(*list, item, (list_itemno) 1);
            return(TRUE);
            }
        }

    return(FALSE);
}


/********************************************
*                                           *
* delete a list altogether and free storage *
*                                           *
********************************************/

extern void
delete_list(list_block **list)
{
    if(*list)
        {
        list_free(*list);
        list_deregister(*list);
        dispose((void **) list);
        }
}


/********************************
*                               *
*  make a copy of a list        *
*  NB. dst must be initialised  *
*                               *
********************************/

extern intl
duplicate_list(list_block **dst, /*const*/ list_block **src)
{
    list_itemno item, nitems;
    list_itemp it;
    LIST *s_lptr, *d_lptr;
    intl res;

    delete_list(dst);

    if(*src)
        {
        item = 0;
        nitems = list_numitem(*src);

        while(item < nitems)
            {
            it = list_gotoitem((void *) *src, item);

            if(it)
                {
                s_lptr = (LIST *) it->i.inside;

                d_lptr = add_list_entry(dst, strlen((char *) s_lptr->value) + 1, &res);
                if(!d_lptr)
                    {
                    delete_list(dst);
                    return(res);
                    }

                /* new item creation may have moved source */
                it = list_gotoitem((void *) *src, item);
                s_lptr = (LIST *) it->i.inside;

                d_lptr->key = s_lptr->key;
                strcpy((char *) d_lptr->value, (char *) s_lptr->value);
                }

            ++item;
            }
        }

    return(1);
}


/***************************************
*                                      *
* initialise list for sequence and     *
* return the first element in the list *
*                                      *
***************************************/

extern LIST *
first_in_list(list_block **list)
{
    list_itemp it;

    if(!*list)
        return(NULL);

    it = list_gotoitem(*list, (list_itemno) 0);

    return(it ? (LIST *) it->i.inside : NULL);
}


/**************************************
*                                     *
* return the next element in the list *
*                                     *
**************************************/

extern LIST *
next_in_list(list_block **list)
{
    list_itemp it;

    if(!*list)
        return(NULL);

    if(list_atitem(*list) >= list_numitem(*list))
        return(NULL);

    it = list_gotoitem(*list, list_atitem(*list) + 1);

    return(it ? (LIST *) it->i.inside : NULL);
}


/**********************************************
*                                             *
* search the given list for the given target, *
* returning a pointer to the element          *
*                                             *
**********************************************/

extern LIST *
search_list(list_block **list, word32 key)
{
    list_itemno item;
    list_itemp  itp;

    if(!*list)
        return(NULL);

    item = searchkey(list, key, &itp);
    if(item < 0)
        return(NULL);

    return((LIST *) itp->i.inside);
}


/********************************************************
*                                                       *
* search list for key; return item number               *
* SKS made it return item pointer iff item number valid *
*                                                       *
********************************************************/

static list_itemno
searchkey(list_block **list, word32 key, list_itemp *itp)
{
    list_itemno i;
    list_itemp it;

    tracef2("searchkey(&%p, %ld)\n", list, key);

    for(i = 0; i < list_numitem(*list); i++)
        {
        it = list_gotoitem(*list, i);
        if(!it)
            continue;

        tracef3("comparing item %d key %d with key %d\n",
                i, ((LIST *) it->i.inside)->key, key);

        if(((LIST *) it->i.inside)->key == key)
            {
            tracef1("key matched at item %d\n", i);
            if(itp)
                *itp = it;
            return(i);
            }
        }

    tracef0("key not found in list\n");

    return(LISTS_ERR_NOTFOUND);
}

/* end of lists.c */
