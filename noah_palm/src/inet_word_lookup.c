#include "inet_word_lookup.h"

#define connectionProgressDialogTitle   "Looking Up Word..."

#define netLibName "Net.lib"

typedef enum ConnectionStage_ {
    stageResolvingAddress,
    stageOpeningConnection,
    stageSendingRequest,
    stageReceivingResponse,
} ConnectionStage;

typedef struct ConnectionData_ 
{
    UInt16 netLibRefNum;
    ConnectionStage connectionStage;
    NetIPAddr* serverIpAddress;
    const Char* wordToFind;
    Char* request;
    Int16 bytesToSend;
    Int16 bytesSent;
    Char* response;
    Int16 bytesReceived;    
    NetSocketRef socket;
} ConnectionData;

inline static void CloseSocket(ConnectionData* connData)
{
    Int16 status=0;
    Err error=errNone;
    Assert(connData->socket);    status=NetLibSocketClose(connData->netLibRefNum, connData->socket, evtWaitForever, &error);
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
    if (connData->socket)        CloseSocket(connData);
    if (connData->netLibRefNum)
        DisposeNetLib(connData);
    if (connData->request)
    {
        new_free(connData->request);
        connData->request=NULL;
    }
    if (connData->response)
    {
        new_free(connData->response);
        connData->response=NULL;
    }
    new_free(connData);
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
    infoPtr=NetLibGetHostByName(connData->netLibRefNum, serverAddress, infoBuf, addressResolveTimeout, &error);
    if (infoPtr && !error)
    {
        Assert(netSocketAddrINET==infoPtr->addrType);
        Assert(infoBuf->address[0]);
        *connData->serverIpAddress=NetHToNL(infoBuf->address[0]);
    }    
OnError: 
    if (infoBuf) 
        new_free(infoBuf);
    return error;
}

static Boolean ConnectionProgressCallback(PrgCallbackDataPtr callbackData)
{
    ConnectionData* connData=(ConnectionData*)callbackData->userDataP;
    const Char* message=NULL;
    UInt16 msgLen=0;
    switch (connData->connectionStage)
    {
        case stageResolvingAddress:
            message="Resolving host adress";
            break;
        
        case stageOpeningConnection:
            message="Opening connection";
            break;
            
        case stageSendingRequest:
            message="Sending request";
            break;
            
        case stageReceivingResponse:
            message="Receiving response";
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
        MemMove(callbackData->textP, message, msgLen);        StrCopy(callbackData->textP+msgLen, "...");
    }        
    return true;
}

static Boolean HandleConnectionProgressEvents(ProgressPtr progress)
{
    EventType event;
    Boolean notCancelled=true;
    do {
        EvtGetEvent(&event, 0);
        if (event.eType!=nilEvent && !PrgHandleEvent(progress, &event))
            notCancelled=!PrgUserCancel(progress);
    } while (notCancelled && event.eType!=nilEvent);
    return notCancelled;
}

static Err ShowConnectionProgress(ConnectionData* connData)
{
    Err error=errNone;
    Boolean notCancelled=true;
    ProgressPtr progress=PrgStartDialog(connectionProgressDialogTitle, ConnectionProgressCallback, connData);
    if (!progress)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    PrgUpdateDialog(progress, error, 0, "start", false);
    notCancelled=HandleConnectionProgressEvents(progress);
    if (notCancelled)
    {
        if (stageResolvingAddress==connData->connectionStage)
        {
            error=ResolveServerAddress(connData);
            PrgUpdateDialog(progress, error, 0, "resolve", false);            notCancelled=HandleConnectionProgressEvents(progress);
        }
    }        
OnError:
    if (progress)
        PrgStopDialog(progress, false);    return error;    
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
OnError:
    if (connData)
        FreeConnectionData(connData);
    return error;
}
