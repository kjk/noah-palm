/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#include "inet_word_lookup.h"
#include "inet_connection.h"
#include "inet_definition_format.h"
#include "cookie_request.h"

#define wordLookupURL           "/palm.php?pv=^0&cv=^1&c=^2&get_word=^3"

#define genericURL                  "/palm.php?pv=^0&cv=^1&c=^2&^3"
#define randomWordRequestParam "get_random_word"
#define recentLookupsRequestParam "recent_lookups"

#define randomWordURL           "/palm.php?pv=^0&cv=^1&c=^2&get_random_word"

#ifdef DEBUG

static const char* deviceWordsListResponse = 
    "WORDLIST\xd" "\xa" 
    "word\xd" "\xa" 
    "work\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\x0";
  
static const char* deviceMessageResponse = 
    "MSG\xd" "\xa" 
    "There's a new version of iNoah available.\xd" "\xa" 
    "Please visit www.arslexis.com to get it.\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\x0";
    
/*
void testParseResponse(char *txt)
{
    Err              err;
    ConnectionData   connData;

    ebufInitWithStr(&connData.response, txt);

    err = ParseResponse(&connData);
    Assert( errNone == err );
}
*/

#endif

inline static Err WordLookupRenderStatusText(const char* word, ConnectionStage stage, UInt16 bytesReceived, const char* baseText, ExtensibleBuffer& statusText)
{
    Err error=errNone;
    char* text=TxtParamString(baseText, word, NULL, NULL, NULL);
    if (text)
    {
        ebufResetWithStr(&statusText, text);
        MemPtrFree(text);
        if (stageReceivingResponse==stage)
        {
            static const UInt16 bytesBufferSize=28; // it will be placed in code segment, so no worry about globals
            char buffer[bytesBufferSize];
            Int16 bytesLen=0;
            if (bytesReceived<1024)
                bytesLen=StrPrintF(buffer, " %d bytes", bytesReceived);
            else
            {
                UInt16 bri=bytesReceived/1024;
                UInt16 brf=((bytesReceived%1024)+51)/102; // round to 1/10
                Assert(brf<=10);
                if (brf==10)
                {
                    bri++;
                    brf=0;
                }
                char formatString[9];
                StrCopy(formatString, " %d.%d kB");
                NumberFormatType numFormat=static_cast<NumberFormatType>(PrefGetPreference(prefNumberFormat));
                char dontCare;
                LocGetNumberSeparators(numFormat, &dontCare, formatString+3); // change decimal separator in place
                bytesLen=StrPrintF(buffer, formatString, bri, brf);                
            }
            Assert(bytesLen<bytesBufferSize);
            if (bytesLen>0)
                ebufAddStrN(&statusText, buffer, bytesLen);
        }
        ebufAddChar(&statusText, chrNull);
    }
    else 
        error=memErrNotEnoughSpace;
    return error;
}

static Err WordLookupStatusTextRenderer(void* context, ConnectionStage stage, UInt16 responseLength, ExtensibleBuffer& statusBuffer)
{
    const char* baseText=NULL;
    switch (stage) 
    {
        case stageResolvingAddress:
            baseText="Resolving host address";
            break;
        case stageOpeningConnection:
            baseText="Opening connection";
            break;
        case stageSendingRequest:
            baseText="Requesting: ^0";
            break;                        
        case stageReceivingResponse:
            baseText="Retrieving: ^0";
            break;
        case stageFinished:
            baseText="Finished";
            break;
        default:
            Assert(false);
    }
    ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
    Assert(wordBuffer);
    const char* word=ebufGetDataPointer(wordBuffer);
    Assert(word);
    return WordLookupRenderStatusText(word, stage, responseLength, baseText, statusBuffer);
}

static void WordLookupContextDestructor(void* context)
{
    ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
    Assert(wordBuffer);
    ebufDelete(wordBuffer);
}

