/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "cw_defs.h"

#ifndef NOAH_PRO
#error "only works in NOAH_PRO"
#endif

#ifndef SIMPLE_DICT
#error "SIMPLE_DICT not defined"
#endif

#include "simple_support.h"
#include "extensible_buffer.h"
#include "fs.h"
 
static ExtensibleBuffer g_buf = { 0 };

void simple_delete(void *data)
{
    SimpleInfo *si;

    ebufFreeData(&g_buf);

    if (!data)
        return;

    si = (SimpleInfo *) data;

    pcFree(&si->defPackContext);

    if (si->wci)
        wcFree(si->wci);

    if (si->curDefData)
        new_free(si->curDefData);

    new_free(data);
}

void *simple_new(void)
{
    SimpleInfo *        si = NULL;
    SimpleFirstRecord * firstRecord;
    int                 recWithComprData;
    int                 recWithWordCache;
    int                 firstRecWithWords;

    si = (SimpleInfo *) new_malloc_zero(sizeof(SimpleInfo));
    if (NULL == si)
        goto Error;

    firstRecord = (SimpleFirstRecord *) CurrFileLockRecord(0);
    if (NULL == firstRecord)
        goto Error;

    Assert(sizeof(SimpleFirstRecord) == CurrFileGetRecordSize(0));

    si->recordsCount = CurrFileGetRecordsCount();
    si->wordsCount = firstRecord->wordsCount;

    si->wordsRecsCount = firstRecord->wordsRecsCount;
    si->defsLenRecsCount = firstRecord->defsLenRecsCount;
    si->defsRecsCount = firstRecord->defsRecsCount;

    si->maxWordLen = firstRecord->maxWordLen;
    si->maxDefLen = firstRecord->maxDefLen;
    si->posRecsCount = firstRecord->posRecsCount;

    si->curDefData = (unsigned char *) new_malloc(si->maxDefLen + 2);
    if (NULL == si->curDefData)
        goto Error;

    recWithWordCache = 1;
    recWithComprData = 2;
    firstRecWithWords = 4;

    si->wci = wcInit(si->wordsCount, recWithComprData, recWithWordCache, 
                        firstRecWithWords, si->wordsRecsCount, si->maxWordLen);

    if (NULL == si->wci)
        goto Error;

    if (!pcInit(&si->defPackContext, 3))
        goto Error;

  Exit:
    CurrFileUnlockRecord(0);
    return (void *) si;
  Error:
    if (si)
        simple_delete((void *) si);
    si = NULL;
    goto Exit;
}

long simple_get_words_count(void *data)
{
    return ((SimpleInfo *) data)->wordsCount;
}

long simple_get_first_matching(void *data, char *word)
{
    return wcGetFirstMatching(((SimpleInfo *) data)->wci, word);
}

char *simple_get_word(void *data, long wordNo)
{
    SimpleInfo *si;
    si = (SimpleInfo *) data;

    Assert(wordNo < si->wordsCount);

    if (wordNo >= si->wordsCount)
    {
        return NULL;
    }
    return wcGetWord(si->wci, wordNo);
}

