#include "inet_word_lookup.h"

#define connectionProgressDialogTitle   "Looking Up Word"

#define netLibName "Net.lib"

#define strCrLf "\r\n"

static const Int16 percentsNotSupported=-1;

typedef enum ConnectionStage_ {
    stageResolvingAddress,
    stageOpeningConnection,
    stageSendingRequest,
    stageReceivingResponse,
    stageFinished,
} ConnectionStage;

typedef struct ConnectionData_ 
{
    UInt16 netLibRefNum;
    ConnectionStage connectionStage;
    NetIPAddr* serverIpAddress;
    const Char* wordToFind;
    ExtensibleBuffer request;
    Int16 bytesSent;
    ExtensibleBuffer response;
    UInt16 responseStep;
    NetSocketRef socket;
} ConnectionData;

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
        connData->wordToFind=wordToFind;
        connData->serverIpAddress=serverIpAddress;
        if (*serverIpAddress)
            connData->connectionStage++;
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

static Boolean ConnectionProgressCallback(PrgCallbackDataPtr callbackData)
{
    if (!callbackData->error)
    {
        ConnectionData* connData=(ConnectionData*)callbackData->userDataP;
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
    }
    else
    {
        if (callbackData->message)
            StrNCopy(callbackData->textP, "Error: ", callbackData->textLen);
        else            
            StrNCopy(callbackData->textP, "Connection failed", callbackData->textLen);
    }        
    if (callbackData->message)
        StrNCat(callbackData->textP, callbackData->message, callbackData->textLen);
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

static Err ResolveServerAddress(ConnectionData* connData, Int16* percents, const Char** errorMessage)
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
        connData->connectionStage++;
    } 
    else
        *errorMessage="unable to resolve host address";
OnError: 
    if (infoBuf) 
        new_free(infoBuf);
    *percents=percentsNotSupported;        
    return error;
}

static Err OpenConnection(ConnectionData* connData, Int16* percents, const Char** errorMessage)
{
    Err error=errNone;
    Assert(*connData->serverIpAddress);
    connData->socket=NetLibSocketOpen(connData->netLibRefNum, netSocketAddrINET, netSocketTypeStream, netSocketProtoIPTCP, 
        MillisecondsToTicks(socketOpenTimeout), &error);
    if (-1==connData->socket)
    {
        Assert(error);
        *errorMessage="unable to open socket";
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
            *errorMessage="unable to connect socket";
        }
        else 
            connData->connectionStage++;
    }
    *percents=percentsNotSupported;
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
    ebufAddStr(buffer, " HTTP/1.1\r\nHost: ");
    ebufAddStr(buffer, serverAddress);
    ebufAddStr(buffer, "\r\nAccept-Encoding: identity\r\nAccept: text/plain\r\nConnection: close\r\n\r\n");
OnError:        
    return error;    
}

static Err SendRequest(ConnectionData* connData, Int16* percents, const Char** errorMessage)
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
            *errorMessage="not enough memory to prepare request";
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
        *percents=PercentProgress(NULL, connData->bytesSent, totalSize);
        if (connData->bytesSent==totalSize)
        {
            ebufFreeData(&connData->request);
            connData->bytesSent=0;
            connData->connectionStage++;
        } 
    }
    else 
    {
        if (!result)
        {
            Assert(netErrSocketClosedByRemote==error);
            *errorMessage="connection closed by remote host";
        }
        else
        {
            Assert(error);
            *errorMessage="error while sending request";
        }
        ebufFreeData(&connData->request);
        connData->bytesSent=0;
        *percents=percentsNotSupported;
    }    
OnError:
    return error;    
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
        const Char* lineEnd=StrFind(lineBegin, bodyEnd, strCrLf);
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

static Err ParseStatusLine(const Char* lineStart, const Char* lineEnd)
{
    Err error=appErrMalformedResponse;
    const Char* statusBegin=StrFind(lineStart, lineEnd, " ")+1;
    if (statusBegin<lineEnd-3 && 0==StrNCmp(statusBegin, "200", 3))
        error=errNone;
    return error;
}

