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
#define registrationFailedResponse     "REGISTRATION_FAILED\n"
#define registrationFailedResponseLen  sizeof(registrationFailedResponse)-1
#define registrationOkResponse     "REGISTRATION_OK\n"
#define registrationOkResponseLen  sizeof(registrationOkResponse)-1

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
#if 0
        /* unfortunately this can be called as a result of redisplay-last-word
           which might happen after we enter registration code but before we
           get the next word with reg code */
        // shouldn't get this if we're registered
        Assert(0==StrLen(appContext->prefs.regCode)); 
#endif
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

static void UpdateHistoryButtons(AppContext *appContext)
{

    UInt16  index;
    void*   object;

    FormType *form = FrmGetActiveForm();
    index = FrmGetObjectIndex(form, backButton);
    Assert(frmInvalidObjectId!=index);
    object = FrmGetObjectPtr(form, index);

    if (FHistoryCanGoBack(appContext))
    {
        CtlSetEnabled((ControlType*)object, true);
        CtlSetGraphics((ControlType*)object, backBitmap, NULL);
    }
    else
    {
        CtlSetEnabled((ControlType*)object, false);
        CtlSetGraphics((ControlType*)object, backDisabledBitmap, NULL);
    }

    index = FrmGetObjectIndex(form, forwardButton);
    Assert(frmInvalidObjectId!=index);
    object = FrmGetObjectPtr(form, index);

    if (FHistoryCanGoForward(appContext))
    {
        CtlSetEnabled((ControlType*)object, true);
        CtlSetGraphics((ControlType*)object, forwardBitmap, NULL);
    }
    else
    {
        CtlSetEnabled((ControlType*)object, false);
        CtlSetGraphics((ControlType*)object, forwardDisabledBitmap, NULL);
    }
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

    if (errNone!=error)
        return error;

    if (NULL==word)
        return errNone;
    Assert( errNone == error );

    FldClearInsert(FrmGetFormPtr(formDictMain), fieldWordInput, word);

    // TODO: need to filter out random word requests (use a flag on appContext?)
    AddWordToHistory(appContext, word);
    UpdateHistoryButtons(appContext);
    return error;
}

static Int16 StrSortComparator(void* str1, void* str2, Int32)
{
    const char* s1=*(const char**)str1;
    const char* s2=*(const char**)str2;
    return StrCompare(s1, s2);
}

static const char *ExtractLine(const char **txt, int *txtLenLeft, int *lineLenOut)
{
    const char *result = *txt;

    Assert(txt);
    Assert(*txt);

    if (0==*txtLenLeft)
    {
        *lineLenOut=0;
        return result;
    }

    const char *curPos = *txt;
    int         curLenLeft = *txtLenLeft;
    while( (*curPos != '\n') && (curLenLeft>0) )
    {
        ++curPos;
        --curLenLeft;
    }
    *lineLenOut = curPos - *txt;
    if (*curPos=='\n')
    {
        ++curPos;
        Assert(curLenLeft>0);
        --curLenLeft;
    }
    *txt = curPos;
    *txtLenLeft = curLenLeft;

    return result;
}

#ifdef DEBUG
extern void testExtractLine();
void testExtractLine()
{
    const char *txt, *txtOrig;
    int         txtLenLeft;
    const char *extractedLine;
    int         lineLen;

    txt = ""; txtOrig = txt;
    txtLenLeft = StrLen(txt);
    extractedLine = ExtractLine(&txt,&txtLenLeft,&lineLen);
    Assert(extractedLine==txtOrig);
    Assert(txtLenLeft==0);

    txt = "\n"; txtOrig = txt;
    txtLenLeft = StrLen(txt);
    extractedLine = ExtractLine(&txt,&txtLenLeft,&lineLen);
    Assert( (extractedLine==txtOrig) && (0==lineLen) && (0==txtLenLeft));
    extractedLine = ExtractLine(&txt,&txtLenLeft,&lineLen);
    Assert((0==txtLenLeft) && (0==lineLen));

    txt = "me\nhim\n"; txtOrig = txt;
    txtLenLeft = StrLen(txt);
    extractedLine = ExtractLine(&txt,&txtLenLeft,&lineLen);
    Assert( (extractedLine==txtOrig) && (2==lineLen) && (4==txtLenLeft));
    extractedLine = ExtractLine(&txt,&txtLenLeft,&lineLen);
    Assert( (extractedLine == (txtOrig+3)) && (3==lineLen) && (0==txtLenLeft));
    extractedLine = ExtractLine(&txt,&txtLenLeft,&lineLen);
    Assert((0==txtLenLeft) && (0==lineLen));
}
#endif

