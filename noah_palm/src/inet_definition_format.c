#include "inet_definition_format.h"
#include "blowfish.h"

#define wordsListResponse       "WORDLIST"
#define wordsListResponseLen    8
#define messageResponse         "MESSAGE"
#define messageResponseLen      7
#define errorMessageResponse    "ERROR"
#define errorMessageResponseLen 6
#define cookieResponse          "COOKIE\n"
#define cookieResponseLen       7
#define definitionResponse      "DEF\n"
#define definitionResponseLen   4

static Err ExpandPartOfSpeechSymbol(Char symbol, const Char** partOfSpeech)
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

static Err ConvertSynonymsBlock(AppContext* appContext, const Char* word, const Char* begin, const Char* end, ExtensibleBuffer* out)
{
    Err error=errNone;
    const UInt16 wordLen=StrLen(word);
    UInt16 synsCount=0;
    while (begin<end) 
    {
        const Char* synBegin=begin+1;
        if (synBegin<end) 
        {  
            const Char* synEnd=StrFind(synBegin, end, "\n!");
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
                    ebufAddStrN(out, (Char*)synBegin, synLen);
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

static Err ConvertDefinitionBlock(const Char* begin, const Char* end, ExtensibleBuffer* out)
{
    Err error=errNone;
    while (begin<end)
    {
        UInt8 tag=(*begin=='@'?FORMAT_DEFINITION:FORMAT_EXAMPLE);
        const Char* defBegin=begin+1;
        if (defBegin<end)
        {
            const Char* defEnd=StrFindOneOf(defBegin, end, "@#");
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
                ebufAddChar(out, (Char)tag);
                ebufAddChar(out, ' ');
                if (FORMAT_EXAMPLE==tag)
                    ebufAddChar(out, '\"');
                ebufAddStrN(out, (Char*)defBegin, defEnd-defBegin);
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


static Err ConvertInetToDisplayableFormat(AppContext* appContext, const Char* word, const Char* begin, const Char* end, ExtensibleBuffer* out)
{
    Err error=errNone;
    if (FormatWantsWord(appContext))
    {
        ebufAddChar(out, FORMAT_TAG);
        ebufAddChar(out, FORMAT_SYNONYM);
        ebufAddStr(out, (Char*)word);
        ebufAddChar(out, '\n');
    }
    begin=StrFind(begin, end, "!");
    while (begin<end)
    {
        const Char* partBlock=StrFind(begin, end, "\n$");
        if (partBlock<end)
        {
            const Char* partOfSpeech=(partBlock+=1)+1;
            if (partOfSpeech<end)
            {
                error=ExpandPartOfSpeechSymbol(*partOfSpeech, &partOfSpeech);
                if (!error)
                {
                    ebufAddChar(out, FORMAT_TAG);
                    ebufAddChar(out, FORMAT_POS);
                    ebufAddChar(out, 149);
                    ebufAddStr(out, " (");
                    ebufAddStr(out, (Char*)partOfSpeech);
                    ebufAddStr(out, ") ");
                    error=ConvertSynonymsBlock(appContext, word, begin, partBlock, out);
                    if (!error)
                    {
                        const Char* defBlock=StrFindOneOf(partBlock+1, end, "@#");
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
                        break;
                }                    
                else 
                    break;
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

Err ProcessDefinitionResponse(AppContext* appContext, const char* word, const char* responseBegin, const char* responseEnd)
{
    Err error=errNone;
    responseBegin+=definitionResponseLen;
    if (responseBegin<responseEnd)
    {
        ExtensibleBuffer buffer;
        ebufInit(&buffer, 0);
        error=ConvertInetToDisplayableFormat(appContext, word, responseBegin, responseEnd, &buffer);
        if (!error)
        {
            ebufAddChar(&buffer, chrNull);
            ebufWrapBigLines(&buffer);
            ebufSwap(&buffer, &appContext->currentDefinition);
            diSetRawTxt(appContext->currDispInfo, ebufGetDataPointer(&appContext->currentDefinition));
            ebufResetWithStr(&appContext->currentWordBuf, (Char*)word);
            ebufAddChar(&appContext->currentWordBuf, chrNull);
        }
        ebufFreeData(&buffer);
    }
    else
        error=appErrMalformedResponse;        
    return error;
}

static UInt16 IterateWordsList(const Char* responseBegin, const Char* responseEnd, Char** targetList=NULL)
{
    UInt16 wordsCount=0;
//    const Char* wordBegin=StrFind(responseBegin, responseEnd, "\n")+1;
    const Char* wordBegin=responseBegin+8;
    while (wordBegin<responseEnd)
    {
        const Char* wordEnd=StrFind(wordBegin, responseEnd, "\n");
        if (wordBegin!=wordEnd)
        {
            if (targetList)
            {
                UInt16 wordLength=wordEnd-wordBegin;
                Char*& targetWord=targetList[wordsCount];
                targetWord=static_cast<Char*>(new_malloc_zero(sizeof(Char)*(wordLength+1)));
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

static Err ProcessWordsListResponse(AppContext* appContext, const Char* responseBegin, const Char* responseEnd)
{
    Err error=errNone;
    Assert(!appContext->wordsList);
    UInt16 wordsCount=IterateWordsList(responseBegin, responseEnd);
    if (wordsCount)
    {
        Char** wordsStorage=static_cast<Char**>(new_malloc_zero(sizeof(Char*)*wordsCount));
        if (wordsStorage)
        {
            IterateWordsList(responseBegin, responseEnd, wordsStorage);
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

static Err ProcessMessageResponse(AppContext* appContext, const Char* responseBegin, const Char* responseEnd)
{
    Err error=errNone;
//    const Char* messageBegin=StrFind(responseBegin, responseEnd, "\n")+1;
    const Char* messageBegin=responseBegin+7;
    if (messageBegin<responseEnd)
    {
        ExtensibleBuffer buffer;
        ebufInit(&buffer, 0);
        while (messageBegin<responseEnd)
        {
            const Char* messageEnd=StrFind(messageBegin, responseEnd, "\n");
            if (messageBegin<messageEnd)
            {
                ebufAddStrN(&buffer, const_cast<Char*>(messageBegin), messageEnd-messageBegin);
                ebufAddChar(&buffer, '\n');
            }
            if (messageEnd<responseEnd)
                messageBegin=messageEnd+1;
            else 
                messageBegin=responseEnd;
        }                
        ebufWrapBigLines(&buffer);
        ebufSwap(&appContext->lastMessage, &buffer);
        diSetRawTxt(appContext->currDispInfo, ebufGetDataPointer(&appContext->lastMessage));
        ebufFreeData(&buffer);
    }
    else 
        error=appErrMalformedResponse;
    return error;
}

static Err ProcessCookieResponse(AppContext* appContext, const Char* responseBegin, const Char* responseEnd)
{
    Err error=errNone;
    const Char* cookieBegin=responseBegin+cookieResponseLen;

    Assert(cookieBegin<=responseEnd);
    
    if (cookieBegin<responseEnd)
    {
        const Char* cookieEnd=StrFind(cookieBegin, responseEnd, "\n");
        UInt16 cookieLength=cookieEnd-cookieBegin;
        if (cookieLength>0 && cookieLength<=MAX_COOKIE_LENGTH)
        {
            MemMove(appContext->prefs.cookie, cookieBegin, cookieLength);
            appContext->prefs.cookie[cookieLength]=chrNull;
        }
        else 
            error=appErrMalformedResponse;
    }
    else 
        error=appErrMalformedResponse;
    return error;
}

Err ProcessResponse(AppContext* appContext, const char* word, const char* begin, const char* end, ResponseParsingResult& result)
{
    Assert(word);
    Assert(begin);
    Assert(end);
    Err error=errNone;
    if (StrStartsWith(begin, end, cookieResponse))
    {
        error=ProcessCookieResponse(appContext, begin, end);
        if (!error)
            result=responseCookie;
    }
    else if (StrStartsWith(begin, end, wordsListResponse))
    {
        error=ProcessWordsListResponse(appContext, begin, end);
        if (!error)
            result=responseWordsList;
    }
    else if (StrStartsWith(begin, end, messageResponse))
    {
        error=ProcessMessageResponse(appContext, begin, end);
        if (!error)
            result=responseMessage;
    }
    else if (StrStartsWith(begin, end, errorMessageResponse))
    {
        error=ProcessMessageResponse(appContext, begin, end);
        if (!error)
            result=responseErrorMessage;
    }
    else if (StrStartsWith(begin, end, definitionResponse))
    {
        error=ProcessDefinitionResponse(appContext, word, begin, end);
        if (!error)
            result=responseDefinition;
    } 
    else
        error=appErrMalformedResponse;
        
    if (appErrMalformedResponse==error)
        FrmAlert(alertMalformedResponse);
    else if (!error)
    {
        if (ebufGetDataPointer(&appContext->lastResponse)!=begin)
        {
            ebufReset(&appContext->lastResponse);
            ebufAddStrN(&appContext->lastResponse, const_cast<Char*>(begin), end-begin);
        }
    }
    return error;           
}