static Err WordLookupPrepareRequest(const char* cookie, const char* word, ExtensibleBuffer& buffer)
{
    Err     error=errNone;
    char *  url=NULL;
    Assert(word);
    UInt16  wordLength=StrLen(word);
    char *  urlEncodedWord;
    error=StrUrlEncode(word, word+wordLength, &urlEncodedWord, &wordLength);
    if (error)
        goto OnError;
    
    url=TxtParamString(wordLookupURL, PROTOCOL_VERSION, CLIENT_VERSION, cookie, urlEncodedWord);
    new_free(urlEncodedWord);
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

static Err WordLookupResponseProcessor(AppContext* appContext, void* context, const char* responseBegin, const char* responseEnd)
{
    ResponseParsingResult result;
    Err error=ProcessResponse(appContext, responseBegin, responseEnd, 
        responseDefinition|responseMessage|responseErrorMessage|responseWordsList, result);
    if (!error) 
    {
        switch (result)
        {
            case responseDefinition:
                appContext->mainFormContent=mainFormShowsDefinition;
                appContext->firstDispLine=0;
                break;
                
            case responseMessage:
            case responseErrorMessage:
                appContext->mainFormContent=mainFormShowsMessage;
                appContext->firstDispLine=0;
                break;
                
            case responseWordsList:
                FrmPopupForm(formWordsList);
                break;
                
            default:
                Assert(false);
        }
    }
    return error;
}

void StartWordLookup(AppContext* appContext, const Char* word)
{
    if (!HasCookie(appContext->prefs))
        StartCookieRequestWithWordLookup(appContext, word);
    else
    {
        ExtensibleBuffer urlBuffer;
        ebufInit(&urlBuffer, 0);
        Err error=WordLookupPrepareRequest(appContext->prefs.cookie, word, urlBuffer);
        if (!error) 
        {
            const char* requestUrl=ebufGetDataPointer(&urlBuffer);
            ExtensibleBuffer* context=ebufNew();
            if (context)
            {
                ebufInitWithStr(context, const_cast<char*>(word));
                StartConnection(appContext, context, requestUrl, WordLookupStatusTextRenderer,
                    WordLookupResponseProcessor, WordLookupContextDestructor);
            }
            else 
                FrmAlert(alertMemError);            
        }
        else 
            FrmAlert(alertMemError);
        ebufFreeData(&urlBuffer);
    }
}

static Err PrepareGenericRequest(const char* cookie, const char* param, ExtensibleBuffer& buffer)
{
    Assert(cookie);
    Assert(param);
    char* urlEncParam=NULL;
    UInt16 paramLength=StrLen(param);
    Err error=StrUrlEncode(param, param+paramLength, &urlEncParam, &paramLength);
    if (!error)
    {    
        char* url=TxtParamString(genericURL, PROTOCOL_VERSION, CLIENT_VERSION, cookie, urlEncParam);
        if (url)
        {
            ebufAddStr(&buffer, url);
            MemPtrFree(url);
            ebufAddChar(&buffer, chrNull);
        }
        else
            error=memErrNotEnoughSpace;
        new_free(urlEncParam);
    }
    return error;
}

void StartRandomWordLookup(AppContext* appContext)
{
    if (!HasCookie(appContext->prefs))
        StartCookieRequestWithWordLookup(appContext, NULL);
    else
    {
        ExtensibleBuffer urlBuffer;
        ebufInit(&urlBuffer, 0);
        Err error=PrepareGenericRequest(appContext->prefs.cookie, randomWordRequestParam, urlBuffer);
        if (!error) 
        {
            const char* requestUrl=ebufGetDataPointer(&urlBuffer);
            ExtensibleBuffer* context=ebufNew();
            if (context)
            {
                ebufInitWithStr(context, "random word");
                StartConnection(appContext, context, requestUrl, WordLookupStatusTextRenderer,
                    WordLookupResponseProcessor, WordLookupContextDestructor);
            }
            else 
                FrmAlert(alertMemError);            
        }
        else 
            FrmAlert(alertMemError);
        ebufFreeData(&urlBuffer);
    }
}

static Err RecentLookupsResponseProcessor(AppContext* appContext, void* context, const char* responseBegin, const char* responseEnd)
{
    ResponseParsingResult result;
    Err error=ProcessResponse(appContext, responseBegin, responseEnd, 
        responseWordsList|responseMessage|responseErrorMessage, result);
    if (!error) 
    {
        if (responseWordsList==result)
            FrmPopupForm(formWordsList);
        else if (responseMessage==result || responseErrorMessage==result)
        {
            appContext->mainFormContent=mainFormShowsMessage;
            appContext->firstDispLine=0;
        }
        else
            Assert(false);
    }        
    return error;
}

void StartRecentLookupsRequest(AppContext* appContext)
{
    if (!HasCookie(appContext->prefs))        FrmAlert(alertNoLookupsYet);
    else 
    {
        ExtensibleBuffer urlBuffer;
        ebufInit(&urlBuffer, 0);
        Err error=PrepareGenericRequest(appContext->prefs.cookie, recentLookupsRequestParam, urlBuffer);
        if (!error) 
        {
            const char* requestUrl=ebufGetDataPointer(&urlBuffer);
            StartConnection(appContext, NULL, requestUrl, GeneralStatusTextRenderer,
                RecentLookupsResponseProcessor, NULL);
        }
        else 
            FrmAlert(alertMemError);
        ebufFreeData(&urlBuffer);
    }
}

