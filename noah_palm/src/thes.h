/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
 #ifndef _THES_H_
#define _THES_H_

#include <PalmOS.h>
#include <PalmCompatibility.h>
#include "common.h"
#include "word_compress.h"
#include "extensible_buffer.h"
#include "display_info.h"

#include "thes_rcp.h"

#include "fs.h"
#include "fs_mem.h"


/* information about the area for displaying definitions:
   start x, start y, number of lines (dy).
   assume that width is full screen (160) */
#define DRAW_DI_Y 0
#define DRAW_DI_LINES 13

#define  THES_CREATOR      'TheS'
#define  ROGET_TYPE        'rget'
#define  THES_PREF_TYPE    'thpr'

#define WORD_MAX_LEN 40

/* Preferences database consists of multiple records.
   Every record contains preferences for a given module
   (ie. separate record for main thes preferences, sepearate
   module for WordNet-specific preferences etc.
   Each record begins with a 4-byte id. If a program doesn't
   know the id, it just ignores the record. This allows a flexible
   upgrades of the program.
 */

/* id for Thes v 1.0 preferences */
/* #define Thes10Pref      0x43212206 */
/* id for Thes v 1.0 per-database preferences */
/* #define ThesDB10Pref    0x43212214 */

#define Thes11Pref      0x43212216

/* structure of the general preferences record */
typedef struct
{
    Boolean                 fDelVfsCacheOnExit;
    StartupAction           startupAction;
    ScrollType              tapScrollType;
    ScrollType              hwButtonScrollType;
    DatabaseStartupAction   dbStartupAction;
    char                    lastDbName[dmDBNameLength];
    char                    lastWord[dmDBNameLength];
} ThesPrefs;

#define HISTORY_ITEMS 5

typedef struct
{
    AbstractFile       *dicts[MAX_DICTS];
    int                dictsCount;
    NoahErrors         err;
    DisplayInfo        *currDispInfo;
    ExtensibleBuffer   *helpDipsBuf;
    long               currentWord;
    long               wordsCount;
    int                firstDispLine;
    long               listItemOffset;
    Boolean            fListDisabled;
    long               prevTopItem;
    int                penUpsToConsume;
    long               prevSelectedWord;
    long               selectedWord;
    char               lastWord[WORD_MAX_LEN];
    int                historyCount;
    long               wordHistory[HISTORY_ITEMS];
    ThesPrefs          prefs;
    Boolean            fFirstRun; /* is this first run or not */
} GlobalData;

#endif
