/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include "thes.h"
#include "roget_support.h"
#include "common.h"
#include "fs.h"

extern GlobalData gd;

/* temporary buffer for keeping the data gathered during unpacking */
ExtensibleBuffer g_buf = { 0 };

static char *get_pos_txt(int pos, int type)
{
    return get_nth_txt(type * 4 + pos, "(noun)\0(verb)\0(adj.)\0(adv.)\0N.\0V.\0Adj.\0Adv.\0Noun\0Verb\0Adjective\0Adverb\0");
}

static void roget_delete(void *data)
{
    ebufFreeData(&g_buf);

    if (!data)
        return;

    if (((RogetInfo *) data)->wci)
        wcFree(((RogetInfo *) data)->wci);

    new_free(data);
    return;
}

static void *roget_new(void)
{
    RogetInfo *info = NULL;
    RogetFirstRecord *firstRecord;
#ifdef DEBUG
    long recSize;
#endif

    info = (RogetInfo *) new_malloc(sizeof(RogetInfo));
    if (NULL == info)
        goto Error;

    firstRecord = (RogetFirstRecord *) vfsLockRecord(0);

    info->recordsCount = vfsGetRecordsCount();
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

#ifdef DEBUG
    recSize = vfsGetRecordSize(info->wordsInSynCountRec);
    Assert(recSize == info->synsetsCount);
#endif
    if (NULL == info->wci)
        goto Error;

  Exit:
    vfsUnlockRecord(0);
    return (void *) info;
  Error:
    if (info)
        roget_delete((void *) info);
    info = NULL;
    goto Exit;
}


static long roget_get_words_count(void *data)
{
    return ((RogetInfo *) data)->wordsCount;
}

static long roget_get_first_matching(void *data, char *word)
{
    return wcGetFirstMatching(((RogetInfo *) data)->wci, word);
}

static char *roget_get_word(void *data, long wordNo)
{
    RogetInfo *info;
    info = (RogetInfo *) data;

    Assert(wordNo < info->wordsCount);

    if (wordNo >= info->wordsCount)
        return NULL;
    return wcGetWord(info->wci, wordNo);
}

static Err roget_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di)
{
    RogetInfo *info = NULL;
    long word_count = 0;
    unsigned char *pos_rec_data;
    UInt16 *word_nums;
    long i, j;
    long thisWordNo;
    char *word;
    char *pos_txt;
    char *rawTxt;
    int pos;

    unsigned char *words_in_syn_count_data = NULL;
    long words_left;
    int words_in_syn_rec;
    int word_present_p;

    info = (RogetInfo *) data;

    Assert(data);
    Assert((wordNo >= 0) && (wordNo < info->wordsCount));
    Assert(dx > 0);
    Assert(di);

    ebufReset(&g_buf);

    pos_rec_data = (unsigned char *) vfsLockRecord(info->synPosRec);
    words_in_syn_count_data = (unsigned char *) vfsLockRecord(info->wordsInSynCountRec);
    words_in_syn_rec = info->wordsInSynFirstRec;
    word_nums = (UInt16 *) vfsLockRecord(words_in_syn_rec);
    words_left = vfsGetRecordSize(words_in_syn_rec) / 2;

    for (i = 0; i < info->synsetsCount; i++)
    {
        word_present_p = 0;
        word_count = (int) words_in_syn_count_data[0];
        word_count += 2;
        ++words_in_syn_count_data;

        for (j = 0; j < word_count; j++)
        {
            thisWordNo = (UInt32) word_nums[j];
            if (wordNo == thisWordNo)
            {
                word_present_p = 1;
            }
        }

        if (word_present_p)
        {
            ebufAddChar(&g_buf, 149);
            ebufAddChar(&g_buf, ' ');

            pos = pos_rec_data[i / 4];

            pos = (pos >> (2 * (i % 4))) & 0x3;

            pos_txt = get_pos_txt(pos, 0);
            ebufAddStr(&g_buf, pos_txt);
            ebufAddChar(&g_buf, ' ');

            for (j = 0; j < word_count; j++)
            {
                thisWordNo = (UInt32) word_nums[j];
                word = roget_get_word((void *) info, thisWordNo);
                ebufAddStr(&g_buf, word);
                if (j != (word_count - 1))
                {
                    ebufAddStr(&g_buf, ", ");
                }
            }
            ebufAddChar(&g_buf, '\n');
        }
        word_nums += word_count;
        words_left -= word_count;
        Assert(words_left >= 0);
        if (0 == words_left)
        {
            vfsUnlockRecord(words_in_syn_rec);
            ++words_in_syn_rec;
            word_nums = (UInt16 *) vfsLockRecord(words_in_syn_rec);
            words_left = vfsGetRecordSize(words_in_syn_rec) / 2;
        }
    }

    vfsUnlockRecord(info->wordsInSynCountRec);
    vfsUnlockRecord(info->wordsInSynCountRec);
    vfsUnlockRecord(info->synPosRec);

    ebufAddChar(&g_buf, '\0');
    ebufWrapBigLines(&g_buf);

    rawTxt = ebufGetDataPointer(&g_buf);
    diSetRawTxt(di, rawTxt);
    return 0;
}

void setRogetAsCurrentDict(void)
{
    gd.currentDict.objectNew = &roget_new;
    gd.currentDict.objectDelete = &roget_delete;
    gd.currentDict.getWordsCount = &roget_get_words_count;
    gd.currentDict.getFirstMatching = &roget_get_first_matching;
    gd.currentDict.getWord = &roget_get_word;
    gd.currentDict.getDisplayInfo = &roget_get_display_info;
}
