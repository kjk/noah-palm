/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

/* only supported for Noah Pro, will bomb if tried with Noah Lite */
#include "wn_pro_support.h"
#include "pronunciation.h"

static SynsetsInfo *si_new(AbstractFile* file, long synsetsCount, int firstWordsNumRec,
                           int firstSynsetInfoRec,
                           int bytesPerWordNum, Boolean fastP);
static void si_free(AbstractFile* file, SynsetsInfo * si);
static void si_init(AbstractFile* file, SynsetsInfo * si, int firstWordsNumRec,
                    int firstSynsetInfoRec, int bytesPerWordNum);
static void si_destroy(AbstractFile* file, SynsetsInfo * si);
static void si_position_at_synset(AbstractFile* file, SynsetsInfo * si, long synset);
static int si_get_words_count(AbstractFile* file, SynsetsInfo * si, long synset);
static int si_get_pos(AbstractFile* file, SynsetsInfo * si, long synset);
static long si_total_words_count(AbstractFile* file, SynsetsInfo * si, long synset);
static void si_position_at_word(AbstractFile* file, SynsetsInfo * si, long synset, int word_idx);
static int si_word_in_synset_forward_p(AbstractFile* file, SynsetsInfo * si, long *synset,
                                       long wordNo);
static int si_word_in_synset_backward_p(AbstractFile* file, SynsetsInfo * si, long *synset,
                                        long wordNo);
static long si_get_word_no(AbstractFile* file, SynsetsInfo * si, long synset, int word_idx);

void *wn_new(AbstractFile* file)
{
    WnInfo *wi = NULL;
    WnFirstRecord *firstRecord = NULL;
    int rec_with_words_compr_data;
    int rec_with_defs_compr_data;
    int recWithWordCache;
    int firstRecWithWords;
    //to handle pronunciation
    unsigned char     *data = NULL;
    PronInFirstRecord *pronInFirstRecord = NULL;
    AppContext        *appContext = GetAppContext();

    wi = (WnInfo *) new_malloc_zero(sizeof(WnInfo));
    if (NULL == wi)
    {
        LogG("wn_new() new_malloc(sizeof(WnInfo)) failed" );
        goto Error;
    }

    firstRecord = (WnFirstRecord *) fsLockRecord(file, 0);
    if (NULL == firstRecord)
    {
        LogG("wn_new() CurrFileLockRecord(0) failed" );
        goto Error;
    }
    wi->file=file;
    ebufInit(&wi->buffer, 0);
    ebufInit(&wi->bufferTemp, 0);
    wi->recordsCount = fsGetRecordsCount(file);
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

    wi->synCountRec = wi->recordsCount - 1;
    wi->curDefData = (unsigned char *) new_malloc(wi->maxDefLen + 2);
    if (NULL == wi->curDefData)
        goto Error;

    recWithWordCache  = 1;
    firstRecWithWords = 4;
#ifdef DEMO
    rec_with_defs_compr_data  = 2;
    rec_with_words_compr_data = 3;
#else
    rec_with_defs_compr_data  = 3;
    rec_with_words_compr_data = 2;
#endif
    //set false by default
    appContext->pronData.isPronInUsedDictionary = false;
    if (sizeof(WnFirstRecord) == fsGetRecordSize(file, 0))
    {
        wi->fastP = false;
    }
    else
    {
        wi->fastP = true;
        wi->cacheEntries = fsGetRecordSize(file, 0) - sizeof(WnFirstRecord);
        if (wi->cacheEntries % sizeof(SynWordCountCache) != 0)
        {
            /*sizeof(SynWordCountCache) = 8
              data and identifier about pronunciation is added at the end
              of first record. It looks like this: "pronunXXYYZZ"
              where XX - recordNo with Index
                    YY - first record with pronData
                    ZZ - number of that records
                    pronunc - string to detect if it's pronunciation
                    sizeof(data and id) = 12
            */
            
            data = (unsigned char *) firstRecord;
            data += (fsGetRecordSize(file, 0) - sizeof(PronInFirstRecord));
            pronInFirstRecord = (PronInFirstRecord *) &data[0];
                
            if(StrNCompare("pronun",(char *)pronInFirstRecord->idString,6)==0)
            {
                appContext->pronData.isPronInUsedDictionary = true;
                appContext->pronData.recordWithPronIndex = (unsigned int) pronInFirstRecord->recordWithPronIndex;
                appContext->pronData.firstRecordWithPron = (unsigned int) pronInFirstRecord->firstRecordWithPron;
                appContext->pronData.numberOfPronRecords = (unsigned int) pronInFirstRecord->numberOfPronRecords;
                //change cacheEntries
                wi->cacheEntries -= sizeof(PronInFirstRecord);
                wi->synCountRec -= (pronInFirstRecord->numberOfPronRecords + 1); //+1 for index
                if(wi->cacheEntries == 0)
                    wi->fastP = false;
            }

            if (wi->cacheEntries % sizeof(SynWordCountCache) != 0)
            {
                // TODO: invalid data
            }
        }
        wi->cacheEntries = wi->cacheEntries / sizeof(SynWordCountCache);
    }

    wi->wci = wcInit(file, wi->wordsCount, rec_with_words_compr_data,
                      recWithWordCache,
                      firstRecWithWords, wi->wordsRecsCount,
                      wi->maxWordLen);

    if (NULL == wi->wci)
    {
        goto Error;
    }

    wi->si = si_new(file, wi->synsetsCount,
                    4 + wi->wordsRecsCount + wi->synsetsInfoRecsCount,
                    4 + wi->wordsRecsCount, wi->bytesPerWordNum,
                    wi->fastP);

    if (NULL == wi->si)
    {
        goto Error;
    }

    if (!pcInit(file, &wi->defPackContext, rec_with_defs_compr_data))
    {
        goto Error;
    }

  Exit:
    if ( NULL != firstRecord ) fsUnlockRecord(file, 0);
    return wi;
  Error:
    if (wi)
    {
        wn_delete(wi);
        wi = NULL;
    }
    goto Exit;
}

