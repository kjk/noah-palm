/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#ifndef _I_NOAH_H_
#define _I_NOAH_H_

#include "i_noah_rcp.h"

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
    
    Char                    userId[MAX_USER_ID_LENGTH+1];
    Boolean               registrationNeeded;
} iNoahPrefs;

typedef iNoahPrefs AppPrefs;

#define appVersionMajor 0
#define appVersionMinor 5

#define appPreferencesVersion 0x0101
#define appPreferencesId    0

typedef enum AppEvent_
{
    connectionProgressEvent=firstUserEvent,
} AppEvent;

typedef enum MainFormContent_
{
    mainFormShowsAbout,
    mainFormShowsDefinition,
    mainFormShowsMessage,
} MainFormContent;

#endif
