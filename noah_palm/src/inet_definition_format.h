#ifndef __INET_DEFINITION_FORMAT_H__
#define __INET_DEFINITION_FORMAT_H__

#include "common.h"

typedef enum ResponseParsingResult_
{
    responseDefinition=1,
    responseWordsList=2,
    responseMessage=4,
    responseErrorMessage=8,
    responseCookie=16,
    responseRegistrationFailed=32,
    responseRegistrationOk=64,
} ResponseParsingResult;    

extern Err ProcessResponse(AppContext* appContext, const Char* responseBegin, const Char* responseEnd, UInt16 allowedResponsesMask, ResponseParsingResult& result);
extern Err ProcessDefinitionResponse(AppContext* appContext, const Char* responseBegin, const Char* responseEnd);

#endif
