/*
  Copyright (C) 2000 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

/* only supported for Noah Pro, will bomb if tried with Noah Lite */
#ifdef NOAH_PRO
#include "noah_pro.h"
#endif
#include "wn_pro_support.h"
#include "extensible_buffer.h"
#include "common.h"

static SynsetsInfo *si_new(long synsetsCount, int firstWordsNumRec,
                           int firstSynsetInfoRec,
                           int bytesPerWordNum, Boolean fastP);
static void si_free(SynsetsInfo * si);
static void si_init(SynsetsInfo * si, int firstWordsNumRec,
                    int firstSynsetInfoRec, int bytesPerWordNum);
static void si_destroy(SynsetsInfo * si);
static void si_position_at_synset(SynsetsInfo * si, long synset);
static int si_get_words_count(SynsetsInfo * si, long synset);
static int si_get_pos(SynsetsInfo * si, long synset);
static long si_total_words_count(SynsetsInfo * si, long synset);
static void si_position_at_word(SynsetsInfo * si, long synset, int word_idx);
static int si_word_in_synset_forward_p(SynsetsInfo * si, long *synset,
                                       long wordNo);
static int si_word_in_synset_backward_p(SynsetsInfo * si, long *synset,
                                        long wordNo);
static long si_get_word_no(SynsetsInfo * si, long synset, int word_idx);

/* this buffer holds a copy of previous buffer + leading zero, for the
purpose of giving this data to DisplayInfo */
static ExtensibleBuffer g_buf_tmp = { 0 };

/* this buffer will hold word definition currently being constructed inside
   wn_get_display_info */
static ExtensibleBuffer g_buf = { 0 };

/* for displaying progressive definitions */
DisplayInfo g_di_tmp = { 0 };

void *wn_new(void)
{
    WnInfo *wi = NULL;
    WnFirstRecord *firstRecord;
    int rec_with_words_compr_data;
    int rec_with_defs_compr_data;
    int recWithWordCache;
    int firstRecWithWords;

    wi = (WnInfo *) new_malloc(sizeof(WnInfo));
    if (NULL == wi)
        goto Error;

    firstRecord = (WnFirstRecord *) CurrFileLockRecord(0);
    if (NULL == firstRecord)
        goto Error;

    wi->recordsCount = CurrFileGetRecordsCount();
    wi->wordsCount = firstRecord->wordsCount;
    wi->synsetsCount = firstRecord->synsetsCount;

    wi->wordsRecsCount = firstRecord->wordsRecsCount;
    wi->synsetsInfoRecsCount = firstRecord->synsetsInfoRecsCount;
    wi->wordsNumRecsCount = firstRecord->wordsNumRecsCount;
    wi->defsLenRecsCount = firstRecord->defsLenRecsCount;
    wi->defsRecsCount = firstRecord->defsRecsCount;

    wi->maxWordLen = firstRecord->maxWordLen;
    wi->maxDefLen = firstRecord->maxDefLen;
    wi->maxComprDefLen = firstRecord->maxComprDefLen;
    wi->bytesPerWordNum = firstRecord->bytesPerWordNum;
    wi->maxWordsPerSynset = firstRecord->maxWordsPerSynset;

    wi->synCountRec = wi->recordsCount - 1;
    wi->curDefData = (unsigned char *) new_malloc(wi->maxDefLen + 2);
    if (NULL == wi->curDefData)
        goto Error;

    recWithWordCache = 1;
    firstRecWithWords = 4;
#ifndef DEMO
    rec_with_words_compr_data = 2;
    rec_with_defs_compr_data = 3;
#endif
#ifdef DEMO
    rec_with_defs_compr_data = 2;
    rec_with_words_compr_data = 3;
#endif

    if (sizeof(WnFirstRecord) == CurrFileGetRecordSize(0))
    {
        wi->fastP = false;
    }
    else
    {
        wi->fastP = true;
        wi->cacheEntries = CurrFileGetRecordSize(0) - sizeof(WnFirstRecord);
        wi->cacheEntries = wi->cacheEntries / sizeof(SynWordCountCache);
    }

    wi->wci = wcInit(wi->wordsCount, rec_with_words_compr_data,
                      recWithWordCache,
                      firstRecWithWords, wi->wordsRecsCount,
                      wi->maxWordLen);

    if (NULL == wi->wci)
    {
        goto Error;
    }

    wi->si = si_new(wi->synsetsCount,
                    4 + wi->wordsRecsCount + wi->synsetsInfoRecsCount,
                    4 + wi->wordsRecsCount, wi->bytesPerWordNum,
                    wi->fastP);

    if (NULL == wi->si)
    {
        goto Error;
    }

    if (!pcInit(&wi->defPackContext, rec_with_defs_compr_data))
    {
        goto Error;
    }

  Exit:
    CurrFileUnlockRecord(0);
    return (void *) wi;
  Error:
    if (wi)
    {
        wn_delete((void *) wi);
        wi = NULL;
    }
    goto Exit;
}

