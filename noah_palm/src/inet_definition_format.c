#include "inet_definition_format.h"
#include "blowfish.h"

#define wordsListResponse       "WORDLIST\n"
#define wordsListResponseLen    sizeof(wordsListResponse)-1
#define messageResponse         "MESSAGE\n"
#define messageResponseLen      sizeof(messageResponse)-1
#define errorMessageResponse    "ERROR\n"
#define errorMessageResponseLen sizeof(errorMessageResponse)-1
#define cookieResponse          "COOKIE\n"
#define cookieResponseLen       sizeof(cookieResponse)-1
#define definitionResponse      "DEF\n"
#define definitionResponseLen   sizeof(definitionResponse)-1

#define pronunciationTag        "PRON"
#define pronunciationTagLen    sizeof(pronunciationTag)-1
#define requestsLeftTag         "REQUESTS_LEFT"
#define requestsLeftTagLen      sizeof(requestsLeftTag)-1


static Err ExpandPartOfSpeechSymbol(char symbol, const char** partOfSpeech)
{
    Err error=errNone;
    switch (symbol)
    {
        case 'v': 
            *partOfSpeech="verb";
            break;
        case 'n':
            *partOfSpeech="noun";
            break;
        case 'r':
            *partOfSpeech="adv.";
            break;
        case 's':
        case 'a':
            *partOfSpeech="adj.";
            break;
        default:
            error=appErrMalformedResponse;         
    }
    return error;            
}

static Err ConvertSynonymsBlock(AppContext* appContext, const char* word, const char* begin, const char* end, ExtensibleBuffer* out)
{
    Err error=errNone;
    const UInt16 wordLen=StrLen(word);
    UInt16 synsCount=0;
    while (begin<end) 
    {
        const char* synBegin=begin+1;
        if (synBegin<end) 
        {  
            const char* synEnd=StrFind(synBegin, end, "\n!");
            begin=synEnd+1;
            StrTrimTail(synBegin, &synEnd);
            if (synBegin<synEnd)
            {
                UInt16 synLen=synEnd-synBegin;
                if (!FormatWantsWord(appContext) || !(wordLen==synLen && StrStartsWith(synBegin, synEnd, word)))
                {
                    if (synsCount++) 
                        ebufAddChar(out, ',');
                    else 
                    {
                        ebufAddChar(out, FORMAT_TAG);
                        ebufAddChar(out, FORMAT_WORD);
                    }
                    ebufAddChar(out, ' ');
                    ebufAddStrN(out, (char*)synBegin, synLen);
                }
            }
            else
            {
                error=appErrMalformedResponse;                
                break;
            }                
        }
        else
        {
            error=appErrMalformedResponse;
            break;
        }                            
    }
    if (!error && synsCount)
        ebufAddChar(out, '\n');
    return error;
}

static Err ConvertDefinitionBlock(const char* begin, const char* end, ExtensibleBuffer* out)
{
    Err error=errNone;
    while (begin<end)
    {
        UInt8 tag=(*begin=='@'?FORMAT_DEFINITION:FORMAT_EXAMPLE);
        const char* defBegin=begin+1;
        if (defBegin<end)
        {
            const char* defEnd=StrFindOneOf(defBegin, end, "@#");
            while (defEnd<end)
            {
                if (StrStartsWith(defEnd-1, defEnd, "\n"))
                    break;
                else                    
                    defEnd=StrFindOneOf(defEnd+1, end, "@#");
            }
            begin=defEnd;
            StrTrimTail(defBegin, &defEnd);
            if (defBegin<defEnd)
            {
                ebufAddChar(out, FORMAT_TAG);
                ebufAddChar(out, (char)tag);
                ebufAddChar(out, ' ');
                if (FORMAT_EXAMPLE==tag)
                    ebufAddChar(out, '\"');
                ebufAddStrN(out, (char*)defBegin, defEnd-defBegin);
                if (FORMAT_EXAMPLE==tag)
                    ebufAddChar(out, '\"');
                ebufAddChar(out, '\n');                    
            }
            else
            {
                error=appErrMalformedResponse;
                break;
            }
        }
        else
        {
            error=appErrMalformedResponse;
            break;
        }            
    }
    return error;
}


