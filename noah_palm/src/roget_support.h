/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _ROGET_SUPPORT_H_
#define _ROGET_SUPPORT_H_

#include "word_compress.h"
#include "display_info.h"

#ifdef DEMO
#define  ROGET_TYPE      'rgde'
#else
#define  ROGET_TYPE      'rget'
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

struct RogetInfo
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

    WcInfo      *wci;
    PackContext defPackContext;
};

struct RogetInfo   *RogetNew(void);
void        RogetDelete(struct RogetInfo *info);
long        RogetGetWordsCount(struct RogetInfo *info);
long        RogetGetFirstMatching(struct RogetInfo *info, char *word);
char        *RogetGetWord(struct RogetInfo *info, long wordNo);
Err         RogetGetDisplayInfo(struct RogetInfo *info, long wordNo, int dx, DisplayInfo * di);
#endif
