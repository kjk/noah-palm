/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#include "inet_word_lookup.h"
#include "http_response.h"

#define connectionProgressDialogTitle   "Looking Up Word"

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
    Int16 percentsStatus;
    const Char* errorMessage;
    NetIPAddr* serverIpAddress;
    const Char* wordToFind;
    ExtensibleBuffer request;
    Int16 bytesSent;
    ExtensibleBuffer response;
    UInt16 responseStep;
    NetSocketRef socket;
} ConnectionData;

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
    ConnectionData* connData=(ConnectionData*)new_malloc_zero(sizeof(ConnectionData));
    Assert(wordToFind);
    Assert(serverIpAddress);
    if (connData)
    {
        ebufInit(&connData->request, 0); // seems not required, but hell knows
        ebufInit(&connData->response, 0);
        connData->percentsStatus=percentsNotSupported;
        connData->errorMessage="unknown error";
        connData->wordToFind=wordToFind;
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
    new_free(connData);
}

inline static void RenderPercents(Char buffer[], Int16 percents)
{
    if (percentsNotSupported==percents)
        buffer[0]=chrNull;
    else 
    {
        buffer[0]=' ';
        PercentProgress(buffer+1, percents, 100);
    }        
}

static Boolean ConnectionProgressCallback(PrgCallbackDataPtr callbackData)
{
    ConnectionData* connData=(ConnectionData*)callbackData->userDataP;
    if (!callbackData->error)
    {
        const Char* message=NULL;
        UInt16 msgLen=0;
        switch (connData->connectionStage)
        {
            case stageResolvingAddress:
                message="Resolving host adress...";
                break;
            
            case stageOpeningConnection:
                message="Opening connection...";
                break;
                
            case stageSendingRequest:
                message="Sending request...";
                break;
                
            case stageReceivingResponse:
                message="Receiving response...";
                break;
                
            case stageFinished:
                message="Done";
                break;
                
            default:
                Assert(false);
        } 
        msgLen=StrLen(message);
        if (msgLen<callbackData->textLen) 
            StrCopy(callbackData->textP, message);
        else        
        {
            msgLen=callbackData->textLen-4;
            MemMove(callbackData->textP, message, msgLen);
            StrCopy(callbackData->textP+msgLen, "...");
        }      
        if (connData->percentsStatus!=percentsNotSupported)
        {
            RenderPercents(callbackData->message, connData->percentsStatus);
            StrNCat(callbackData->textP, callbackData->message, callbackData->textLen);
        }            
    }
    else
    {
        StrNCopy(callbackData->textP, "Error: ", callbackData->textLen);
        StrNCat(callbackData->textP, connData->errorMessage, callbackData->textLen);
    }        
    return true;
}

static Boolean HandleConnectionProgressEvents(ProgressPtr progress, Boolean waitForever)
{
    EventType event;
    Boolean cancelled=false;
    Int32 timeout=0;
    if (waitForever) timeout=evtWaitForever;
    do {
        EvtGetEvent(&event, timeout);
        if (event.eType!=nilEvent && !PrgHandleEvent(progress, &event))
            cancelled=PrgUserCancel(progress);
        if (waitForever && ctlEnterEvent==event.eType)
            cancelled=true;
    } while (!cancelled && event.eType!=nilEvent);
    return !cancelled;
}

static Err ResolveServerAddress(ConnectionData* connData)
{
    Err error=errNone;
    NetHostInfoBufType* infoBuf=(NetHostInfoBufType*)new_malloc_zero(sizeof(NetHostInfoBufType));
    NetHostInfoType* infoPtr=NULL;
    if (!infoBuf)
    {
        error=memErrNotEnoughSpace;
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
        connData->errorMessage="unable to resolve host address";
OnError: 
    if (infoBuf) 
        new_free(infoBuf);
    connData->percentsStatus=percentsNotSupported;        
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
        connData->errorMessage="unable to open socket";
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
            connData->errorMessage="unable to connect socket";
        }
        else 
            AdvanceConnectionStage(connData);
    }
    connData->percentsStatus=percentsNotSupported;
    return error;
}

