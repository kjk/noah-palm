/*
  Copyright (C) 2000-2004 Krzysztof Kowalczyk
  Owner: Krzysztof Kowalczyk

  Helper for parsing HTTP response.
*/

#include "http_response.h"

/* Parse HTTP response line which is of format:
   HTTP_PROTOCOL_NAME NUMERIC_RESPONSE_CODE TXT_RESPONSE_CODE 
   Return NUMERIC_RESPONSE_CODE or -1 in case of parsing error.
   line must be zero-terminated string. */
static int ParseResponseLine(char *line)
{
    char *  numericCodeStart;

    while ( ('\0' != *line) && ( ' ' != *line) )
        ++line;

    if ( '\0' == *line )
    {
        // reached end of string without findint NUMERIC_RESPONSE_CODE => bad
        return -1;
    }

    ++line;
    numericCodeStart = line;
    
    while ( ('\0' != *line) && ( ' ' != *line) )
        ++line;

    if ( '\0' == *line )
    {
        // not sure but I assume TXT_RESPONSE_CODE is required and we didn't
        // find it => bad
        return -1;
    }

    Int32 responseCode;

    if ( errNone != StrAToIEx(numericCodeStart, line, &responseCode, 10 ) )
        return -1;

    // TODO: should switch to UInt32 for all those lengths in the future
    // for now just assert
    Assert( responseCode < 32000 );
    return (int)responseCode;
}

class HttpResponseIterator
{
protected:
    char *  _data;
    int     _dataSize;
    char *  _currData;
    int     _dataLeft;
    ExtensibleBuffer _currLineData;
public:
    HttpResponseIterator(char *data, int dataSize);
    ~HttpResponseIterator();
    void    reset();
    HTTPErr getNextHttpLine(char **line, int *lineSize);
    char *  getRest(int *restSize);
};

void HttpResponseIterator::reset()
{
    _currData = _data;
    _dataLeft = _dataSize;
    ebufFreeData( &_currLineData );
}

HttpResponseIterator::HttpResponseIterator(char *data, int dataSize)
{
    Assert(data);
    Assert(dataSize>0);
    _data = data;
    _dataSize = dataSize;
    _currData = _data;
    _dataLeft = _dataSize;
    ebufInit(&_currLineData, 0);
}

HttpResponseIterator::~HttpResponseIterator()
{
    ebufFreeData( &_currLineData );
}

/* Return next line from HTTP response. Lines are divided HTTP_LINE_ENDING.
   Strips HTTP_LINE_ENDING, terminates the string with zero.
   Client cannot modify the buffer and it's only valid until
   calling getNextHttpLine() again. */
HTTPErr HttpResponseIterator::getNextHttpLine(char **line, int *lineSize)
{
    char *  lineStart = _currData;
    int     currLineSize;

    while (_dataLeft>0)
    {
        if ( '\r' == _currData[0] )
        {
            if (_dataLeft<2)
                return HTTPErr_ParsingError;

            if ( '\n' == _currData[1] )
            {
                // found the end of current line. Return it
                currLineSize = (int) (_currData-lineStart);
                _dataLeft -= 2;
                _currData += 2;
                ebufFreeData(&_currLineData);
                ebufInitWithStrN(&_currLineData, lineStart, currLineSize);
                // TODO: make sure that the string ebuf is always null-terminated
                // so I don't need this
                ebufAddChar(&_currLineData, '\0');
                *line = ebufGetDataPointer(&_currLineData);
                Assert( *line );
                Assert( StrLen(*line) == currLineSize );
                *lineSize = currLineSize;
                return HTTPErr_OK;
            }
            else
            {
                // can it happen per HTTP spec?
                Assert( false );
            }
        }
        ++_currData;
        --_dataLeft;
    }
    Assert( false );
    return HTTPErr_ParsingError;
}

/* Returns a remaining of the text */
char *HttpResponseIterator::getRest(int *restSize)
{
    *restSize = _dataLeft;
    return _currData;
}

