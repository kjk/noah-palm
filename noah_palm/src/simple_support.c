/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "simple_support.h"

#ifndef NOAH_PRO
#error "only works in NOAH_PRO"
#endif

#ifndef SIMPLE_DICT
#error "SIMPLE_DICT not defined"
#endif

//static ExtensibleBuffer g_buf = { 0 };

void simple_delete(void *data)
{
    SimpleInfo *si=(SimpleInfo*)data;
    if (!si)
        return;

    ebufFreeData(&si->buffer);

    pcFree(&si->defPackContext);

    if (si->wci)
        wcFree(si->wci);

    if (si->curDefData)
        new_free(si->curDefData);

    new_free(data);
}

void *simple_new(AbstractFile* file)
{
    SimpleInfo *        si = NULL;
    SimpleFirstRecord * firstRecord;
    int                 recWithComprData;
    int                 recWithWordCache;
    int                 firstRecWithWords;

    si = (SimpleInfo *) new_malloc_zero(sizeof(SimpleInfo));
    if (NULL == si)
        goto Error;

    firstRecord = (SimpleFirstRecord *) fsLockRecord(file, 0);
    if (NULL == firstRecord)
        goto Error;

    Assert(sizeof(SimpleFirstRecord) == fsGetRecordSize(file, 0));
    si->file=file;
    ebufInit(&si->buffer, 0);
    si->recordsCount = fsGetRecordsCount(file);
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

    si->wci = wcInit(file, si->wordsCount, recWithComprData, recWithWordCache, 
                        firstRecWithWords, si->wordsRecsCount, si->maxWordLen);

    if (NULL == si->wci)
        goto Error;

    if (!pcInit(file, &si->defPackContext, 3))
        goto Error;

  Exit:
    fsUnlockRecord(file, 0);
    return si;
  Error:
    if (si)
        simple_delete(si);
    si = NULL;
    goto Exit;
}

long simple_get_words_count(void *data)
{
    return ((SimpleInfo *) data)->wordsCount;
}

long simple_get_first_matching(void *data, char *word)
{
    return wcGetFirstMatching(((SimpleInfo *) data)->file, ((SimpleInfo *) data)->wci, word);
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
    return wcGetWord(si->file, si->wci, wordNo);
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
    ebufReset(&si->buffer);

    for (i = 0; i < wordNo; i++)
    {
        if (0 == defLensDataLeft)
        {
            if (defLensData)
            {
                fsUnlockRecord(si->file, recWithDefLens - 1);
            }
            defLensData = (unsigned char *) fsLockRecord(si->file, recWithDefLens);
            if (NULL == defLensData)
                return NULL;
            defLensDataLeft = fsGetRecordSize(si->file, recWithDefLens);
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
            fsUnlockRecord(si->file, recWithDefLens - 1);
        }
        defLensData = (unsigned char *) fsLockRecord(si->file, recWithDefLens);
        if (!defLensData)
            return NULL;
        defLensDataLeft = fsGetRecordSize(si->file, recWithDefLens);
        ++recWithDefLens;
        idxInRec = 0;
    }
    defDataSize = (UInt32) defLensData[idxInRec++];
    --defLensDataLeft;
    if (255 == defDataSize)
    {
        defDataSize = (UInt32) ((UInt32) defLensData[idxInRec] * 256 +  defLensData[idxInRec + 1]);
    }
    fsUnlockRecord(si->file, recWithDefLens - 1);

    while (defOffset >= fsGetRecordSize(si->file, defRecord))
    {
        defOffset -= fsGetRecordSize(si->file, defRecord);
        ++defRecord;
    }

    ebufAddChar(&si->buffer, FORMAT_TAG);
    ebufAddChar(&si->buffer, FORMAT_POS);
    ebufAddChar(&si->buffer, 149);
    ebufAddChar(&si->buffer, ' ');

    Assert(si->posRecsCount <= 1);
    posData = (unsigned char *) fsLockRecord(si->file, posRecord);
    if (NULL == posData)
        return NULL;
    posData += wordNo / 4;

    partOfSpeech = (posData[0] >> (2 * (wordNo % 4))) & 3;
    ebufAddStr(&si->buffer, GetWnPosTxt(partOfSpeech));
    ebufAddChar(&si->buffer, ' ');
    
    ebufAddChar(&si->buffer, FORMAT_TAG);
    ebufAddChar(&si->buffer, FORMAT_WORD);
    
    word = simple_get_word((void *) si, wordNo);
    ebufAddStr(&si->buffer, word);
    ebufAddChar(&si->buffer, '\n');

    fsUnlockRecord(si->file, posRecord);
    Assert((defOffset >= 0) && (defOffset < 66000));
    Assert((defDataSize >= 0));

    defData = (unsigned char *) fsLockRecord(si->file, defRecord);
    if (NULL == defData)
        return NULL;
    defData += defOffset;

    pcReset(&si->defPackContext, defData, 0);
    pcUnpack(&si->defPackContext, defDataSize, si->curDefData, &unpackedLen);

    Assert((unpackedLen > 0) && (unpackedLen <= si->maxDefLen));
    si->curDefLen = unpackedLen;
    unpackedDef = si->curDefData;
    fsUnlockRecord(si->file, defRecord);

    ebufAddChar(&si->buffer, FORMAT_TAG);
    ebufAddChar(&si->buffer, FORMAT_DEFINITION);

    ebufAddStrN(&si->buffer, (char *) unpackedDef, unpackedLen);
    ebufAddChar(&si->buffer, '\0');
    ebufWrapBigLines(&si->buffer);

    rawTxt = ebufGetDataPointer(&si->buffer);

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

    ebufReset(&si->buffer);

    ebufAddChar(&si->buffer, 149);
    ebufAddChar(&si->buffer, ' ');
    word = simple_get_word(si, wordNo);
    ebufAddStr(&si->buffer, word);
    ebufAddChar(&si->buffer, '\n');

    ebufAddStr(&si->buffer, "\n    This is only a DEMO version of");
    ebufAddStr(&si->buffer, "\n    Noah Pro without definitions. To");
    ebufAddStr(&si->buffer, "\n    find out how to get full version");
    ebufAddStr(&si->buffer, "\n    go to:");
    ebufAddStr(&si->buffer, "\n                WWW.ARSLEXIS.COM\n");

    ebufAddChar(&si->buffer, '\0');
    ebufWrapBigLines(&si->buffer);

    rawTxt = ebufGetTxtCopy(&si->buffer);
    diSetRawTxt(di, rawTxt);
    return 0;
}
#endif /* DEMO */

