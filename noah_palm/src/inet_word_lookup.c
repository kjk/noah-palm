/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#include "inet_word_lookup.h"
#include "inet_definition_format.h"
#include "http_response.h"

#define netLibName "Net.lib"

static const Int16 percentsNotSupported=-1;

typedef enum ConnectionStage_ {
    stageResolvingAddress,
    stageOpeningConnection,
    stageSendingRequest,
    stageReceivingResponse,
    stageFinished,
    stageInvalid
} ConnectionStage;

typedef struct ConnectionData_ 
{
    UInt16 netLibRefNum;
    ConnectionStage connectionStage;
    ExtensibleBuffer statusText;
    NetIPAddr* serverIpAddress;
    ExtensibleBuffer wordToFind;
    ExtensibleBuffer request;
    Int16 bytesSent;
    ExtensibleBuffer response;
    NetSocketRef socket;
} ConnectionData;

static void SendLookupProgressEvent(LookupProgressEventFlag flag, Err error=errNone)
{
    EventType event;
    MemSet(&event, sizeof(event), 0);
    event.eType=static_cast<eventsEnum>(lookupProgressEvent);
    Assert(sizeof(LookupProgressEventData)<=sizeof(event.data));
    LookupProgressEventData* data=reinterpret_cast<LookupProgressEventData*>(&event.data);
    data->flag=flag;
    data->error=error;
    EvtAddEventToQueue(&event);
}

static void AdvanceConnectionStage(ConnectionData* connData)
{
    ConnectionStage newStage = stageInvalid;
    switch( connData->connectionStage )
    {
        case stageResolvingAddress:
            newStage = stageOpeningConnection;
            break;
        case stageOpeningConnection:
            newStage = stageSendingRequest;
            break;
        case stageSendingRequest:
            newStage = stageReceivingResponse;
            break;
       case stageReceivingResponse:
            newStage = stageFinished;
            break;
       default:
            Assert( false );
            break;
    }
    connData->connectionStage = newStage;
}

static void RenderStatusText(ConnectionData* connData, const Char* baseText)
{
    Assert(connData);
    Char* text=TxtParamString(baseText, ebufGetDataPointer(&connData->wordToFind), NULL, NULL, NULL);
    if (text)
    {
        ebufResetWithStr(&connData->statusText, text);
        MemPtrFree(text);
        if (stageReceivingResponse==connData->connectionStage)
        {
            static const UInt16 bytesBufferSize=28; // it will be placed in code segment, so no worry about globals
            UInt16 bytesReceived=ebufGetDataSize(&connData->response);
            Char buffer[bytesBufferSize];
            Int16 bytesLen=0;
            if (bytesReceived<1024)
                bytesLen=StrPrintF(buffer, " %d bytes", bytesReceived);
            else
            {
                UInt16 bri=bytesReceived/1024;
                UInt16 brf=((bytesReceived%1024)+51)/102; // round to 1/10
                Char formatString[9];
                StrCopy(formatString, " %d.%d kB");
                NumberFormatType numFormat=static_cast<NumberFormatType>(PrefGetPreference(prefNumberFormat));
                Char dontCare;                LocGetNumberSeparators(numFormat, &dontCare, formatString+3); // change decimal separator in place
                bytesLen=StrPrintF(buffer, formatString, bri, brf);                
            }
            Assert(bytesLen<bytesBufferSize);
            if (bytesLen>0)
                ebufAddStrN(&connData->statusText, buffer, bytesLen);
        }
        ebufAddChar(&connData->statusText, chrNull);
    }
}

const Char* GetLookupStatusText(AppContext* appContext)
{
    Assert(appContext);
    ConnectionData* connData=static_cast<ConnectionData*>(appContext->currentLookupData);
    Assert(connData);
    switch (connData->connectionStage) 
    {
        case stageResolvingAddress:
            RenderStatusText(connData, "Resolving host address");
            break;
        case stageOpeningConnection:
            RenderStatusText(connData, "Opening connection");
            break;
        case stageSendingRequest:
            RenderStatusText(connData, "Requesting: ^0");
            break;                        
        case stageReceivingResponse:
            RenderStatusText(connData, "Retrieving: ^0");
            break;
        case stageFinished:
            RenderStatusText(connData, "Finished");
            break;
        default:
            Assert(false);
    }
    return ebufGetDataPointer(&connData->statusText);
}

inline static void CloseSocket(ConnectionData* connData)
{
    Int16 status=0;
    Err error=errNone;
    Assert(connData->socket);
    status=NetLibSocketClose(connData->netLibRefNum, connData->socket, evtWaitForever, &error);
    Assert(status==0 && !error);
    connData->socket=0;
}