HttpResponse::HttpResponse(char *data, int dataSize)
{
    Assert(data);
    Assert(dataSize>0);

    _data              = data;
    _dataSize          = dataSize;
    _headersCount      = 0;
    _fResponseParsed   = false;
    _responseCode      = -1;
    _headersKeysValues = NULL;
    ebufInit(&_chunkedBody, 0);
}

HttpResponse::~HttpResponse()
{
    int     i;

    if ( _headersKeysValues )
    {
        for (i=0; i<_headersCount*2; i++)
        {
            if (_headersKeysValues[i])
                new_free(_headersKeysValues[i]);
        }
        new_free(_headersKeysValues);
    }
    ebufFreeData(&_chunkedBody);
}

/* Parse HTTP header in form "key: value", allocate memory for both
   key (without the ending ":") and value, put them in keyOut,
   valOut. Caller needs to free that memory with new_free */
static HTTPErr SplitHttpHeader(char *hdr, char **keyOut, char **valueOut )
{
    char    *colonPos = hdr;

    while ( (*colonPos!=':') && (*colonPos!='\0') )
        ++colonPos;

    if ( '\0' == *colonPos )
        return HTTPErr_InvalidHttpHeader;

    int keyLen = (colonPos-hdr);
    char *key = (char*) new_malloc_zero(keyLen+1);
    if ( NULL == key )
        return HTTPErr_NoMem;

    StrNCopy(key,hdr,keyLen);

    char * valueStart = colonPos + 1;
    // There's always a space after colon, so skip it,
    // error if it's not there
    if ( ' ' != *valueStart )
        return HTTPErr_InvalidHttpHeader;
    else
        ++valueStart;

    char * valueEnd = valueStart;
    while ( '\0' != *valueEnd )
        ++valueEnd;

    int valueLen = (int) (valueEnd - valueStart);

    char *value = (char*)new_malloc_zero(valueLen+1);
    if ( NULL == value )
    {
        new_free( key );
        return HTTPErr_NoMem;
    }

    StrNCopy(value,valueStart,valueLen);
    *keyOut = key;
    *valueOut = value;
    return HTTPErr_OK;
}

static Err ParseChunkHeader(const Char* headerBegin, const Char* headerEnd, UInt16* chunkSize)
{
    Err error=errNone;
    const Char* end=StrFind(headerBegin, headerEnd, " ");
    Int32 size=0;
    error=StrAToIEx(headerBegin, end, &size, 16);
    if (error)
    {
        error=appErrMalformedResponse;
        goto OnError;
    }
    Assert(size>=0);
    *chunkSize=size;

OnError:
    return error;                
}

