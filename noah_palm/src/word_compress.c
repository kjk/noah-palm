/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include "common.h"
#include "word_compress.h"
#include "fs.h"

typedef struct
{
    UInt32  wordNo;
    UInt16  record;
    UInt16  offset;
} WordCacheEntry;

static void word_cache_init(WordCache * wc)
{
    int i;
    for (i = 0; i < WORDS_CACHE_SIZE; i++)
    {
        wc->words[i] = NULL;
        wc->wordsNo[i] = -1;
        wc->slotLen[i] = 0;
    }
    wc->nextFreeSlot = 0;
}

static void word_cache_free(WordCache * wc)
{
    int i;
    for (i = 0; i < WORDS_CACHE_SIZE; i++)
    {
        if (wc->words[i])
        {
            new_free(wc->words[i]);
            wc->words[i] = NULL;
        }
    }
    wc->nextFreeSlot = 0;
}

/*
Add a word to a cache. If no free slot present, the oldest
word in a cache get evicted.
*/
static void word_cache_add_word(WordCache * wc, UInt32 wordNo, char *word)
{
    int index;
    int wordLen;

    Assert(word);

    wordLen = StrLen(word) + 1;
    index = wc->nextFreeSlot;

    if (wc->slotLen[index] < wordLen)
    {
        if (wc->words[index])
        {
            new_free(wc->words[index]);
        }
        wc->words[index] = (char *) new_malloc(wordLen);
        wc->slotLen[index] = wordLen;
    }

    if (NULL == wc->words[index])
    {
        wc->wordsNo[index] = -1;
        wc->slotLen[index] = 0;
        return;
    }

    MemMove(wc->words[index], word, wordLen);
    wc->wordsNo[index] = wordNo;
    wc->nextFreeSlot = (wc->nextFreeSlot + 1) % WORDS_CACHE_SIZE;
}

/*
Search the cache for the word number wordNo.

Return word if in cache, NULL if not.
*/
static char *word_cache_get_word(WordCache * wc, UInt32 wordNo)
{
    int i;
    for (i = 0; i < WORDS_CACHE_SIZE; i++)
    {
        if (wc->wordsNo[i] == (long)wordNo)
        {
            return wc->words[i];
        }
    }
    return NULL;
}

/*
Initialize the PackContext structure.

Return true if success, false on failure.
*/
Boolean pcInit(PackContext * pc, int recWithComprData)
{
    unsigned char   *data;
    int             *dataInt;
    int             maxComprStrLen;
    int             i, j, k;
    long            dataLen;

    Assert(recWithComprData > 0);

    data = (unsigned char *) CurrFileLockRecord(recWithComprData);
    if (NULL == data)
        return false;

    dataLen = CurrFileGetRecordSize(recWithComprData);

    dataInt = (int *) data;
    maxComprStrLen = dataInt[0];
    data += 2 + maxComprStrLen * 2;
    dataLen -= 2 + maxComprStrLen * 2;

    pc->packData = (char *) new_malloc(dataLen);
    if (NULL == pc->packData)
        goto Error;

    MemMove(pc->packData, data, dataLen);
    data = (unsigned char *) pc->packData;

    j = 0;
    for (i = 1; i <= maxComprStrLen; i++)
    {
        for (k = 0; k < dataInt[i]; k++)
        {
            pc->packStringLen[j] = i;
            pc->packString[j] = (char *) data;
            data += i;
            ++j;
        }
    }
    Assert(j == 256);
    CurrFileUnlockRecord(recWithComprData);
    return true;
  Error:
    pcFree(pc);
    CurrFileUnlockRecord(recWithComprData);
    return false;
}


void pcFree(PackContext * pc)
{
    if (NULL != pc->packData)
    {
        new_free(pc->packData);
        pc->packData = NULL;
    }
}

void pcReset(PackContext * pc, unsigned char *data, long offset)
{
    Assert(pc);
    Assert(offset >= 0);
    pc->currData = data;
    pc->currOffset = offset;
    pc->currPackStr = -1;
    pc->currPackStrLen = 0;
    pc->currPackStrUsed = 0;
}