/* Given a list of words, one word per line, extract those words and return
as char ** array. List of words must end by an empty line (and there must be
nothing after that empty line). The list might be empty, in which case we return
NULL and set wordsCount to 0. We might also return NULL when there is an error
parsing the response, in which case wordsCount is set to -1. */
static char **ExtractWordsFromListResponse(const char*txt, int txtLen, int *wordsCountOut)
{
    int          wordsCount=0;
    const char * txtTmp;
    const char * word;
    char **      words;
    int          txtLenLeft;
    int          phase;
    int          lineLen;
    int          wordNo;

    Assert(txt);
    Assert(txtLen>0);
    Assert(wordsCountOut);

    // phase==0 - calculating number of words
    // pahse==1 - extracting the words
    for (phase=0; phase<=1; phase++)
    {
        txtTmp = txt;
        txtLenLeft = txtLen;
        wordNo = 0;
        // TODO: handle out of mem
        if (1==phase)
            words=static_cast<char**>(new_malloc_zero(sizeof(char*)*wordsCount));

        while (true)
        {
            word=ExtractLine(&txtTmp,&txtLenLeft,&lineLen);
            // we skip an empty line
            // TODO: we should check, that this is the last line
            if (0==lineLen)
            {
                if (0==txtLenLeft)
                    break;
                continue;
            }
            if (0==phase)
                ++wordsCount;
            else
            {
                words[wordNo] = static_cast<char*>(new_malloc_zero(sizeof(char)*(lineLen+1)));
                StrNCopy(words[wordNo], word, lineLen);
                ++wordNo;
            }                
        }
    }
    Assert(wordNo==wordsCount);
    *wordsCountOut = wordsCount;
    return words;
}

static Err ProcessWordsListResponse(AppContext* appContext, const char* responseBegin, const char* responseEnd)
{
    char **      wordsList;
    int          wordsCount;
    const char * wordsListStart;
    int          wordsListLen;

    Assert(!appContext->wordsList);

    wordsListStart = responseBegin + wordsListResponseLen;
    wordsListLen = responseEnd - wordsListStart;
    Assert( wordsListLen>=0 );

    wordsList = ExtractWordsFromListResponse(wordsListStart,wordsListLen,&wordsCount);

    if (NULL==wordsList)
    {
        if (-1==wordsCount)
            return appErrMalformedResponse;
        appContext->wordsList=NULL;
        appContext->wordsInListCount=0;
        return errNone;
    }

    SysQSort(wordsList, wordsCount, sizeof(const char*), StrSortComparator, NULL);
    appContext->wordsList=wordsList;
    appContext->wordsInListCount=wordsCount;
    return errNone;
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
    // slightly laxed parsing - we don't check if there's anything after that
    // (since it shouldn't). but that's ok
    else if ((responseRegistrationFailed & resMask) && StrStartsWith(begin, end, registrationFailedResponse))
    {
        SendEvtWithType(evtRegistrationFailed);
    }
    // slightly laxed parsing - we don't check if there's anything after that
    // (since it shouldn't). but that's ok
    else if ((responseRegistrationOk & resMask) && StrStartsWith(begin, end, registrationOkResponse))
    {
        SendEvtWithType(evtRegistrationOk);
    }
    else
        error=appErrMalformedResponse;

    if (error)
    {
        if (appErrMalformedResponse==error)
        {
            // we used to do just FrmAlert() here but this is not healthy and leaves
            // the form in a broken state (objects not re-drawn, text field disabled
            // instead post a message telling form event handling code to show the alert
            SendEvtWithType(evtShowMalformedAlert);
#ifdef DEBUG
            DumpStrToMemo(begin,end);
#endif
        }
        return error;
    }

    if ( (responseMessage==result) || (responseErrorMessage==result) || (responseDefinition==result) )
    {
        if (ebufGetDataPointer(&appContext->lastResponse)!=begin)
        {
            ebufReset(&appContext->lastResponse);
            ebufAddStrN(&appContext->lastResponse, const_cast<char*>(begin), end-begin);
        }
    }
    return errNone;           
}