static Err ConvertInetToDisplayableFormat(AppContext* appContext, const char* word, const char* begin, const char* end, ExtensibleBuffer* out)
{
    Err error=errNone;
    if (FormatWantsWord(appContext))
    {
        ebufAddChar(out, FORMAT_TAG);
        ebufAddChar(out, FORMAT_SYNONYM);
        ebufAddStr(out, (char*)word);
    }

    // PRON tag is optional
    if (StrStartsWith(begin, end, pronunciationTag))
    {
        const char* pronBegin=begin+pronunciationTagLen+1;
        if ( !(pronBegin<end) )
            return appErrMalformedResponse;

        const char* pronEnd=StrFind(pronBegin, end, "\n");
        if (NULL==pronEnd)
            return appErrMalformedResponse;

        if (appContext->prefs.fEnablePronunciation)
        {
            if (appContext->prefs.fShowPronunciation)
            {
                ebufAddChar(out, FORMAT_TAG);
                ebufAddChar(out, FORMAT_PRONUNCIATION);
                ebufAddStr(out, " (");
                ebufAddStrN(out, const_cast<char*>(pronBegin), pronEnd-pronBegin);
                ebufAddChar(out, ')');
            }
        }
        begin=pronEnd+1;
    }

    // REQUESTS_LEFT tag is optional
    if (StrStartsWith(begin,end,requestsLeftTag))
    {
        // shouldn't get this if we're registered
        Assert(0==StrLen(appContext->prefs.regCode));
        const char* reqLeftBegin=begin+requestsLeftTagLen+1;
        if ( !(reqLeftBegin<end) )
            return appErrMalformedResponse;
        const char* reqLeftEnd=StrFind(reqLeftBegin, end, "\n");
        if (NULL==reqLeftEnd)
            return appErrMalformedResponse;

        // TODO: parse the number
    }

    if (ebufGetDataSize(out))
        ebufAddChar(out, '\n');
    begin=StrFind(begin, end, "!");
    while (begin<end)
    {
        const char* partBlock=StrFind(begin, end, "\n$");
        if (partBlock<end)
        {
            const char* partOfSpeech=(partBlock+=1)+1;
            if (partOfSpeech<end)
            {
                error=ExpandPartOfSpeechSymbol(*partOfSpeech, &partOfSpeech);
                if (error)
                    return error;

                ebufAddChar(out, FORMAT_TAG);
                ebufAddChar(out, FORMAT_POS);
                ebufAddChar(out, 149);
                ebufAddStr(out, " (");
                ebufAddStr(out, (char*)partOfSpeech);
                ebufAddStr(out, ") ");
                error=ConvertSynonymsBlock(appContext, word, begin, partBlock, out);
                if (error)
                    return error;

                const char* defBlock=StrFindOneOf(partBlock+1, end, "@#");
                while (defBlock<end)
                {
                    if (StrStartsWith(defBlock-1, defBlock, "\n"))
                        break;
                    else                    
                        defBlock=StrFindOneOf(defBlock+1, end, "@#");
                }
                if (defBlock<end)
                {
                    begin=StrFind(defBlock, end, "\n!");
                    if (begin<end) 
                        begin++;
                    error=ConvertDefinitionBlock(defBlock, begin, out);
                    if (error)
                        break;
                }
                else
                {
                    error=appErrMalformedResponse;
                    break;
                }                        
            }
            else
            {
                error=appErrMalformedResponse;
                break;
            }       
        }
    }
    return error;
}