inline static void DisposeNetLib(ConnectionData* connData) 
{
    Err error=errNone;
    Assert(connData->netLibRefNum);
    error=NetLibClose(connData->netLibRefNum, false);
    Assert(!error);
    connData->netLibRefNum=0;
}

static Err InitializeNetLib(ConnectionData* connData)
{
    Err error=errNone;
    Assert(connData);
    error=SysLibFind(netLibName, &connData->netLibRefNum);
    if (!error)
    {
        Err ifError=errNone;
        UInt16 openCount=0;
        NetLibOpenCount(connData->netLibRefNum, &openCount);
        error=NetLibOpen(connData->netLibRefNum, &ifError);
        if (netErrAlreadyOpen==error)
            error=errNone;
        if (!error && ifError)
        {
            UInt16 newOpenCount=0;
            NetLibOpenCount(connData->netLibRefNum, &newOpenCount);
            if (newOpenCount>openCount)
                DisposeNetLib(connData);
            error=ifError;                
        }            
        if (error)
        {
            connData->netLibRefNum=0;
            FrmAlert(alertUnableToInitializeNetLibrary);
        }           
    }
    else
        FrmAlert(alertNetworkLibraryNotFound);
    return error;
}

inline static ConnectionData* CreateConnectionData(const Char* wordToFind, NetIPAddr* serverIpAddress) 
{
    ConnectionData* connData=static_cast<ConnectionData*>(new_malloc_zero(sizeof(ConnectionData)));
    Assert(wordToFind);
    Assert(serverIpAddress);
    if (connData)
    {
        ebufInit(&connData->request, 0); // seems not required, but hell knows
        ebufInit(&connData->response, 0);
        ebufInitWithStr(&connData->wordToFind, const_cast<Char*>(wordToFind));
        ebufAddChar(&connData->wordToFind, chrNull);
        ebufInit(&connData->statusText, 0);
        connData->serverIpAddress=serverIpAddress;
        if (*serverIpAddress)
            AdvanceConnectionStage(connData);
    }
    return connData;
}   

static void FreeConnectionData(ConnectionData* connData)
{
    Assert(connData);
    if (connData->socket)
        CloseSocket(connData);
    if (connData->netLibRefNum)
        DisposeNetLib(connData);
    ebufFreeData(&connData->request);
    ebufFreeData(&connData->response);
    ebufFreeData(&connData->statusText);
    ebufFreeData(&connData->wordToFind);
    new_free(connData);
}

static Err ResolveServerAddress(ConnectionData* connData)
{
    Err error=errNone;
    NetHostInfoBufType* infoBuf=static_cast<NetHostInfoBufType*>(new_malloc_zero(sizeof(NetHostInfoBufType)));
    NetHostInfoType* infoPtr=NULL;
    if (!infoBuf)
    {
        error=memErrNotEnoughSpace;
        FrmAlert(alertMemError);
        goto OnError;
    }      
    Assert(0==*connData->serverIpAddress);
    infoPtr=NetLibGetHostByName(connData->netLibRefNum, serverAddress, infoBuf, MillisecondsToTicks(addressResolveTimeout), &error);
    if (infoPtr && !error)
    {
        Assert(netSocketAddrINET==infoPtr->addrType);
        Assert(infoBuf->address[0]);
        *connData->serverIpAddress=infoBuf->address[0];
        AdvanceConnectionStage(connData);
    } 
    else
        FrmCustomAlert(alertCustomError, "Unable to resolve host address.", NULL, NULL);
OnError: 
    if (infoBuf) 
        new_free(infoBuf);
    return error;
}

static Err OpenConnection(ConnectionData* connData)
{
    Err error=errNone;
    Assert(*connData->serverIpAddress);
    connData->socket=NetLibSocketOpen(connData->netLibRefNum, netSocketAddrINET, netSocketTypeStream, netSocketProtoIPTCP, 
        MillisecondsToTicks(socketOpenTimeout), &error);
    if (-1==connData->socket)
    {
        Assert(error);
        FrmCustomAlert(alertCustomError, "Unable to open socket.", NULL, NULL);
    }
    else
    {
        Int16 result=0;
        NetSocketAddrINType address;
        address.family=netSocketAddrINET;
        address.port=NetHToNS(serverPort);
        address.addr=NetHToNL(*connData->serverIpAddress);
        result=NetLibSocketConnect(connData->netLibRefNum, connData->socket, (NetSocketAddrType*)&address, sizeof(address), 
            MillisecondsToTicks(socketConnectTimeout), &error);
        if (-1==result)
        {
            Assert(error);
            FrmCustomAlert(alertCustomError, "Unable to connect socket.", NULL, NULL);
        }
        else 
            AdvanceConnectionStage(connData);
    }
    return error;
}