void wn_delete(void *data)
{
    WnInfo *wi;

    ebufFreeData(&g_buf);
    ebufFreeData(&g_buf_tmp);
    diFreeData(&g_di_tmp);

    if (!data)
        return;

    wi = (WnInfo *) data;

    pcFree(&wi->defPackContext);

    if (wi->wci)
        wcFree(wi->wci);

    if (wi->curDefData)
        new_free(wi->curDefData);

    if (wi->si)
        si_free(wi->si);

    new_free(data);
}

long wn_get_words_count(void *data)
{
    return ((WnInfo *) data)->wordsCount;
}

long wn_get_first_matching(void *data, char *word)
{
    return wcGetFirstMatching(((WnInfo *) data)->wci, word);
}

char *wn_get_word(void *data, long wordNo)
{
    WnInfo *wi;
    wi = (WnInfo *) data;

    Assert(wordNo < wi->wordsCount);

    if (wordNo >= wi->wordsCount)
        return NULL;
    return wcGetWord(wi->wci, wordNo);
}

#define DRAW_DI_X 0
#define DRAW_DI_Y 0
#define DRAW_DI_LINES 13

#define CACHE_SPAN 1000

SynWordCountCache *si_create_cache(SynsetsInfo * si)
{
    long total_words_count = 0;
    long i;
    long cacheEntries;
    long cache_entry_no = 0;
    SynWordCountCache *swcc = NULL;

    Assert(si);

    cacheEntries = (si->synsetsCount / CACHE_SPAN) + 1;
    swcc =
        (SynWordCountCache *) new_malloc(cacheEntries *
                                         sizeof(SynWordCountCache));

    if (NULL == swcc)
        return NULL;

    total_words_count = si_get_words_count(si, 0);
    for (i = 1; i < si->synsetsCount; i++)
    {
        if (0 == (i % CACHE_SPAN))
        {
            Assert(cache_entry_no < cacheEntries);
            swcc[cache_entry_no].synsetNo = i;
            swcc[cache_entry_no].wordsCount = total_words_count;
            ++cache_entry_no;
        }
        total_words_count += si_get_words_count(si, i);
    }
    swcc[cache_entry_no].synsetNo = si->synsetsCount;
    swcc[cache_entry_no].wordsCount = total_words_count;
    return swcc;
}

SynsetsInfo *si_new(long synsetsCount, int firstWordsNumRec,  int firstSynsetInfoRec, int bytesPerWordNum, Boolean fastP)
{
    SynsetsInfo *si = NULL;

    si = (SynsetsInfo *) new_malloc(sizeof(SynsetsInfo));
    if (NULL == si)
        return NULL;
    si_init(si, firstWordsNumRec, firstSynsetInfoRec,
            bytesPerWordNum);
    si->synsetsCount = synsetsCount;
    if (fastP)
    {
        si->swcc = NULL;
    }
    else
    {
        si->swcc = si_create_cache(si);
        if (NULL == si->swcc)
        {
            si_free(si);
            return NULL;
        }
    }
    return si;
}

