/*
  Copyright (C) 2000-2004 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyi
*/

#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include "common.h"

#define HTTP_GET_METHOD "GET"
#define HTTP_VERSION "HTTP/1.0"
#define HTTP_LINE_ENDING "\r\n"
#define HTTP_HOST_HDR "Host"
#define HTTP_CONTENT_LENGTH_HDR "Content-Length"
#define HTTP_CONTENT_TYPE_HDR "Content-Type"
#define HTTP_TRANSFER_ENCODING_HDR "Transfer-Encoding"
#define HTTP_TRANSFER_ENCODING_CHUNKED "chunked"

typedef enum HTTPErr_ {
    HTTPErr_OK = 0,
    HTTPErr_InvalidHttpHeader,
    HTTPErr_ParsingError,
    HTTPErr_NoMem,
} HTTPErr;

class HttpResponse
{
private:
    char *   _data;
    int      _dataSize;
    int      _headersCount;
    Boolean  _fResponseParsed;
    int      _responseCode;
    // [2*i] keeps a key, [2*i+1] keeps value
    char **  _headersKeysValues;
    char *   _body;
    int      _bodySize;

public:
    HttpResponse(char *data, int dataSize);
    ~HttpResponse();
    HTTPErr parseResponse();
    int     getResponseCode();
    int     getHeadersCount();
    char *  getHeaderKey(int i);
    char *  getHeaderValue(int i);
    char *  getHeaderValueByKey(char *hdrKey);
    char *  getBody(int *bodySize);
    Boolean fChunked();
};

#endif
