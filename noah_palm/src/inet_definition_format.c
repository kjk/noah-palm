#include "inet_definition_format.h"

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
                if (!FormatWantsWord(appContext) || !(wordLen==synLen && 0==StrNCmp(synBegin, word, synLen)))
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
                if (0==StrNCmp(defEnd-2, "\r\n", 2))
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
                            if (0==StrNCmp(defBlock-2, "\r\n", 2))
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

Err PrepareDisplayInfo(AppContext* appContext, const Char* word, const Char* responseBegin, const Char* responseEnd)
{
    Err error=errNone;
    ExtensibleBuffer buffer;
    ebufInit(&buffer, 0);
    error=ConvertInetToDisplayableFormat(appContext, word, responseBegin, responseEnd, &buffer);
    if (!error)
    {
        if (!appContext->currDispInfo)
            appContext->currDispInfo=diNew();
        if (appContext->currDispInfo)
        {
            ebufAddChar(&buffer, chrNull);
            ebufWrapBigLines(&buffer);
            ebufSwap(&buffer, &appContext->currentDefinition);
            diSetRawTxt(appContext->currDispInfo, ebufGetDataPointer(&appContext->currentDefinition));
            ebufResetWithStr(&appContext->currentWordBuf, (Char*)word);
            ebufAddChar(&appContext->currentWordBuf, chrNull);
        }
        else
        {
            FrmAlert(alertMemError);
            error=memErrNotEnoughSpace;            
        }            
    }
    else
        FrmAlert(alertMalformedResponse);
    ebufFreeData(&buffer);
    return error;
}