unsigned char pcGetChar(PackContext * pc)
{
    unsigned char code;

    Assert(pc->currPackStrUsed <= pc->currPackStrLen);
    if (pc->currPackStrUsed == pc->currPackStrLen)
    {
        /* FIXME: here we can run into a problem: in the compressor
           (word-packer-build-word-pdb-data) I mark the end of the db
           with 0, which should be a code that corresponds to 0. bummer
         */
        code = pc->currData[pc->currOffset++];
        // TODO: this is a total hack, it fixes the problem with word "gro"
        // (word no 51256 in wn_ex_full but is this really correct?
// line below was the old code and line after that is a fixed (?) thing
//        if (0 == (int) code && (pc->currOffset == pc->lastCharInRecordOffset + 1))
        if (pc->currOffset >= pc->lastCharInRecordOffset + 1)
        {
            pc->currPackStrLen = 0;
            pc->currPackStrUsed = 0;
            return 0;
        }
        pc->currPackStrLen = pc->packStringLen[code];
        pc->currPackStr = (int) code;
        Assert((pc->currPackStr >= 0) && (pc->currPackStr < 256));
        pc->currPackStrUsed = 0;
    }
    code = (pc->packString[pc->currPackStr])[pc->currPackStrUsed];
    ++pc->currPackStrUsed;
    return code;
}

void pcUngetChar(PackContext * pc)
{
    unsigned char code;

    if (pc->currPackStrUsed > 0)
    {
        --pc->currPackStrUsed;
    }
    else
    {
        Assert(pc->currOffset > 0);
        --pc->currOffset;
        code = pc->currData[pc->currOffset];
        pc->currPackStrLen = pc->packStringLen[code];
        pc->currPackStr = (int) code;
        pc->currPackStrUsed = pc->currPackStrLen - 1;
    }
}

void pcUnpack(PackContext * pc, int packedLen, unsigned char *buf, int *unpackedLen)
{
    unsigned char code;

    Assert(pc);
    Assert(buf);
    Assert(unpackedLen);
    Assert(packedLen > 0);

    *unpackedLen = 0;
    while (packedLen > 0)
    {
        code = pc->currData[pc->currOffset++];
        pc->currPackStr = (int) code;
        pc->currPackStrLen = pc->packStringLen[code];
        pc->currPackStrUsed = pc->currPackStrLen;
        *unpackedLen += pc->currPackStrLen;
        MemMove(buf, pc->packString[code], pc->currPackStrLen);
        buf += pc->currPackStrLen;
        --packedLen;
    }
    return;
}

/*
Init word decompressor.

Input:
   wordsCount             number of words
   recWithWordCache    record that keeps info with pointers to
                          uncompressed words
   firstRecWithWords   first record that contains compressed words
   recordsWithwords_count  number of records with compressed words
   maxWordLen             maximum length of uncompressed word
 */
WcInfo *wcInit(UInt32 wordsCount, int recWithComprData,
        int recWithWordCache, int firstRecWithWords,
        int recsWithWordsCount, int maxWordLen)
{
    WcInfo *wci;
    UInt16 recordSize;

    wci = (WcInfo *) new_malloc_zero(sizeof(WcInfo));
    if (NULL == wci)
        goto Error;

    if (!pcInit(&wci->packContext, recWithComprData))
    {
        goto Error;
    }

    wci->buf = (char *) new_malloc(maxWordLen + 2);
    if (NULL == wci->buf)
        goto Error;

    wci->prevWord = (char *) new_malloc(maxWordLen + 2);
    if (NULL == wci->prevWord)
        goto Error;

    word_cache_init(&wci->wordCache);

    wci->lastWord = 0xffffffff;
    wci->maxWordLen = maxWordLen;
    wci->wordsCount = wordsCount;
    wci->recWithWordCache = recWithWordCache;
    wci->firstRecWithWords = firstRecWithWords;
    wci->recsWithWordsCount = recsWithWordsCount;
    recordSize = CurrFileGetRecordSize(recWithWordCache);
    wci->cacheEntriesCount = recordSize / sizeof(WordCacheEntry);
    Assert(recordSize == wci->cacheEntriesCount * sizeof(WordCacheEntry));

    return wci;
  Error:
    if (wci)
    {
        wcFree(wci);
        new_free(wci);
    }
    LogG( "wcInit() failed" );
    return NULL;
}

Boolean wcFree(WcInfo * wci)
{
    if (NULL == wci)
        return true;

    word_cache_free(&wci->wordCache);

    pcFree(&wci->packContext);

    if (wci->prevWord)
        new_free((void *) wci->prevWord);

    if (wci->buf)
        new_free((void *) wci->buf);

    new_free((void *) wci);
    return true;
}

