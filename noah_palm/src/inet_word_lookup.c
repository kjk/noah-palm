/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#include "inet_word_lookup.h"
#include "inet_connection.h"
#include "inet_definition_format.h"
#include "inet_registration.h"

#define wordLookupURL           "/dict-raw.php?word=^0&ver=^1&uid=^2"

#ifdef DEBUG

static const char* deviceWordsListResponse = 
    "WORD_LIST\xd" "\xa" 
    "word\xd" "\xa" 
    "work\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\xd" "\xa" 
    "\x0";
  
static const char* deviceMessageResponse = 
    "MESSAGE\xd" "\xa" 
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

inline static Err WordLookupRenderStatusText(const Char* word, ConnectionStage stage, UInt16 bytesReceived, const Char* baseText, ExtensibleBuffer& statusText)
{
    Err error=errNone;
    Char* text=TxtParamString(baseText, word, NULL, NULL, NULL);
    if (text)
    {
        ebufResetWithStr(&statusText, text);
        MemPtrFree(text);
        if (stageReceivingResponse==stage)
        {
            static const UInt16 bytesBufferSize=28; // it will be placed in code segment, so no worry about globals
            Char buffer[bytesBufferSize];
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
                Char formatString[9];
                StrCopy(formatString, " %d.%d kB");
                NumberFormatType numFormat=static_cast<NumberFormatType>(PrefGetPreference(prefNumberFormat));
                Char dontCare;
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
    const Char* word=ebufGetDataPointer(wordBuffer);
    Assert(word);
    return WordLookupRenderStatusText(word, stage, responseLength, baseText, statusBuffer);
}

static void WordLookupContextDestructor(void* context)
{
    ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
    Assert(wordBuffer);
    ebufDelete(wordBuffer);
}

static Err WordLookupPrepareRequest(const Char* word, ExtensibleBuffer& buffer)
{
    Err error=errNone;
    Char* url=NULL;
    Assert(word);
    UInt16 wordLength=StrLen(word);
    Char* urlEncodedWord;
    error=StrUrlEncode(word, word+wordLength, &urlEncodedWord, &wordLength);
    if (error)
        goto OnError;
    Char versionBuffer[8];
    StrPrintF(versionBuffer, "%hd.%hd", (short)appVersionMajor, (short)appVersionMinor);
    url=TxtParamString(wordLookupURL, urlEncodedWord, versionBuffer, NULL, NULL);
    new_free(urlEncodedWord);
    if (!url)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    ebufResetWithStr(&buffer, url);
    MemPtrFree(url);
    ebufAddChar(&buffer, chrNull);
OnError:        
    return error;    
}

static Err WordLookupResponseProcessor(AppContext* appContext, void* context, const Char* responseBegin, const Char* responseEnd)
{
    ExtensibleBuffer* wordBuffer=static_cast<ExtensibleBuffer*>(context);
    Assert(wordBuffer);
    const Char* word=ebufGetDataPointer(wordBuffer);
    Assert(word);
    ResponseParsingResult result=ProcessResponse(appContext, word, responseBegin, responseEnd);
    switch (result)
    {
        case responseOneWord:
            appContext->mainFormContent=mainFormShowsDefinition;
            appContext->firstDispLine=0;
            break;
            
        case responseMessage:
            appContext->mainFormContent=mainFormShowsMessage;
            appContext->firstDispLine=0;
            break;
            
        case responseWordsList:
            FrmPopupForm(formWordsList);
            
        case responseWordNotFound:
            FrmCustomAlert(alertWordNotFound, word, NULL, NULL);
            break;

        case responseError:
            FrmAlert(alertMalformedResponse);
            break;
            
        default:
            Assert(false);
    }
    return errNone;    
}

void StartWordLookup(AppContext* appContext, const Char* word)
{
    if (appContext->prefs.registrationNeeded)
    {
        Assert(StrLen(appContext->prefs.userId));
        StartRegistrationWithLookup(appContext, appContext->prefs.userId, word);
    }
    else
    {
        ExtensibleBuffer urlBuffer;
        ebufInit(&urlBuffer, 0);
        Err error=WordLookupPrepareRequest(word, urlBuffer);
        if (!error) 
        {
            const Char* requestUrl=ebufGetDataPointer(&urlBuffer);
            ExtensibleBuffer* context=ebufNew();
            if (context)
            {
                ebufInitWithStr(context, const_cast<Char*>(word));
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
