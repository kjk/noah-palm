/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "roget_support.h"
#include "common.h"

/* temporary buffer for keeping the data gathered during unpacking */
ExtensibleBuffer g_buf = { 0 };

static char *GetPosTxt(int pos, int type)
{
    return GetNthTxt(type * 4 + pos, "(noun)\0(verb)\0(adj.)\0(adv.)\0N.\0V.\0Adj.\0Adv.\0Noun\0Verb\0Adjective\0Adverb\0");
}

struct RogetInfo *RogetNew(void)
{
    struct RogetInfo *info = NULL;
    RogetFirstRecord *firstRecord;
#ifdef DEBUG
    long recSize;
#endif
    Assert( GetCurrentFile() );

    info = (struct RogetInfo *) new_malloc_zero(sizeof(struct RogetInfo));
    if (NULL == info) goto Error;

    info->recordsCount = CurrFileGetRecordsCount();
    if ( info->recordsCount > 100 )
    {
        // TODO: report corruption
        return NULL;
    }

    firstRecord = (RogetFirstRecord *) CurrFileLockRecord(0);

    if ( NULL == firstRecord )
    {
        LogG( "RogetNew(): CurrFileLockRecord(0) returned NULL" );
        goto Error;
    }

    info->wordsCount = firstRecord->wordsCount;
    info->wordsRecordsCount = firstRecord->wordsRecordsCount;
    info->synsetsCount = firstRecord->synsetsCount;
    info->maxWordLen = firstRecord->maxWordLen;
    info->wordsInSynRecs = firstRecord->wordsInSynRecs;
    info->wordCacheInfoRec = 1;
    info->wordPackDataRec = 2;
    info->firstWordsRec = 3;
    info->wordsInSynFirstRec = info->firstWordsRec + info->wordsRecordsCount;
    info->wordsInSynCountRec = info->wordsInSynFirstRec + info->wordsInSynRecs;
    info->synPosRec = info->wordsInSynCountRec + 1;

    info->wci = wcInit(info->wordsCount, info->wordPackDataRec,
                        info->wordCacheInfoRec,
                        info->firstWordsRec,
                        info->wordsRecordsCount, info->maxWordLen);

    if (NULL == info->wci)
    {
        CurrFileUnlockRecord(0);
        goto Error;
    }
#ifdef DEBUG
    recSize = CurrFileGetRecordSize(info->wordsInSynCountRec);
    Assert(recSize == info->synsetsCount);
#endif

    CurrFileUnlockRecord(0);
    return info;
Error:
    LogG( "RogetNew() failed" );
    if (info)  RogetDelete(info);
    return NULL;
}


void RogetDelete(struct RogetInfo *info)
{
    ebufFreeData(&g_buf);

    if (info)
    {
        if (info->wci)
            wcFree(info->wci);
        new_free(info);
    }
}

long RogetGetWordsCount(struct RogetInfo *info)
{
    return info->wordsCount;
}

long RogetGetFirstMatching(struct RogetInfo *info, char *word)
{
    return wcGetFirstMatching(info->wci, word);
}

char *RogetGetWord(struct RogetInfo *info, long wordNo)
{
    Assert(wordNo < info->wordsCount);

    if (wordNo >= info->wordsCount)
        return NULL;
    return wcGetWord(info->wci, wordNo);
}

Err RogetGetDisplayInfo(struct RogetInfo *info, long wordNo, int dx, DisplayInfo * di)
{
    long wordCount = 0;
    unsigned char *posRecData;
    UInt16 *wordNums;
    long i, j;
    long thisWordNo;
    char *word;
    char *posTxt;
    char *rawTxt;
    int pos;

    unsigned char *words_in_syn_count_data = NULL;
    long words_left;
    int wordsInSynRec;
    int word_present_p;

    Assert(info);
    Assert((wordNo >= 0) && (wordNo < info->wordsCount));
    Assert(dx > 0);
    Assert(di);

    ebufReset(&g_buf);

    posRecData = (unsigned char *) CurrFileLockRecord(info->synPosRec);
    words_in_syn_count_data = (unsigned char *) CurrFileLockRecord(info->wordsInSynCountRec);
    wordsInSynRec = info->wordsInSynFirstRec;
    wordNums = (UInt16 *) CurrFileLockRecord(wordsInSynRec);
    words_left = CurrFileGetRecordSize(wordsInSynRec) / 2;

    for (i = 0; i < info->synsetsCount; i++)
    {
        word_present_p = 0;
        wordCount = (int) words_in_syn_count_data[0];
        wordCount += 2;
        ++words_in_syn_count_data;

        for (j = 0; j < wordCount; j++)
        {
            thisWordNo = (UInt32) wordNums[j];
            if (wordNo == thisWordNo)
            {
                word_present_p = 1;
            }
        }

        if (word_present_p)
        {
            ebufAddChar(&g_buf, 149);
            ebufAddChar(&g_buf, ' ');

            pos = posRecData[i / 4];

            pos = (pos >> (2 * (i % 4))) & 0x3;

            posTxt = GetPosTxt(pos, 0);
            ebufAddStr(&g_buf, posTxt);
            ebufAddChar(&g_buf, ' ');

            for (j = 0; j < wordCount; j++)
            {
                thisWordNo = (UInt32) wordNums[j];
                word = RogetGetWord(info, thisWordNo);
                ebufAddStr(&g_buf, word);
                if (j != (wordCount - 1))
                {
                    ebufAddStr(&g_buf, ", ");
                }
            }
            ebufAddChar(&g_buf, '\n');
        }
        wordNums += wordCount;
        words_left -= wordCount;
        Assert(words_left >= 0);
        if (0 == words_left)
        {
            CurrFileUnlockRecord(wordsInSynRec);
            ++wordsInSynRec;
            wordNums = (UInt16 *) CurrFileLockRecord(wordsInSynRec);
            words_left = CurrFileGetRecordSize(wordsInSynRec) / 2;
        }
    }

    CurrFileUnlockRecord(info->wordsInSynCountRec);
    CurrFileUnlockRecord(info->wordsInSynCountRec);
    CurrFileUnlockRecord(info->synPosRec);

    ebufAddChar(&g_buf, '\0');
    ebufWrapBigLines(&g_buf);

    rawTxt = ebufGetDataPointer(&g_buf);
    diSetRawTxt(di, rawTxt);
    return 0;
}

