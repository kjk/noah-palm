/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/
#ifndef _WORD_COMPRESS_H_
#define _WORD_COMPRESS_H_

#include <PalmOS.h>

typedef struct
{
    char            *packData;
    int             packStringLen[256];
    char            *packString[256];

    unsigned char   *currData;
    long            currOffset;
    long            lastCharInRecordOffset;
    int             currPackStr;
    int             currPackStrLen;
    int             currPackStrUsed;
} PackContext;

#define WORDS_CACHE_SIZE 20

typedef struct
{
    char    *words[WORDS_CACHE_SIZE];
    long    wordsNo[WORDS_CACHE_SIZE];
    int     slotLen[WORDS_CACHE_SIZE];
    int     nextFreeSlot;
}
WordCache;

typedef struct
{
    UInt32      wordsCount;
    int         recWithWordCache;
    int         firstRecWithWords;
    int         recsWithWordsCount;
    int         maxWordLen;
    char        *prevWord;
    char        *buf;
    int         cacheEntriesCount;
    UInt32      lastWord;   /* most recently accessed word */
    PackContext packContext;
    WordCache   wordCache;
} WcInfo;

Boolean         pcInit(PackContext * pc, int recWithComprData);
void            pcFree(PackContext * pc);
void            pcReset(PackContext * pc, unsigned char *data, long offset);
unsigned char   pcGetChar(PackContext * pc);
void            pcUngetChar(PackContext * pc);
void            pcUnpack(PackContext * pc, int packedLen, unsigned char *buf, int *unpackedLen);

Boolean p_substr(char *si, char *s2);
Boolean p_isubstr(char *si, char *s2);
int     p_strcmp(char *s1, char *s2);
int     p_istrcmp(char *s1, char *s2);
int     p_instrcmp(char *s1, char *s2, int maxLen );

WcInfo    *wcInit(UInt32 wordsCount, int recWithComprData,
                int recWithWordCache, int firstRecWithWords,
                int recsWithWordsCount, int maxWordLen);
Boolean   wcFree(WcInfo * wci);
char      *wcGetWord(WcInfo * wci, UInt32 wordNo);
UInt32    wcGetFirstMatching(WcInfo * wci, char *word);
void      wcUnpackWord(WcInfo * wci, char *prevWord, char *unpacked);
#endif
