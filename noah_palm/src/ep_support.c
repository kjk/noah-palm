
/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "ep_support.h"

#ifndef EP_DICT
#error "EP_DICT not defined"
#endif

#define POLISH_CHARS "±êæ³ñó¶¼¿"
#define LATIN_CHARS "aeclnószz"

/* change all polish characters into latin ones */
static void unpolishString(char *str, int strLen)
{
    char c;
    int i, j;
    const char* polishChars=POLISH_CHARS;
    const char* latinChars=LATIN_CHARS;

    for (i = 0; i < strLen; i++)
    {
        c = str[i];
        for (j = 0; (j < 9) && (polishChars[j] != c); j++)
        {};
        if (j < 9)
        {
            str[i] = latinChars[j];
        }
    }
}

/*
type:
    0 - long polish
    1 - short english
*/
static char *getCatName(AbstractFile* file, int cat, int type)
{
    unsigned char *data = NULL;
    int i = 0;
    int names_count;
    int names_types;

    /* have to keep this record locked from outside,
       since this returns pointer inside record */
    data = (unsigned char *) fsLockRecord(file, 1);
    if (NULL == data)
        return NULL;
    names_count = ((int *) data)[0];
    names_types = ((int *) data)[1];
    data += 2 * 2;

    Assert(type < names_types);
    Assert(cat < names_count);

    while (i < cat + type * names_count)
    {
        ++i;
        while (*data)
            ++data;
        ++data;
    }

    fsUnlockRecord(file, 1);
    return (char *) data;
}

void *epNew(AbstractFile* file)
{
    EngPolInfo *epi = NULL;
    FirstRecord *first_rec;

/*       StrCopy(polishChars, "±êæ³ñó¶¼¿" ); */
/*       StrCopy(latinChars, "aeclnószz" ); */

    epi = (EngPolInfo *) new_malloc_zero(sizeof(EngPolInfo));
    if (NULL == epi)
        return NULL;
        
    epi->file=file;
    ebufInit(&epi->buffer, 0);      

    if (!pcInit(file, &epi->packContext, 4))
        goto Error;

    epi->recordsCount = fsGetRecordsCount(file);
    first_rec = (FirstRecord *) fsLockRecord(file, 0);
    if (NULL == first_rec)
        goto Error;

    epi->wordsCount = first_rec->wordsCount;
    epi->defLenRecordsCount = first_rec->defLenRecordsCount;
    epi->wordRecordsCount = first_rec->wordRecordsCount;
    epi->defRecordsCount = first_rec->defRecordsCount;
    epi->maxWordLen = first_rec->maxWordLen;
    epi->maxDefLen = first_rec->maxDefLen;
    epi->maxComprDefLen = first_rec->maxComprDefLen;

    epi->curDefData = (unsigned char *) new_malloc(epi->maxDefLen + 2);
    if (NULL == epi->curDefData)
        goto Error;

    epi->wci = wcInit(file, epi->wordsCount, 3, 2, 6,  epi->wordRecordsCount, epi->maxWordLen);
    if (NULL == epi->wci)
        goto Error;

  Exit:
    fsUnlockRecord(file, 0);
    return epi;
  Error:
    epDelete(epi);
    epi = NULL;
    goto Exit;
}

void epDelete(void *data)
{
    EngPolInfo *epi=(EngPolInfo *) data;
    if (!epi)
        return;

    ebufFreeData(&epi->buffer);

    pcFree(&epi->packContext);

    if (epi->wci)
        wcFree(epi->wci);

    if (epi->curDefData)
        new_free(epi->curDefData);

    new_free(data);
}

long epGetWordsCount(void *data)
{
    return ((EngPolInfo *) data)->wordsCount;
}

long epGetFirstMatching(void *data, char *word)
{
    return wcGetFirstMatching(((EngPolInfo *) data)->file, ((EngPolInfo *) data)->wci, word);
}

char *epGetWord(void *data, long wordNo)
{
    EngPolInfo *epi;
    epi = (EngPolInfo *) data;
    Assert(wordNo < epi->wordsCount);

    if (wordNo >= epi->wordsCount)
        return NULL;
    return wcGetWord(epi->file, epi->wci, wordNo);
}

