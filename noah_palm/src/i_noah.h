/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#ifndef _I_NOAH_H_
#define _I_NOAH_H_

#include "i_noah_rcp.h"

#define  I_NOAH_CREATOR      'iNoA'
#define  I_NOAH_PREF_TYPE    'inpr'

#define  APP_CREATOR   I_NOAH_CREATOR
#define  APP_PREF_TYPE I_NOAH_PREF_TYPE

#define MAX_COOKIE_LENGTH 64

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
    Boolean                 dontShowPronunciation;
} iNoahPrefs;

#define HasCookie(prefs) (StrLen((prefs).cookie)>0)

typedef iNoahPrefs AppPrefs;

#define AppPrefId     0x341272a9

#define PREF_REC_MIN_SIZE 4

#define     PROTOCOL_VERSION "1"
#define     CLIENT_VERSION   "0.5"

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
