/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include "wn_lite_ex_support.h"
#include "common.h"
#include "extensible_buffer.h"

/* this thing handles records that contain sets of 24-bit numbers,
   when the last number in the set has 24-bit set to 1 (thus
   limiting numbers to 23-bits). It makes getting consecutive
   numbers easier */
typedef struct
{
    int             firstRecord;
    int             recordsCount;
    int             currentRecord;
    unsigned char   *data;
    long            bytesLeftInRecord;
} numIter;

static long get_24bit_number(unsigned char *data, int *lastNumberP);
static void numIterInit(numIter * ni, int firstRecord,  int recsCount);
static void numIterDestroy(numIter * ni);
static void numIterLockRecord(numIter * ni, int record);
static void numIterUnlockRecord(numIter * ni, int record);
static long numIterGetNextNumber(numIter * ni,  int *lastNumberP);
static void numIterSkipNumbers(numIter * ni,  long numCount,  long *firstLemmaInRecord);
static ExtensibleBuffer g_buf = { 0 };

void wnlex_delete(void *data)
{
    WnLiteInfo *wi;

    ebufFreeData(&g_buf);
    if (!data)
        return;

    wi = (WnLiteInfo *) data;

    pcFree(&wi->defPackContext);

    if (wi->wci)
        wcFree(wi->wci);

    if (wi->curDefData)
        new_free(wi->curDefData);

    if (wi->firstLemmaInWordInfoRec)
        new_free(wi->firstLemmaInWordInfoRec);

    if (wi->synsets)
        new_free(wi->synsets);

    new_free(data);
    return;
}

void *wnlex_new(void)
{
    WnLiteInfo *        wi = NULL;
    WnLiteFirstRecord * firstRecord = NULL;
    int                 recWithComprData;
    int                 recWithWordCache;
    int                 firstRecWithWords;
    int                 i;

    wi = (WnLiteInfo *) new_malloc_zero(sizeof(WnLiteInfo));
    if (NULL == wi)
    {
        LogG("wnlex_new(), new_malloc(sizeof(WnLiteInfo)) failed" );
        goto Error;
    }

    firstRecord = (WnLiteFirstRecord *) CurrFileLockRecord(0);
    if ( NULL == firstRecord )
    {
        LogG("wnlex_new(), CurrFileLockRecord(0) failed" );
        goto Error;
    }

    wi->recordsCount = CurrFileGetRecordsCount();
    LogV1("wnlex_new(), recs count=%ld", (long)wi->recordsCount);
    wi->wordsCount = firstRecord->wordsCount;
    wi->synsetsCount = firstRecord->synsetsCount;
    wi->wordsRecordsCount = firstRecord->wordsRecordsCount;
    wi->wordsInfoRecordsCount = firstRecord->wordsInfoRecordsCount;
    wi->synsetDefLenRecordsCount = firstRecord->synsetDefLenRecordsCount;
    wi->synsetDefRecordsCount = firstRecord->synsetDefRecordsCount;
    wi->maxWordLen = firstRecord->maxWordLen;
    wi->maxDefLen = firstRecord->maxDefLen;
    wi->maxComprDefLen = firstRecord->maxComprDefLen;
    wi->maxSynsetsForWord = firstRecord->maxSynsetsForWord;

    LogV1("wnlex_new(), words count=%ld", (long)wi->wordsCount);

    wi->firstLemmaInWordInfoRec = (long *) new_malloc((1 + wi->wordsInfoRecordsCount) * sizeof(long));

    if (NULL == wi->firstLemmaInWordInfoRec)
    {
        LogV1("wnlex_new(), new_mallloc() failed, wi->firstLemmaInWordInfoRec is NULL, wi->wordsInfoRecordsCount=%ld", (long)wi->wordsInfoRecordsCount);
        goto Error;
    }

    for (i = 0; i < wi->wordsInfoRecordsCount; i++)
    {
        wi->firstLemmaInWordInfoRec[i] = (&firstRecord->firstLemmaInWordInfoRec)[i];
    }
    /* guardian */
    wi->firstLemmaInWordInfoRec[wi->wordsInfoRecordsCount] = wi->wordsCount + 1;

    wi->synsets = (SynsetDef *) new_malloc(wi->maxSynsetsForWord * sizeof(SynsetDef));
    if (NULL == wi->synsets)
    {
        LogG("wnlex_new(), new_mallloc() failed, wi->synsets is NULL");
        goto Error;
    }
    wi->curDefData = (unsigned char *) new_malloc(wi->maxDefLen + 2);
    if (NULL == wi->curDefData)
    {
        LogG("wnlex_new(), new_mallloc() failed, wi->curDefData is NULL");
        goto Error;
    }

    recWithComprData = 2;
    recWithWordCache = 1;
    firstRecWithWords = 4 + wi->synsetDefLenRecordsCount + wi->wordsInfoRecordsCount;

    wi->wci = (WcInfo *) wcInit(wi->wordsCount, recWithComprData,
                                 recWithWordCache, firstRecWithWords,
                                 wi->wordsRecordsCount, wi->maxWordLen);

    if (NULL == wi->wci)
    {
        LogG("wnlex_new(), wcInit() failed");
        goto Error;
    }

    if ( !pcInit(&wi->defPackContext, 3) )
    {
        LogG("wnlex_new(), pcInit() failed");
        goto Error;
    }

Exit:
    if (NULL != firstRecord) CurrFileUnlockRecord(0);
    LogG( "wnlex_new() ok" );
    return (void *) wi;
Error:
    LogG( "wnlex_new() failed" );
    if (wi)
        wnlex_delete((void *) wi);
    wi = NULL;
    goto Exit;
}

