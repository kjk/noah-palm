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

#if 0
/* format of preferences database for 0.5 version. We need that for migration
   code */
typedef struct
{
    StartupAction           startupAction;
    ScrollType              hwButtonScrollType;
    ScrollType              navButtonScrollType;
    char                    lastWord[WORD_MAX_LEN];
    DisplayPrefs            displayPrefs;

    // how do we sort bookmarks
    BookmarkSortType        bookmarksSortType;

    char                    cookie[MAX_COOKIE_LENGTH+1];
    Boolean                 fShowPronunciation;
} iNoah05Prefs;
#endif

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

#include "PrefsStore.hpp"

#define PREFS_DB_NAME "iNoah Prefs"

#define p_startupAction         0
#define p_hwButtonScrolType     1
#define p_navButtonScrollType   2
#define p_lastWord              3
#define p_bookmarksSortType     4
#define p_cookie                5
#define p_fShowPronunciation    6

#if 0
    StartupAction           startupAction;
    ScrollType              hwButtonScrollType;
    ScrollType              navButtonScrollType;
    char                    lastWord[WORD_MAX_LEN];
    DisplayPrefs            displayPrefs;

    // how do we sort bookmarks
    BookmarkSortType        bookmarksSortType;

    char                    cookie[MAX_COOKIE_LENGTH+1];
    Boolean                 fShowPronunciation;
    /* run-time switch for pron-related features. It's just so that I can develop
    code for pronunciation and ship it without pronunciation enabled. */
    Boolean                 fEnablePronunciation;
#endif

static void SavePreferencesInoah2(AppContext* appContext)
{
    Err                 err;
    AppPrefs *          prefs = &(appContext->prefs);
    PrefsStoreWriter    prefsStore(PREFS_DB_NAME, APP_CREATOR, APP_PREF_TYPE);

    err = prefsStore.ErrSetInt(p_startupAction, (int)prefs->startupAction );
    Assert( errNone != err );
    err = prefsStore.ErrSetInt(p_hwButtonScrolType, (int)prefs->hwButtonScrollType);
    Assert( errNone != err );
    err = prefsStore.ErrSavePreferences();
    Assert( errNone == err ); // can't do much more

};

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
    
    LogInit( appContext, "c:\\inoah_log.txt" );
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

    appContext->prefs.fEnablePronunciation = false;

    SyncScreenSize(appContext);
    
    error=DIA_Init(&appContext->diaSettings);
    if (error) 
        goto OnError;
    
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
    if (error)
    {
        // impossible has happen and we have to respect that
        // if we don't quit we might get a blank screen due
        // to form not being there (as in MainFormLoad() we
        // delete the form if there's an error)
        SendStopEvent();
    }
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
    if (error)
        return error;

    switch (cmd)
    {
        case sysAppLaunchCmdNormalLaunch:
            error=AppLaunch();
            break;
            
        case sysAppLaunchCmdNotify:
            error=AppHandleSysNotify((SysNotifyParamType*)cmdPBP);
            break;
    }
    return error;
}

