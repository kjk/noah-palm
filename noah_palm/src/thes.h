/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
 #ifndef _THES_H_
#define _THES_H_

#include "thes_rcp.h"

#define  THES_CREATOR      'TheS'
#define  THES_PREF_TYPE    'thpr'
#ifdef DEMO
#define  ROGET_TYPE        'rgde'
#else
#define  ROGET_TYPE        'rget'
#endif

#define APP_CREATOR THES_CREATOR
#define APP_PREF_TYPE THES_PREF_TYPE

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
#define AppPrefId Thes11Pref


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
    char *                  lastDbUsedName;
    DisplayPrefs            displayPrefs;
} ThesPrefs;

typedef ThesPrefs AppPrefs;

#define PREF_REC_MIN_SIZE 4

#define SUPPORT_DATABASE_NAME "Thesaurus_Temp"
#define APP_NAME "Thesaurus"

extern Err AppPerformResidentLookup(Char* term);

#endif
