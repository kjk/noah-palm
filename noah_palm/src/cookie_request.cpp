/**
 * @file cookie_request.cpp
 * Implementation of cookie request connection.
 *
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 *
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#include "cookie_request.h"
#include "device_info.h"
#include "inet_connection.h"
#include "inet_word_lookup.h"
#include "inet_definition_format.h"

/**
 * URL that forms cookie request.
 */
#define cookieRequestUrl "/palm.php?pv=^0&cv=^1&get_cookie&di=^2"

/**
 * Callback used by internet connection framework to process valid response.
 * 
 * @see ConnectionResponseProcessor
 */
static Err CookieRequestResponseProcessor(AppContext* appContext, void* context, const Char* responseBegin, const Char* responseEnd)
{
    ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
    Assert(wordBuffer);
    const Char* word=ebufGetDataPointer(wordBuffer);
    ResponseParsingResult result;
    Err error=ProcessResponse(appContext, word, responseBegin, responseEnd, result);
    if (!error)
    {
        if (responseCookie==result)
        {
            Assert(HasCookie(appContext->prefs));
            StartWordLookup(appContext, word);
        }
        else if (responseMessage!=result && responseErrorMessage!=result)
        {
            error=appErrMalformedResponse;
            FrmAlert(alertMalformedResponse);
        }
    }
    return error;
}

static void CookieRequestContextDestructor(void* context)
{
    ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
    Assert(wordBuffer);
    ebufDelete(wordBuffer);
}

static Err CookieRequestPrepareRequest(ExtensibleBuffer& buffer)
{
    Err error=errNone;
    ExtensibleBuffer deviceIdBuffer;
    ebufInit(&deviceIdBuffer, 0);
    RenderDeviceIdentifier(&deviceIdBuffer);
    ebufAddChar(&deviceIdBuffer, chrNull);
    Char* deviceId=ebufGetDataPointer(&deviceIdBuffer);
    UInt16 deviceIdLength=ebufGetDataSize(&deviceIdBuffer);
    error=StrUrlEncode(deviceId, deviceId+deviceIdLength, &deviceId, &deviceIdLength);
    if (error)
        goto OnError;
    ebufFreeData(&deviceIdBuffer);
    
    Char protocolVersionBuffer[8];
    StrPrintF(protocolVersionBuffer, "%hd", (short)protocolVersion);
    
    Char clientVersionBuffer[8];
    StrPrintF(clientVersionBuffer, "%hd.%hd", (short)appVersionMajor, (short)appVersionMinor);
    
    Char* url=TxtParamString(cookieRequestUrl, protocolVersionBuffer, clientVersionBuffer, deviceId, NULL);
    new_free(deviceId);
    if (!url)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    ebufAddStr(&buffer, url);
    MemPtrFree(url);
    ebufAddChar(&buffer, chrNull);
OnError:        
    return error;    
    
}

void StartCookieRequestWithWordLookup(AppContext* appContext, const Char* word)
{
    ExtensibleBuffer request;
    ebufInit(&request, 0);
    Err error=CookieRequestPrepareRequest(request);
    if (!error)
    {
        ExtensibleBuffer* wordBuffer=ebufNew();
        if (wordBuffer)
        {
            ebufInitWithStr(wordBuffer, const_cast<Char*>(word));
            StartConnection(appContext, wordBuffer, ebufGetDataPointer(&request), GeneralStatusTextRenderer, 
                CookieRequestResponseProcessor,  CookieRequestContextDestructor);
        }
        else
            FrmAlert(alertMemError);
    }
    else
        FrmAlert(alertMemError);    
    ebufFreeData(&request);
}
