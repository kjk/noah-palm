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
#define APP_CREATOR I_NOAH_CREATOR

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

#define appVersion 0x0050

#define appVersionMajor (appVersion>>8)
#define appVersionMinor (appVersion & 0xff)

#define appPreferencesVersion 0x0100
#define appPreferencesId    0

typedef enum AppEvent_
{
    lookupProgressEvent=firstUserEvent,
} AppEvent;

typedef enum MainFormContent_
{
    mainFormShowsAbout,
    mainFormShowsDefinition,
    mainFormShowsMessage,
} MainFormContent;

#endif
