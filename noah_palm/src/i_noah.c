/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#include "common.h"
#include "five_way_nav.h"
#include "main_form.h"
#include "preferences_form.h"
#include "inet_connection.h"
#include "bookmarks.h"
#include "words_list_form.h"

#ifdef DEBUG
// This is a full HTTP response we get on device when asking for word "glib"
#pragma warn_a5_access off
unsigned char deviceGlibResponse[] = 
    "HTTP/1.0 200 OK\xd" "\xa" 
    "Date: Thu, 25 Dec 2003 10:44:23 GMT\xd" "\xa" 
    "Server: Apache/1.3.22 (Unix)  (Red-Hat/Linux) mod_ssl/2.8.4 OpenSSL/0.9.6b DAV/1.0.2 PHP/4.1.2 mod_perl/1.24_01\xd" "\xa" 
    "X-Powered-By: PHP/4.1.2\xd" "\xa" 
    "Content-Type: text/html\xd" "\xa" 
    "Connection: close\xd" "\xa" 
    "\xd" "\xa" 
    "!glib\xd" "\xa" 
    "!glib-tongued\xd" "\xa" 
    "!smooth-tongued\xd" "\xa" 
    "$s\xd" "\xa" 
    "@artfully persuasive in speech\xd" "\xa" 
    "#a glib tongue\xd" "\xa" 
    "#a smooth-tongued hypocrite\xd" "\xa" 
    "!glib\xd" "\xa" 
    "!pat\xd" "\xa" 
    "!slick\xd" "\xa" 
    "$s\xd" "\xa" 
    "@having only superficial plausibility\xd" "\xa" 
    "#glib promises\xd" "\xa" 
    "#a slick commercial\xd" "\xa" 
    "!glib\xd" "\xa" 
    "$s\xd" "\xa" 
    "@marked by lack of intellectual depth\xd" "\xa" 
    "#glib generalizations\xd" "\xa" 
    "#a glib response to a complex question\xd" "\xa" 
    "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\x0";

#endif

static const UInt32 ourMinVersion = sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0);
static const UInt32 kPalmOS20Version = sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0);

// Create a blob containing serialized preferences.
// Caller needs to free returned memory
static void *SerializePreferencesInoah(AppContext* appContext, long *pBlobSize)
{
    AppPrefs *      prefs;
    long            blobSize;
    char *          prefsBlob;
    UInt32 *        prefRecordId;

    Assert(appContext);
    Assert(pBlobSize);

    prefs = &appContext->prefs;
    blobSize = sizeof(UInt32) + sizeof(AppPrefs);
    prefsBlob = (char*)new_malloc(blobSize);
    if (NULL == prefsBlob)
        return NULL;
    prefRecordId = (UInt32*)prefsBlob;
    *prefRecordId = AppPrefId;
    MemMove(prefsBlob+sizeof(UInt32), (char*)prefs, sizeof(AppPrefs));
    *pBlobSize = blobSize;
    return prefsBlob;
}


// Given a blob containing serialized prefrences deserilize the blob
// and set the preferences accordingly.
static void DeserializePreferencesInoah(AppContext* appContext, unsigned char *prefsBlob, long blobSize)
{
    AppPrefs * prefs;

    Assert(prefsBlob);
    Assert(blobSize > 8);

    prefs = &appContext->prefs;
    /* skip the 4-byte signature of the preferences record */
    Assert( IsValidPrefRecord(prefsBlob) );
    prefsBlob += 4;
    blobSize -= 4;

    Assert( blobSize == sizeof(AppPrefs) );

    MemMove((char*)prefs,prefsBlob, blobSize);
}

static void LoadPreferencesInoah(AppContext* appContext)
{
    DmOpenRef    db;
    UInt         recNo;
    void *       recData;
    MemHandle    recHandle;
    UInt         recsCount;

    db = DmOpenDatabaseByTypeCreator(APP_PREF_TYPE, APP_CREATOR, dmModeReadWrite);
    if (!db) return;
    recsCount = DmNumRecords(db);
    for (recNo = 0; recNo < recsCount; recNo++)
    {
        recHandle = DmQueryRecord(db, recNo);
        recData = MemHandleLock(recHandle);
        if ( (MemHandleSize(recHandle)>=PREF_REC_MIN_SIZE) && IsValidPrefRecord(recData) )
        {
            DeserializePreferencesInoah(appContext, (unsigned char*)recData, MemHandleSize(recHandle) );
            MemHandleUnlock(recHandle);
            break;
        }
        MemHandleUnlock(recHandle);
    }
    DmCloseDatabase(db);

}