void si_init(SynsetsInfo * si, int firstWordsNumRec, int firstSynsetInfoRec, int bytesPerWordNum)
{
    Assert(si);
    Assert((2 == bytesPerWordNum) || (3 == bytesPerWordNum));
    si->firstWordsNumRec = firstWordsNumRec;
    si->firstSynsetInfoRec = firstSynsetInfoRec;
    si->bytesPerWordNum = bytesPerWordNum;
    si->synsetDataStart = NULL;
    si->synsetData = NULL;
    si->synsetDataLeft = 0;
    si->wordsNumData = NULL;
    si->wordsNumDataLeft = 0;
    si->currSynset = 0xffffff;
    si->currSynsetWordsNum = 0xffffff;
    si->currWord = -1;
    si->curSynsetInfoRec = -1;
    si->curWordsNumRec = -1;
}

void si_destroy(SynsetsInfo * si)
{
    Assert(si);
    if (si->curWordsNumRec != -1)
    {
        CurrFileUnlockRecord(si->curWordsNumRec);
    }
    if (si->curSynsetInfoRec != -1)
    {
        CurrFileUnlockRecord(si->curSynsetInfoRec);
    }
    si_init(si, si->firstWordsNumRec, si->firstSynsetInfoRec,
            si->bytesPerWordNum);
}

void si_free(SynsetsInfo * si)
{
    si_destroy(si);
    if (si->swcc)
    {
        new_free(si->swcc);
    }
    new_free(si);
}

void si_position_at_synset(SynsetsInfo * si, long synset)
{
    long rec_idx;
    int rec;

    Assert(si);

    /* make common cases quick */
    if ((synset == si->currSynset + 1) && (si->synsetDataLeft > 2))
    {
        ++si->synsetData;
        --si->synsetDataLeft;
        si->currSynset = synset;
        return;
    }
    if ((synset == si->currSynset - 1)
        && (si->synsetData > si->synsetDataStart))
    {
        --si->synsetData;
        ++si->synsetDataLeft;
        si->currSynset = synset;
        return;
    }
    if (synset == si->currSynset)
        return;

    /* find the record that has data for this synset */
    rec_idx = synset;
    rec = si->firstSynsetInfoRec;
    while (rec_idx >= CurrFileGetRecordSize(rec))
    {
        rec_idx -= CurrFileGetRecordSize(rec);
        ++rec;
    }
    if (si->curSynsetInfoRec != rec)
    {
        if (si->curSynsetInfoRec != -1)
        {
            CurrFileUnlockRecord(si->curSynsetInfoRec);
        }
        si->curSynsetInfoRec = rec;
        si->synsetDataStart = (unsigned char *) CurrFileLockRecord(rec);
        if (NULL == si->synsetDataStart)
            return;
    }
    si->synsetData = si->synsetDataStart + rec_idx;
    si->synsetDataLeft = CurrFileGetRecordSize(rec) - rec_idx;
    si->currSynset = synset;
}


int si_get_words_count(SynsetsInfo * si, long synset)
{
    int wordsCount;

    Assert(si);
    Assert(synset < 200000);

    si_position_at_synset(si, synset);

    Assert(synset == si->currSynset);

    wordsCount = *si->synsetData & 63;

    if (0 == wordsCount)
    {
        wordsCount = 1;
    }
    Assert(wordsCount > 0);

    return wordsCount;
}

int si_get_pos(SynsetsInfo * si, long synset)
{
    int pos;

    Assert(si);

    si_position_at_synset(si, synset);
    Assert(synset == si->currSynset);
    pos = (*si->synsetData >> 6);

    return pos;
}

