/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
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

#define MAX_DBS  4

#define  NOAH_LITE_CREATOR     'NoaH'


/* information about the area for displaying definitions:
   start x, start y, number of lines (dy).
   assume that width is full screen (160) */
#define DRAW_DI_Y 17
#define DRAW_DI_LINES 13

typedef struct
{
    NoahErrors          err;
    void               *dictData;
    int                currentDb;
    Dict               currentDict;
    Vfs                *currVfs;
    MemData            dm;
    int                dbsCount;
    DBInfo             foundDbs[MAX_DBS];
    DisplayInfo        *currDispInfo;
    ExtensibleBuffer   *helpDipsBuf;
    long               currentWord;
    long               wordsCount;
    int                firstDispLine;
    int                osVersion;
    long               listItemOffset;
    long               prevTopItem;
    long               maxListItems;
    int                penUpsToConsume;
    long               prevSelectedWord;
    long               selectedWord;
    Boolean            memInitedP;
    Boolean            memWorksP;
    Vfs                memVfs;
    MemData            memVfsData;
} GlobalData;

#endif
