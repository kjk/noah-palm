#ifndef __INET_DEFINITION_FORMAT_H__
#define __INET_DEFINITION_FORMAT_H__

#include "common.h"

typedef enum ResponseParsingResult_
{
    responseError,
    responseMalformed=responseError,
    responseWordNotFound,
    responseOneWord,
    responseWordsList,
    responseMessage,
} ResponseParsingResult;    

extern ResponseParsingResult ProcessResponse(AppContext* appContext, const Char* word, const Char* responseBegin, const Char* responseEnd);
extern Err ProcessOneWordResponse(AppContext* appContext, const Char* word, const Char* responseBegin, const Char* responseEnd);

#endif