UInt32 last_word_in_cache(WcInfo * wci, WordCacheEntry * cache, int cacheNo)
{
    if (cacheNo < (wci->cacheEntriesCount - 1))
    {
        return cache[cacheNo + 1].wordNo;
    }
    else
    {
        return wci->wordsCount;
    }
}
/*
  wordNo   - 0...wordsCount-1
*/
char *wcGetWord(WcInfo * wci, UInt32 wordNo)
{
    WordCacheEntry  *cache;
    int             cacheNo = 0;
    UInt16          record;
    UInt16          offset;
    char            *data;
    UInt32          currWord;
    UInt32          firstWord;
    char            *word;
    long            recordSize;

    Assert(wordNo < wci->wordsCount);
    if (wordNo >= wci->wordsCount)
        return NULL;

    word = word_cache_get_word(&wci->wordCache, wordNo);
    if (word)
        return word;

    cache = (WordCacheEntry *) CurrFileLockRecord(wci->recWithWordCache);
    if (NULL == cache)
        return NULL;

    /* find the cache that contains this word */
    while (last_word_in_cache(wci, cache, cacheNo) <= wordNo)
    {
        ++cacheNo;
    }
    record = cache[cacheNo].record + wci->firstRecWithWords;
    offset = cache[cacheNo].offset;
    firstWord = cache[cacheNo].wordNo;
    CurrFileUnlockRecord(wci->recWithWordCache);

    data = (char *) CurrFileLockRecord(record);
    recordSize = CurrFileGetRecordSize(record);

    /* special case - this is the first word in this cache entry */
    if (firstWord == wordNo)
    {
        wci->prevWord[0] = '\0';
        pcReset(&wci->packContext, (unsigned char *) data, offset);
        wci->packContext.lastCharInRecordOffset = recordSize;
        wcUnpackWord(wci, wci->prevWord, wci->prevWord);
        goto Exit;
    }

    /* special case - this is the next word after last
       unpacked word */
    if (wordNo == wci->lastWord + 1)
    {
        wcUnpackWord(wci, wci->prevWord, wci->prevWord);
        goto Exit;
    }

    /* TODO: special case: if last unpacked word is in the same
       cache entry and is < wordNo - start unpacking from there
       instead of the beginning */
    currWord = firstWord;
    wci->prevWord[0] = '\0';
    pcReset(&wci->packContext, (unsigned char *) (data + offset), 0);
    wci->packContext.lastCharInRecordOffset = recordSize - offset;
    wcUnpackWord(wci, wci->prevWord, wci->prevWord);
    do
    {
        wcUnpackWord(wci, wci->prevWord, wci->prevWord);
        ++currWord;
    }
    while (currWord != wordNo);
  Exit:
    wci->lastWord = wordNo;
    CurrFileUnlockRecord(record);
    word_cache_add_word(&wci->wordCache, wordNo, wci->prevWord);
    return wci->prevWord;
}
void wc_get_firstWord_from_cache(WcInfo * wci, WordCacheEntry * cache, int cacheNo, char *buf)
{
    char    *data;
    UInt16  record;
    UInt16  offset;
    long    recordSize;

    Assert(wci);
    Assert(cache);
    Assert((cacheNo >= 0) && (cacheNo < wci->cacheEntriesCount));
    Assert(buf);

    record = cache[cacheNo].record + wci->firstRecWithWords;
    offset = cache[cacheNo].offset;

    data = (char *) CurrFileLockRecord(record);
    if (NULL == data)
        return;
    recordSize = CurrFileGetRecordSize(record);
    buf[0] = '\0';
    pcReset(&wci->packContext, (unsigned char *) (data + offset), 0);
    wci->packContext.lastCharInRecordOffset = recordSize - offset;
    wcUnpackWord(wci, buf, buf);
    CurrFileUnlockRecord(record);
}

/*
  return the number of a first word that is matching
  a given word */