void wn_delete(void *data)
{
    WnInfo *wi=(WnInfo*)data;
    if (!data)
        return;

    ebufFreeData(&wi->buffer);
    ebufFreeData(&wi->bufferTemp);
    diFreeData(&wi->displayInfo);


    pcFree(&wi->defPackContext);

    if (wi->wci)
        wcFree(wi->wci);

    if (wi->curDefData)
        new_free(wi->curDefData);

    if (wi->si)
        si_free(wi->file, wi->si);

    new_free(data);
}

long wn_get_words_count(void *data)
{
    return ((WnInfo *) data)->wordsCount;
}

long wn_get_first_matching(void *data, char *word)
{
    return wcGetFirstMatching(((WnInfo *) data)->file, ((WnInfo *) data)->wci, word);
}

char *wn_get_word(void *data, long wordNo)
{
    WnInfo *wi;
    wi = (WnInfo *) data;

    Assert(wordNo < wi->wordsCount);

    if (wordNo >= wi->wordsCount)
        return NULL;
    return wcGetWord(wi->file, wi->wci, wordNo);
}

#define CACHE_SPAN 1000

static SynWordCountCache *si_create_cache(AbstractFile* file, SynsetsInfo * si)
{
    long total_words_count = 0;
    long i;
    long cacheEntries;
    long cache_entry_no = 0;
    SynWordCountCache *swcc = NULL;

    Assert(si);

    cacheEntries = (si->synsetsCount / CACHE_SPAN) + 1;
    swcc = (SynWordCountCache *) new_malloc(cacheEntries * sizeof(SynWordCountCache));

    if (NULL == swcc)
        return NULL;

    total_words_count = si_get_words_count(file, si, 0);
    for (i = 1; i < si->synsetsCount; i++)
    {
        if (0 == (i % CACHE_SPAN))
        {
            Assert(cache_entry_no < cacheEntries);
            swcc[cache_entry_no].synsetNo = i;
            swcc[cache_entry_no].wordsCount = total_words_count;
            ++cache_entry_no;
        }
        total_words_count += si_get_words_count(file, si, i);
    }
    swcc[cache_entry_no].synsetNo = si->synsetsCount;
    swcc[cache_entry_no].wordsCount = total_words_count;
    return swcc;
}

SynsetsInfo *si_new(AbstractFile* file, long synsetsCount, int firstWordsNumRec,  int firstSynsetInfoRec, int bytesPerWordNum, Boolean fastP)
{
    SynsetsInfo *si = NULL;

    si = (SynsetsInfo *) new_malloc_zero(sizeof(SynsetsInfo));
    if (NULL == si)
        return NULL;
    si_init(file, si, firstWordsNumRec, firstSynsetInfoRec,
            bytesPerWordNum);
    si->synsetsCount = synsetsCount;
    if (fastP)
    {
        si->swcc = NULL;
    }
    else
    {
        si->swcc = si_create_cache(file, si);
        if (NULL == si->swcc)
        {
            si_free(file, si);
            return NULL;
        }
    }
    return si;
}

