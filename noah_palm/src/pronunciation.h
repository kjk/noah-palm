/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Szymon Knitter
*/


/**
 * @file pronunciation.h
 * Interface of pronunciation.c
 * @author Szymon Knitter (szknitter@wp.pl) 
 */
#ifndef _PRONUNCIATION_H_
#define _PRONUNCIATION_H_

//WORD_MAX_LEN
#define PRON_COMPRESED_MAX_LEN      40
#define PRON_DECOMPRESED_MAX_LEN    42

typedef struct
{
    unsigned char idString[6];
    unsigned int  recordWithPronIndex;
    unsigned int  firstRecordWithPron;
    unsigned int  numberOfPronRecords;
}PronInFirstRecord;

typedef struct _PronunciationData
{
    UInt16  recordWithPronIndex;
    UInt16  firstRecordWithPron;
    UInt16  numberOfPronRecords;
    Boolean isPronInUsedDictionary;
}PronunciationData;

Boolean pronAddPronunciationToBuffer(struct _AppContext *appContext, ExtensibleBuffer *buf, AbstractFile *file, long wordNo, char *word);
char* pronTranslateDecompresed(struct _AppContext *appContext, unsigned char *decompresed);
void  pronDisplayHelp(struct _AppContext* appContext, char* pronHitted); //set pronHitted to NULL if you want std. info

#endif