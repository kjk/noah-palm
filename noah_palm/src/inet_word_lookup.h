/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#ifndef __INET_WORD_LOOKUP_H__
#define __INET_WORD_LOOKUP_H__

#include "common.h"
#include <NetMgr.h>

#define serverAddress                "www.arslexis.com"
#define serverPort                      80
#define serverRelativeURL           "/dict-raw.php?word=^0&ver=^1"
#define maxResponseLength        8192               // reasonable limit so malicious response won't use all available memory
#define responseBufferSize          256                 // size of single chunk used to retrieve server response
#define addressResolveTimeout   20000              // timeouts in milliseconds
#define socketOpenTimeout         1000
#define socketConnectTimeout     10000
#define transmitTimeout              5000

extern void StartLookup(AppContext* appContext, const Char* word);
extern void PerformLookupTask(AppContext* appContext);
extern void AbortCurrentLookup(AppContext* appContext, Boolean sendNotifyEvent);
extern const Char* GetLookupStatusText(AppContext* appContext);

#define LookupInProgress(appContext) (NULL!=(appContext)->currentLookupData)

typedef enum LookupProgressEventFlag_ 
{
    lookupStarted,
    lookupProgress,
    lookupFinished,
} LookupProgressEventFlag;

typedef struct LookupProgressEventData_ 
{
    LookupProgressEventFlag flag;
} LookupProgressEventData;        

#ifdef DEBUG
void testParseResponse(char *txt);
#endif

#endif