UInt32 wcGetFirstMatching(WcInfo * wci, char *word)
{
    WordCacheEntry  *cache;
    int             cacheNo = 0;
    UInt32          currWord;
    int             compRes;
    int             record;
    long            offset;
    char            *data;
    UInt32          lastWordNo;
    long            recordSize;

    Assert(wci);
    Assert(word);

    cache = (WordCacheEntry *) CurrFileLockRecord(wci->recWithWordCache);
    if (NULL == cache)
    {
        return 0;
    }
    cacheNo = -1;
    do
    {
        ++cacheNo;
        wc_get_firstWord_from_cache(wci, cache, cacheNo + 1, wci->buf);
        compRes = p_istrcmp(word, wci->buf);
/*        compRes = StrCaselessCompare(word, wci->buf); */
    } while ((cacheNo < (wci->cacheEntriesCount - 2)) && (compRes >= 0));

    /* if we finished with compRes>=0 means that word is greater or equal
       the first word in cacheNo+1, so it must be there, since it's the
       last cache entry */
    if (compRes >= 0)
    {
        ++cacheNo;
        /* does it really point at the last cache? */
        Assert(cacheNo == wci->cacheEntriesCount - 1);
    }

    /* we know that word must be in cacheNo, just find it */
    record = cache[cacheNo].record + wci->firstRecWithWords;
    offset = cache[cacheNo].offset;

    data = (char *) CurrFileLockRecord(record);
    if (NULL == data)
    {
        return 0;
    }
    recordSize = CurrFileGetRecordSize(record);
    pcReset(&wci->packContext, (unsigned char *) data, offset);
    wci->packContext.lastCharInRecordOffset = recordSize;

    currWord = cache[cacheNo].wordNo - 1;
    lastWordNo = last_word_in_cache(wci, cache, cacheNo);
    wci->buf[0] = '\0';
    do
    {
        ++currWord;
        wcUnpackWord(wci, wci->buf, wci->buf);
        compRes = p_istrcmp(word, wci->buf);
/*        compRes = StrCaselessCompare(word, wci->buf); */
    }
    while ((currWord < (lastWordNo - 1)) && (compRes > 0));

    if (compRes <= 0)
    {
        if (currWord > cache[cacheNo].wordNo)
        {
            if (!p_isubstr(word, wci->buf))
            {
                --currWord;
            }
            else
            {
                wci->lastWord = currWord;
                MemMove(wci->prevWord, wci->buf, StrLen(wci->buf) + 1);
            }
        }
    }
    else
    {
        currWord = lastWordNo - 1;
    }
    CurrFileUnlockRecord(record);
    CurrFileUnlockRecord(wci->recWithWordCache);
    /* make sure we won't try to be too smart in wcGetWord */
    wci->lastWord = 0xffffff;
    return currWord;
}

/*
Unpack a word.

Input:
  packed - points at packed word
  prevWord - previously unpacked word
  unpacked - where to put unpacked word, may be
    the same as prevWord for efficient in-place
    unpacking

Return:
  pointer to next packed word
 */
void wcUnpackWord(WcInfo * wci, char *prevWord, char *unpacked)
{
    unsigned char c;

    c = pcGetChar(&wci->packContext);
    if (c < 32)
    {
        if (prevWord != unpacked)
            MemMove(unpacked, prevWord, c);
        prevWord += c;
        unpacked += c;
        c = pcGetChar(&wci->packContext);
    }

    while (c > 31)
    {
        *unpacked++ = c;
        c = pcGetChar(&wci->packContext);
    }
    pcUngetChar(&wci->packContext);
    *unpacked = 0;
}

char char_to_lower(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return c - 'A' + 'a';
    return c;
}

/* return true if s1 is a substring of s2 */
Boolean p_isubstr(char *si, char *s2)
{
    while (*si)
    {
        if (char_to_lower(*si++) != char_to_lower(*s2++))
            return false;
    }
    return true;
}

/* return true if s1 is a substring of s2 */
Boolean p_substr(char *si, char *s2)
{
    while (*si)
    {
        if (*si++ != *s2++)
            return false;
    }
    return true;
}

int p_istrcmp(char *s1, char *s2)
{
    char c1;
    char c2;

    while (1)
    {
        c1 = char_to_lower(*s1++);
        c2 = char_to_lower(*s2++);
        if (c1 == c2)
        {
            if (c1 == 0)
            {
                return 0;
            }
        }
        else
        {
            if (c1 == 0)
                return -1;
            if (c2 == 0)
                return 1;
            if (c1 > c2)
                return 1;
            else
                return -1;
        }
    }

}

int p_instrcmp(char *s1, char *s2, int maxLen )
{
    char c1;
    char c2;

    while (maxLen  > 0)
    {
        c1 = char_to_lower(*s1++);
        c2 = char_to_lower(*s2++);
        if (c1 == c2)
        {
            if (c1 == 0)
            {
                return 0;
            }
        }
        else
        {
            if (c1 == 0)
                return -1;
            if (c2 == 0)
                return 1;
            if (c1 > c2)
                return 1;
            else
                return -1;
        }
        --maxLen ;
    }
    return 0;
}

int p_strcmp(char *s1, char *s2)
{
    char c1;
    char c2;

    while (1)
    {
        c1 = *s1++;
        c2 = *s2++;
        if (c1 == c2)
        {
            if (c1 == 0)
            {
                return 0;
            }
        }
        else
        {
            if (c1 == 0)
                return -1;
            if (c2 == 0)
                return 1;
            if (c1 > c2)
                return 1;
            else
                return -1;
        }
    }
}
