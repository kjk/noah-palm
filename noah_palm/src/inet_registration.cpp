/**
 * @file inet_registration.cpp
 * Implementation of iNoah internet registration functionality.
 *
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 *
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */

#include "inet_registration.h"
#include "inet_definition_format.h"
#include "inet_connection.h"
#include "cookie_request.h"

/**
 * URL that forms registration request.
 */
#define registrationRequestUrl "/dict-2.php?pv=^0&cv=^1&c=^2&register=^3"

/**
 * Prepares request string to be sent to the server.
 * Fills @c buffer with properly formatted server-relative url that contains user and device id.
 * @param serialNumber serial number.
 * @param buffer @c ExtensibleBuffer to be filled with request string. 
 * Will contain valid data on successful return.
 * @return standard PalmOS error code (@c errNone on success).
 */
static Err RegistrationPrepareRequest(const char* cookie, const char* serialNumber, ExtensibleBuffer& buffer)
{
    Err error=errNone;
    UInt16 snLength=StrLen(serialNumber);
    char* urlEncSn=NULL;
    error=StrUrlEncode(serialNumber, serialNumber+snLength+1, &urlEncSn, &snLength); // StrUrlEncode preserves null-terminator
    if (!error) 
    {
        UInt16 cookieLength=StrLen(cookie);
        char* urlEncCookie=NULL;
        error=StrUrlEncode(cookie, cookie+cookieLength+1, &urlEncCookie, &cookieLength);
        if (!error)
        {
            char* url=TxtParamString(registrationRequestUrl, PROTOCOL_VERSION, CLIENT_VERSION, urlEncCookie, urlEncSn);
            if (url)
            {
                ebufAddStr(&buffer, url);
                MemPtrFree(url);
            }
            else
                error=memErrNotEnoughSpace;
            new_free(urlEncCookie);
        }
        new_free(urlEncSn);
    }
    return error;    
}

/**
 * Callback used by internet connection framework to process valid response.
 * Shows registration outcome as an alert (info or error etc.).
 * @see ConnectionResponseProcessor
 */
static Err RegistrationResponseProcessor(AppContext* appContext, void* context, const Char* responseBegin, const Char* responseEnd)
{
    ResponseParsingResult result;
    Err error=ProcessResponse(appContext, responseBegin, responseEnd, 
        responseRegistrationOk|responseRegistrationFailed, result);

    Assert( error || (responseRegistrationOk==result) || (responseRegistrationFailed==result) );
    return error;
}


void StartRegistration(AppContext* appContext, const char* serialNumber)
{
    Assert(serialNumber);
    if (!HasCookie(appContext->prefs))
        StartCookieRequestWithRegistration(appContext, serialNumber);
    else 
    {
        ExtensibleBuffer requestBuffer;
        ebufInit(&requestBuffer, 0);
        Err error=RegistrationPrepareRequest(appContext->prefs.cookie, serialNumber, requestBuffer);
        if (!error)
        {
            const Char* requestText=ebufGetDataPointer(&requestBuffer);
            StartConnection(appContext, NULL, requestText, GeneralStatusTextRenderer,
                RegistrationResponseProcessor, NULL);
        }
        else
            FrmAlert(alertMemError);
        ebufFreeData(&requestBuffer);
    }
}