static void SavePreferencesInoah(AppContext* appContext)
{
    DmOpenRef      db;
    UInt           recNo;
    UInt           recsCount;
    Boolean        fRecFound = false;
    Err            err;
    void *         recData;
    long           recSize;
    MemHandle      recHandle;
    void *         prefsBlob;
    long           blobSize;
    Boolean        fRecordBusy = false;

    prefsBlob = SerializePreferencesInoah(appContext, &blobSize );
    if ( NULL == prefsBlob ) return;

    db = DmOpenDatabaseByTypeCreator(APP_PREF_TYPE, APP_CREATOR, dmModeReadWrite);
    if (!db)
    {
        err = DmCreateDatabase(0, "iNoah Prefs", APP_CREATOR,  APP_PREF_TYPE, false);
        if ( errNone != err)
            return;

        db = DmOpenDatabaseByTypeCreator(APP_PREF_TYPE, APP_CREATOR, dmModeReadWrite);
        if (!db)
            return;
    }
    recNo = 0;
    recsCount = DmNumRecords(db);
    while (recNo < recsCount)
    {
        recHandle = DmGetRecord(db, recNo);
        fRecordBusy = true;
        recData = MemHandleLock(recHandle);
        recSize = MemHandleSize(recHandle);
        if (IsValidPrefRecord(recData))
        {
            fRecFound = true;
            break;
        }
        MemPtrUnlock(recData);
        DmReleaseRecord(db, recNo, true);
        fRecordBusy = false;
        ++recNo;
    }

    if (fRecFound && blobSize>recSize)
    {
        /* need to resize the record */
        MemPtrUnlock(recData);
        DmReleaseRecord(db,recNo,true);
        fRecordBusy = false;
        recHandle = DmResizeRecord(db, recNo, blobSize);
        if ( NULL == recHandle )
            return;
        recData = MemHandleLock(recHandle);
        Assert( MemHandleSize(recHandle) == blobSize );        
    }

    if (!fRecFound)
    {
        recNo = 0;
        recHandle = DmNewRecord(db, &recNo, blobSize);
        if (!recHandle)
            goto CloseDbExit;
        recData = MemHandleLock(recHandle);
        fRecordBusy = true;
    }

    DmWrite(recData, 0, prefsBlob, blobSize);
    MemPtrUnlock(recData);
    if (fRecordBusy)
        DmReleaseRecord(db, recNo, true);
CloseDbExit:    
    DmCloseDatabase(db);
    new_free( prefsBlob );
}

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
    UInt32 romVersion;
    Err error=FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    Assert(!error);
    if (romVersion < requiredVersion)
    {
        if ((launchFlags & 
            (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
            (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
        {
            FrmAlert (alertRomIncompatible);
            if (romVersion < kPalmOS20Version)
                AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
        }
        error=sysErrRomIncompatible;
    }
    return error;
}

static Err AppInit(AppContext* appContext)
{
    Err     error=errNone;
    UInt32  value=0;

    MemSet(appContext, sizeof(*appContext), 0);
    error=FtrSet(APP_CREATOR, appFtrContext, (UInt32)appContext);
    if (error) 
        goto OnError;
    error=FtrSet(APP_CREATOR, appFtrLeaksFile, 0);
    if (error) 
        goto OnError;
    
    LogInit( appContext, "c:\\iNoah_log.txt" );
    InitFiveWay(appContext);
    
    ebufInit(&appContext->currentDefinition, 0);
    ebufInit(&appContext->currentWordBuf, 0);
    ebufInit(&appContext->lastResponse, 0);
    ebufInit(&appContext->lastMessage, 0);
    
    appContext->currBookmarkDbType = bkmInvalid; // has anybody idea why it isn't 0?

    appContext->prefs.startupAction      = startupActionNone;
    appContext->prefs.hwButtonScrollType = scrollPage;
    appContext->prefs.navButtonScrollType = scrollPage;
    appContext->prefs.bookmarksSortType  = bkmSortByTime;

    appContext->prefs.displayPrefs.listStyle = 2;
    SetDefaultDisplayParam(&appContext->prefs.displayPrefs, false, false);

    appContext->currDispInfo=diNew();
    if (!appContext->currDispInfo)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }

    LoadPreferencesInoah(appContext);

    SyncScreenSize(appContext);
    
// define _DONT_DO_HANDLE_DYNAMIC_INPUT_ to disable Pen Input Manager operations
#ifndef _DONT_DO_HANDLE_DYNAMIC_INPUT_
    error=DIA_Init(&appContext->diaSettings);
    if (error) 
        goto OnError;
#endif  
    
OnError:
    return error;
}

static void AppDispose(AppContext* appContext)
{
    Err error=errNone;
    if (ebufGetDataSize(&appContext->currentWordBuf))
    {
        StrNCopy(appContext->prefs.lastWord, ebufGetDataPointer(&appContext->currentWordBuf), WORD_MAX_LEN-1);
        appContext->prefs.lastWord[WORD_MAX_LEN-1]=chrNull;
    }
    SavePreferencesInoah(appContext);
    FrmCloseAllForms();
    error=DIA_Free(&appContext->diaSettings);
    Assert(!error);
    
    if (ConnectionInProgress(appContext))
        AbortCurrentConnection(appContext, false);
    
    if (appContext->currDispInfo)
    {
        diFree(appContext->currDispInfo);
        appContext->currDispInfo=NULL;
    }
    ebufFreeData(&appContext->currentWordBuf);
    ebufFreeData(&appContext->currentDefinition);
    ebufFreeData(&appContext->lastResponse);
    ebufFreeData(&appContext->lastMessage);
    
    if (appContext->wordsList)
    {
        while (appContext->wordsInListCount)
        {
            Char* word=appContext->wordsList[--appContext->wordsInListCount];
            if (word)
                new_free(word);
        }
        new_free(appContext->wordsList);
        appContext->wordsList=NULL;
    }
}

static void AppLoadForm(AppContext* appContext, const EventType* event)
{
    Err error=errNone;
    switch (event->data.frmLoad.formID)
    {
        case formDictMain:
            error=MainFormLoad(appContext);
            break;
            
        case formPrefs:
            error=PreferencesFormLoad(appContext);
            break;
            
        case formDisplayPrefs:
            error=DisplayPrefsFormLoad(appContext);
            break;

        case formBookmarks:
            error=BookmarksFormLoad(appContext);
            break;
            
        case formWordsList:
            error=WordsListFormLoad(appContext);
            break;
        
        default:
            Assert(false);
    }
    Assert(!error);
}

static Boolean AppHandleEvent(AppContext* appContext, EventType* event)
{
    Boolean handled=false;
    switch (event->eType)
    {
        case frmLoadEvent:
            AppLoadForm(appContext, event);
            handled=true;
            break;
    }
    return handled;
}

static void AppEventLoop(AppContext* appContext)
{
    Err error=errNone;
    EventType event;
    do 
    {
        Int32 timeout=evtWaitForever;
        if (ConnectionInProgress(appContext)) 
            timeout=0;
        EvtGetEvent(&event, timeout);
        if (appStopEvent!=event.eType)
        {
            if (nilEvent==event.eType && ConnectionInProgress(appContext))
                PerformConnectionTask(appContext);
            else 
            {                
                if (!SysHandleEvent(&event))
                    if (!MenuHandleEvent(0, &event, &error))
                        if (!AppHandleEvent(appContext, &event))
                            FrmDispatchEvent(&event);
            }
        }                                        
    } while (appStopEvent!=event.eType);
}

static Err AppLaunch() 
{
    Err error=errNone;
    AppContext* appContext=(AppContext*)MemPtrNew(sizeof(AppContext));
    if (!appContext)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }     
    error=AppInit(appContext);
    if (error) 
        goto OnError;
    FrmGotoForm(formDictMain);
#if 0
    testParseResponse((char*)deviceGlibResponse);
#endif

    AppEventLoop(appContext);
    AppDispose(appContext);
OnError: 
    if (appContext) 
        MemPtrFree(appContext);
    return error;
}


UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    Err error=RomVersionCompatible (ourMinVersion, launchFlags);
    if (!error) {
        switch (cmd)
        {
            case sysAppLaunchCmdNormalLaunch:
                error=AppLaunch();
                break;
                
            case sysAppLaunchCmdNotify:
                error=AppHandleSysNotify((SysNotifyParamType*)cmdPBP);
                break;
        }
    }       
    return error;
}

