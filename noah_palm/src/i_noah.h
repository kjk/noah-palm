/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#ifndef _I_NOAH_H_
#define _I_NOAH_H_

#include "i_noah_rcp.h"

#define  I_NOAH_CREATOR      'STRT'
#define APP_CREATOR I_NOAH_CREATOR

#define MAX_COOKIE_LENGTH 12

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
    
    Char                    cookie[MAX_COOKIE_LENGTH+1];
} iNoahPrefs;

#define HasCookie(prefs) (0<StrLen((prefs).cookie))

typedef iNoahPrefs AppPrefs;

#define appVersionMajor 0
#define appVersionMinor 5

#define appPreferencesVersion 0x0101
#define appPreferencesId    0

#define protocolVersion 1

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
