/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _EP_SUPPORT_H_
#define _EP_SUPPORT_H_

#include "common.h"

typedef struct
{
    char *  data;
    int     defLen;
} EngPolDef;

typedef struct
{
    long    wordsCount;
    int     defLenRecordsCount;
    int     wordRecordsCount;
    int     defRecordsCount;
    int     maxWordLen;
    int     maxDefLen;
    int     maxComprDefLen;
} FirstRecord;

typedef struct _EngPolInfo
{
    int         recordsCount;
    long        wordsCount;
    int         defLenRecordsCount;
    int         wordRecordsCount;
    int         defRecordsCount;
    int         maxWordLen;
    int         maxDefLen;
    int         maxComprDefLen;
    int         posNamesRecord;
    WcInfo *    wci;
    int         curDefLen;
    PackContext     packContext;
    unsigned char * curDefData;
    AbstractFile* file;
    ExtensibleBuffer buffer;
} EngPolInfo;

/* PROTOTYPES for private interface */
enum EP_RENDER_TYPE
{
    eprt_compact = 1, 
    eprt_multiline 
};

void *  epNew(AbstractFile* file);
void    epDelete(void *data);
long    epGetWordsCount(void *data);
long    epGetFirstMatching(void *data, char *word);
char *  epGetWord(void *data, long wordNo);
Err     epGetDisplayInfo(void *data, long wordNo, Int16 dx, DisplayInfo * di);

#endif
