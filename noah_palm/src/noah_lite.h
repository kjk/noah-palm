/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _NOAH_LITE_H_
#define _NOAH_LITE_H_

#include "noah_lite_rcp.h"
#include <PalmOS.h>
/* #include <PalmCompatibility.h> */

#include "common.h"
#include "display_info.h"
#include "extensible_buffer.h"
#include "fs.h"
#include "fs_mem.h"

#define  NOAH_LITE_CREATOR     'NoaH'

/* information about the area for displaying definitions:
   start x, start y, number of lines (dy).
   assume that width is full screen (160) */
#define DRAW_DI_Y 0
#define DRAW_DI_LINES 13

typedef struct
{
    char                    lastWord[WORD_MAX_LEN];
} NoahLitePrefs;

typedef struct
{
    AbstractFile  *     dicts[MAX_DICTS];
    int                 dictsCount;
    NoahErrors          err;
    DisplayInfo *       currDispInfo;
    ExtensibleBuffer *  helpDipsBuf;
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
} GlobalData;

#endif