/* how many total words are with all synsets up to this one */
long si_total_words_count(SynsetsInfo * si, long synset)
{
    long total_words_count = 0;
    long i = 0;
    long synsetNo = 0;
    SynWordCountCache *swcc = NULL;
    unsigned char *data = NULL;

    Assert(si);
    Assert(synset < si->synsetsCount);

    if (NULL == si->swcc)
    {
        data = ((unsigned char *) CurrFileLockRecord(0)) + sizeof(WnFirstRecord);
        if (NULL == data)
            return 0;
        swcc = (SynWordCountCache *) data;
    }
    else
    {
        swcc = si->swcc;
    }

    while (synset > swcc[i].synsetNo)
    {
        total_words_count = swcc[i].wordsCount;
        synsetNo = swcc[i].synsetNo;
        ++i;
    }

    for (i = synsetNo; i < synset; i++)
    {
        total_words_count += si_get_words_count(si, i);
    }
    if (data != NULL)
    {
        CurrFileUnlockRecord(0);
    }
    return total_words_count;
}

typedef enum
{ dir_forward, dir_backward }
direction;

void si_position_at_word(SynsetsInfo * si, long synset, int word_idx)
{
    int rec;
    long rec_idx;
    long total_words_count;
    long syn;
    long move_by;
    direction dir;

    Assert(si);

    if (((si->currSynsetWordsNum > synset)
         && (si->currSynsetWordsNum - synset > 1))
        || ((si->currSynsetWordsNum < synset)
            && (synset - si->currSynsetWordsNum > 1))
        || (0 == si->wordsNumDataLeft))
    {
        /* just do it from scratch */
        if (si->curWordsNumRec != -1)
        {
            CurrFileUnlockRecord(si->curWordsNumRec);
        }
        rec = si->firstWordsNumRec;
        total_words_count = si_total_words_count(si, synset) + word_idx;
        rec_idx = total_words_count * si->bytesPerWordNum;

        while (rec_idx >= CurrFileGetRecordSize(rec))
        {
            rec_idx -= CurrFileGetRecordSize(rec);
            ++rec;
        }
        si->curWordsNumRec = rec;
        si->wordsNumData = (unsigned char *) CurrFileLockRecord(rec);
        if (NULL == si->wordsNumData)
            return;
        si->wordsNumData += rec_idx;
        si->wordsNumDataLeft = CurrFileGetRecordSize(rec) - rec_idx;
        si->currSynsetWordsNum = synset;
        si->currWord = word_idx;
        return;
    }

    /* advance max by 1 */
    if ((si->currSynsetWordsNum == synset) && (word_idx == si->currWord))
    {
        return;
    }

    if ((synset > si->currSynsetWordsNum) ||
        ((synset == si->currSynsetWordsNum) && (word_idx > si->currWord)))
    {
        /* go forward */
        dir = dir_forward;
        if (synset == si->currSynsetWordsNum)
        {
            /* within synset its only by difference in word numbers */
            move_by = word_idx - si->currWord;
        }
        else
        {
            move_by =
                si_get_words_count(si,
                                   si->currSynsetWordsNum) - si->currWord;
            move_by += word_idx;
            for (syn = si->currSynsetWordsNum + 1; syn < synset; syn++)
            {
                move_by += si_get_words_count(si, syn);
            }
        }
        move_by = move_by * si->bytesPerWordNum;
        if (move_by < si->wordsNumDataLeft)
        {
            si->wordsNumDataLeft -= move_by;
            si->wordsNumData += move_by;
        }
        else
        {
            /* FIXME: this only work if within the distance of one record */
            move_by -= si->wordsNumDataLeft;
            rec = si->curWordsNumRec;
            CurrFileUnlockRecord(rec);
            ++rec;
            si->wordsNumData = (unsigned char *) CurrFileLockRecord(rec);
            if (NULL == si->wordsNumData)
                return;
            si->wordsNumDataLeft = CurrFileGetRecordSize(rec);
            Assert(move_by < si->wordsNumDataLeft);
            si->wordsNumData += move_by;
            si->wordsNumDataLeft -= move_by;
            si->curWordsNumRec = rec;
        }
    }
    else
    {
        /* must be backward */
        dir = dir_backward;
        if (synset == si->currSynsetWordsNum)
        {
            Assert(word_idx < si->currSynsetWordsNum);
            move_by = si->currSynsetWordsNum - word_idx;
        }
        else
        {
            move_by = si->currWord;
            for (syn = si->currSynsetWordsNum - 1; syn > synset; --syn)
            {
                move_by += si_get_words_count(si, syn);
            }
            move_by += (si_get_words_count(si, syn) - word_idx);
            move_by = move_by * si->bytesPerWordNum;
            rec = si->curWordsNumRec;
            if (move_by <=
                (CurrFileGetRecordSize(rec) - si->wordsNumDataLeft))
            {
                /* within the record */
                si->wordsNumDataLeft += move_by;
                si->wordsNumData -= move_by;
            }
            else
            {
                move_by -=
                    (CurrFileGetRecordSize(rec) - si->wordsNumDataLeft);
                CurrFileUnlockRecord(rec);
                --rec;
                si->wordsNumData = (unsigned char *) CurrFileLockRecord(rec);
                if (NULL == si->wordsNumData)
                    return;
                si->wordsNumData += (CurrFileGetRecordSize(rec) - move_by);
                si->wordsNumDataLeft = move_by;
                si->curWordsNumRec = rec;
            }
        }
    }

    si->currSynsetWordsNum = synset;
    si->currWord = word_idx;
}

