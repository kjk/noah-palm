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
#include "inet_registration.h"

/**
 * URL that forms cookie request.
 */
#define cookieRequestUrl "/palm.php?pv=^0&cv=^1&get_cookie&di=^2"

static Err CookieRequestResponseProcessorCommon(AppContext* appContext, void* context, const char* responseBegin, const char* responseEnd, const char*& wordOut)
{
    ResponseParsingResult   result;
    Err error=ProcessResponse(appContext, responseBegin, responseEnd, result);
    if (!error)
    {
        if (responseCookie==result)
        {
            Assert(HasCookie(appContext->prefs));
            if (context)
            {
                ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
                const char* word=ebufGetDataPointer(wordBuffer);
                Assert(word);
                wordOut=word;
            }
            else
                wordOut=NULL;
        }
        else if (responseMessage!=result && responseErrorMessage!=result)
        {
            error=appErrMalformedResponse;
            FrmAlert(alertMalformedResponse);
        }
        else
        {
            appContext->mainFormContent=mainFormShowsMessage;
            appContext->firstDispLine=0;
        }
    }
    return error;
}

/**
 * Callback used by internet connection framework to process valid response.
 * if context is NULL we should follow up with GET_RANDOM_WORD request,
 * otherwise with GET_WORD request
 * @see ConnectionResponseProcessor
 */
static Err CookieRequestResponseProcessorWithLookup(AppContext* appContext, void* context, const char* responseBegin, const char* responseEnd)
{
    const char* word=NULL;
    Err error=CookieRequestResponseProcessorCommon(appContext, context, responseBegin, responseEnd, word);
    if (!error)
    {
        if (word)
            StartWordLookup(appContext, word);
        else
            StartRandomWordLookup(appContext);
    }            
    return error;       
}

/**
 * Callback used by internet connection framework to process valid response.
 * Extracts serial number from @c context and calls @c StartRegistration(AppContext*, const char*).
 * @see ConnectionResponseProcessor
 */
static Err CookieRequestResponseProcessorWithRegistration(AppContext* appContext, void* context, const char* responseBegin, const char* responseEnd)
{
    const char* serialNumber=NULL;
    Err error=CookieRequestResponseProcessorCommon(appContext, context, responseBegin, responseEnd, serialNumber);
    if (!error)
    {
        Assert(serialNumber);
        StartRegistration(appContext, serialNumber);
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
                    CookieRequestResponseProcessorWithLookup,  CookieRequestContextDestructor);
            }
            else
                FrmAlert(alertMemError);
        }
        else
        {
            // this means GET_RANDOM_WORD in CookieRequestResponseProcessor
            StartConnection(appContext, NULL, ebufGetDataPointer(&request), GeneralStatusTextRenderer, 
                CookieRequestResponseProcessorWithLookup,  CookieRequestContextDestructor);
        }
            
    }
    else
        FrmAlert(alertMemError);    
    ebufFreeData(&request);
}

void StartCookieRequestWithRegistration(AppContext* appContext, const char* serialNumber)
{
    ExtensibleBuffer request;
    ebufInit(&request, 0);
    Err error=CookieRequestPrepareRequest(request);
    if (!error)
    {
        ExtensibleBuffer* snBuffer=ebufNew();
        if (snBuffer)
        {
            ebufInitWithStr(snBuffer, const_cast<char*>(serialNumber));
            StartConnection(appContext, snBuffer, ebufGetDataPointer(&request), GeneralStatusTextRenderer, 
                CookieRequestResponseProcessorWithRegistration, CookieRequestContextDestructor);
        }
        else
            FrmAlert(alertMemError);
    }
    else
        FrmAlert(alertMemError);    
    ebufFreeData(&request);
}
