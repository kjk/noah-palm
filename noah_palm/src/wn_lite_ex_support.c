/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "wn_lite_ex_support.h"

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
static void numIterDestroy(AbstractFile* file, numIter * ni);
static void numIterLockRecord(AbstractFile* file, numIter * ni, int record);
static void numIterUnlockRecord(AbstractFile* file, numIter * ni, int record);
static long numIterGetNextNumber(AbstractFile* file, numIter * ni,  int *lastNumberP);
static void numIterSkipNumbers(AbstractFile* file, numIter * ni,  long numCount,  long *firstLemmaInRecord);

//static ExtensibleBuffer g_buf = { 0 };

void wnlex_delete(void *data)
{
    WnLiteInfo *wi= (WnLiteInfo *) data;
    if (!data)
        return;

    ebufFreeData(&wi->buffer);

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

void *wnlex_new(AbstractFile* file)
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
    ebufInit(&wi->buffer, 0);
    firstRecord = (WnLiteFirstRecord *) fsLockRecord(file, 0);
    if ( NULL == firstRecord )
    {
        LogG("wnlex_new(), CurrFileLockRecord(0) failed" );
        goto Error;
    }
    wi->file=file;
    wi->recordsCount = fsGetRecordsCount(file);
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

    wi->wci = (WcInfo *) wcInit(file, wi->wordsCount, recWithComprData,
                                 recWithWordCache, firstRecWithWords,
                                 wi->wordsRecordsCount, wi->maxWordLen);

    if (NULL == wi->wci)
    {
        LogG("wnlex_new(), wcInit() failed");
        goto Error;
    }

    if ( !pcInit(file, &wi->defPackContext, 3) )
    {
        LogG("wnlex_new(), pcInit() failed");
        goto Error;
    }

Exit:
    if (NULL != firstRecord) fsUnlockRecord(file, 0);
    LogG( "wnlex_new() ok" );
    return wi;
Error:
    LogG( "wnlex_new() failed" );
    if (wi)
        wnlex_delete(wi);
    wi = NULL;
    goto Exit;
}

long wnlex_get_words_count(void *data)
{
    return ((WnLiteInfo *) data)->wordsCount;
}

long wnlex_get_first_matching(void *data, char *word)
{
    return wcGetFirstMatching(((WnLiteInfo *) data)->file, ((WnLiteInfo *) data)->wci, word);
}