static void epGetDef(void *data, long wordNo)
{
    EngPolInfo *epi;
    int record;
    long offset = 0;
    long defLen = 0;
    void *defData;

    epi = (EngPolInfo *) data;

    Assert(epi);
    Assert(wordNo < epi->wordsCount);

    if ( !get_defs_record(epi->file, wordNo, 5, 1, 5 + epi->defLenRecordsCount + epi->wordRecordsCount, &record, &offset, &defLen) )
    {
        return;
    }

    defData = fsLockRecord(epi->file, record);
    if (NULL == defData)
        return;

    MemSet(epi->curDefData, epi->maxDefLen + 2, 0xff);

    pcReset(&epi->packContext, (unsigned char *) defData, offset);
    pcUnpack(&epi->packContext, defLen, epi->curDefData, &epi->curDefLen);
    Assert(epi->curDefLen <= epi->maxDefLen);

    fsUnlockRecord(epi->file, record);
}

static int epd_get_homonyms_count(EngPolDef * epd)
{
    char *data = epd->data;
    int len_so_far = 0;
    int homonymsCount = 0;

    while (len_so_far < epd->defLen)
    {
        if (data[len_so_far] == (char) 1)
        {
            ++homonymsCount;
            len_so_far += 2;
        }
        else
        {
            ++len_so_far;
        }
    }
    return homonymsCount;
}

/*  homNo = 0..homonymsCount-1 */
static char *epd_get_homonym(EngPolDef * epd, int homNo)
{
    int len_so_far = 0;
    char *data = epd->data;

    while (len_so_far < epd->defLen)
    {
        if (data[len_so_far] == 1)
        {
            if (homNo == 0)
            {
                return &(data[len_so_far]);
            }
            else
            {
                --homNo;
                len_so_far += 2;
            }
        }
        else
        {
            ++len_so_far;
        }
    }
    /* invalid homNo */
    return NULL;
}

/* homNo = 0..homonymsCount-1 */
static int epd_get_homonym_gram(EngPolDef * epd, int homNo)
{
    char *data;

    data = epd_get_homonym(epd, homNo);
    if (data)
        return data[1];
    else
        return -1;
}

/* meaningNo = 0...meaningsCount */
static char *epd_get_meaning(EngPolDef * epd, int homNo, int meaningNo)
{
    char *data;
    int len_so_far;

    data = epd_get_homonym(epd, homNo);
    if (!data)
        return NULL;
    /* skip 1 (homonym) and homonyms grammatical category */
    len_so_far = 2;
    /* scan until next homonym (1) or the end of dsc looking
       for all meanings (2) */
    while ((len_so_far < epd->defLen) && (data[len_so_far] != 1))
    {
        if (data[len_so_far] == 2)
        {
            if (meaningNo == 0)
                return &(data[len_so_far]);
            else
                --meaningNo;
        }
        ++len_so_far;
    }
    /* invalid meaningNo */
    return NULL;
}

/* homNo = 0..homonymsCount-1 */
static int epd_get_meanings_count(EngPolDef * epd, int homNo)
{
    char *data;
    int meaningsCount = 0;
    int len_so_far;
    data = epd_get_homonym(epd, homNo);
    if (!data)
        return -1;
    /* skip 1 (homonym) and homonyms grammatical category */
    len_so_far = (data - epd->data) + 2;
    data = epd->data;
    /* scan until next homonym (1) or the end of dsc looking
       for all meanings (2) */
    while ((len_so_far < epd->defLen) && (data[len_so_far] != 1))
    {
        if (data[len_so_far] == 2)
            ++meaningsCount;
        ++len_so_far;
    }
    return meaningsCount;
}

static char *epd_get_meaning_dsc(EngPolDef * epd, int homNo, int meaningNo)
{
    char *data;

    data = epd_get_meaning(epd, homNo, meaningNo);
    if (!data)
        return NULL;

    return data + 1;
}

static int epd_get_meaning_dsc_len(EngPolDef * epd, int homNo, int meaningNo)
{
    char *data;
    int len = 0;
    int lenLeft;

    data = epd_get_meaning_dsc(epd, homNo, meaningNo);
    if (!data)
        return 0;
    lenLeft = epd->defLen - (data - epd->data);
    while ((len < lenLeft) && (data[len] != 1) && (data[len] != 2)  && (data[len] != 3))
    {
        ++len;
    }
    return len;
}