static Err PrepareRequest(ConnectionData* connData)
{
    Err error=errNone;
    Char* url=NULL;
    ExtensibleBuffer* buffer=&connData->request;
    ebufResetWithStr(buffer, "GET ");
    url=TxtParamString(serverRelativeURL, ebufGetDataPointer(&connData->wordToFind), NULL, NULL, NULL);
    if (!url)
    {
        error=memErrNotEnoughSpace;
        ebufFreeData(buffer);
        goto OnError;
    }
    ebufAddStr(buffer, url);
    MemPtrFree(url);
    ebufAddStr(buffer, " HTTP/1.0\r\nHost: ");
    ebufAddStr(buffer, serverAddress);
    ebufAddStr(buffer, "\r\nAccept-Encoding: identity\r\nAccept: text/plain\r\nConnection: close\r\n\r\n");
OnError:        
    return error;    
}

static Err SendRequest(ConnectionData* connData)
{
    Err error=errNone;
    Int16 result=0;
    Char* request=NULL;
    UInt16 requestLeft=0;
    UInt16 totalSize=ebufGetDataSize(&connData->request);
    if (!totalSize)
    {
        error=PrepareRequest(connData);
        if (error)
        {
            FrmAlert(alertMemError);
            goto OnError;
        }
        totalSize=ebufGetDataSize(&connData->request);
    }
    request=ebufGetDataPointer(&connData->request)+connData->bytesSent;
    requestLeft=totalSize-connData->bytesSent;
    result=NetLibSend(connData->netLibRefNum, connData->socket, request, requestLeft, 0, NULL, 0, 
        MillisecondsToTicks(transmitTimeout), &error);
    if (result>0)
    {
        Assert(!error);
        connData->bytesSent+=result;
        if (connData->bytesSent==totalSize)
        {
            ebufFreeData(&connData->request);
            connData->bytesSent=0;
            AdvanceConnectionStage(connData);
        } 
    }
    else 
    {
        if (!result)
        {
            Assert(netErrSocketClosedByRemote==error);
            FrmCustomAlert(alertCustomError, "Connection closed by remote host.", NULL, NULL);
        }
        else
        {
            Assert(error);
            FrmCustomAlert(alertCustomError, "Error while sending request.", NULL, NULL);
        }
        ebufFreeData(&connData->request);
        connData->bytesSent=0;
    }    
OnError:
    return error;    
}

static Err ParseResponse(ConnectionData* connData)
{
    Err              error=errNone;
    ExtensibleBuffer bufResponseBody;
    char *           responseBody;
    int              responseBodySize;

    HttpResponse httpResponse(
        ebufGetDataPointer(&connData->response),
        ebufGetDataSize(&connData->response) );

    if ( HTTPErr_OK != httpResponse.parseResponse() )
    {
        error = appErrMalformedResponse;
        goto OnError;
    }

    if ( 200 != httpResponse.getResponseCode() )
    {
        error = appErrMalformedResponse;
        goto OnError;
    }

    responseBody = httpResponse.getBody(&responseBodySize);
    Assert( NULL != responseBody );

    ebufInitWithStrN(&bufResponseBody, responseBody, responseBodySize);
    ebufSwap(&bufResponseBody, &connData->response);

    ebufFreeData(&bufResponseBody);
OnError:        
    return error;
    
}

static void DumpConnectionResponseToMemo(ConnectionData * connData)
{
    ExtensibleBuffer *  ebufResp;
    char *              data;
    int                 dataSize;
    
    ebufResp = &connData->response;
    data     = ebufGetDataPointer( ebufResp );
    dataSize = ebufGetDataSize( ebufResp );
    Assert( dataSize > 0 );
    CreateNewMemo( data, dataSize );
}