char *wnlex_get_word(void *data, long wordNo)
{
    WnLiteInfo *wi;
    wi = (WnLiteInfo *) data;

    Assert(wordNo < wi->wordsCount);

    if (wordNo >= wi->wordsCount)
        return NULL;
    return wcGetWord(wi->file, wi->wci, wordNo);
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

void numIterDestroy(AbstractFile* file, numIter * ni)
{
    Assert(ni);
    if (-1 != ni->currentRecord)
    {
        numIterUnlockRecord(file, ni, ni->currentRecord);
        numIterInit(ni, 0, 0);
    }
}

void numIterLockRecord(AbstractFile* file, numIter * ni, int record)
{
    Assert(ni);
    Assert((record >= ni->firstRecord) &&  (record < (ni->firstRecord + ni->recordsCount)));

    if (-1 != ni->currentRecord)
        fsUnlockRecord(file, ni->currentRecord);

    ni->currentRecord = record;
    ni->data = (unsigned char *) fsLockRecord(file, record);
    ni->bytesLeftInRecord = fsGetRecordSize(file, record);
    Assert(0 == (ni->bytesLeftInRecord % 3));
}

void numIterUnlockRecord(AbstractFile* file, numIter * ni, int record)
{
    Assert(ni);
    Assert((record >= ni->firstRecord) &&  (record < (ni->firstRecord + ni->recordsCount)));
    fsUnlockRecord(file, record);
    ni->data = NULL;
    ni->currentRecord = -1;
}

long numIterGetNextNumber(AbstractFile* file, numIter * ni, int *lastNumberP)
{
    long number;

    Assert(ni);
    Assert(lastNumberP);

    if (-1 == ni->currentRecord)
    {
        numIterLockRecord(file, ni, ni->firstRecord);
    }
    else if (0 == ni->bytesLeftInRecord)
    {
        ++ni->currentRecord;
        numIterLockRecord(file, ni, ni->currentRecord);
    }

    number = get_24bit_number(ni->data, lastNumberP);
    ni->data += 3;
    ni->bytesLeftInRecord -= 3;
    return number;
}

void numIterSkipNumbers(AbstractFile* file, numIter * ni, long numCount,  long *firstLemmaInRecord)
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

    numIterLockRecord(file, ni, ni->firstRecord + idx);

    while (numCount > 0)
    {
        do
        {
            if (0 == ni->bytesLeftInRecord)
            {
                ++ni->currentRecord;
                numIterLockRecord(file, ni, ni->currentRecord);
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
    AppContext      *appContext = GetAppContext();

    wi = (WnLiteInfo *) data;

    Assert(data);
    Assert((wordNo >= 0) && (wordNo < wi->wordsCount));
    Assert(dx > 0);
    Assert(di);

    ebufReset(&wi->buffer);

    /* first find all synsets this word belongs to */
    numIterInit(&ni, 4 + wi->synsetDefLenRecordsCount,  wi->wordsInfoRecordsCount);
    numIterSkipNumbers(wi->file, &ni, wordNo, wi->firstLemmaInWordInfoRec);

    /* so we're positioned at the right word */
    synsetsCount = 0;
    do
    {
        synsetNo = numIterGetNextNumber(wi->file, &ni, &lastNumberP);
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

    if ( !get_defs_records(wi->file, synsetsCount, 4, wi->synsetDefLenRecordsCount, 4 + wi->synsetDefLenRecordsCount +  wi->wordsRecordsCount +  wi->wordsInfoRecordsCount, wi->synsets) )
    {
        /* TODO: repleace with some meaningful err* */
        return 1;
    }

    if(FormatWantsWord(appContext))
    {
        ebufAddChar(&wi->buffer, FORMAT_TAG);
        ebufAddChar(&wi->buffer, FORMAT_SYNONYM);
        word = wnlex_get_word(wi, wordNo);
        ebufAddStr(&wi->buffer, word);
        ebufAddChar(&wi->buffer, '\n');
    }

    currentSynset = 0;
    while (currentSynset < synsetsCount)
    {
/*          defData = (unsigned char *)CurrFileLockRecord(wi->synsets[currentSynset].record); */
/*          defData += wi->synsets[currentSynset].offset; */
/*          defDataSize = wi->synsets[currentSynset].dataSize; */

        defDataSize = wi->synsets[currentSynset].dataSize;
        defData = (unsigned char *) fsLockRegion(wi->file, wi->synsets[currentSynset].record, wi->synsets[currentSynset].offset, defDataSize);
        Assert((defDataSize >= 4) && (defDataSize <= wi->maxComprDefLen));
        defDataCopy = defData;


        ebufAddChar(&wi->buffer, FORMAT_TAG);
        ebufAddChar(&wi->buffer, FORMAT_POS);

        ebufAddChar(&wi->buffer, 149);
        ebufAddChar(&wi->buffer, ' ');
        partOfSpeech = defData[0];

        switch (partOfSpeech)
        {
        case 'n':
            ebufAddStr(&wi->buffer, "(noun.) ");
            break;
        case 'a':
        case 's':
            ebufAddStr(&wi->buffer, "(adj.) ");
            break;
        case 'r':
            ebufAddStr(&wi->buffer, "(adv.) ");
            break;
        case 'v':
            ebufAddStr(&wi->buffer, "(verb) ");
            break;
        default:
            Assert(0);
            break;
        }

        ++defData;
        --defDataSize;

        /*TODO: remove word from list of synonyms!!!*/
        ebufAddChar(&wi->buffer, FORMAT_TAG);
        ebufAddChar(&wi->buffer, FORMAT_WORD);

        /* read all words */
        do
        {
            thisWordNo = get_24bit_number(defData, &lastWordP);
            defData += 3;
            defDataSize -= 3;

            word = wnlex_get_word(wi, thisWordNo);
            ebufAddStr(&wi->buffer, word);
            if (lastWordP)
            {
                ebufAddChar(&wi->buffer, '\n');
            }
            else
            {
                ebufAddStr(&wi->buffer, ", ");
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
        fsUnlockRegion(wi->file, (char*)defDataCopy);

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
                ebufAddChar(&wi->buffer, FORMAT_TAG);
                ebufAddChar(&wi->buffer, FORMAT_DEFINITION);
                ebufAddStr(&wi->buffer, " ");
            }
            else
            {
                ebufAddChar(&wi->buffer, FORMAT_TAG);
                ebufAddChar(&wi->buffer, FORMAT_EXAMPLE);
                ebufAddStr(&wi->buffer, " \"");
            }

            ebufAddStrN(&wi->buffer, (char *) unpackedDef - len, len);
            if (0 == unpackedDef[0])
            {
                ebufAddStr(&wi->buffer, " \"");
            }
            ebufAddChar(&wi->buffer, '\n');
            --unpackedLen;
            ++unpackedDef;
            Assert(unpackedLen >= 0);
        }
        ++currentSynset;
    }

    numIterDestroy(wi->file, &ni);

    ebufAddChar(&wi->buffer, '\0');
    ebufWrapBigLines(&wi->buffer);

    rawTxt = ebufGetDataPointer(&wi->buffer);

    diSetRawTxt(di, rawTxt);
    return 0;
}