static int epd_get_examples_count(EngPolDef * epd, int homNo, int meaningNo)
{
    char *data;
    int len_so_far;
    int examplesCount = 0;

    data = epd_get_meaning(epd, homNo, meaningNo);
    if (!data)
        return -1;

    /* skip 2 (meaning id) */
    len_so_far = (data - epd->data) + 1;
    data = epd->data;

    while ((len_so_far < epd->defLen) && (data[len_so_far] != 1)
           && (data[len_so_far] != 2))
    {
        if (data[len_so_far] == 3)
            ++examplesCount;
        ++len_so_far;
    }
    return examplesCount / 2;
}

/* exampleNo = 0..examplesCount*2-1 */
static char *epd_get_example(EngPolDef * epd, int homNo, int meaningNo, int exampleNo)
{
    char *data;
    int len_so_far;

    data = epd_get_meaning(epd, homNo, meaningNo);
    if (!data)
        return NULL;

    /* skip 2 (meaning id) */
    len_so_far = 1;
    /* look for examples (begin with 3) until end of buffer or another
       homonym/meaning */
    while ((len_so_far < epd->defLen) && (data[len_so_far] != 1)  && (data[len_so_far] != 2))
    {
        if (data[len_so_far] == 3)
        {
            if (exampleNo == 0)
                return &(data[len_so_far + 1]);
            else
                --exampleNo;
        }
        ++len_so_far;
    }
    return NULL;
}

static int epd_get_example_len(EngPolDef * epd, int homNo, int meaningNo, int exampleNo)
{
    char *data;
    int lenLeft;
    int len = 0;

    data = epd_get_example(epd, homNo, meaningNo, exampleNo);
    if (!data)
        return 0;
    lenLeft = epd->defLen - (data - epd->data);
    /* go at the beginning and go until end of dsc or another 
       homonym/meaning/example found */
    while ((len < lenLeft) && (data[len] != 1) && (data[len] != 2) && (data[len] != 3))
    {
        ++len;
    }
    return len;
}

static char *epd_get_example_eng(EngPolDef * epd, int homNo, int meaningNo, int exampleNo)
{
    return epd_get_example(epd, homNo, meaningNo, exampleNo * 2);
}

static char *epd_get_example_pol(EngPolDef * epd, int homNo, int meaningNo, int exampleNo)
{
    return epd_get_example(epd, homNo, meaningNo, exampleNo * 2 + 1);
}


static int epd_get_example_eng_len(EngPolDef * epd, int homNo, int meaningNo,
                        int exampleNo)
{
    return epd_get_example_len(epd, homNo, meaningNo, exampleNo * 2);
}

static int epd_get_example_pol_len(EngPolDef * epd, int homNo, int meaningNo,
                        int exampleNo)
{
    return epd_get_example_len(epd, homNo, meaningNo, exampleNo * 2 + 1);
}

/* in a buffer add spaces after ';',')','(','<' etc. */
static void ep_fix_description_formatting(ExtensibleBuffer * buf)
{
    char *txt;
    int len;
    int pos;
    Boolean wasSpaceBefore;

    /* now add spaces where necessary */
    txt = ebufGetDataPointer(buf);
    len = ebufGetDataSize(buf);
    wasSpaceBefore = false;
    pos = 0;
    while (pos < len)
    {
        /* find places where we should put a space and insert it */
        if (!wasSpaceBefore)
        {
            switch (txt[pos])
            {
            case '(':
            case '<':
                /* insert before */
                if (pos > 0)
                {
                    ebufInsertChar(buf, ' ', pos);
                    len = ebufGetDataSize(buf);
                    txt = ebufGetDataPointer(buf);
                }
                break;
            case ')':
            case ';':
            case '>':
                if ((pos < len + 1) && (txt[pos + 1] != ';'))
                {
                    /* insert space after */
                    ++pos;
                    ebufInsertChar(buf, ' ', pos);
                    len = ebufGetDataSize(buf);
                    txt = ebufGetDataPointer(buf);
                }
                break;
            default:
                break;
            }
        }
        wasSpaceBefore = false;
        if (txt[pos] == ' ')
            wasSpaceBefore = true;
        ++pos;
    }
}


