/*
  Copyright (C) 2000,2001, 2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _WN_SUPPORT_H_
#define _WN_SUPPORT_H_

#include <PalmOS.h>
#include "noah_pro.h"
#include "word_compress.h"
#include "display_info.h"

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
    unsigned char   *wordsNumData;
    unsigned char   *synsetData;
    unsigned char   *synsetDataStart;
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

typedef struct
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
    int     maxWordsPerSynset;
    int     synCountRec;
    Boolean fastP;
    int     cacheEntries;
    int     curDefLen;
    WcInfo  *wci;

    SynsetDef       *synsets;
    unsigned char   *curDefData;

    PackContext     defPackContext;
    SynsetsInfo     *si;
}WnInfo;

void setWnproAsCurrentDict(void);

#endif
