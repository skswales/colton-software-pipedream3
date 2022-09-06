/* stubs.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       stubs.h - provides stubs for XX_fn() for overlays
 * Author:      RJM 10-April-1989
*/

#ifndef __program__stubs_h
#define __program__stubs_h

#if defined(OVERLAYS)

#if defined(HELP_OVERLAY)
extern void overlay_Help_fn(void);
#endif

#if defined(SPELL_OVERLAY)
extern void overlay_AutoSpellCheck_fn(void);
extern void overlay_CheckDocument_fn(void);
extern void overlay_BrowseDictionary_fn(void);
extern void overlay_Anagrams_fn(void);
extern void overlay_Subgrams_fn(void);
extern void overlay_DumpDictionary_fn(void);
extern void overlay_MergeFileWithDict_fn(void);
extern void overlay_InsertWordInDict_fn(void);
extern void overlay_DeleteWordFromDict_fn(void);
extern void overlay_LockDictionary_fn(void);
extern void overlay_UnlockDictionary_fn(void);
extern void overlay_CreateUserDict_fn(void);
extern void overlay_OpenUserDict_fn(void);
extern void overlay_CloseUserDict_fn(void);
extern void overlay_DisplayOpenDicts_fn(void);
extern void overlay_PackUserDict_fn(void);
#endif

#if defined(PRINT_OVERLAY)
extern void overlay_Print_fn(void);
extern void overlay_PageLayout_fn(void);
extern void overlay_PrinterConfig_fn(void);
extern void overlay_MicrospacePitch_fn(void);
extern void overlay_SetMacroParm_fn(void);
extern void overlay_PrintStatus_fn(void);
#endif

#if defined(SEARCH_OVERLAY)
extern void overlay_Search_fn(void);
extern void overlay_Replace_fn(void);
extern void overlay_NextMatch_fn(void);
extern void overlay_PrevMatch_fn(void);
#endif

#if defined(FILES_OVERLAY)
extern void overlay_LoadFile_fn(void);
extern void overlay_NameFile_fn(void);
extern void overlay_SaveFile_fn(void);
extern void overlay_SaveFileAsIs_fn(void);
extern void overlay_SaveInitFile_fn(void);
extern void overlay_FirstFile_fn(void);
extern void overlay_NextFile_fn(void);
extern void overlay_PrevFile_fn(void);
extern void overlay_LastFile_fn(void);
#endif

#else   /* OVERLAYS */

#define Help_fn                 overlay_Help_fn

#define AutoSpellCheck_fn       overlay_AutoSpellCheck_fn
#define CheckDocument_fn        overlay_CheckDocument_fn
#define BrowseDictionary_fn     overlay_BrowseDictionary_fn
#define Anagrams_fn             overlay_Anagrams_fn
#define Subgrams_fn             overlay_Subgrams_fn
#define DumpDictionary_fn       overlay_DumpDictionary_fn
#define MergeFileWithDict_fn    overlay_MergeFileWithDict_fn
#define InsertWordInDict_fn     overlay_InsertWordInDict_fn
#define DeleteWordFromDict_fn   overlay_DeleteWordFromDict_fn
#define LockDictionary_fn       overlay_LockDictionary_fn
#define UnlockDictionary_fn     overlay_UnlockDictionary_fn
#define CreateUserDict_fn       overlay_CreateUserDict_fn
#define OpenUserDict_fn         overlay_OpenUserDict_fn
#define CloseUserDict_fn        overlay_CloseUserDict_fn
#define DisplayOpenDicts_fn     overlay_DisplayOpenDicts_fn
#define PackUserDict_fn         overlay_PackUserDict_fn

#define Print_fn                overlay_Print_fn
#define PageLayout_fn           overlay_PageLayout_fn
#define PrinterConfig_fn        overlay_PrinterConfig_fn
#define MicrospacePitch_fn      overlay_MicrospacePitch_fn
#define SetMacroParm_fn         overlay_SetMacroParm_fn
#define PrintStatus_fn          overlay_PrintStatus_fn

#define Search_fn               overlay_Search_fn
#define Replace_fn              overlay_Replace_fn
#define NextMatch_fn            overlay_NextMatch_fn
#define PrevMatch_fn            overlay_PrevMatch_fn

#define LoadFile_fn             overlay_LoadFile_fn
#define NameFile_fn             overlay_NameFile_fn
#define SaveFile_fn             overlay_SaveFile_fn
#define SaveFileAsIs_fn         overlay_SaveFileAsIs_fn
#define SaveInitFile_fn         overlay_SaveInitFile_fn
#define FirstFile_fn            overlay_FirstFile_fn
#define NextFile_fn             overlay_NextFile_fn
#define PrevFile_fn             overlay_PrevFile_fn
#define LastFile_fn             overlay_LastFile_fn

#endif /* OVERLAYS */

#endif /* __program__stubs_h */

/* end of stubs.h */
