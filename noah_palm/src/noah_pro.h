/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _NOAH_PRO_H_
#define _NOAH_PRO_H_

#define  NOAH_PRO_CREATOR    'NoAH'
#define  NOAH_PREF_TYPE      'pref'

#define APP_CREATOR NOAH_PRO_CREATOR
#define APP_PREF_TYPE NOAH_PREF_TYPE

// old ids for preference records
// #define Noah10Pref      0x43212205

/* id for Noah Pro >v2.0 preferences */
#define Noah21Pref      0x43212208

#define AppPrefId Noah21Pref

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

#include "better_formatting.h"
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
    DisplayPrefs            displayPrefs;
} NoahPrefs;

typedef NoahPrefs AppPrefs;

/* Global data for Noah Pro */
/*
typedef struct
{
    AbstractFile *     dicts[MAX_DICTS];
    int                dictsCount;
    NoahErrors         err;
    DisplayInfo *      currDispInfo;
    ExtensibleBuffer * helpDispBuf;
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
    char *             wordHistory[HISTORY_ITEMS];
    NoahPrefs          prefs;
    Boolean            fFirstRun; // is this first run or not 
    long               ticksEventTimeout;
#ifdef DEBUG
    long               currentStressWord;
#endif

#include "common_global_data.h"
  
} GlobalData;

extern GlobalData gd;
*/

#define PREF_REC_MIN_SIZE 4+5

#define SUPPORT_DATABASE_NAME "NoahPro_Temp"
#define APP_NAME "Noah Pro"

extern Err AppPerformResidentLookup(Char* term);

#endif