static Err ReceiveResponse(ConnectionData* connData)
{
    Err error=errNone;
    Int16 bytesReceived=0;
    Char buffer[responseBufferSize];
    bytesReceived=NetLibReceive(connData->netLibRefNum, connData->socket, buffer, responseBufferSize, 0, NULL, 0, 
        MillisecondsToTicks(transmitTimeout), &error);
    if (bytesReceived>0) 
    {
        Assert(!error);
        if (ebufGetDataSize(&connData->response)+bytesReceived<maxResponseLength)
            ebufAddStrN(&connData->response, buffer, bytesReceived);
        else
        {
            error=appErrMalformedResponse;
            FrmAlert(alertMalformedResponse);
            ebufFreeData(&connData->response);
        }            
    }
    else if (!bytesReceived)
    {
        Assert(!error);
        AdvanceConnectionStage(connData);
        Int16 result=NetLibSocketShutdown(connData->netLibRefNum, connData->socket, netSocketDirBoth, MillisecondsToTicks(transmitTimeout), &error);
        Assert( -1 != result ); // will get asked to drop into debugger (so that we can diagnose it) when fails; It's not a big deal, so continue anyway.

#ifdef DEBUG
        // just for debuggin purposes, dump the full connection response to a MemoPad
        // DumpConnectionResponseToMemo(connData);
#endif
        error=ParseResponse(connData);
        if (error)
        {          
            FrmAlert(alertMalformedResponse);
            ebufFreeData(&connData->response);
        }            
    }
    else 
    {
        Assert(error);
        FrmCustomAlert(alertCustomError, "Error while receiving response.", NULL, NULL);
        ebufFreeData(&connData->response);
    }
    return error;
}

typedef Err (ConnectionStageHandler)(ConnectionData*);

static ConnectionStageHandler* GetConnectionStageHandler(const ConnectionData* connData)
{
    ConnectionStageHandler* handler=NULL;
    switch (connData->connectionStage)
    {
        case stageResolvingAddress:
            handler=ResolveServerAddress;
            break;
            
        case stageOpeningConnection:
            handler=OpenConnection;
            break;           
            
        case stageSendingRequest:
            handler=SendRequest;
            break;
            
        case stageReceivingResponse:
            handler=ReceiveResponse;
            break;
            
        case stageFinished:
            break;            
            
        default:
            Assert(false);
    }
    return handler;
}

void AbortCurrentLookup(AppContext* appContext, Boolean sendNotifyEvent, Err error)
{
    Assert(appContext);
    ConnectionData* connData=static_cast<ConnectionData*>(appContext->currentLookupData);
    Assert(connData);
    appContext->currentLookupData=NULL;
    FreeConnectionData(connData);
    if (sendNotifyEvent)
        SendLookupProgressEvent(lookupFinished, error);
}

void StartLookup(AppContext* appContext, const Char* wordToFind)
{
    ConnectionData* connData=CreateConnectionData(wordToFind, &appContext->serverIpAddress);
    if (!connData)
    {
        FrmAlert(alertMemError);
        return;
    }
    Boolean wasInProgress=LookupInProgress(appContext);
    if (wasInProgress)
        AbortCurrentLookup(appContext, false);
    Err error=InitializeNetLib(connData);
    if (!error)
    {
        appContext->currentLookupData=connData;
        SendLookupProgressEvent(wasInProgress?lookupProgress:lookupStarted);
    }            
    else 
    {
        FreeConnectionData(connData);
        if (wasInProgress)
            SendLookupProgressEvent(lookupFinished, netErrUserCancel);
    }
}

void PerformLookupTask(AppContext* appContext)
{
    Assert(appContext);
    ConnectionData* connData=static_cast<ConnectionData*>(appContext->currentLookupData);
    Assert(connData);
    ConnectionStageHandler* handler=GetConnectionStageHandler(connData);
    if (handler)
    {
        Err error=(*handler)(connData);
        if (error)
            AbortCurrentLookup(appContext, true, error);
        else
            SendLookupProgressEvent(lookupProgress);
    }
    else
    {
        const Char* begin=ebufGetDataPointer(&connData->response);
        const Char* end=begin+ebufGetDataSize(&connData->response);
        Err error=errNone;
        if (0==StrNCmp(begin, noDefnitionResponse, end-begin))
        {
            error=appErrWordNotFound;
            const Char* word = ebufGetDataPointer(&connData->wordToFind);
            FrmCustomAlert(alertWordNotFound, word, NULL, NULL);
        }
        else
        {
            error=PrepareDisplayInfo(appContext, ebufGetDataPointer(&connData->wordToFind), begin, end);
            if (!error)
                ebufSwap(&connData->response, &appContext->lastResponse);
        }            
        AbortCurrentLookup(appContext, true, error);
    }
}

#ifdef DEBUG
void testParseResponse(char *txt)
{
    Err              err;
    ConnectionData   connData;

    ebufInitWithStr(&connData.response, txt);

    err = ParseResponse(&connData);
    Assert( errNone == err );
}
#endif
