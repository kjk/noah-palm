/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#ifndef __INET_WORD_LOOKUP_H__
#define __INET_WORD_LOOKUP_H__

#include "inet_definition_format.h"
#include <NetMgr.h>

typedef enum LookupProgressEventFlag_ 
{
    lookupStarted,
    lookupProgress,
    lookupFinished,
} LookupProgressEventFlag;

typedef struct LookupProgressEventData_ 
{
    LookupProgressEventFlag flag;
    ResponseParsingResult result;
} LookupProgressEventData;        

extern void StartLookup(AppContext* appContext, const Char* word);
extern void PerformLookupTask(AppContext* appContext);
extern void AbortCurrentLookup(AppContext* appContext, Boolean sendNotifyEvent, ResponseParsingResult result=responseError);
extern const Char* GetLookupStatusText(AppContext* appContext);

#define LookupInProgress(appContext) (NULL!=(appContext)->currentLookupData)


#ifdef DEBUG
void testParseResponse(char *txt);
#endif

#endif