static void ep_dsc_to_raw_txt(EngPolInfo* epi, unsigned char *defData, int defData_len, char **rawTxt, enum EP_RENDER_TYPE renderType)
{
    int len = 0;
    int defLen = 0;
    int homonymsCount;
    int meaningsCount;
    int examplesCount;
    int i, j, k;
    int gramCat;
    char *txt = NULL;
    char *dsc = NULL;
    EngPolDef eng_pol_def;
    EngPolDef *epDsc;

    epDsc = &eng_pol_def;
    epDsc->data = (char *) defData;
    epDsc->defLen = defData_len;

    *rawTxt = NULL;

    ebufReset(&epi->buffer);

    /* need this for getCatName() to work correctly */
    if (NULL == fsLockRecord(epi->file, 1))
        return;

    homonymsCount = epd_get_homonyms_count(epDsc);
    for (i = 0; i < homonymsCount; i++)
    {
        ebufAddChar(&epi->buffer, '1' + i);
        ebufAddStr(&epi->buffer, ". ");
        gramCat = epd_get_homonym_gram(epDsc, i);

        ebufAddStr(&epi->buffer, getCatName(epi->file, gramCat, 0));
        if (renderType == eprt_multiline)
        {
            ebufAddChar(&epi->buffer, '\n');
        }

        meaningsCount = epd_get_meanings_count(epDsc, i);
        for (j = 0; j < meaningsCount; j++)
        {
            dsc = epd_get_meaning_dsc(epDsc, i, j);
            if (dsc)
            {
                ebufAddStr(&epi->buffer, "  ");
                ebufAddChar(&epi->buffer, 'a' + j);
                ebufAddStr(&epi->buffer, ". ");
                defLen = epd_get_meaning_dsc_len(epDsc, i, j);
                ebufAddStrN(&epi->buffer, dsc, defLen);
                if (renderType == eprt_multiline)
                {
                    ebufAddChar(&epi->buffer, '\n');
                }
                else
                {
                    ebufAddChar(&epi->buffer, ' ');
                }
            }
            examplesCount = epd_get_examples_count(epDsc, i, j);
            for (k = 0; k < examplesCount; k++)
            {
                if (renderType == eprt_multiline)
                {
                    ebufAddStr(&epi->buffer, "    ");
                }
                else
                {
                    ebufAddStr(&epi->buffer, "  ");
                }
                txt = epd_get_example_eng(epDsc, i, j, k);
                len = epd_get_example_eng_len(epDsc, i, j, k);
                ebufAddStrN(&epi->buffer, txt, len);
                ebufAddStr(&epi->buffer, " - ");
                txt = epd_get_example_pol(epDsc, i, j, k);
                len = epd_get_example_pol_len(epDsc, i, j, k);
                ebufAddStrN(&epi->buffer, txt, len);
                if (renderType == eprt_multiline)
                {
                    ebufAddChar(&epi->buffer, '\n');
                }
            }
        }
    }
    ebufAddChar(&epi->buffer, '\0');

    ep_fix_description_formatting(&epi->buffer);

    ebufWrapBigLines(&epi->buffer,true);
    txt = ebufGetTxtCopy(&epi->buffer);
    unpolishString(txt, StrLen(txt));
    *rawTxt = txt;
    fsUnlockRecord(epi->file, 1);
}

/*
  Get description for a given word formatted to be directly
  displayed on the screen.

  Params:
    data    - pointer to EngPolInfo struct
    wordNo    - 0..wordsCount-1
    dx        - width of the display we'll be displaying this description on
    di        - DisplayInfo struct to be filled in
 */
Err epGetDisplayInfo(void *data, long wordNo, Int16 dx, DisplayInfo * di)
{
    char *rawTxt;
    EngPolInfo *epi;

    epi = (EngPolInfo *) data;
    Assert(data);
    Assert(wordNo < epi->wordsCount);

    if (wordNo >= epi->wordsCount)
    {
        return NULL;
    }

    epGetDef(data, wordNo);

    ep_dsc_to_raw_txt(epi, epi->curDefData, epi->curDefLen, &rawTxt, eprt_multiline);
    if (!rawTxt)
    {
        return NULL;
    }

    diSetRawTxt(di, rawTxt);

    return 0;
}

