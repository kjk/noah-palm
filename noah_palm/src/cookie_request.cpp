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
 * if context is NULL we should follow up with GET_RANDOM_WORD request,
 * otherwise with GET_WORD request
 * @see ConnectionResponseProcessor
 */
static Err CookieRequestResponseProcessor(AppContext* appContext, void* context, const char* responseBegin, const char* responseEnd)
{
    const char*             word=NULL;
    ResponseParsingResult   result;

    if (context)
    {
        ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
        Assert(wordBuffer);
        word=ebufGetDataPointer(wordBuffer);
    }
    Err error=ProcessResponse(appContext, word, responseBegin, responseEnd, result);
    if (!error)
    {
        if (responseCookie==result)
        {
            Assert(HasCookie(appContext->prefs));
            if (word)
                StartWordLookup(appContext, word);
            else
                StartRandomWordLookup(appContext);
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
    Err                 error=errNone;
    ExtensibleBuffer    deviceIdBuffer;
    ebufInit(&deviceIdBuffer, 0);
    RenderDeviceIdentifier(&deviceIdBuffer);
    ebufAddChar(&deviceIdBuffer, chrNull);
    char *              deviceId=ebufGetDataPointer(&deviceIdBuffer);
    UInt16              deviceIdLength=ebufGetDataSize(&deviceIdBuffer);

    error=StrUrlEncode(deviceId, deviceId+deviceIdLength, &deviceId, &deviceIdLength);
    if (error)
        goto OnError;
    ebufFreeData(&deviceIdBuffer);
    
    char * url=TxtParamString(cookieRequestUrl, PROTOCOL_VERSION, CLIENT_VERSION, deviceId, NULL);
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

// do a GET_COOKIE request followed by GET_WORD request or GET_RANDOM_WORD
// request (if word is NULL)
void StartCookieRequestWithWordLookup(AppContext* appContext, const char* word)
{
    ExtensibleBuffer request;
    ebufInit(&request, 0);
    Err error=CookieRequestPrepareRequest(request);
    if (!error)
    {
        if (word)
        {
            ExtensibleBuffer* wordBuffer=ebufNew();
            if (wordBuffer)
            {
                ebufInitWithStr(wordBuffer, const_cast<char*>(word));
                StartConnection(appContext, wordBuffer, ebufGetDataPointer(&request), GeneralStatusTextRenderer, 
                    CookieRequestResponseProcessor,  CookieRequestContextDestructor);
            }
            else
                FrmAlert(alertMemError);
        }
        else
        {
            // this means GET_RANDOM_WORD in CookieRequestResponseProcessor
            StartConnection(appContext, NULL, ebufGetDataPointer(&request), GeneralStatusTextRenderer, 
                CookieRequestResponseProcessor,  CookieRequestContextDestructor);
        }
            
    }
    else
        FrmAlert(alertMemError);    
    ebufFreeData(&request);
}
