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

/**
 * URL that forms registration request.
 */
#define registrationRequestUrl "/palm.php?pv=^0&cv=^1&c=^2&register=^3"

/**
 * Prepares request string to be sent to the server.
 * Fills @c buffer with properly formatted server-relative url that contains user and device id.
 * @param userId user id.
 * @param buffer @c ExtensibleBuffer to be filled with request string. 
 * Will contain valid data on successful return.
 * @return standard PalmOS error code (@c errNone on success).
 */
static Err RegistrationPrepareRequest(const Char* userId, ExtensibleBuffer& buffer)
{
    Err error=errNone;
/*
    const Char* deviceId=0;
    UInt16 deviceIdLength=0;
    Err error=SysGetDeviceId(reinterpret_cast<UInt8**>(const_cast<Char**>(&deviceId)), &deviceIdLength);
    if (!error)
    {
        Char* urlEncDid=NULL;
        UInt16 urlEncDidLength=0;
        error=StrUrlEncode(deviceId, deviceId+deviceIdLength, &urlEncDid, &urlEncDidLength);
        if (!error)
        {
            UInt16 uidLength=StrLen(userId);
            Char* urlEncUid=NULL;
            error=StrUrlEncode(userId, userId+uidLength, &urlEncUid, &uidLength); // StrUrlEncode preserves null-terminator
            if (!error) 
            {
                ExtensibleBuffer urlEncDidBuffer; // urlEncDid still needs to be null-terminated :-(
                ebufInitWithStrN(&urlEncDidBuffer, urlEncDid, urlEncDidLength);
                ebufAddChar(&urlEncDidBuffer, chrNull);
                Char* url=TxtParamString(registrationRequestUrl, urlEncUid, ebufGetDataPointer(&urlEncDidBuffer), NULL, NULL);
                if (url)
                {
                    ebufAddStr(&buffer, url);
                    MemPtrFree(url);
                }
                else
                    error=memErrNotEnoughSpace;
                ebufFreeData(&urlEncDidBuffer);
                new_free(urlEncUid);
            }
            new_free(urlEncDid);
        }
    }
*/    
    return error;    
}

/**
 * Callback used by internet connection framework to process valid response.
 * Shows registration outcome as an alert (info or error etc.).
 * @see ConnectionResponseProcessor
 */
static Err RegistrationResponseProcessor(AppContext* appContext, void* context, const Char* responseBegin, const Char* responseEnd)
{
    Err error=errNone;
/*    
    if (StrStartsWith(responseBegin, responseEnd, registrationSuccessfulResponse) ||
        StrStartsWith(responseBegin, responseEnd, alreadyRegisteredResponse))
    {
        ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
        Assert(wordBuffer);
        appContext->prefs.registrationNeeded=false;
        const Char* word=ebufGetDataPointer(wordBuffer);
        StartWordLookup(appContext, word);
    }
    else if (StrStartsWith(responseBegin, responseEnd, multipleNotAllowedResponse))
        FrmAlert(alertMultipleRegistration);
    else 
    {
        FrmAlert(alertMalformedResponse);
        error=appErrMalformedResponse;
    }
*/    
    return error;
}


void StartRegistration(AppContext* appContext, const Char* serialNumber)
{
    Assert(serialNumber);
    ExtensibleBuffer requestBuffer;
    ebufInit(&requestBuffer, 0);
    Err error=RegistrationPrepareRequest(serialNumber, requestBuffer);
    if (!error)
    {
        const Char* requestText=ebufGetDataPointer(&requestBuffer);
        StartConnection(appContext, NULL, requestText, GeneralStatusTextRenderer,
            RegistrationResponseProcessor, NULL);
    }
    if (error)
        FrmAlert(alertMemError);
    ebufFreeData(&requestBuffer);
}