#ifndef DEMO
Err simple_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di)
{
    SimpleInfo *    si = NULL;
    char *          word;
    char *          rawTxt;
    int             unpackedLen;
    unsigned char * unpackedDef = NULL;
    long            i;

    unsigned char * posData;
    int             posRecord;
    int             partOfSpeech;
    unsigned char * defLensData = NULL;
    long            defLensDataLeft = 0;
    long            defDataSize = 0;
    unsigned char * defData = NULL;
    int             defRecord = 0;
    long            defOffset = 0;
    long            idxInRec;
    int             recWithDefLens = 0;

    si = (SimpleInfo *) data;

    Assert(data);
    Assert((wordNo >= 0) && (wordNo < si->wordsCount));
    Assert(dx > 0);
    Assert(di);

    recWithDefLens = 4 + si->wordsRecsCount;
    defRecord = recWithDefLens + si->defsLenRecsCount;
    posRecord = defRecord + si->defsRecsCount;

    defLensDataLeft = 0;
    defLensData = NULL;
    idxInRec = 0;
    ebufReset(&g_buf);

    for (i = 0; i < wordNo; i++)
    {
        if (0 == defLensDataLeft)
        {
            if (defLensData)
            {
                CurrFileUnlockRecord(recWithDefLens - 1);
            }
            defLensData = (unsigned char *) CurrFileLockRecord(recWithDefLens);
            if (NULL == defLensData)
                return NULL;
            defLensDataLeft = CurrFileGetRecordSize(recWithDefLens);
            ++recWithDefLens;
            idxInRec = 0;
        }
        defDataSize = (UInt32) defLensData[idxInRec++];
        --defLensDataLeft;
        if (255 == defDataSize)
        {
            defDataSize = (UInt32) ((UInt32) defLensData[idxInRec] * 256 +  defLensData[idxInRec + 1]);
            idxInRec += 2;
            defLensDataLeft -= 2;
        }
        defOffset += defDataSize;
    }
    if (0 == defLensDataLeft)
    {
        if (defLensData)
        {
            CurrFileUnlockRecord(recWithDefLens - 1);
        }
        defLensData = (unsigned char *) CurrFileLockRecord(recWithDefLens);
        if (!defLensData)
            return NULL;
        defLensDataLeft = CurrFileGetRecordSize(recWithDefLens);
        ++recWithDefLens;
        idxInRec = 0;
    }
    defDataSize = (UInt32) defLensData[idxInRec++];
    --defLensDataLeft;
    if (255 == defDataSize)
    {
        defDataSize = (UInt32) ((UInt32) defLensData[idxInRec] * 256 +  defLensData[idxInRec + 1]);
    }
    CurrFileUnlockRecord(recWithDefLens - 1);

    while (defOffset >= CurrFileGetRecordSize(defRecord))
    {
        defOffset -= CurrFileGetRecordSize(defRecord);
        ++defRecord;
    }

    ebufAddChar(&g_buf, 149);
    ebufAddChar(&g_buf, ' ');

    Assert(si->posRecsCount <= 1);
    posData = (unsigned char *) CurrFileLockRecord(posRecord);
    if (NULL == posData)
        return NULL;
    posData += wordNo / 4;

    partOfSpeech = (posData[0] >> (2 * (wordNo % 4))) & 3;
    ebufAddStr(&g_buf, GetWnPosTxt(partOfSpeech));
    ebufAddChar(&g_buf, ' ');
    word = simple_get_word((void *) si, wordNo);
    ebufAddStr(&g_buf, word);
    ebufAddChar(&g_buf, '\n');

    CurrFileUnlockRecord(posRecord);
    Assert((defOffset >= 0) && (defOffset < 66000));
    Assert((defDataSize >= 0));

    defData = (unsigned char *) CurrFileLockRecord(defRecord);
    if (NULL == defData)
        return NULL;
    defData += defOffset;

    pcReset(&si->defPackContext, defData, 0);
    pcUnpack(&si->defPackContext, defDataSize, si->curDefData, &unpackedLen);

    Assert((unpackedLen > 0) && (unpackedLen <= si->maxDefLen));
    si->curDefLen = unpackedLen;
    unpackedDef = si->curDefData;
    CurrFileUnlockRecord(defRecord);

    ebufAddStrN(&g_buf, (char *) unpackedDef, unpackedLen);
    ebufAddChar(&g_buf, '\0');
    ebufWrapBigLines(&g_buf);

    rawTxt = ebufGetDataPointer(&g_buf);

    diSetRawTxt(di, rawTxt);
    return 0;
}
#endif

#ifdef DEMO
Err simple_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di)
{
    SimpleInfo *            si = NULL;
    char *                  word;
    char *                  rawTxt;
    static ExtensibleBuffer g_buf = { 0 };

    si = (SimpleInfo *) data;

    Assert(data);
    Assert((wordNo >= 0) && (wordNo < si->wordsCount));
    Assert(dx > 0);
    Assert(di);

    ebufReset(&g_buf);

    ebufAddChar(&g_buf, 149);
    ebufAddChar(&g_buf, ' ');
    word = simple_get_word((void *) si, wordNo);
    ebufAddStr(&g_buf, word);
    ebufAddChar(&g_buf, '\n');

    ebufAddStr(&g_buf, "\n    This is only a DEMO version of");
    ebufAddStr(&g_buf, "\n    Noah Pro without definitions. To");
    ebufAddStr(&g_buf, "\n    find out how to get full version");
    ebufAddStr(&g_buf, "\n    go to:");
    ebufAddStr(&g_buf, "\n                WWW.ARSLEXIS.COM\n");

    ebufAddChar(&g_buf, '\0');
    ebufWrapBigLines(&g_buf);

    rawTxt = ebufGetTxtCopy(&g_buf);
    diSetRawTxt(di, rawTxt);
    return 0;
}
#endif /* DEMO */

