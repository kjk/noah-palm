#ifndef __INET_CONNECTION_H__
#define __INET_CONNECTION_H__

#include "common.h"
#include <NetMgr.h>

typedef enum ConnectionStage_ {
    stageResolvingAddress,
    stageOpeningConnection,
    stageSendingRequest,
    stageReceivingResponse,
    stageFinished,
    stageInvalid
} ConnectionStage;

typedef enum ConnectionProgressEventFlag_ 
{
    connectionStarted,
    connectionProgress,
    connectionFinished,
} ConnectionProgressEventFlag;

typedef struct ConnectionProgressEventData_ 
{
    ConnectionProgressEventFlag flag;
} ConnectionProgressEventData;        

typedef Err (ConnectionStatusRenderer)(void* context, ConnectionStage stage, UInt16 responseLength, ExtensibleBuffer& statusBuffer);
typedef Err (ConnectionResponseProcessor)(AppContext* appContext, void* context, const Char* responseBegin, const Char* responseEnd);
typedef void (ConnectionContextDestructor)(void* context);

extern void StartConnection(AppContext* appContext, void* context, const Char* requestUrl,
                                          ConnectionStatusRenderer* connStatusRenderer,
                                          ConnectionResponseProcessor* connResponseProcessor,
                                          ConnectionContextDestructor* connContextDestructor);

extern void PerformConnectionTask(AppContext* appContext);

extern void AbortCurrentConnection(AppContext* appContext, Boolean sendNotifyEvent);

extern const Char* GetConnectionStatusText(AppContext* appContext);

#define ConnectionInProgress(appContext) (NULL!=(appContext)->currentConnectionData)


#endif