long wnlex_get_words_count(void *data)
{
    return ((WnLiteInfo *) data)->wordsCount;
}

long wnlex_get_first_matching(void *data, char *word)
{
    return wcGetFirstMatching(((WnLiteInfo *) data)->wci, word);
}

char *wnlex_get_word(void *data, long wordNo)
{
    WnLiteInfo *wi;
    wi = (WnLiteInfo *) data;

    Assert(wordNo < wi->wordsCount);

    if (wordNo >= wi->wordsCount)
        return NULL;
    return wcGetWord(wi->wci, wordNo);
}

long get_24bit_number(unsigned char *data, int *lastNumberP)
{
    long number;
    Assert(data);
    Assert(lastNumberP);

    if ((data[0] & 0x80))
        *lastNumberP = 1;
    else
        *lastNumberP = 0;
    number =
        ((long) (data[0] & 0x7f)) * 256 * 256 + 256 * ((long) data[1]) +
        (long) data[2];
    Assert(number >= 0);
    return number;
}

void numIterInit(numIter * ni, int firstRecord, int recsCount)
{
    Assert(ni);
    ni->firstRecord = firstRecord;
    ni->recordsCount = recsCount;
    ni->currentRecord = -1;
    ni->data = NULL;
}

void numIterDestroy(numIter * ni)
{
    Assert(ni);
    if (-1 != ni->currentRecord)
    {
        numIterUnlockRecord(ni, ni->currentRecord);
        numIterInit(ni, 0, 0);
    }
}

void numIterLockRecord(numIter * ni, int record)
{
    Assert(ni);
    Assert((record >= ni->firstRecord) &&  (record < (ni->firstRecord + ni->recordsCount)));

    if (-1 != ni->currentRecord)
        CurrFileUnlockRecord(ni->currentRecord);

    ni->currentRecord = record;
    ni->data = (unsigned char *) CurrFileLockRecord(record);
    ni->bytesLeftInRecord = CurrFileGetRecordSize(record);
    Assert(0 == (ni->bytesLeftInRecord % 3));
}

void numIterUnlockRecord(numIter * ni, int record)
{
    Assert(ni);
    Assert((record >= ni->firstRecord) &&  (record < (ni->firstRecord + ni->recordsCount)));
    CurrFileUnlockRecord(record);
    ni->data = NULL;
    ni->currentRecord = -1;
}

long numIterGetNextNumber(numIter * ni, int *lastNumberP)
{
    long number;

    Assert(ni);
    Assert(lastNumberP);

    if (-1 == ni->currentRecord)
    {
        numIterLockRecord(ni, ni->firstRecord);
    }
    else if (0 == ni->bytesLeftInRecord)
    {
        ++ni->currentRecord;
        numIterLockRecord(ni, ni->currentRecord);
    }

    number = get_24bit_number(ni->data, lastNumberP);
    ni->data += 3;
    ni->bytesLeftInRecord -= 3;
    return number;
}

void numIterSkipNumbers(numIter * ni, long numCount,  long *firstLemmaInRecord)
{
    Boolean lastNumberP;
    int     idx;

    Assert(ni);
    Assert(firstLemmaInRecord);
    Assert(NULL == ni->data);

    idx = 0;
    while (numCount >= firstLemmaInRecord[idx + 1])
        ++idx;

    numCount -= firstLemmaInRecord[idx];

    numIterLockRecord(ni, ni->firstRecord + idx);

    while (numCount > 0)
    {
        do
        {
            if (0 == ni->bytesLeftInRecord)
            {
                ++ni->currentRecord;
                numIterLockRecord(ni, ni->currentRecord);
            }
            if (ni->data[0] & 0x80)
                lastNumberP = true;
            else
                lastNumberP = false;
            ni->data += 3;
            ni->bytesLeftInRecord -= 3;
        }
        while (!lastNumberP);
        --numCount;
    }
}