/*
  returns:
  0 - word not in a synset
  1 - word not in a synset and the smallest word in the synset is bigger
    than the word, so we can go back
  2 - word in a synset
*/
int si_word_in_synset_forward_p(SynsetsInfo * si, long *synset, long wordNo)
{
    long    wordNumFound;
    long    smallestWordNo = -1;
    int     i, j, wordsCount;
    long    wordsNumDataLeft;
    long    maxSynset;
    int     retValue;
    unsigned char *synsetData = NULL;
    unsigned char *wordNums = NULL;

    si_position_at_synset(si, *synset);
    synsetData = si->synsetData;
    maxSynset = si->synsetDataLeft;

    si_position_at_word(si, *synset, 0);
    wordNums = si->wordsNumData;
    wordsNumDataLeft = (long) si->wordsNumDataLeft;

    /* we rely on the fact, that data for one synset doesn't
       cross boundaries of records. I hope it's true */
    if (3 == si->bytesPerWordNum)
    {
        for (j = 0; j < maxSynset; j++)
        {
            wordsCount = *synsetData++ & 63;
            for (i = 0; i < wordsCount; i++)
            {
                wordNumFound =
                    ((long) (wordNums[0])) * 256 * 256 +
                    256 * ((long) wordNums[1]) + (long) wordNums[2];
                if (wordNo == wordNumFound)
                {
                    *synset += j;
                    retValue = 2;
                    goto Exit;
                }
                wordNums += 3;
                if (wordNumFound < smallestWordNo)
                {
                    smallestWordNo = wordNumFound;
                }
            }
            wordsNumDataLeft -= wordsCount * 3;
            if (smallestWordNo > wordNo)
            {
                *synset += j;
                retValue = 1;
                goto Exit;
            }
            Assert(wordsNumDataLeft >= 0);
            if (0 == wordsNumDataLeft)
            {
                *synset += j;
                retValue = 0;
                goto Exit;
            }
        }
        *synset += (maxSynset - 1);
        retValue = 0;
        goto Exit;
    }
    else
    {
        Assert(2 == si->bytesPerWordNum);
        for (j = 0; j < maxSynset; j++)
        {
            wordsCount = *synsetData++ & 63;
            for (i = 0; i < wordsCount; i++)
            {
                wordNumFound =
                    ((long) (wordNums[0])) * 256 + (long) wordNums[1];
                wordNums += 2;
                if (wordNo == wordNumFound)
                {
                    *synset += j;
                    retValue = 2;
                    goto Exit;
                }
                if (wordNumFound < smallestWordNo)
                {
                    smallestWordNo = wordNumFound;
                }
            }
            if (smallestWordNo > wordNo)
            {
                *synset += j;
                retValue = 1;
                goto Exit;
            }
            wordsNumDataLeft -= wordsCount * 2;
            Assert(wordsNumDataLeft >= 0);
            if (0 == wordsNumDataLeft)
            {
                *synset += j;
                retValue = 0;
                goto Exit;
            }
        }
        *synset += (maxSynset - 1);
        retValue = 0;
        goto Exit;
    }
  Exit:
    si_destroy(si);
    return retValue;
}

