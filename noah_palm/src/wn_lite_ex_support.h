/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _WN_LITE_EX_SUPPORT_H_
#define _WN_LITE_EX_SUPPORT_H_

#include "common.h"
#include "word_compress.h"
#include "display_info.h"


typedef struct
{
    long    wordsCount;
    long    synsetsCount;
    int     synsetDefLenRecordsCount;
    int     wordsInfoRecordsCount;
    int     wordsRecordsCount;
    int     synsetDefRecordsCount;
    int     maxWordLen;
    int     maxDefLen;
    int     maxComprDefLen;
    int     maxSynsetsForWord;
    long    firstLemmaInWordInfoRec;
} WnLiteFirstRecord;

typedef struct
{
    int     recordsCount;
    long    wordsCount;
    long    synsetsCount;
    int     wordsRecordsCount;
    int     wordsInfoRecordsCount;
    int     synsetDefLenRecordsCount;
    int     synsetDefRecordsCount;
    int     maxWordLen;
    int     maxDefLen;
    int     maxComprDefLen;
    int     maxSynsetsForWord;
    long    *firstLemmaInWordInfoRec;
    int     curDefLen;
    WcInfo  *wci;
    SynsetDef       *synsets;
    unsigned char   *curDefData;
    PackContext     defPackContext;
}WnLiteInfo;

void    *wnlex_new(void);
void    wnlex_delete(void *data);
long    wnlex_get_words_count(void *data);
long    wnlex_get_first_matching(void *data, char *word);
char    *wnlex_get_word(void *data, long wordNo);
Err     wnlex_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di);
#endif