static Err ParseResponse(ConnectionData* connData)
{
    Err error=errNone;
    const Char* responseBegin=ebufGetDataPointer(&connData->response);
    const Char* responseEnd=responseBegin+ebufGetDataSize(&connData->response);
    const Char* lineBegin=responseBegin;
    const Char* lineEnd=StrFind(lineBegin, responseEnd, strCrLf);
    const Char* bodyBegin=NULL;
    error=ParseStatusLine(lineBegin, lineEnd);
    if (error)
        goto OnError;
    bodyBegin=StrFind(lineEnd+2, responseEnd, strCrLf strCrLf)+4;
    if (bodyBegin<responseEnd)
    {        
        ExtensibleBuffer buffer;
        ebufInit(&buffer, 0);
        error=ParseChunkedBody(bodyBegin, responseEnd, &buffer);
        if (!error)
            ebufSwap(&buffer, &connData->response);
        ebufFreeData(&buffer);
    }
    else 
        error=appErrMalformedResponse;
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

/* Create new MemoPad record with a given header and body of the memo.
   header can be NULL, if bodySize is -1 => calc it from body via StrLen */
static void CreateNewMemoRec(DmOpenRef dbMemo, char *header, char *body, int bodySize)
{
    UInt16      newRecNo;
    MemHandle   newRecHandle;
    void *      newRecData;
    UInt32      newRecSize;
    Char        null = '\0';

    if (-1 == bodySize)
       bodySize = StrLen(body);

    if ( NULL == header )
         header = "";

    newRecSize = StrLen(header) + bodySize + 1;

    newRecHandle = DmNewRecord(dbMemo,&newRecNo,newRecSize);
    if (!newRecHandle)
        return;

    newRecData = MemHandleLock(newRecHandle);

    DmWrite(newRecData,0,header,StrLen(header));
    DmWrite(newRecData,StrLen(header),body,bodySize);
    DmWrite(newRecData,StrLen(header) + bodySize,&null,1);
    MemHandleUnlock(newRecHandle);

    DmReleaseRecord(dbMemo,newRecNo,true);
}

/* Create a new memo with a given memoBody of size of memoBodySize.
   If memoBodySize is -1 => calculate the size via StrLen(memoBody). */
static void CreateNewMemo(char *memoBody, int memoBodySize)
{
    LocalID     id;
    UInt16      cardno;
    UInt32      type,creator;

    DmOpenRef   dbMemo;

    // check all the open database and see if memo is currently open
    dbMemo = DmNextOpenDatabase(NULL);
    while (dbMemo)
    {
        DmOpenDatabaseInfo(dbMemo,&id, NULL,NULL,&cardno,NULL);
        DmDatabaseInfo(cardno,id,
            NULL,NULL,NULL, NULL,NULL,NULL,
            NULL,NULL,NULL, &type,&creator);

        if( ('DATA' == type)  && ('memo' == creator) )
            break;
        dbMemo = DmNextOpenDatabase(dbMemo);
    }

    // we either found memo db, in which case dbTmp points to it, or
    // didn't find it, in which case dbTmp is NULL
    if (NULL == dbMemo)
        dbMemo = DmOpenDatabaseByTypeCreator('DATA','memo',dmModeReadWrite);

    if (NULL != dbMemo)
        CreateNewMemoRec(dbMemo, NULL, memoBody, memoBodySize);

    if (dbMemo)
        DmCloseDatabase(dbMemo);
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

static Err ReceiveResponse(ConnectionData* connData, Int16* percents, const Char** errorMessage)
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
            *percents=PercentProgress(NULL, CalculateDummyProgress(connData->responseStep++), 100);
            ebufAddStrN(&connData->response, buffer, result);
        }
        else
        {
            error=appErrMalformedResponse;
            *errorMessage="server returned malformed response";
            ebufFreeData(&connData->response);
            *percents=percentsNotSupported;
        }            
    }
    else if (!result)
    {
        Assert(!error);
        *percents=100;
        connData->connectionStage++;
        result=NetLibSocketShutdown(connData->netLibRefNum, connData->socket, netSocketDirBoth, MillisecondsToTicks(transmitTimeout), &error);
        Assert( -1 != result ); // will get asked to drop into debugger (so that we can diagnose it) when fails; It's not a big deal, so continue anyway.

#ifdef DEBUG
        // just for debuggin purposes, dump the full connection response to a MemoPad
        // DumpConnectionResponseToMemo(connData);
#endif
        error=ParseResponse(connData);
        if (error)
        {          
            *errorMessage="server returned malformed response";
            ebufFreeData(&connData->response);
            *percents=percentsNotSupported;
        }            
    }
    else 
    {
        Assert(error);
        *errorMessage="error while receiving response";
        ebufFreeData(&connData->response);
        *percents=percentsNotSupported;
    }
    return error;
}

typedef Err (ConnectionStageHandler)(ConnectionData*, Int16*, const Char**);

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

static Err ShowConnectionProgress(ConnectionData* connData)
{
    Char percentBuffer[6];
    const Char* errorMessage=NULL;
    Err error=errNone;
    Boolean notCancelled=true;
    ConnectionStageHandler* handler=NULL;
    Int16 percents=percentsNotSupported;
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
            error=(*handler)(connData, &percents, &errorMessage);
            if (prevStage!=connData->connectionStage)
                percents=percentsNotSupported;
            RenderPercents(percentBuffer, percents);
            PrgUpdateDialog(progress, error, connData->connectionStage, error?errorMessage:percentBuffer, true);
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
