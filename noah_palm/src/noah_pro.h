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
//#define Noah10Pref      0x43212205
// id for Noah Pro >v2.0 preferences
//#define Noah21Pref      0x43212208

// id for Noah Pro >=v3.0 preferences
// #define Noah30Pref      0x43212209

// id for Noah Pro >=v3.1 preferences
#define AppPrefId       0x4321220a

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
    StartupAction           startupAction;
    ScrollType              hwButtonScrollType;
    ScrollType              navButtonScrollType;
    DatabaseStartupAction   dbStartupAction;
    Boolean                 fResidentModeEnabled;
    char                    lastWord[WORD_MAX_LEN];
    char *                  lastDbUsedName;
    DisplayPrefs            displayPrefs;
    // how do we sort bookmarks
    BookmarkSortType        bookmarksSortType;
} NoahPrefs;

typedef NoahPrefs AppPrefs;

#define PREF_REC_MIN_SIZE 4+5

#define SUPPORT_DATABASE_NAME "NoahPro_Temp"
#define APP_NAME "Noah Pro"

extern Err AppPerformResidentLookup(Char* term);

#endif