void si_init(AbstractFile* file, SynsetsInfo * si, int firstWordsNumRec, int firstSynsetInfoRec, int bytesPerWordNum)
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

void si_destroy(AbstractFile* file, SynsetsInfo * si)
{
    Assert(si);
    if (si->curWordsNumRec != -1)
    {
        fsUnlockRecord(file, si->curWordsNumRec);
    }
    if (si->curSynsetInfoRec != -1)
    {
        fsUnlockRecord(file, si->curSynsetInfoRec);
    }
    si_init(file, si, si->firstWordsNumRec, si->firstSynsetInfoRec,
            si->bytesPerWordNum);
}

void si_free(AbstractFile* file, SynsetsInfo * si)
{
    si_destroy(file, si);
    if (si->swcc)
    {
        new_free(si->swcc);
    }
    new_free(si);
}

void si_position_at_synset(AbstractFile* file, SynsetsInfo * si, long synset)
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
    while (rec_idx >= fsGetRecordSize(file, rec))
    {
        rec_idx -= fsGetRecordSize(file, rec);
        ++rec;
    }
    if (si->curSynsetInfoRec != rec)
    {
        if (si->curSynsetInfoRec != -1)
        {
            fsUnlockRecord(file, si->curSynsetInfoRec);
        }
        si->curSynsetInfoRec = rec;
        si->synsetDataStart = (unsigned char *) fsLockRecord(file, rec);
        if (NULL == si->synsetDataStart)
            return;
    }
    si->synsetData = si->synsetDataStart + rec_idx;
    si->synsetDataLeft = fsGetRecordSize(file, rec) - rec_idx;
    si->currSynset = synset;
}

/* return number of words in a given synset */
int si_get_words_count(AbstractFile* file, SynsetsInfo * si, long synset)
{
    int wordsCount;

    Assert(si);
    Assert(synset < 200000);

    si_position_at_synset(file, si, synset);

    Assert(synset == si->currSynset);

    wordsCount = *si->synsetData & 63;

    if (0 == wordsCount)
    {
        wordsCount = 1;
    }
    Assert(wordsCount > 0);

    return wordsCount;
}

/* get part of speech (i.e. noun, verb etc.) of this synset */
int si_get_pos(AbstractFile* file, SynsetsInfo * si, long synset)
{
    int pos;

    Assert(si);

    si_position_at_synset(file, si, synset);
    Assert(synset == si->currSynset);
    pos = (*si->synsetData >> 6);

    return pos;
}

/* how many total words are with all synsets up to this one */
long si_total_words_count(AbstractFile* file, SynsetsInfo * si, long synset)
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
        data = ((unsigned char *) fsLockRecord(file, 0)) + sizeof(WnFirstRecord);
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
        total_words_count += si_get_words_count(file, si, i);
    }
    if (data != NULL)
    {
        fsUnlockRecord(file, 0);
    }
    return total_words_count;
}

typedef enum
{ dir_forward, dir_backward }
direction;

