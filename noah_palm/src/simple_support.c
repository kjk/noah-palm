/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifdef NOAH_PRO
/* only supported for Noah Pro, should bomb if used with Noah Lite */
#include "noah_pro.h"
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
    SimpleInfo *si = NULL;
    SimpleFirstRecord *firstRecord;
    int recWithComprData;
    int recWithWordCache;
    int firstRecWithWords;

    si = (SimpleInfo *) new_malloc(sizeof(SimpleInfo));
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
    SimpleInfo *si = NULL;
    char *word;
    char *rawTxt;
    int unpackedLen;
    unsigned char *unpackedDef = NULL;
    long i;

    unsigned char *pos_data;
    int pos_record;
    int partOfSpeech;
    unsigned char *def_lens_data = NULL;
    long def_lens_data_left = 0;
    long defDataSize = 0;
    unsigned char *defData = NULL;
    int def_record = 0;
    long def_offset = 0;
    long idx_in_rec;
    int rec_with_def_lens = 0;

    si = (SimpleInfo *) data;

    Assert(data);
    Assert((wordNo >= 0) && (wordNo < si->wordsCount));
    Assert(dx > 0);
    Assert(di);

    rec_with_def_lens = 4 + si->wordsRecsCount;
    def_record = rec_with_def_lens + si->defsLenRecsCount;
    pos_record = def_record + si->defsRecsCount;

    def_lens_data_left = 0;
    def_lens_data = NULL;
    idx_in_rec = 0;
    ebufReset(&g_buf);

    for (i = 0; i < wordNo; i++)
    {
        if (0 == def_lens_data_left)
        {
            if (def_lens_data)
            {
                CurrFileUnlockRecord(rec_with_def_lens - 1);
            }
            def_lens_data = (unsigned char *) CurrFileLockRecord(rec_with_def_lens);
            if (NULL == def_lens_data)
                return NULL;
            def_lens_data_left = CurrFileGetRecordSize(rec_with_def_lens);
            ++rec_with_def_lens;
            idx_in_rec = 0;
        }
        defDataSize = (UInt32) def_lens_data[idx_in_rec++];
        --def_lens_data_left;
        if (255 == defDataSize)
        {
            defDataSize = (UInt32) ((UInt32) def_lens_data[idx_in_rec] * 256 +  def_lens_data[idx_in_rec + 1]);
            idx_in_rec += 2;
            def_lens_data_left -= 2;
        }
        def_offset += defDataSize;
    }
    if (0 == def_lens_data_left)
    {
        if (def_lens_data)
        {
            CurrFileUnlockRecord(rec_with_def_lens - 1);
        }
        def_lens_data = (unsigned char *) CurrFileLockRecord(rec_with_def_lens);
        if (!def_lens_data)
            return NULL;
        def_lens_data_left = CurrFileGetRecordSize(rec_with_def_lens);
        ++rec_with_def_lens;
        idx_in_rec = 0;
    }
    defDataSize = (UInt32) def_lens_data[idx_in_rec++];
    --def_lens_data_left;
    if (255 == defDataSize)
    {
        defDataSize = (UInt32) ((UInt32) def_lens_data[idx_in_rec] * 256 +  def_lens_data[idx_in_rec + 1]);
    }
    CurrFileUnlockRecord(rec_with_def_lens - 1);

    while (def_offset >= CurrFileGetRecordSize(def_record))
    {
        def_offset -= CurrFileGetRecordSize(def_record);
        ++def_record;
    }

    ebufAddChar(&g_buf, 149);
    ebufAddChar(&g_buf, ' ');

    Assert(si->posRecsCount <= 1);
    pos_data = (unsigned char *) CurrFileLockRecord(pos_record);
    if (NULL == pos_data)
        return NULL;
    pos_data += wordNo / 4;

    partOfSpeech = (pos_data[0] >> (2 * (wordNo % 4))) & 3;
    ebufAddStr(&g_buf, GetWnPosTxt(partOfSpeech));
    ebufAddChar(&g_buf, ' ');
    word = simple_get_word((void *) si, wordNo);
    ebufAddStr(&g_buf, word);
    ebufAddChar(&g_buf, '\n');

    CurrFileUnlockRecord(pos_record);
    Assert((def_offset >= 0) && (def_offset < 66000));
    Assert((defDataSize >= 0));

    defData = (unsigned char *) CurrFileLockRecord(def_record);
    if (NULL == defData)
        return NULL;
    defData += def_offset;

    pcReset(&si->defPackContext, defData, 0);
    pcUnpack(&si->defPackContext, defDataSize, si->curDefData, &unpackedLen);

    Assert((unpackedLen > 0) && (unpackedLen <= si->maxDefLen));
    si->curDefLen = unpackedLen;
    unpackedDef = si->curDefData;
    CurrFileUnlockRecord(def_record);

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
    SimpleInfo *si = NULL;
    char *word;
    char *rawTxt;
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

