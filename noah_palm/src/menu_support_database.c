#include "menu_support_database.h"

#define supportDatabaseType 'TEMP'
#define bitmapResourceType 'Tbmp'


static Err OpenSupportDatabase(DmOpenRef* dbRef)
{
    Err         error;
    DmOpenRef   db=DmOpenDatabaseByTypeCreator(supportDatabaseType, APP_CREATOR, dmModeReadWrite|dmModeLeaveOpen);

    if (!db)
    {
        error=DmGetLastErr();
        Assert(error);
        return error;
    }

    if (dbRef) 
        *dbRef=db;
    error=FtrSet(APP_CREATOR, appFtrMenuSupportDbRefNum, (UInt32)db);

OnError:
    return error;
}

static Err CreateSupportDatabase(const char* name, UInt16 bitmapId)
{
    DmOpenRef   db=0;
    MemHandle   trgBmp;
    UInt32      bmpSize;
    Err         error;
    MemPtr      srcBmpPtr;
    MemPtr      trgBmpPtr;
    Err         nError;
    MemHandle   srcBmp;

    error=DmCreateDatabase(0, name, APP_CREATOR, supportDatabaseType, true);
    if (error)
        goto OnError;

    srcBmp=DmGet1Resource(bitmapResourceType, bitmapId);
    if (!srcBmp) 
    {
        error=DmGetLastErr();
        Assert(error);
        goto OnError;
    }
    bmpSize=MemHandleSize(srcBmp);
    Assert(bmpSize);

    error=OpenSupportDatabase(&db);
    if (error) 
        goto OnErrorReleaseBmp;
    Assert(db);
    
    trgBmp=DmNewResource(db, bitmapResourceType, bitmapId, bmpSize);
    if (!trgBmp) 
    {
        error=DmGetLastErr();
        Assert(error);
        goto OnErrorReleaseBmp;
    }
    
    srcBmpPtr=MemHandleLock(srcBmp);
    Assert(srcBmpPtr);
    
    trgBmpPtr=MemHandleLock(trgBmp);
    Assert(trgBmpPtr);
    
    error=DmWrite(trgBmpPtr, 0, srcBmpPtr, bmpSize);
    nError=MemHandleUnlock(trgBmp);
    Assert(!nError);
    nError=DmReleaseResource(trgBmp);
    Assert(!nError);
    nError=MemHandleUnlock(srcBmp);
    Assert(!nError);
OnErrorReleaseBmp:
    nError=DmReleaseResource(srcBmp);
    Assert(!nError);
OnError:
    if (dmErrAlreadyExists==error) 
        error=errNone;
    return error;
}

Err PrepareSupportDatabase(const char* name, UInt16 bitmapId) 
{
    UInt32  value;
    Err     error;
    LocalID localId=DmFindDatabase(0, name);

    if (localId) 
    {
        error=FtrGet(APP_CREATOR, appFtrMenuSupportDbRefNum, &value);
        if (ftrErrNoSuchFeature==error || (!error && !value)) 
            error=OpenSupportDatabase(NULL);
    }
    else
        error=CreateSupportDatabase(name, bitmapId);
    return error;
}

Err DisposeSupportDatabase(const char* name) 
{
    Err         error;
    UInt32      value=0;
    DmOpenRef   db=0;
    LocalID     localId=0;

    error=FtrGet(APP_CREATOR, appFtrMenuSupportDbRefNum, &value);
    if (errNone!=error)
        return error;

    if (value) 
    {
        db=(DmOpenRef)value;
        error=DmCloseDatabase(db);
        if (error) 
            goto OnError;
        localId=DmFindDatabase(0, name);
        Assert(localId);
        error=DmDeleteDatabase(0, localId);
    }
    else 
    {
        localId=DmFindDatabase(0, name);
        if (localId) 
            error=DmDeleteDatabase(0, localId);
    }
OnError:
    return error;
}