void si_position_at_word(AbstractFile* file, SynsetsInfo * si, long synset, int word_idx)
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
            fsUnlockRecord(file, si->curWordsNumRec);
        }
        rec = si->firstWordsNumRec;
        total_words_count = si_total_words_count(file, si, synset) + word_idx;
        rec_idx = total_words_count * si->bytesPerWordNum;

        while (rec_idx >= fsGetRecordSize(file, rec))
        {
            rec_idx -= fsGetRecordSize(file, rec);
            ++rec;
        }
        si->curWordsNumRec = rec;
        si->wordsNumData = (unsigned char *) fsLockRecord(file, rec);
        if (NULL == si->wordsNumData)
            return;
        si->wordsNumData += rec_idx;
        si->wordsNumDataLeft = fsGetRecordSize(file, rec) - rec_idx;
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
                si_get_words_count(file, si,
                                   si->currSynsetWordsNum) - si->currWord;
            move_by += word_idx;
            for (syn = si->currSynsetWordsNum + 1; syn < synset; syn++)
            {
                move_by += si_get_words_count(file, si, syn);
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
            fsUnlockRecord(file, rec);
            ++rec;
            si->wordsNumData = (unsigned char *) fsLockRecord(file, rec);
            if (NULL == si->wordsNumData)
                return;
            si->wordsNumDataLeft = fsGetRecordSize(file, rec);
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
                move_by += si_get_words_count(file, si, syn);
            }
            move_by += (si_get_words_count(file, si, syn) - word_idx);
            move_by = move_by * si->bytesPerWordNum;
            rec = si->curWordsNumRec;
            if (move_by <=
                (fsGetRecordSize(file, rec) - si->wordsNumDataLeft))
            {
                /* within the record */
                si->wordsNumDataLeft += move_by;
                si->wordsNumData -= move_by;
            }
            else
            {
                move_by -=
                    (fsGetRecordSize(file, rec) - si->wordsNumDataLeft);
                fsUnlockRecord(file, rec);
                --rec;
                si->wordsNumData = (unsigned char *) fsLockRecord(file, rec);
                if (NULL == si->wordsNumData)
                    return;
                si->wordsNumData += (fsGetRecordSize(file, rec) - move_by);
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
int si_word_in_synset_forward_p(AbstractFile* file, SynsetsInfo * si, long *synset, long wordNo)
{
    long    wordNumFound;
    long    smallestWordNo = -1;
    int     i, j, wordsCount;
    long    wordsNumDataLeft;
    long    maxSynset;
    int     retValue;
    unsigned char *synsetData = NULL;
    unsigned char *wordNums = NULL;

    si_position_at_synset(file, si, *synset);
    synsetData = si->synsetData;
    maxSynset = si->synsetDataLeft;

    si_position_at_word(file, si, *synset, 0);
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
    si_destroy(file, si);
    return retValue;
}

int si_word_in_synset_backward_p(AbstractFile* file, SynsetsInfo * si, long *synset, long wordNo)
{
    long    wordNumFound;
    int     i, j, wordsCount;
    unsigned char *wordNums;
    unsigned char *synsetData;
    long    wordsNumDataLeft;
    long    maxSynset;
    int     retValue;

    si_position_at_synset(file, si, *synset);
    synsetData = si->synsetData;
    maxSynset = fsGetRecordSize(file, si->curSynsetInfoRec) - si->synsetDataLeft;

    /* position at the last word in this synset, so we can go backward */
    wordsCount = *synsetData & 63;
    si_position_at_word(file, si, *synset, 0);
    wordsNumDataLeft =
    fsGetRecordSize(file, si->curWordsNumRec) - si->wordsNumDataLeft;
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
#endif
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
#endif
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
    si_destroy(file, si);
    return retValue;
}

long si_get_word_no(AbstractFile* file, SynsetsInfo * si, long synset, int word_idx)
{
    long wordNumFound;
    unsigned char *data;

    Assert(word_idx < si_get_words_count(file, si, synset));

    si_position_at_word(file, si, synset, word_idx);
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
static int wn_get_synset_count(WnInfo * wi, long wordNo)
{
    unsigned char * data;
    int             synset_count;

    if (!wi->fastP)
    {
        return 0;
    }
    data = (unsigned char *) fsLockRecord(wi->file, wi->synCountRec);

    if (NULL == data)
        return 0;

    data += (wordNo / 4);

    synset_count = *data;
    synset_count = synset_count >> (2 * (wordNo % 4));
    synset_count &= 0x3;

    fsUnlockRecord(wi->file, wi->synCountRec);
    return synset_count;
}

/**
 *  Sorts only last inserted word!
 */
static Boolean wn_pseudoSortBuffer(ExtensibleBuffer *buf)
{
    int  i, n;
    char *txt;
    int  len;
    int  last = -1, prevLast = -1, prevPrevLast = -1;
    int  first = -1;

    txt = ebufGetDataPointer(buf);
    len = ebufGetDataSize(buf);
    
    //if buffer without '\0' at the end
    if(txt[len-1] != '\0')
        len++;
        
    //get number of elements (n)
    n = 0;
    for(i = 0; i < len-1; i++)
        if(txt[i] == (char)FORMAT_TAG)
            if(txt[i+1] == (char)FORMAT_POS)
            {
                if(first == -1)
                    first = i;
                prevPrevLast = prevLast;
                prevLast = last;
                last = i;
                n++;
            }
        
    if(n < 2)
        return false;
    Assert(last >= 0);
    Assert(first >= 0);
    Assert(prevLast >= 0);
    //we have at least 2 elements
    if(CmpPos(&txt[last], &txt[prevLast]) < 0)
    {   
        //need to sort
        i = 0;
        while(1)
        {
            if(txt[i] == (char)FORMAT_TAG)
                if(txt[i+1] == (char)FORMAT_POS)
                    if(CmpPos(&txt[last], &txt[i]) < 0)
                    {
                        XchgInBuffer(txt, i, last, len-1);
                        return true;
                    }
            i++;
        }        
    }

    //nothing sorted but...
    if(CmpPos(&txt[last], &txt[prevLast]) == 0)
    {
        if(n <= 2)
            return true; //we added 2nd word and it have the same POS ("1) " and "2) " added)
        else
        {
            Assert(prevPrevLast >= 0);
            if(CmpPos(&txt[prevPrevLast], &txt[prevLast]) != 0)
                return true; //we added 2nd with the same POS ("1) " and "2) " added)
        }
    }    
    else
    {
        if(CmpPos(&txt[first], &txt[prevLast]) == 0)
            return true; //we added 2nd diffrent POS ("I " and "II " added)
    }
    return false;
}


Err wn_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di)
{
    WnInfo *    wi = NULL;
    char *      rawTxt = NULL;
    long        defDataSize = 0;
    int         len;
    char *      word;
    int         unpackedLen;

    int         syn_count;
    int         syn_found_count = 0;
    int         wordsCount;
    int         partOfSpeech;
    long        wordNumFound;
    long        w;
    int         first_rec_with_defs_len;
    int         first_rec_with_defs;
    int         defs_record = 0;
    long        def_offset = 0;
    char *      txt = NULL;
    long        syn;
    int         res;
    long        start_syn;
    direction   dir;
    Boolean     end_p;
    AppContext* appContext=GetAppContext();

    SynsetsInfo *   si;
    unsigned char * defData = NULL;
    unsigned char * unpackedDef = NULL;

    wi = (WnInfo *) data;

    Assert(data);
    Assert((wordNo >= 0) && (wordNo < wi->wordsCount));
    Assert(dx > 0);
    Assert(di);

    ebufReset(&wi->buffer);

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

    if(FormatWantsWord(appContext))
    {
        ebufAddChar(&wi->buffer, FORMAT_TAG);
        ebufAddChar(&wi->buffer, FORMAT_SYNONYM);
        word = wn_get_word((void *) wi, wordNo);
        ebufAddStr(&wi->buffer, word);
        ebufAddChar(&wi->buffer, '\n');
        //pronunciation
        if(appContext->pronData.isPronInUsedDictionary
            && appContext->prefs.displayPrefs.enablePronunciation
        )
        {
            ebufAddChar(&wi->buffer, FORMAT_TAG);
            ebufAddChar(&wi->buffer, FORMAT_PRONUNCIATION);
            if(pronAddPronunciationToBuffer(appContext, &wi->buffer, wi->file, wordNo, word))
                ebufAddChar(&wi->buffer, '\n');
        }
    }
    //make sure that temp buffer is not empty
    ebufCopyBuffer(&wi->bufferTemp, &wi->buffer);
    ebufAddChar(&wi->bufferTemp, '\0');

    while (!end_p)
    {
        Assert(syn < wi->synsetsCount);

        if (dir == dir_forward)
        {
            res = si_word_in_synset_forward_p(wi->file, si, &syn, wordNo);
        }
        else
        {
            res = si_word_in_synset_backward_p(wi->file, si, &syn, wordNo);
        }

        /* if word found add up the definition */
        if (2 == res)
        {
            ebufAddChar(&wi->buffer, FORMAT_TAG);
            ebufAddChar(&wi->buffer, FORMAT_POS);

            ebufAddChar(&wi->buffer, 149);
            ebufAddChar(&wi->buffer, ' ');

            partOfSpeech = si_get_pos(wi->file, si, syn);
            txt = GetWnPosTxt(partOfSpeech);
            ebufAddStr(&wi->buffer, txt);

            wordsCount = si_get_words_count(wi->file, si, syn);
            Assert(wordsCount > 0);

            if(wordsCount > 1 && FormatWantsWord(appContext))
            {
                ebufAddChar(&wi->buffer, FORMAT_TAG);
                ebufAddChar(&wi->buffer, FORMAT_WORD);
            }
            else
            if(!FormatWantsWord(appContext))
            {
                ebufAddChar(&wi->buffer, FORMAT_TAG);
                ebufAddChar(&wi->buffer, FORMAT_WORD);
            }
            //all words
            if(!FormatWantsWord(appContext))
                for (w = 0; w < wordsCount; w++)
                {
                    wordNumFound = si_get_word_no(wi->file, si, syn, w);
                    word = wn_get_word((void *) wi, wordNumFound);
                
                    ebufAddStr(&wi->buffer, word);
                    if (w == wordsCount - 1)
                    {
                        txt = "\n";
                    }
                    else
                    {
                        txt = ", ";
                    }
                    ebufAddStr(&wi->buffer, txt);
                }
            else
            //only synonyms!
            if(wordsCount > 1)
            {  
                wordsCount--;
                for (w = 0; w < wordsCount; w++)
                {
                    wordNumFound = si_get_word_no(wi->file, si, syn, w);
                    if(wordNo == wordNumFound)
                    {
                        w++;
                        wordsCount++;
                        wordNumFound = si_get_word_no(wi->file, si, syn, w);
                    }
                    word = wn_get_word((void *) wi, wordNumFound);
                    ebufAddStr(&wi->buffer, word);
                    if (w == wordsCount - 1)
                    {
                        txt = "\n";
                    }
                    else
                    {
                        txt = ", ";
                    }
                    ebufAddStr(&wi->buffer, txt);
                }
            }

            first_rec_with_defs_len = 4 + wi->wordsRecsCount +
                wi->synsetsInfoRecsCount + wi->wordsNumRecsCount;

            first_rec_with_defs =
                first_rec_with_defs_len + wi->defsLenRecsCount;

            if ( !get_defs_record(wi->file, syn, first_rec_with_defs_len,
                            wi->defsLenRecsCount, first_rec_with_defs,
                            &defs_record, &def_offset, &defDataSize, wi->wci) )
            {
                return NULL;
            }

            Assert(defs_record > 4);
            Assert((def_offset >= 0) && (def_offset < 66000));
            Assert((defDataSize >= 0));

            defData = (unsigned char *) fsLockRecord(wi->file, defs_record);
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
            fsUnlockRecord(wi->file, defs_record);

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
                    ebufAddChar(&wi->buffer, FORMAT_TAG);
                    ebufAddChar(&wi->buffer, FORMAT_DEFINITION);
                    txt = " ";
                }
                else
                {
                    ebufAddChar(&wi->buffer, FORMAT_TAG);
                    ebufAddChar(&wi->buffer, FORMAT_EXAMPLE);
                    txt = " \"";
                }

                ebufAddStr(&wi->buffer, txt);
                ebufAddStrN(&wi->buffer, (char *) (unpackedDef - len), len);

                if (0 == unpackedDef[0])
                {
                    ebufAddStr(&wi->buffer, " \"");
                }
                ebufAddChar(&wi->buffer, '\n');
                --unpackedLen;
                ++unpackedDef;
                Assert(unpackedLen >= 0);
            }

            //pseudo sort - only one definition may be not in right place
            if(appContext->prefs.displayPrefs.listStyle!=0)
                if(wn_pseudoSortBuffer(&wi->buffer))
                {
                    SetGlobalBackColor(appContext);
                    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, appContext->screenWidth - DRAW_DI_X - 8, appContext->screenHeight - DRAW_DI_Y - FRM_RSV_H);
                }

            ebufCopyBuffer(&wi->bufferTemp, &wi->buffer);
            ebufAddChar(&wi->bufferTemp, '\0');

//speed = TimGetTicks();
            ebufWrapBigLines(&wi->bufferTemp,false);
//        
//appContext->recordSpeedTicksCount += TimGetTicks()-speed;

            rawTxt = ebufGetDataPointer(&wi->bufferTemp);
            diSetRawTxt(&wi->displayInfo, rawTxt);
            if (0 == syn_found_count)
            {
                SetGlobalBackColor(appContext);            
                ClearRectangle(DRAW_DI_X, DRAW_DI_Y, appContext->screenWidth - DRAW_DI_X - 8, appContext->screenHeight - DRAW_DI_Y - FRM_RSV_H);
            }
            DrawDisplayInfo(&wi->displayInfo, 0, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
            SetScrollbarState(&wi->displayInfo, appContext->dispLinesCount, 0);
            DrawWord(SEARCH_TXT, appContext->screenHeight-FRM_RSV_H+5);

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

    si_destroy(wi->file, si);

//    ebufAddChar(&wi->buffer, '\0');
//    ebufWrapBigLines(&wi->buffer,false);
    //used to skip ebufWrapBigLines()
    ebufCopyBuffer(&wi->buffer,&wi->bufferTemp);

    rawTxt = ebufGetDataPointer(&wi->buffer);
    diSetRawTxt(di, rawTxt);
    SetScrollbarState(di, appContext->dispLinesCount, 0);
    return 0;
}
