/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "roget_support.h"
#include "common.h"

/* temporary buffer for keeping the data gathered during unpacking */
//ExtensibleBuffer g_buf = { 0 };

static char *GetPosTxt(int pos, int type)
{
    return GetNthTxt(type * 4 + pos, "(noun)\0(verb)\0(adj.)\0(adv.)\0N.\0V.\0Adj.\0Adv.\0Noun\0Verb\0Adjective\0Adverb\0");
}

RogetInfo * RogetNew(AbstractFile* file)
{
    RogetInfo* info = NULL;
    RogetFirstRecord* firstRecord;
#ifdef DEBUG
    long recSize;
#endif
    Assert( file );

    info = (RogetInfo *) new_malloc_zero(sizeof(RogetInfo));
    if (NULL == info) goto Error;
    ebufInit(&info->buffer, 0);
    info->file=file;
    info->recordsCount = fsGetRecordsCount(file);
    if ( info->recordsCount > 100 )
    {
        // TODO: report corruption
        return NULL;
    }

    firstRecord=(RogetFirstRecord*)fsLockRecord(file, 0);

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

    info->wci = wcInit(file, info->wordsCount, info->wordPackDataRec,
                        info->wordCacheInfoRec,
                        info->firstWordsRec,
                        info->wordsRecordsCount, info->maxWordLen);

    if (NULL == info->wci)
    {
        fsUnlockRecord(file, 0);
        goto Error;
    }
#ifdef DEBUG
    recSize = fsGetRecordSize(file, info->wordsInSynCountRec);
    Assert(recSize == info->synsetsCount);
#endif

    fsUnlockRecord(file, 0);
    return info;
Error:
    LogG( "RogetNew() failed" );
    if (info)  RogetDelete(info);
    return NULL;
}


void RogetDelete(RogetInfo *info)
{
    if (info)
    {
        if (info->wci)
            wcFree(info->wci);
        ebufFreeData(&info->buffer); 
        new_free(info);
    }
}

long RogetGetWordsCount(RogetInfo *info)
{
    return info->wordsCount;
}

long RogetGetFirstMatching(RogetInfo *info, char *word)
{
    return wcGetFirstMatching(info->file, info->wci, word);
}

char *RogetGetWord(RogetInfo *info, long wordNo)
{
    Assert(wordNo < info->wordsCount);

    if (wordNo >= info->wordsCount)
        return NULL;
    return wcGetWord(info->file, info->wci, wordNo);
}

Err RogetGetDisplayInfo(RogetInfo *info, long wordNo, int dx, DisplayInfo * di)
{
    long            wordCount = 0;
    long            wordCountTemp = 0;
    unsigned char * posRecData;
    UInt16 *        wordNums;
    long            i, j;
    long            thisWordNo;
    char *          word;
    char *          posTxt;
    char *          rawTxt;
    int             pos;
    AppContext    * appContext = GetAppContext();

    unsigned char *words_in_syn_count_data = NULL;
    long words_left;
    int wordsInSynRec;
    int word_present_p;

    Assert(info);
    Assert((wordNo >= 0) && (wordNo < info->wordsCount));
    Assert(dx > 0);
    Assert(di);

    ebufReset(&info->buffer);

    posRecData = (unsigned char *) fsLockRecord(info->file, info->synPosRec);
    words_in_syn_count_data = (unsigned char *) fsLockRecord(info->file, info->wordsInSynCountRec);
    wordsInSynRec = info->wordsInSynFirstRec;
    wordNums = (UInt16 *) fsLockRecord(info->file, wordsInSynRec);
    words_left = fsGetRecordSize(info->file, wordsInSynRec) / 2;

    if(FormatWantsWord(appContext))
    {
        ebufAddChar(&info->buffer, FORMAT_TAG);
        ebufAddChar(&info->buffer, FORMAT_SYNONYM);
        word = RogetGetWord(info, wordNo);
        ebufAddStr(&info->buffer, word);
        ebufAddChar(&info->buffer, '\n');
    }
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
            ebufAddChar(&info->buffer, FORMAT_TAG);
            ebufAddChar(&info->buffer, FORMAT_POS);
            ebufAddChar(&info->buffer, 149);
            ebufAddChar(&info->buffer, ' ');

            pos = posRecData[i / 4];

            pos = (pos >> (2 * (i % 4))) & 0x3;

            posTxt = GetPosTxt(pos, 0);
            ebufAddStr(&info->buffer, posTxt);
            ebufAddChar(&info->buffer, ' ');

            if(!FormatWantsWord(appContext))
            {
                ebufAddChar(&info->buffer, FORMAT_TAG);
                ebufAddChar(&info->buffer, FORMAT_WORD);
    
                for (j = 0; j < wordCount; j++)
                {
                    thisWordNo = (UInt32) wordNums[j];
                    word = RogetGetWord(info, thisWordNo);
                    ebufAddStr(&info->buffer, word);
                    if (j != (wordCount - 1))
                    {
                        ebufAddStr(&info->buffer, ", ");
                    }
                }
                ebufAddChar(&info->buffer, '\n');
            }
            else
            if(FormatWantsWord(appContext) && wordCount > 1)
            {
                ebufAddChar(&info->buffer, FORMAT_TAG);
                ebufAddChar(&info->buffer, FORMAT_WORD);
                wordCountTemp = wordCount;
                wordCountTemp--;
                for (j = 0; j < wordCountTemp; j++)
                {
                    thisWordNo = (UInt32) wordNums[j];
                    if(thisWordNo == wordNo)
                    {
                        j++;
                        wordCountTemp++;
                        thisWordNo = (UInt32) wordNums[j];
                    }
                    word = RogetGetWord(info, thisWordNo);
                    ebufAddStr(&info->buffer, word);
                    if (j != (wordCount - 1))
                    {
                        ebufAddStr(&info->buffer, ", ");
                    }
                }
                ebufAddChar(&info->buffer, '\n');
            }
        }
        wordNums += wordCount;
        words_left -= wordCount;
        Assert(words_left >= 0);
        if (0 == words_left)
        {
            fsUnlockRecord(info->file, wordsInSynRec);
            ++wordsInSynRec;
            wordNums = (UInt16 *) fsLockRecord(info->file, wordsInSynRec);
            words_left = fsGetRecordSize(info->file, wordsInSynRec) / 2;
        }
    }

    fsUnlockRecord(info->file, info->wordsInSynCountRec);
    fsUnlockRecord(info->file, info->wordsInSynCountRec);
    fsUnlockRecord(info->file, info->synPosRec);

    ebufAddChar(&info->buffer, '\0');
    ebufWrapBigLines(&info->buffer);

    rawTxt = ebufGetDataPointer(&info->buffer);
    diSetRawTxt(di, rawTxt);
    return 0;
}

