/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _NOAH_PRO_H_
#define _NOAH_PRO_H_

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "common.h"

#include "extensible_buffer.h"
#include "display_info.h"

#include "fs.h"

#include "fs_mem.h"

#ifdef FS_VFS
#include "fs_vfs.h"
#endif

#define  NOAH_PRO_CREATOR    'NoAH'
#define  NOAH_PREF_TYPE      'pref'

/* information about the area for displaying definitions:
   start x, start y, number of lines (dy).
   assume that width is full screen (160) */
#define DRAW_DI_Y 0
#define DRAW_DI_LINES 13

#if 0
// old ids for preference records
#define Noah10Pref      0x43212205
#endif

/* id for Noah Pro >v2.0 preferences */
#define Noah21Pref      0x43212208

/* id for Noah Pro v 1.0 per-database preferences, no longer used */
/* #define NoahDB10Pref    0x43212213 */

/* Preferences database consists of multiple records.
   Every record contains preferences for a given module
   (ie. separate record for main Noah preferences, sepearate
   module for WordNet-specific preferences etc.
   Each record begins with a 4-byte id. If a program doesn't
   know the id, it just ignores the record. This allows a flexible
   upgrades of the program.
 */

/* structure of the general preferences record */
typedef struct
{
    Boolean                 fDelVfsCacheOnExit;
    StartupAction           startupAction;
    ScrollType              tapScrollType;
    ScrollType              hwButtonScrollType;
    DatabaseStartupAction   dbStartupAction;
    char                    lastWord[WORD_MAX_LEN];
    char *                  lastDbUsedName;
} NoahPrefs;

/* Global data for Noah Pro */
typedef struct
{
    AbstractFile *     dicts[MAX_DICTS];
    int                dictsCount;
    NoahErrors         err;
    DisplayInfo *      currDispInfo;
    ExtensibleBuffer * helpDipsBuf;
    long               currentWord;
    long               wordsCount;
    int                firstDispLine;
    long               listItemOffset;
    long               prevTopItem;
    int                penUpsToConsume;
    long               prevSelectedWord;
    long               selectedWord;
    Boolean            prefsPresentP;
    char               lastWord[WORD_MAX_LEN];
    int                historyCount;
    long               wordHistory[HISTORY_ITEMS];
    NoahPrefs          prefs;
    Boolean            fFirstRun; /* is this first run or not */
    long               ticksEventTimeout;
#ifdef DEBUG
    long               currentStressWord;
#endif
} GlobalData;

#endif