static Err PrepareRequest(ConnectionData* connData)
{
    Err error=errNone;
    Char* url=NULL;
    ExtensibleBuffer* buffer=&connData->request;
    ebufResetWithStr(buffer, "GET ");
    url=TxtParamString(serverRelativeURL, connData->wordToFind, NULL, NULL, NULL);
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
            connData->errorMessage="not enough memory to prepare request";
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
        connData->percentsStatus=PercentProgress(NULL, connData->bytesSent, totalSize);
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
            connData->errorMessage="connection closed by remote host";
        }
        else
        {
            Assert(error);
            connData->errorMessage="error while sending request";
        }
        ebufFreeData(&connData->request);
        connData->bytesSent=0;
        connData->percentsStatus=percentsNotSupported;
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

inline static UInt16 CalculateDummyProgress(UInt16 step)
{
    double progress=0.5;
    while (step--) 
        progress/=2;
    progress=1.0-progress;
    return progress*100;
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
    Int16 result=0;
    Char buffer[responseBufferSize];
    result=NetLibReceive(connData->netLibRefNum, connData->socket, buffer, responseBufferSize, 0, NULL, 0, 
        MillisecondsToTicks(transmitTimeout), &error);
    if (result>0) 
    {
        Assert(!error);
        if (ebufGetDataSize(&connData->response)+result<maxResponseLength)
        {
            connData->percentsStatus=PercentProgress(NULL, CalculateDummyProgress(connData->responseStep++), 100);
            ebufAddStrN(&connData->response, buffer, result);
        }
        else
        {
            error=appErrMalformedResponse;
            connData->errorMessage="server returned malformed response";
            ebufFreeData(&connData->response);
        }            
    }
    else if (!result)
    {
        Assert(!error);
        connData->percentsStatus=100;
        AdvanceConnectionStage(connData);
        result=NetLibSocketShutdown(connData->netLibRefNum, connData->socket, netSocketDirBoth, MillisecondsToTicks(transmitTimeout), &error);
        Assert( -1 != result ); // will get asked to drop into debugger (so that we can diagnose it) when fails; It's not a big deal, so continue anyway.

#ifdef DEBUG
        // just for debuggin purposes, dump the full connection response to a MemoPad
        // DumpConnectionResponseToMemo(connData);
#endif
        error=ParseResponse(connData);
        if (error)
        {          
            connData->errorMessage="server returned malformed response";
            ebufFreeData(&connData->response);
        }            
    }
    else 
    {
        Assert(error);
        connData->errorMessage="error while receiving response";
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

static Err ShowConnectionProgress(ConnectionData* connData)
{
    Char percentBuffer[6];
    Err error=errNone;
    Boolean notCancelled=true;
    ConnectionStageHandler* handler=NULL;
    ProgressPtr progress=PrgStartDialog(connectionProgressDialogTitle, ConnectionProgressCallback, connData);
    if (!progress)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    PrgUpdateDialog(progress, error, connData->connectionStage, NULL, true);
    do 
    {
        handler=GetConnectionStageHandler(connData);
        if (handler)
        {        
            ConnectionStage prevStage=connData->connectionStage;
            error=(*handler)(connData);
            if (prevStage!=connData->connectionStage)
                connData->percentsStatus=percentsNotSupported;
            PrgUpdateDialog(progress, error, connData->connectionStage, percentBuffer, true);
            notCancelled=HandleConnectionProgressEvents(progress, error);
        } 
    } while (notCancelled && !error && handler);
    if (!notCancelled)
        error=netErrUserCancel;
OnError:
    if (progress)
        PrgStopDialog(progress, false);
    return error;    
}

Err INetWordLookup(const Char* word, NetIPAddr* serverIpAddress, Char** response, UInt16* responseLength)
{
    Err error=errNone;
    ConnectionData* connData=CreateConnectionData(word, serverIpAddress);
    if (!connData)
    {
        error=memErrNotEnoughSpace;
        FrmAlert(alertMemError);
        goto OnError;
    }
    error=InitializeNetLib(connData);
    if (error)
        goto OnError;
    error=ShowConnectionProgress(connData);
    if (error)
        goto OnError;
    *responseLength=ebufGetDataSize(&connData->response);
    *response=ebufGetTxtOwnership(&connData->response);
OnError:
    if (connData)
        FreeConnectionData(connData);
    return error;
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
