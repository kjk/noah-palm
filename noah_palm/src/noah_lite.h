/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _NOAH_LITE_H_
#define _NOAH_LITE_H_

#include "noah_lite_rcp.h"

#define  NOAH_LITE_CREATOR     'NoaH'
#define APP_CREATOR NOAH_LITE_CREATOR


typedef struct
{
    char                lastWord[WORD_MAX_LEN];
} NoahLitePrefs;

typedef NoahLitePrefs AppPrefs;

#define APP_NAME "Noah Lite"


/*
typedef struct
{
    AbstractFile  *     dicts[MAX_DICTS];
    int                 dictsCount;
    NoahErrors          err;
    DisplayInfo *       currDispInfo;
    ExtensibleBuffer *  helpDispBuf;
    long                currentWord;
    long                wordsCount;
    int                 firstDispLine;
    long                listItemOffset;
    long                prevTopItem;
    int                 penUpsToConsume;
    long                prevSelectedWord;
    long                selectedWord;
    char                lastWord[WORD_MAX_LEN];
    long                ticksEventTimeout;
    NoahLitePrefs       prefs;
#ifdef DEBUG
    long                currentStressWord;
#endif

#include "common_global_data.h"

} GlobalData;
*/

#endif
