/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _WN_SUPPORT_H_
#define _WN_SUPPORT_H_

#include "common.h"
#include "word_compress.h"

typedef struct
{
    long    synsetNo;
    long    wordsCount;
} SynWordCountCache;

typedef struct
{
    long            synsetsCount;
    int             firstWordsNumRec;
    int             curWordsNumRec;
    int             firstSynsetInfoRec;
    int             curSynsetInfoRec;
    unsigned char * wordsNumData;
    unsigned char * synsetData;
    unsigned char * synsetDataStart;
    long            synsetDataLeft;
    long            wordsNumDataLeft;
    int             bytesPerWordNum;
    long            currSynset;
    int             currWord;
    long            currSynsetWordsNum;

    SynWordCountCache *swcc;
} SynsetsInfo;

typedef struct
{
    long    wordsCount;
    long    synsetsCount;
    int     wordsRecsCount;
    int     synsetsInfoRecsCount;
    int     wordsNumRecsCount;
    int     defsLenRecsCount;
    int     defsRecsCount;
    int     maxWordLen;
    int     maxDefLen;
    int     maxComprDefLen;
    int     bytesPerWordNum;
    int     maxWordsPerSynset;
} WnFirstRecord;

typedef struct _WnInfo
{
    int     recordsCount;
    long    wordsCount;
    long    synsetsCount;
    int     wordsRecsCount;
    int     synsetsInfoRecsCount;
    int     wordsNumRecsCount;
    int     defsLenRecsCount;
    int     defsRecsCount;
    int     maxWordLen;
    int     maxDefLen;
    int     maxComprDefLen;
    int     bytesPerWordNum;
    int     synCountRec;
    Boolean fastP;
    int     cacheEntries;
    int     curDefLen;
    WcInfo *wci;

    SynsetDef *     synsets;
    unsigned char * curDefData;

    PackContext     defPackContext;
    SynsetsInfo *   si;
    
    AbstractFile* file;
    ExtensibleBuffer bufferTemp;
    ExtensibleBuffer buffer;
    DisplayInfo displayInfo;
    
} WnInfo;

void *  wn_new(AbstractFile* file);
void    wn_delete(void *data);
long    wn_get_words_count(void *data);
long    wn_get_first_matching(void *data, char *word);
char *  wn_get_word(void *data, long wordNo);
Err     wn_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di);

#endif
