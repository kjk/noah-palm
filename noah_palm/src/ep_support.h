/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _EP_SUPPORT_H_
#define _EP_SUPPORT_H_

#define  ENGPOL_TYPE       'enpl'

#include "display_info.h"
#include "extensible_buffer.h"
#include "word_compress.h"

typedef struct
{
    char    *data;
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

typedef struct
{
    int     recordsCount;
    long    wordsCount;
    int     defLenRecordsCount;
    int     wordRecordsCount;
    int     defRecordsCount;
    int     maxWordLen;
    int     maxDefLen;
    int     maxComprDefLen;
    int     posNamesRecord;
    WcInfo  *wci;
    int     curDefLen;
    PackContext     packContext;
    unsigned char   *curDefData;
} EngPolInfo;

/* PROTOTYPES for private interface */
enum EP_RENDER_TYPE
{
    eprt_compact = 1, 
    eprt_multiline 
};

void *epNew(void);
void epDelete(void *data);
long epGetWordsCount(void *data);
long epGetFirstMatching(void *data, char *word);
char *epGetWord(void *data, long wordNo);
Err ep_get_display_info(void *data, long wordNo, Int16 dx, DisplayInfo * di);

#endif