Err ProcessDefinitionResponse(AppContext* appContext, const char* responseBegin, const char* responseEnd)
{
    Err error=errNone;
    const char * word = NULL;

    responseBegin+=definitionResponseLen;
    const char* wordEnd=StrFind(responseBegin, responseEnd, "\n");
    if (wordEnd>responseBegin)
    {
        ExtensibleBuffer wordBuffer;
        ebufInitWithStrN(&wordBuffer, const_cast<char*>(responseBegin), wordEnd-responseBegin);
        ebufAddChar(&wordBuffer, chrNull);
        word=ebufGetDataPointer(&wordBuffer);
        responseBegin=wordEnd+1;
        if (responseBegin<responseEnd)
        {
            ExtensibleBuffer buffer;
            ebufInit(&buffer, 0);
            error=ConvertInetToDisplayableFormat(appContext, word, responseBegin, responseEnd, &buffer);
            if (!error)
            {
                ebufAddChar(&buffer, chrNull);
                ebufWrapBigLines(&buffer,true);
                ebufSwap(&buffer, &appContext->currentDefinition);
                diSetRawTxt(appContext->currDispInfo, ebufGetDataPointer(&appContext->currentDefinition));
                ebufResetWithStr(&appContext->currentWordBuf, (char*)word);
                ebufAddChar(&appContext->currentWordBuf, chrNull);
            }
            ebufFreeData(&buffer);
        }
        else
            error=appErrMalformedResponse;
        ebufFreeData(&wordBuffer);
    }
    else
        error=appErrMalformedResponse;

    if (!error && word)
        FldClearInsert(FrmGetFormPtr(formDictMain), fieldWordInput, word);
    return error;
}

static UInt16 IterateWordsList(const char* responseBegin, const char* responseEnd, char** targetList=NULL)
{
    UInt16      wordsCount=0;
    const char* wordBegin=responseBegin+wordsListResponseLen;
    while (wordBegin<responseEnd)
    {
        const char* wordEnd=StrFind(wordBegin, responseEnd, "\n");
        if (wordBegin!=wordEnd)
        {
            if (targetList)
            {
                UInt16 wordLength=wordEnd-wordBegin;
                char*& targetWord=targetList[wordsCount];
                targetWord=static_cast<char*>(new_malloc_zero(sizeof(char)*(wordLength+1)));
                if (targetWord) 
                {
                    StrNCopy(targetWord, wordBegin, wordLength);
                    targetWord[wordLength]=chrNull;
                }
            }
            wordsCount++;
        }
        if (wordEnd>=responseEnd)
            wordBegin=responseEnd;
        else
            wordBegin=wordEnd+1;
    }
    return wordsCount;
}

static Int16 StrSortComparator(void* str1, void* str2, Int32)
{
    const char* s1=*(const char**)str1;
    const char* s2=*(const char**)str2;
    return StrCompare(s1, s2);
}

static Err ProcessWordsListResponse(AppContext* appContext, const char* responseBegin, const char* responseEnd)
{
    Err error=errNone;
    Assert(!appContext->wordsList);
    UInt16 wordsCount=IterateWordsList(responseBegin, responseEnd);
    if (wordsCount)
    {
        char** wordsStorage=static_cast<char**>(new_malloc_zero(sizeof(char*)*wordsCount));
        if (wordsStorage)
        {
            IterateWordsList(responseBegin, responseEnd, wordsStorage);
            SysQSort(wordsStorage, wordsCount, sizeof(const char*), StrSortComparator, NULL);
            appContext->wordsList=wordsStorage;
            appContext->wordsInListCount=wordsCount;
        }
        else
        {
            FrmAlert(alertMemError);
            error=memErrNotEnoughSpace;
        }
    }
    else
        error=appErrMalformedResponse;    
    return error;
}