int si_word_in_synset_backward_p(SynsetsInfo * si, long *synset, long wordNo)
{
    long    wordNumFound;
    int     i, j, wordsCount;
    unsigned char *wordNums;
    unsigned char *synsetData;
    long    wordsNumDataLeft;
    long    maxSynset;
    int     retValue;

    si_position_at_synset(si, *synset);
    synsetData = si->synsetData;
    maxSynset = CurrFileGetRecordSize(si->curSynsetInfoRec) - si->synsetDataLeft;

    /* position at the last word in this synset, so we can go backward */
    wordsCount = *synsetData & 63;
    si_position_at_word(si, *synset, 0);
    wordsNumDataLeft =
    CurrFileGetRecordSize(si->curWordsNumRec) - si->wordsNumDataLeft;
    wordsNumDataLeft += wordsCount * si->bytesPerWordNum;
    wordNums = si->wordsNumData + wordsCount * si->bytesPerWordNum;

    /* we rely on the fact, that data for one synset doesn't
       cross boundaries of records. I hope it's true */
    if (3 == si->bytesPerWordNum)
    {
        for (j = 0; j < maxSynset; j++)
        {
            wordsCount = *synsetData-- & 63;
            for (i = 0; i < wordsCount; i++)
            {
                wordNums -= 3;
                wordNumFound =
                    ((long) (wordNums[0])) * 256 * 256 +
                    256 * ((long) wordNums[1]) + (long) wordNums[2];
                if (wordNo == wordNumFound)
                {
                    *synset -= j;
                    retValue = 2;
                    goto Exit;
                }
            }
#ifdef DEBUG
            if (wordsNumDataLeft < wordsCount * 3)
            {
                --*synset;
                ++*synset;
                Assert(0);
            }
#endif /* DEBUG */
            wordsNumDataLeft -= wordsCount * 3;
            if (0 == wordsNumDataLeft)
            {
                /* position at a synset in a previous record,
                   but don't go below 0 */
                if (*synset < j + 1)
                {
                    *synset = 0;
                }
                else
                {
                    *synset -= (j + 1);
                }
                retValue = 0;
                goto Exit;
            }
        }
        *synset -= maxSynset;

        retValue = 0;
        goto Exit;
    }
    else
    {
        Assert(2 == si->bytesPerWordNum);
        for (j = 0; j < maxSynset; j++)
        {
            wordsCount = *synsetData-- & 63;
            for (i = 0; i < wordsCount; i++)
            {
                wordNums -= 2;
                wordNumFound =
                    ((long) (wordNums[0])) * 256 + (long) wordNums[1];
                if (wordNo == wordNumFound)
                {
                    *synset -= j;
                    retValue = 2;
                    goto Exit;
                }
            }
#ifdef DEBUG
            if (wordsNumDataLeft < wordsCount * 2)
            {
                --*synset;
                ++*synset;
                Assert(0);
            }
#endif /* DEBUG */
            wordsNumDataLeft -= wordsCount * 2;
            Assert(wordsNumDataLeft >= 0);
            if (0 == wordsNumDataLeft)
            {
                if (*synset < j + 1)
                {
                    *synset = 0;
                }
                else
                {
                    *synset -= (j + 1);
                }
                retValue = 0;
                goto Exit;
            }
        }
        *synset -= maxSynset;
        retValue = 0;
        goto Exit;
    }
  Exit:
    si_destroy(si);
    return retValue;
}