static Err ParseChunkedBody(const Char* bodyBegin, const Char* bodyEnd, ExtensibleBuffer* buffer)
{
    Err error=errNone;
    const Char* lineBegin=bodyBegin;
    UInt16 chunkSize=0;
    do 
    {
        const Char* lineEnd=StrFind(lineBegin, bodyEnd, HTTP_LINE_ENDING);
        if (lineBegin<bodyEnd && lineEnd<bodyEnd && lineBegin<lineEnd)
        {
            const Char* chunkBegin=lineEnd+2;
            const Char* chunkEnd=NULL;
            error=ParseChunkHeader(lineBegin, lineEnd, &chunkSize);
            if (error)
                break;                
            chunkEnd=chunkBegin+chunkSize;
            if (chunkBegin<bodyEnd && chunkEnd<bodyEnd)
            {
                ebufAddStrN(buffer, (char*)chunkBegin, chunkSize);
                lineBegin=chunkEnd+2;             
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
    } while (chunkSize);
OnError:    
    return error;
}

/* parse HTTP response i.e. extract response code and headers */
HTTPErr HttpResponse::parseResponse()
{
    HTTPErr     err = HTTPErr_OK;
    char *      currLine = NULL;
    int         currLineSize;
    HttpResponseIterator   txtParse(_data,_dataSize);

    Assert( !_fResponseParsed );

    // first line is a response code with the format:
    err = txtParse.getNextHttpLine(&currLine, &currLineSize);
    if (HTTPErr_OK != err)
        goto OnError;
    Assert( NULL != currLine );
    _responseCode = ParseResponseLine(currLine);
    if ( -1 == _responseCode )
    {
        err = HTTPErr_ParsingError;
        goto OnError;
    }

    // count the number of headers
    _headersCount = 0;
    while (true)
    {
        err = txtParse.getNextHttpLine(&currLine, &currLineSize);
        if (HTTPErr_OK != err)
            goto OnError;
        // an empty line marks the end of headers
        if (0==currLineSize)
            break;
        ++_headersCount;
    }

    if ( 0 == _headersCount )
    {
        // this can't be, can it?
        err = HTTPErr_OK;
        goto OnError;
    }

    // allocate memory for headers and parse them
    _headersKeysValues = (char**) new_malloc_zero( sizeof(char*) * _headersCount * 2 );
    if ( NULL == _headersKeysValues )
    {
        err = HTTPErr_NoMem;
        goto OnError;
    }

    // must re-do parsing
    txtParse.reset();
    // get status line again
    err = txtParse.getNextHttpLine(&currLine,&currLineSize);
    Assert( HTTPErr_OK == err );
    // parse & remember headers
    for( int i=0; i<_headersCount; i++ )
    {
        err = txtParse.getNextHttpLine(&currLine, &currLineSize);
        Assert( HTTPErr_OK == err );
        err = SplitHttpHeader(currLine, &(_headersKeysValues[2*i]), &(_headersKeysValues[2*i+1]));
        if (HTTPErr_OK != err)
            goto OnError;
    }
    err = txtParse.getNextHttpLine(&currLine,&currLineSize);
    Assert( HTTPErr_OK == err );
    Assert( 0==currLineSize ); // must be empty line == end of headers

    // so the rest should be just body
    _body = txtParse.getRest(&_bodySize);
    
    _fResponseParsed = true;
    if (fChunked())
    {
        Err error=ParseChunkedBody(_body, _body+_bodySize, &_chunkedBody);
        if (error)
        {
            err=HTTPErr_ParsingError;
            _fResponseParsed = false;
            goto OnError;
        }               
    }
    
OnError:
#ifdef DEBUG
    if (err)
        LogStrN(GetAppContext(), _data, _dataSize);
#endif
    return err;
}

int HttpResponse::getHeadersCount()
{
    Assert(_fResponseParsed);
    return _headersCount;
}

int HttpResponse::getResponseCode()
{
    Assert(_fResponseParsed);
    return _responseCode;
}

char *HttpResponse::getHeaderKey(int i)
{
    Assert(_fResponseParsed);
    Assert((i>=0) && (i<_headersCount));
    return _headersKeysValues[2*i];
}

char *HttpResponse::getHeaderValue(int i)
{
    Assert(_fResponseParsed);
    Assert((i>=0) && (i<_headersCount));
    return _headersKeysValues[2*i+1];
}

char *HttpResponse::getBody(int *bodySize)
{
    Assert(_fResponseParsed);
    Assert(bodySize);
    if ( !fChunked() )
    {
        *bodySize = _bodySize;
        return _body;
    }
    else
    {
        *bodySize = ebufGetDataSize(&_chunkedBody);
        return ebufGetDataPointer(&_chunkedBody);            
    }    
}

/* Given hdrKey for the key part of the HTTP header,
   return its value. NULL if header doesn't exist */
char *HttpResponse::getHeaderValueByKey(char *hdrKey)
{
    char    *key;
    Assert(_fResponseParsed);

    for( int i=0; i<_headersCount; i++ )
    {
        key = getHeaderKey(i);
        if (0 == StrCompare(key,hdrKey) )
        {
            return getHeaderValue(i);
        }
    }
    return NULL;
}

/* return true if this is a chunked response i.e. Transfer-Encoding
   is chunked. */
Boolean HttpResponse::fChunked()
{
    char *  value;
    Assert(_fResponseParsed);

    value = getHeaderValueByKey(HTTP_TRANSFER_ENCODING_HDR);
    if ( NULL == value )
        return false;

    if ( 0 == StrCompare( value, HTTP_TRANSFER_ENCODING_CHUNKED ) )
        return true;
    else
    {
        // can it be anything else?
        return false;
    }
}
