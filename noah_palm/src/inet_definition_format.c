#include "inet_definition_format.h"
#include "blowfish.h"

#define noDefnitionResponse "NO DEFINITION"
#define wordsListResponse    "WORD_LIST"
#define messageResponse     "MESSAGE"

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
            const Char* synEnd=StrFind(synBegin, end, "\r\n!");
            begin=synEnd+2;
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
                if (StrStartsWith(defEnd-2, defEnd, "\r\n"))
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
        const Char* partBlock=StrFind(begin, end, "\r\n$");
        if (partBlock<end)
        {
            const Char* partOfSpeech=(partBlock+=2)+1;
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
                            if (StrStartsWith(defBlock-2, defBlock, "\r\n"))
                                break;
                            else                    
                                defBlock=StrFindOneOf(defBlock+1, end, "@#");
                        }
                        if (defBlock<end)
                        {
                            begin=StrFind(defBlock, end, "\r\n!");
                            if (begin<end) begin+=2;
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

Err ProcessOneWordResponse(AppContext* appContext, const Char* word, const Char* responseBegin, const Char* responseEnd)
{
    Err error=errNone;
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
    return error;
}

static UInt16 IterateWordsList(const Char* responseBegin, const Char* responseEnd, Char** targetList=NULL)
{
    UInt16 wordsCount=0;
    const Char* wordBegin=StrFind(responseBegin, responseEnd, "\r\n")+2;
    while (wordBegin<responseEnd)
    {
        const Char* wordEnd=StrFind(wordBegin, responseEnd, "\r\n");
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
            wordBegin=wordEnd+2;
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
    const Char* messageBegin=StrFind(responseBegin, responseEnd, "\r\n")+2;
    if (messageBegin<responseEnd)
    {
        ExtensibleBuffer buffer;
        ebufInit(&buffer, 0);
        while (messageBegin<responseEnd)
        {
            const Char* messageEnd=StrFind(messageBegin, responseEnd, "\r\n");
            if (messageBegin<messageEnd)
            {
                ebufAddStrN(&buffer, const_cast<Char*>(messageBegin), messageEnd-messageBegin);
                ebufAddChar(&buffer, '\n');
            }
            if (messageEnd<responseEnd)
                messageBegin=messageEnd+2;
            else 
                messageBegin=responseEnd;
        }                
        ebufWrapBigLines(&buffer);
        ebufSwap(&appContext->lastMessage, &buffer);
        diSetRawTxt(appContext->currDispInfo, ebufGetDataPointer(&appContext->lastMessage));
    }
    else 
        error=appErrMalformedResponse;
    return error;
}

static Err DecipherResponse(AppContext* appContext, const Char* begin, const Char* end, ExtensibleBuffer& out)
{
    Err error=errNone;
    UInt16 length=end-begin;
    if (length % 8)
        error=appErrMalformedResponse;
    else
    {
        ExtensibleBuffer key;
        ebufInit(&key, 0);
        error=GetAuthorizationKey(appContext, &key);
        if (!error)
        {
            BlowfishCipher* bf=new BlowfishCipher(reinterpret_cast<const UInt8*>(ebufGetDataPointer(&key)), ebufGetDataSize(&key));
            if (bf)
            {
                ebufAddStrN(&out, const_cast<Char*>(begin), end-begin);
                bf->decrypt(reinterpret_cast<UInt8*>(ebufGetDataPointer(&out)), ebufGetDataSize(&out));
                delete bf;
            }
            else 
                error=memErrNotEnoughSpace;
        }
        ebufFreeData(&key);
    }
    return error;
}

ResponseParsingResult ProcessResponse(AppContext* appContext, const Char* word, const Char* begin, const Char* end, Boolean usedAuthentication)
{
    Assert(word);
    Assert(begin);
    Assert(end);
    ResponseParsingResult result=responseError;
    Err error=errNone;
    ExtensibleBuffer decipheredResponse;
    ebufInit(&decipheredResponse, 0);
    if (StrStartsWith(begin, end, noDefnitionResponse))
        result=responseWordNotFound;
    else if (StrStartsWith(begin, end, wordsListResponse))
    {
        error=ProcessWordsListResponse(appContext, begin, end);
        result=responseWordsList;
    }
    else if (StrStartsWith(begin, end, messageResponse))
    {
        error=ProcessMessageResponse(appContext, begin, end);
        result=responseMessage;
    }
    else
    {
        if (usedAuthentication)
        {
            error=DecipherResponse(appContext, begin, end, decipheredResponse);
            if (!error)
            {
                begin=ebufGetDataPointer(&decipheredResponse);
                end=begin+ebufGetDataSize(&decipheredResponse);
            }
        }
        if (!error)
        {
            error=ProcessOneWordResponse(appContext, word, begin, end);
            result=responseOneWord;
        }
    } 
    if (!error)
    {
        if (ebufGetDataPointer(&appContext->lastResponse)!=begin)
        {
            ebufReset(&appContext->lastResponse);
            ebufAddStrN(&appContext->lastResponse, const_cast<Char*>(begin), end-begin);
        }
    }
    else
    {
        if (appErrMalformedResponse==error)
            result=responseMalformed;
        else if (appErrBadAuthorization==error)
            result=responseUnauthorised;
        else 
            result=responseError;            
    }
    ebufFreeData(&decipheredResponse);
    return result;           
}