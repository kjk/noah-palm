/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _SIMPLE_SUPPORT_H_
#define _SIMPLE_SUPPORT_H_

#include <PalmOS.h>
#include "common.h"
#include "display_info.h"
#include "word_compress.h"

typedef struct
{
    // total number of words in the dictionary file
    long    wordsCount;
    // number of records with compressed words
    int     wordsRecsCount;
    // number of records with lengths of compressed definitions
    // (equal to wordsCount)
    int     defsLenRecsCount;
    // number of records with definitions
    int     defsRecsCount;
    int     maxWordLen;
    int     maxDefLen;
    int     hasPosInfoP;
    int     posRecsCount;
} SimpleFirstRecord;

typedef struct
{
    int     recordsCount;
    long    wordsCount;
    int     wordsRecsCount;
    int     defsLenRecsCount;
    int     defsRecsCount;
    int     maxWordLen;
    int     maxDefLen;
    int     posRecsCount;
    int     curDefLen;
    WcInfo  *wci;

    SynsetDef       *synsets;
    unsigned char   *curDefData;
    PackContext     defPackContext;
} SimpleInfo;

void simple_delete(void *data);
void *simple_new(void);
long simple_get_words_count(void *data);
long simple_get_first_matching(void *data, char *word);
char *simple_get_word(void *data, long wordNo);
Err simple_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di);
#endif
