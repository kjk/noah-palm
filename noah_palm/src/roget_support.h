/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _ROGET_SUPPORT_H_
#define _ROGET_SUPPORT_H_

#include <PalmOS.h>
#include "thes.h"
#include "word_compress.h"
#include "display_info.h"

#ifndef DEMO
#define  ROGET_TYPE      'rget'
#endif

#ifdef DEMO
#define  ROGET_TYPE      'rgde'
#endif

typedef struct
{
    long    wordsCount;
    int     wordsRecordsCount;
    int     synsetsCount;
    int     maxWordLen;
    int     wordsInSynRecs;
} RogetFirstRecord;

#define MAX_WORDS_IN_SYNSET 40

typedef struct
{
    int     recordsCount;
    long    wordsCount;
    int     wordsRecordsCount;
    int     synsetsCount;
    int     maxWordLen;
    int     wordsInSynRecs;
    int     wordCacheInfoRec;
    int     wordPackDataRec;
    int     firstWordsRec;
    int     wordsInSynFirstRec;
    int     wordsInSynCountRec;
    int     curDefLen;
    int     synPosRec;

    WcInfo  *wci;
    PackContext defPackContext;
} RogetInfo;

void setRogetAsCurrentDict(void);

#endif
