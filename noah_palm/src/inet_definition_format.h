#ifndef __INET_DEFINITION_FORMAT_H__
#define __INET_DEFINITION_FORMAT_H__

#include "common.h"

typedef enum ResponseParsingResult_
{
    responseWordNotFound,
    responseDefinition,
    responseWordsList,
    responseMessage,
    responseErrorMessage,
    responseCookie,
} ResponseParsingResult;    

extern Err ProcessResponse(AppContext* appContext, const Char* word, const Char* responseBegin, const Char* responseEnd, ResponseParsingResult& result);
extern Err ProcessDefinitionResponse(AppContext* appContext, const Char* word, const Char* responseBegin, const Char* responseEnd);

#endif