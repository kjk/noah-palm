/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _ROGET_SUPPORT_H_
#define _ROGET_SUPPORT_H_

#include "word_compress.h"
#include "extensible_buffer.h"
#include "display_info.h"

typedef struct
{
    long    wordsCount;
    int     wordsRecordsCount;
    int     synsetsCount;
    int     maxWordLen;
    int     wordsInSynRecs;
} RogetFirstRecord;

#define MAX_WORDS_IN_SYNSET 40

typedef struct _RogetInfo
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

    WcInfo *    wci;
    PackContext defPackContext;
    
    AbstractFile* file;
    ExtensibleBuffer buffer;
    
} RogetInfo;

RogetInfo * RogetNew(AbstractFile* file);
void        RogetDelete(RogetInfo* info);
long        RogetGetWordsCount(RogetInfo *info);
long        RogetGetFirstMatching(RogetInfo *info, char *word);
char *      RogetGetWord(RogetInfo *info, long wordNo);
Err         RogetGetDisplayInfo(RogetInfo *info, long wordNo, int dx, DisplayInfo * di);
#endif
