#ifndef __INET_DEFINITION_FORMAT_H__
#define __INET_DEFINITION_FORMAT_H__

#include "common.h"

typedef enum ResponseParsingResult_
{
    responseError,
    responseMalformed,
    responseUnauthorised,
    responseWordNotFound,
    responseOneWord,
    responseWordsList,
    responseMessage,
} ResponseParsingResult;    

extern ResponseParsingResult ProcessResponse(AppContext* appContext, const Char* word, const Char* responseBegin, const Char* responseEnd, Boolean usedAuthentication);
extern Err ProcessOneWordResponse(AppContext* appContext, const Char* word, const Char* responseBegin, const Char* responseEnd);

#endif