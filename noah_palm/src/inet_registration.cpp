/**
 * @file inet_registration.cpp
 * Implementation of iNoah internet registration functionality.
 *
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 *
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */

#include "inet_registration.h"
#include "inet_connection.h"

/**
 * Response returned by server if registration succeeds.
 */
#define registrationSuccessfulResponse "REG_SUCCESS"

/**
 * Response returned by server if given user id is already assigned to this device.
 */
#define alreadyRegisteredResponse      "REG_ALREADY"

/**
 * Response returned by server if given user id is already registered on other device.
 */
#define multipleNotAllowedResponse     "REG_MULTIPLE"

/**
 * URL that forms registration request.
 */
#define registrationRequestUrl "/dict-reg.php?uid=^0&did=^1"

/**
 * Callback used by internet connection framework to render connection status text.
 * @see ConnectionStatusRenderer
 * @return standard PalmOS error code (@c errNone if success - always, as current implementation is trivial).
 */
static Err RegistrationStatusTextRenderer(void* context, ConnectionStage stage, UInt16 responseLength, ExtensibleBuffer& statusBuffer)
{
    const Char* baseText=NULL;
    switch (stage) 
    {
        case stageResolvingAddress:
            baseText="Resolving host address";
            break;
        case stageOpeningConnection:
            baseText="Opening connection";
            break;
        case stageSendingRequest:
            baseText="Registering...";
            break;                        
        case stageReceivingResponse:
            baseText="Retrieving response...";
            break;
        case stageFinished:
            baseText="Finished";
            break;
        default:
            Assert(false);
    }
    ebufAddStr(&statusBuffer, const_cast<Char*>(baseText));
    return errNone;
}

/**
 * Callback used by internet connection framework to free connection context data.
 * Empty, as we don't use context during registration.
 * @see ConnectionContextDestructor
 */
static void RegistrationContextDestructor(void* context)
{
    Assert(NULL==context);
}

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
                urlEncDid=ebufGetDataPointer(&urlEncDidBuffer);
                Char* url=TxtParamString(registrationRequestUrl, urlEncUid, urlEncDid, NULL, NULL);
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
    Assert(NULL==context);
    if (StrStartsWith(responseBegin, responseEnd, registrationSuccessfulResponse))
        FrmAlert(alertRegistrationSuccessful);
    else if (StrStartsWith(responseBegin, responseEnd, alreadyRegisteredResponse))
        FrmAlert(alertAlreadyRegistered);
    else if (StrStartsWith(responseBegin, responseEnd, multipleNotAllowedResponse))
        FrmAlert(alertMultipleRegistration);
    else 
    {
        FrmAlert(alertMalformedResponse);
        error=appErrMalformedResponse;
    }
    return error;
}

void StartRegistration(AppContext* appContext, const Char* userId)
{
    Assert(userId);
    ExtensibleBuffer requestBuffer;
    ebufInit(&requestBuffer, 0);    Err error=RegistrationPrepareRequest(userId, requestBuffer);
    if (!error)
    {
        const Char* requestText=ebufGetDataPointer(&requestBuffer);
        StartConnection(appContext, NULL, requestText, RegistrationStatusTextRenderer,            RegistrationResponseProcessor, RegistrationContextDestructor);
    }
    if (error)
        FrmAlert(alertMemError);
    ebufFreeData(&requestBuffer);
}