Err wnlex_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di)
{
    WnLiteInfo      *wi = NULL;
    long            synsetNo;
    long            thisWordNo;
    int             lastNumberP;
    int             lastWordP;
    long            defDataSize = 0;
    int             len;
    char            partOfSpeech;
    char            *word;
    char            *rawTxt;
    int             unpackedLen;
    int             synsetsCount;
    int             currentSynset;
    int             insertIndex;
    unsigned char   *defData = NULL;
    unsigned char   *defDataCopy = NULL;
    unsigned char   *unpackedDef = NULL;
    numIter         ni;

    wi = (WnLiteInfo *) data;

    Assert(data);
    Assert((wordNo >= 0) && (wordNo < wi->wordsCount));
    Assert(dx > 0);
    Assert(di);

    ebufReset(&g_buf);

    /* first find all synsets this word belongs to */
    numIterInit(&ni, 4 + wi->synsetDefLenRecordsCount,  wi->wordsInfoRecordsCount);
    numIterSkipNumbers(&ni, wordNo, wi->firstLemmaInWordInfoRec);

    /* so we're positioned at the right word */
    synsetsCount = 0;
    do
    {
        synsetNo = numIterGetNextNumber(&ni, &lastNumberP);
        /* insert in sorted order */
        /* TODO: put this in a converter */
        insertIndex = 0;
        while ((insertIndex < synsetsCount) &&
               (synsetNo > (long) wi->synsets[insertIndex].synsetNo))
        {
            ++insertIndex;
        }
        if (insertIndex < synsetsCount)
        {
            /* have to make room for it */
            MemMove(&(wi->synsets[insertIndex + 1]), &(wi->synsets[insertIndex]), (synsetsCount - insertIndex) * sizeof(SynsetDef));
        }
        wi->synsets[insertIndex].synsetNo = synsetNo;
        ++synsetsCount;
        Assert(synsetsCount <= wi->maxSynsetsForWord);
    }
    while (!lastNumberP);

    if ( !get_defs_records(synsetsCount, 4, wi->synsetDefLenRecordsCount, 4 + wi->synsetDefLenRecordsCount +  wi->wordsRecordsCount +  wi->wordsInfoRecordsCount, wi->synsets) )
    {
        /* TODO: repleace with some meaningful err* */
        return 1;
    }

    currentSynset = 0;
    while (currentSynset < synsetsCount)
    {
/*          defData = (unsigned char *)CurrFileLockRecord(wi->synsets[currentSynset].record); */
/*          defData += wi->synsets[currentSynset].offset; */
/*          defDataSize = wi->synsets[currentSynset].dataSize; */

        defDataSize = wi->synsets[currentSynset].dataSize;
        defData = (unsigned char *) CurrFileLockRegion(wi->synsets[currentSynset].record, wi->synsets[currentSynset].offset, defDataSize);
        Assert((defDataSize >= 4) && (defDataSize <= wi->maxComprDefLen));
        defDataCopy = defData;

        ebufAddChar(&g_buf, 149);
        ebufAddChar(&g_buf, ' ');
        partOfSpeech = defData[0];

        switch (partOfSpeech)
        {
        case 'n':
            ebufAddStr(&g_buf, "(noun.) ");
            break;
        case 'a':
        case 's':
            ebufAddStr(&g_buf, "(adj.) ");
            break;
        case 'r':
            ebufAddStr(&g_buf, "(adv.) ");
            break;
        case 'v':
            ebufAddStr(&g_buf, "(verb) ");
            break;
        default:
            Assert(0);
            break;
        }

        ++defData;
        --defDataSize;

        /* read all words */
        do
        {
            thisWordNo = get_24bit_number(defData, &lastWordP);
            defData += 3;
            defDataSize -= 3;

            word = wnlex_get_word((void *) wi, thisWordNo);
            ebufAddStr(&g_buf, word);
            if (lastWordP)
            {
                ebufAddChar(&g_buf, '\n');
            }
            else
            {
                ebufAddStr(&g_buf, ", ");
            }
        }
        while (!lastWordP);

        Assert(defDataSize > 0);
        pcReset(&wi->defPackContext, defData, 0);
        pcUnpack(&wi->defPackContext, defDataSize, wi->curDefData, &unpackedLen);

        Assert((unpackedLen > 0) && (unpackedLen <= wi->maxDefLen));
        wi->curDefLen = unpackedLen;
        unpackedDef = wi->curDefData;
        Assert((1 == unpackedDef[unpackedLen - 1]) || (0 == unpackedDef[unpackedLen - 1]));
/*          CurrFileUnlockRecord(wi->synsets[currentSynset].record); */
        CurrFileUnlockRegion((char*)defDataCopy);

        while (unpackedLen > 0)
        {
            len = 0;
            /* 0 at the end means example, 1 means definition */
            while ((0 != unpackedDef[0]) && (1 != unpackedDef[0]))
            {
                ++len;
                ++unpackedDef;
                --unpackedLen;
            }

            if (1 == unpackedDef[0])
            {
                ebufAddStr(&g_buf, " ");
            }
            else
            {
                ebufAddStr(&g_buf, " \"");
            }

            ebufAddStrN(&g_buf, (char *) unpackedDef - len, len);
            if (0 == unpackedDef[0])
            {
                ebufAddStr(&g_buf, " \"");
            }
            ebufAddChar(&g_buf, '\n');
            --unpackedLen;
            ++unpackedDef;
            Assert(unpackedLen >= 0);
        }
        ++currentSynset;
    }

    numIterDestroy(&ni);

    ebufAddChar(&g_buf, '\0');
    ebufWrapBigLines(&g_buf);

    rawTxt = ebufGetDataPointer(&g_buf);

    diSetRawTxt(di, rawTxt);
    return 0;
}

