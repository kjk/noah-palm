#include "inet_connection.h"
#include "http_response.h"

#define serverAddress                "www.arslexis.com"
#define serverPort                      80
#define maxResponseLength        8192               // reasonable limit so malicious response won't use all available memory
#define responseBufferSize          256                 // size of single chunk used to retrieve server response
#define addressResolveTimeout   20000              // timeouts in milliseconds
#define socketOpenTimeout         1000
#define socketConnectTimeout     10000
#define transmitTimeout              5000

#define netLibName "Net.lib"

typedef struct ConnectionData_ 
{
    UInt16 netLibRefNum;
    ConnectionStage connectionStage;
    ExtensibleBuffer statusText;
    NetIPAddr* serverIpAddress;
    void* context;
    ConnectionStatusRenderer* statusRenderer;
    ConnectionResponseProcessor* responseProcessor;
    ConnectionContextDestructor* contextDestructor;
    ExtensibleBuffer request;
    Int16 bytesSent;
    ExtensibleBuffer response;
    NetSocketRef socket;
} ConnectionData;

static void SendConnectionProgressEvent(ConnectionProgressEventFlag flag)
{
    EventType event;
    MemSet(&event, sizeof(event), 0);
    event.eType=static_cast<eventsEnum>(connectionProgressEvent);
    Assert(sizeof(ConnectionProgressEventData)<=sizeof(event.data));
    ConnectionProgressEventData* data=reinterpret_cast<ConnectionProgressEventData*>(&event.data);
    data->flag=flag;
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

const Char* GetConnectionStatusText(AppContext* appContext)
{
    Assert(appContext);
    ConnectionData* connData=static_cast<ConnectionData*>(appContext->currentConnectionData);
    Assert(connData);
    ebufReset(&connData->statusText);
    if (connData->statusRenderer)
        (*connData->statusRenderer)(connData->context, connData->connectionStage, ebufGetDataSize(&connData->response), connData->statusText);
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

inline static ConnectionData* CreateConnectionData(void* context, NetIPAddr* serverIpAddress, const Char* requestUrl,
                                          ConnectionStatusRenderer* connStatusRenderer,
                                          ConnectionResponseProcessor* connResponseProcessor,
                                          ConnectionContextDestructor* connContextDestructor) 
{
    ConnectionData* connData=static_cast<ConnectionData*>(new_malloc_zero(sizeof(ConnectionData)));
    Assert(serverIpAddress);
    if (connData)
    {
        ebufInitWithStr(&connData->request, "GET ");
        ebufAddStr(&connData->request, const_cast<Char*>(requestUrl));
        ebufAddStr(&connData->request, " HTTP/1.0\r\nHost: ");
        ebufAddStr(&connData->request, serverAddress);
        ebufAddStr(&connData->request, "\r\nAccept-Encoding: identity\r\nAccept: text/plain\r\nConnection: close\r\n\r\n");
        
        ebufInit(&connData->response, 0);
        ebufInit(&connData->statusText, 0);
        connData->context=context;
        connData->serverIpAddress=serverIpAddress;
        connData->statusRenderer=connStatusRenderer;
        connData->responseProcessor=connResponseProcessor;
        connData->contextDestructor=connContextDestructor;
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
    if (connData->contextDestructor && connData->context)
        (*connData->contextDestructor)(connData->context);
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

static Err SendRequest(ConnectionData* connData)
{
    Err error=errNone;
    UInt16 totalSize=ebufGetDataSize(&connData->request);
    const Char* request=ebufGetDataPointer(&connData->request)+connData->bytesSent;
    UInt16 requestLeft=totalSize-connData->bytesSent;
    Int16 result=NetLibSend(connData->netLibRefNum, connData->socket, const_cast<Char*>(request), requestLeft, 0, NULL, 0, 
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

void AbortCurrentConnection(AppContext* appContext, Boolean sendNotifyEvent)
{
    Assert(appContext);
    ConnectionData* connData=static_cast<ConnectionData*>(appContext->currentConnectionData);
    Assert(connData);
    appContext->currentConnectionData=NULL;
    FreeConnectionData(connData);
    if (sendNotifyEvent)
        SendConnectionProgressEvent(connectionFinished);
}

void StartConnection(AppContext* appContext, void* context, const Char* requestText,
                                          ConnectionStatusRenderer* connStatusRenderer,
                                          ConnectionResponseProcessor* connResponseProcessor,
                                          ConnectionContextDestructor* connContextDestructor)
{
    ConnectionData* connData=CreateConnectionData(context, &appContext->serverIpAddress, requestText, 
        connStatusRenderer, connResponseProcessor, connContextDestructor);
    if (!connData)
    {
        FrmAlert(alertMemError);
        return;
    }
    Boolean wasInProgress=ConnectionInProgress(appContext);
    if (wasInProgress)
        AbortCurrentConnection(appContext, false);
    Err error=InitializeNetLib(connData);
    if (!error)
    {
        appContext->currentConnectionData=connData;
        SendConnectionProgressEvent(wasInProgress?connectionProgress:connectionStarted);
    }            
    else 
    {
        FreeConnectionData(connData);
        if (wasInProgress)
            SendConnectionProgressEvent(connectionFinished);
    }
    
}                                          


void PerformConnectionTask(AppContext* appContext)
{
    Assert(appContext);
    ConnectionData* connData=static_cast<ConnectionData*>(appContext->currentConnectionData);
    Assert(connData);
    ConnectionStageHandler* handler=GetConnectionStageHandler(connData);
    if (handler)
    {
        Err error=(*handler)(connData);
        if (error)
            AbortCurrentConnection(appContext, true);
        else
            SendConnectionProgressEvent(connectionProgress);
    }
    else
    {
/*
        const Char* begin=deviceMessageResponse;
        const Char* end=begin+StrLen(begin);
*/        
        const Char* begin=ebufGetDataPointer(&connData->response);
        const Char* end=begin+ebufGetDataSize(&connData->response);
        if (connData->responseProcessor)
            (*connData->responseProcessor)(appContext, connData->context, begin, end);
        AbortCurrentConnection(appContext, true);
    }
}
