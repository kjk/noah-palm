/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#ifndef _I_NOAH_H_
#define _I_NOAH_H_

#include "i_noah_rcp.h"

/**
 * @todo Tweak iNoah settings.
 */
#define  I_NOAH_CREATOR      'STRT'
#define  I_NOAH_PREF_TYPE    'thpr'

#define APP_CREATOR I_NOAH_CREATOR
#define APP_PREF_TYPE I_NOAH_PREF_TYPE

#define AppPrefId 0


/* structure of the general preferences record */
typedef struct
{
    StartupAction           startupAction;
    ScrollType              hwButtonScrollType;
    ScrollType              navButtonScrollType;
    char                    lastWord[WORD_MAX_LEN];
    DisplayPrefs            displayPrefs;

    // how do we sort bookmarks
    BookmarkSortType        bookmarksSortType;
} iNoahPrefs;

typedef iNoahPrefs AppPrefs;

#define PREF_REC_MIN_SIZE 4

#define SUPPORT_DATABASE_NAME "iNoah_Temp"
#define APP_NAME "iNoah"

typedef enum AppEvent_
{
    lookupProgressEvent=firstUserEvent,
} AppEvent;

#endif