static Err ProcessMessageResponse(AppContext* appContext, const char* responseBegin, const char* responseEnd, Boolean isErrorMessage=false)
{
    Err error=errNone;
    const char* messageBegin=responseBegin+(isErrorMessage?errorMessageResponseLen:messageResponseLen);
    if (messageBegin<responseEnd)
    {
        ExtensibleBuffer buffer;
        ebufInit(&buffer, 0);
        while (messageBegin<responseEnd)
        {
            const char* messageEnd=StrFind(messageBegin, responseEnd, "\n");
            if (messageBegin<messageEnd)
            {
                ebufAddStrN(&buffer, const_cast<char*>(messageBegin), messageEnd-messageBegin);
                ebufAddChar(&buffer, '\n');
            }
            if (messageEnd<responseEnd)
                messageBegin=messageEnd+1;
            else 
                messageBegin=responseEnd;
        }                
        ebufAddChar(&buffer, chrNull);
        ebufWrapBigLines(&buffer, true);
        ebufSwap(&appContext->lastMessage, &buffer);
        diSetRawTxt(appContext->currDispInfo, ebufGetDataPointer(&appContext->lastMessage));
        ebufFreeData(&buffer);
    }
    else 
        error=appErrMalformedResponse;
    return error;
}

static Err ProcessCookieResponse(AppContext* appContext, const char* responseBegin, const char* responseEnd)
{
    Err         error=errNone;
    const char* cookieBegin=responseBegin+cookieResponseLen;

    Assert(cookieBegin<=responseEnd);
    
    if (cookieBegin<responseEnd)
    {
        const char* cookieEnd=StrFind(cookieBegin, responseEnd, "\n");
        int cookieLength=cookieEnd-cookieBegin;
        if (cookieLength>0 && cookieLength<sizeof(appContext->prefs.cookie))
        {
            SafeStrNCopy( (char*)appContext->prefs.cookie, sizeof(appContext->prefs.cookie), (char*)cookieBegin, cookieLength);
            appContext->prefs.cookie[cookieLength]=chrNull;
        }
        else 
            error=appErrMalformedResponse;
    }
    else 
        error=appErrMalformedResponse;
    return error;
}

Err ProcessResponse(AppContext* appContext, const char* begin, const char* end, UInt16 resMask, ResponseParsingResult& result)
{
    Assert(begin);
    Assert(end);
    Err error;
    if ((responseCookie & resMask) && StrStartsWith(begin, end, cookieResponse))
    {
        error=ProcessCookieResponse(appContext, begin, end);
        if (!error)
            result=responseCookie;
    }
    else if ((responseWordsList & resMask) && StrStartsWith(begin, end, wordsListResponse))
    {
        error=ProcessWordsListResponse(appContext, begin, end);
        if (!error)
            result=responseWordsList;
    }
    else if ((responseMessage & resMask) && StrStartsWith(begin, end, messageResponse))
    {
        error=ProcessMessageResponse(appContext, begin, end);
        if (!error)
            result=responseMessage;
    }
    else if ((responseErrorMessage & resMask) && StrStartsWith(begin, end, errorMessageResponse))
    {
        error=ProcessMessageResponse(appContext, begin, end, true);
        if (!error)
            result=responseErrorMessage;
    }
    else if ((responseDefinition & resMask) && StrStartsWith(begin, end, definitionResponse))
    {
        error=ProcessDefinitionResponse(appContext, begin, end);
        if (!error)
            result=responseDefinition;
    } 
    else
        error=appErrMalformedResponse;

    if (appErrMalformedResponse==error)
    {
        // we used to do just FrmAlert() here but this is not healthy and leaves
        // the form in a broken state (objects not re-drawn, text field disabled
        // instead post a message telling form event handling code to show the alert
        SendShowMalformedAlert();
#ifdef DEBUG
        DumpStrToMemo(begin,end);
#endif
    }
    else if (!error)
    {
        if (ebufGetDataPointer(&appContext->lastResponse)!=begin)
        {
            ebufReset(&appContext->lastResponse);
            ebufAddStrN(&appContext->lastResponse, const_cast<char*>(begin), end-begin);
        }
    }
    return error;           
}