long si_get_word_no(SynsetsInfo * si, long synset, int word_idx)
{
    long wordNumFound;
    unsigned char *data;

    Assert(word_idx < si_get_words_count(si, synset));

    si_position_at_word(si, synset, word_idx);
    data = si->wordsNumData;
    if (3 == si->bytesPerWordNum)
    {
        wordNumFound =
            ((long) (data[0])) * 256 * 256 + 256 * ((long) data[1]) +
            (long) data[2];
    }
    else
    {
        wordNumFound = ((long) (data[0])) * 256 + (long) data[1];
    }
    return wordNumFound;
}

/*
for a given word return the number of synsets
that this word belongs to (or 0 to mean we
don't have enough data) */
int wn_get_synset_count(WnInfo * wi, long wordNo)
{
    unsigned char *data;
    int synset_count;

    if (!wi->fastP)
    {
        return 0;
    }
    data = (unsigned char *) CurrFileLockRecord(wi->synCountRec);

    if (NULL == data)
        return 0;

    data += (wordNo / 4);

    synset_count = *data;
    synset_count = synset_count >> (2 * (wordNo % 4));
    synset_count &= 0x3;

    CurrFileUnlockRecord(wi->synCountRec);
    return synset_count;
}


Err wn_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di)
{
    WnInfo *wi = NULL;
    char *rawTxt = NULL;
    long defDataSize = 0;
    int len;
    char *word;
    int unpackedLen;

    int syn_count;
    int syn_found_count = 0;
    int wordsCount;
    int partOfSpeech;
    long wordNumFound;
    long w;
    int first_rec_with_defs_len;
    int first_rec_with_defs;
    int defs_record = 0;
    long def_offset = 0;
    char *txt = NULL;
    long syn;
    int res;
    long start_syn;
    direction dir;
    Boolean end_p;
    SynsetsInfo *si;
    unsigned char *defData = NULL;
    unsigned char *unpackedDef = NULL;

    wi = (WnInfo *) data;

    Assert(data);
    Assert((wordNo >= 0) && (wordNo < wi->wordsCount));
    Assert(dx > 0);
    Assert(di);

    ebufReset(&g_buf);

    si = wi->si;

    if (wordNo >= wi->synsetsCount - 4)
    {
        dir = dir_backward;
        start_syn = wi->synsetsCount - 1;
    }
    else
    {
        start_syn = wordNo;
        dir = dir_forward;
    }

    if (wi->fastP)
    {
        syn_count = wn_get_synset_count(wi, wordNo);
    }
    else
    {
        syn_count = 0;
    }

    syn = start_syn;
    end_p = false;

    while (!end_p)
    {
        Assert(syn < wi->synsetsCount);

        if (dir == dir_forward)
        {
            res = si_word_in_synset_forward_p(si, &syn, wordNo);
        }
        else
        {
            res = si_word_in_synset_backward_p(si, &syn, wordNo);
        }

        /* if word found add up the definition */
        if (2 == res)
        {
            ebufAddChar(&g_buf, 149);
            ebufAddChar(&g_buf, ' ');

            partOfSpeech = si_get_pos(si, syn);
            txt = GetWnPosTxt(partOfSpeech);
            ebufAddStr(&g_buf, txt);

            wordsCount = si_get_words_count(si, syn);
            Assert(wordsCount > 0);

            for (w = 0; w < wordsCount; w++)
            {
                wordNumFound = si_get_word_no(si, syn, w);
                word = wn_get_word((void *) wi, wordNumFound);
                ebufAddStr(&g_buf, word);
                if (w == wordsCount - 1)
                {
                    txt = "\n";
                }
                else
                {
                    txt = ", ";
                }
                ebufAddStr(&g_buf, txt);
            }

            first_rec_with_defs_len = 4 + wi->wordsRecsCount +
                wi->synsetsInfoRecsCount + wi->wordsNumRecsCount;

            first_rec_with_defs =
                first_rec_with_defs_len + wi->defsLenRecsCount;

            if ( !get_defs_record(syn, first_rec_with_defs_len,
                            wi->defsLenRecsCount, first_rec_with_defs,
                            &defs_record, &def_offset, &defDataSize) )
            {
                return NULL;
            }

            Assert(defs_record > 4);
            Assert((def_offset >= 0) && (def_offset < 66000));
            Assert((defDataSize >= 0));

            defData = (unsigned char *) CurrFileLockRecord(defs_record);
            if (NULL == defData)
            {
                return NULL;
            }
            defData += def_offset;

            pcReset(&wi->defPackContext, defData, 0);
            pcUnpack(&wi->defPackContext, defDataSize,
                      wi->curDefData, &unpackedLen);
            Assert((unpackedLen > 0) && (unpackedLen <= wi->maxDefLen));
            wi->curDefLen = unpackedLen;
            unpackedDef = wi->curDefData;
            Assert((1 == unpackedDef[unpackedLen - 1]) ||
                   (0 == unpackedDef[unpackedLen - 1]));
            CurrFileUnlockRecord(defs_record);

            while (unpackedLen > 0)
            {
                len = 0;
                while ((0 != unpackedDef[0]) && (1 != unpackedDef[0]))
                {
                    ++len;
                    ++unpackedDef;
                    --unpackedLen;
                }

                if (1 == unpackedDef[0])
                {
                    txt = " ";
                }
                else
                {
                    txt = " \"";
                }
                ebufAddStr(&g_buf, txt);
                ebufAddStrN(&g_buf, (char *) (unpackedDef - len), len);

                if (0 == unpackedDef[0])
                {
                    ebufAddStr(&g_buf, " \"");
                }
                ebufAddChar(&g_buf, '\n');
                --unpackedLen;
                ++unpackedDef;
                Assert(unpackedLen >= 0);
            }

            ebufReset(&g_buf_tmp);
            ebufAddStr(&g_buf_tmp, ebufGetDataPointer(&g_buf));
            ebufAddChar(&g_buf_tmp, '\0');

            ebufWrapBigLines(&g_buf_tmp);
            rawTxt = ebufGetDataPointer(&g_buf_tmp);
            diSetRawTxt(&g_di_tmp, rawTxt);
            if (0 == syn_found_count)
            {
                ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
            }
            DrawDisplayInfo(&g_di_tmp, 0, DRAW_DI_X, DRAW_DI_Y,
                            DRAW_DI_LINES);
            SetScrollbarState(&g_di_tmp, DRAW_DI_LINES, 0);
            DrawWord("Searching...", 149);
            ++syn_found_count;
            if (syn_found_count == syn_count)
                break;
        }

        if ((dir == dir_forward)
            && ((1 == res) || (syn == wi->synsetsCount - 1)))
        {
            dir = dir_backward;
            syn = start_syn;
        }

        if (dir == dir_forward)
        {
            ++syn;
        }
        else
        {
            if (syn == 0)
            {
                end_p = true;
            }
            else
            {
                --syn;
            }
        }
    }

    si_destroy(si);

    ebufAddChar(&g_buf, '\0');
    ebufWrapBigLines(&g_buf);
    rawTxt = ebufGetDataPointer(&g_buf);
    diSetRawTxt(di, rawTxt);
    return 0;
}
