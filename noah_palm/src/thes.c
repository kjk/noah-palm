/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include "roget_support.h"
#include "five_way_nav.h"
#include "resident_lookup_form.h"
#include "resident_browse_form.h"
#include "word_matching_pattern.h"
#include "bookmarks.h"
#include <PrefsStore.hpp>

#ifdef NEVER
static const char helpText[] =
    " This is a demo version. It's fully\n functional but has only 10% of\n the thesaurus data.\n" \
    " Go to www.arslexis.com to get the\n full version and find out about\n latest developments.\n";
#endif

static void DictFoundCBThes(void* context, AbstractFile *file)
{
    AppContext* appContext=(AppContext*)context;
    Assert(appContext);
    Assert( file );
    Assert( THES_CREATOR == file->creator );
    Assert( ROGET_TYPE == file->type );
    if (appContext->dictsCount>=MAX_DICTS)
    {
        AbstractFileFree(file);
        return;
    }

    appContext->dicts[appContext->dictsCount++] = file;
}

#pragma segment Segment2

// Create a blob containing serialized preferences.
// Caller needs to free returned memory
static void *SerializePreferencesThes(AppContext* appContext, long *pBlobSize)
{
    char *          prefsBlob;
    long            blobSize;
    long            blobSizePhaseOne;
    int             phase;
    AppPrefs*       prefs;
    UInt32          prefRecordId = AppPrefId;
    int             i;
    unsigned char   currFilePos;
    AbstractFile *  currFile;

    Assert(appContext);
    Assert( pBlobSize );

    LogG("SerializePreferencesThes()");

    prefs = &appContext->prefs;
    /* phase one: calculate the size of the blob */
    /* phase two: create the blob */
    prefsBlob = NULL;
    for( phase=1; phase<=2; phase++)
    {
        blobSize = 0;
        Assert( 4 == sizeof(prefRecordId) );
        // 1. preferences
        serData( (char*)&prefRecordId, (long)sizeof(prefRecordId), prefsBlob, &blobSize );
        serByte( prefs->startupAction, prefsBlob, &blobSize );
        serByte( prefs->hwButtonScrollType, prefsBlob, &blobSize );
        serByte( prefs->navButtonScrollType, prefsBlob, &blobSize );
        serByte( prefs->dbStartupAction, prefsBlob, &blobSize );
        serByte( prefs->bookmarksSortType, prefsBlob, &blobSize );
        serByte( prefs->fResidentModeEnabled, prefsBlob, &blobSize );

        // 2. number of databases found      
        serByte( appContext->dictsCount, prefsBlob, &blobSize );

        // 3. currently used database
        currFilePos = 0xff;
        for (i=0; i<appContext->dictsCount; i++)
        {
            if( GetCurrentFile(appContext) == appContext->dicts[i] )
            {
                currFilePos = (unsigned char)i;
                break;
            }
        }
        serByte( currFilePos, prefsBlob, &blobSize );

        // 4. list of databases
        // note: we don't really need to store databases from eFS_MEM because
        // we rescan them anyway, but this is easier to code
        for(i=0; i<appContext->dictsCount; i++)
        {
            currFile = appContext->dicts[i];
            Assert( NULL != currFile );
            serByte( currFile->fsType, prefsBlob, &blobSize );
            serLong( currFile->creator, prefsBlob, &blobSize );
            serLong( currFile->type, prefsBlob, &blobSize );
            serString(currFile->fileName, prefsBlob, &blobSize );
            if ( eFS_VFS == currFile->fsType )
            {
                serInt( currFile->volRef, prefsBlob, &blobSize );
            }
        }

        // 5. last word
        serString( (char*)appContext->prefs.lastWord, prefsBlob, &blobSize );

        // 6. number of words in the history
        serInt( appContext->historyCount, prefsBlob, &blobSize );

        // 7. all words in the history
        for (i=0; i<appContext->historyCount; i++)
        {
            serString( appContext->wordHistory[i], prefsBlob, &blobSize );
        }
        /* 8. better formatting data*/
        serData( (char*)&appContext->prefs.displayPrefs, (long)sizeof(appContext->prefs.displayPrefs), prefsBlob, &blobSize );

        if ( 1 == phase )
        {
            Assert( blobSize > 0 );
            blobSizePhaseOne = blobSize;
            prefsBlob = (char*)new_malloc( blobSize );
            if (NULL == prefsBlob)
            {
                LogG("SerializePreferencesThes(): prefsBlob==NULL");
                return NULL;
            }
        }
    }
    Assert( blobSize == blobSizePhaseOne );
    Assert( blobSize > 8 );

    *pBlobSize = blobSize;
    Assert( prefsBlob );
    return prefsBlob;
}

#pragma segment Segment1

// Given a blob containing serialized prefrences deserilize the blob
// and set the preferences accordingly.
static void DeserializePreferencesThes(AppContext* appContext, const unsigned char *prefsBlob, long blobSize)
{
    ThesPrefs *     prefs;
    int             i;
    eFsType         fsType;
    char *          fileName;
    UInt32          creator;
    UInt32          type;
    unsigned char   dbCount;
    unsigned char   currDb;
    AbstractFile *  file;

    Assert( prefsBlob );
    Assert( blobSize > 8 );

    LogG("DeserializePreferencesThes()" );

    prefs = &appContext->prefs;
    /* skip the 4-byte signature of the preferences record */
    Assert( IsValidPrefRecord(prefsBlob) );
    prefsBlob += 4;
    blobSize -= 4;

    // 1. preferences
    prefs->startupAction = (StartupAction) deserByte( &prefsBlob, &blobSize );
    prefs->hwButtonScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->navButtonScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->dbStartupAction = (DatabaseStartupAction) deserByte( &prefsBlob, &blobSize );
    prefs->bookmarksSortType = (BookmarkSortType) deserByte( &prefsBlob, &blobSize );
    prefs->fResidentModeEnabled = (Boolean) deserByte( &prefsBlob, &blobSize );

    // 2. number of databases detected
    dbCount = deserByte( &prefsBlob, &blobSize );

    // 3. currently used database
    currDb = deserByte( &prefsBlob, &blobSize );

    // 4. list of databases
    for(i=0; i<(int)dbCount; i++)
    {
        fsType = (eFsType) deserByte( &prefsBlob, &blobSize );
        Assert( FValidFsType(fsType) );
        // Assert( (eFS_MEM==fsType) || (eFS_VFS==fsType) );
        creator = (UInt32)deserLong( &prefsBlob, &blobSize );
        type = (UInt32)deserLong( &prefsBlob, &blobSize );
        fileName = deserString( &prefsBlob, &blobSize );
        if ( eFS_VFS == fsType )
        {
            file = AbstractFileNewFull( fsType, creator, type, fileName );
            if (NULL==file)
                return;
            file->volRef = (UInt16)deserInt( &prefsBlob, &blobSize );
            // we only remember files on external memory because those in ram
            // are fast to find
            DictFoundCBThes( appContext, file );
        }

        if (i==currDb)
            prefs->lastDbUsedName = fileName;
        else
            new_free(fileName);
    }

    /// 5. last word
    deserStringToBuf( (char*)appContext->prefs.lastWord, WORD_MAX_LEN, &prefsBlob, &blobSize );
    LogV1("DeserializePreferencesThes(), lastWord=%s", appContext->prefs.lastWord );

    // 6. number of words in the history
    appContext->historyCount = deserInt( &prefsBlob, &blobSize );

    // 7. all words in the history
    for (i=0; i<appContext->historyCount; i++)
    {
        appContext->wordHistory[i] = deserString( &prefsBlob, &blobSize );
    }
    /* 8. better formatting data*/
    deserData( (unsigned char*)&appContext->prefs.displayPrefs, (long)sizeof(appContext->prefs.displayPrefs), &prefsBlob, &blobSize );
}

#pragma segment Segment2

void SavePreferencesThes(AppContext* appContext)
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

    prefsBlob = SerializePreferencesThes(appContext, &blobSize );
    if ( NULL == prefsBlob ) return;

    db = DmOpenDatabaseByTypeCreator(THES_PREF_TYPE, THES_CREATOR, dmModeReadWrite);
    if (!db)
    {
        err = DmCreateDatabase(0, "Thes Prefs", THES_CREATOR,  THES_PREF_TYPE, false);
        if ( errNone != err)
        {
            appContext->err = ERR_NO_PREF_DB_CREATE;
            return;
        }

        db = DmOpenDatabaseByTypeCreator(THES_PREF_TYPE, THES_CREATOR, dmModeReadWrite);
        if (!db)
        {
            appContext->err = ERR_NO_PREF_DB_OPEN;
            return;
        }
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

#pragma segment Segment1

static void LoadPreferencesThes(AppContext* appContext)
{
    DmOpenRef    db;
    UInt         recNo;
    void *       recData;
    MemHandle    recHandle;
    UInt         recsCount;
    Boolean      fRecFound = false;

    appContext->fFirstRun = true;
    db = DmOpenDatabaseByTypeCreator(THES_PREF_TYPE, THES_CREATOR, dmModeReadWrite);
    if (!db) return;
    recsCount = DmNumRecords(db);
    for (recNo = 0; (recNo < recsCount) && !fRecFound; recNo++)
    {
        recHandle = DmQueryRecord(db, recNo);
        recData = MemHandleLock(recHandle);
        if ( (MemHandleSize(recHandle)>=PREF_REC_MIN_SIZE) && IsValidPrefRecord(recData) )
        {
            LogG("LoadPreferencesThes(), found prefs record" );
            fRecFound = true;
            appContext->fFirstRun = false;
            DeserializePreferencesThes(appContext, (unsigned char*)recData, MemHandleSize(recHandle) );
        }
        MemPtrUnlock(recData);
    }
    DmCloseDatabase(db);
}

#define IsThesDatabase(creator, type) (THES_CREATOR==(creator) && ROGET_TYPE==(type))

/* called for every file on the external card */
static void VfsFindCbThes( void* context, AbstractFile *file )
{
    AbstractFile *fileCopy;

    /* UNDONE: update progress dialog with a number of files processed */

    if ( !IsThesDatabase(file->creator, file->type) )
        return;

    fileCopy = AbstractFileNewFull( file->fsType, file->creator, file->type, file->fileName );
    if ( NULL == fileCopy ) return;
    fileCopy->volRef = file->volRef;
    DictFoundCBThes( context, fileCopy );
}

static Boolean FDatabaseExists(FS_Settings* fsSettings, AbstractFile *file)
{
    PdbHeader   hdr;

    Assert( eFS_VFS == file->fsType );

    if ( !FVfsPresent(fsSettings) )
        return false;

    if ( !ReadPdbHeader(file->volRef, file->fileName, &hdr ) )
        return false;
    
    if ( IsThesDatabase( hdr.creator, hdr.type ) )
        return true;
    else
        return false;
}

static void RemoveNonexistingDatabases(AppContext* appContext)
{
    int i;
    // if we got a list of databases on eFS_VFS from preferences we need
    // to verify that those databases still exist
    for(i=0; i<appContext->dictsCount; i++)
    {
        if ( !FDatabaseExists(&appContext->fsSettings, appContext->dicts[i]) )
        {
            AbstractFileFree( appContext->dicts[i] );
            MemMove( &(appContext->dicts[i]), &(appContext->dicts[i+1]), (appContext->dictsCount-i-1)*sizeof(appContext->dicts[0]) );
            --appContext->dictsCount;
        }
    }
}

static void ScanForDictsThes(AppContext* appContext, Boolean fAlwaysScanExternal)
{
    FsMemFindDb( THES_CREATOR, ROGET_TYPE, NULL, &DictFoundCBThes, appContext );

    /* TODO: show a progress dialog with a number of files processed so far */

    // only scan external memory card (slow) if this is the first
    // time we run (don't have the list of databases cached in
    // preferences) or we didn't find any databases at all
    // (unless it was over-written by fAlwaysScanExternal flag
    if (fAlwaysScanExternal || appContext->fFirstRun || (0==appContext->dictsCount))
        FsVfsFindDb(&appContext->fsSettings, &VfsFindCbThes, appContext );
}

extern Err AppCommonInit(AppContext* appContext);

Err AppCommonInit(AppContext* appContext)
{
    Err     error;

    MemSet(appContext, sizeof(*appContext), 0);
    error=FtrSet(APP_CREATOR, appFtrContext, (UInt32)appContext);
    if (error) 
        goto OnError;
    error=FtrSet(APP_CREATOR, appFtrLeaksFile, 0);
    if (error) 
        goto OnError;
    
    LogInit( appContext, "c:\\thes_log.txt" );
    InitFiveWay(appContext);

    // disable getting nilEvent
    appContext->ticksEventTimeout = evtWaitForever;
#ifdef DEBUG
    appContext->currentStressWord = 0;
#endif
    appContext->prevSelectedWord = 0xfffff;

    appContext->firstDispLine = -1;
    appContext->prevSelectedWord = -1;

    // fill out the default values for Thesaurus preferences
    // and try to load them from pref database
    appContext->prefs.startupAction      = startupActionNone;
    appContext->prefs.hwButtonScrollType = scrollPage;
    appContext->prefs.navButtonScrollType= scrollPage;
    appContext->prefs.dbStartupAction    = dbStartupActionLast;
    appContext->prefs.lastDbUsedName     = NULL;
    appContext->prefs.bookmarksSortType  = bkmSortByTime;
    appContext->prefs.fResidentModeEnabled = true;

    // fill out the default display preferences
    appContext->prefs.displayPrefs.listStyle = 2;
    SetDefaultDisplayParam(&appContext->prefs.displayPrefs,false,false);
    appContext->ptrOldDisplayPrefs = NULL;

    appContext->bookmarksDb = NULL;
    appContext->currBookmarkDbType = bkmInvalid;
    appContext->fInResidentMode = false;

    SyncScreenSize(appContext);
    FsInit(&appContext->fsSettings);
    LoadPreferencesThes(appContext);
    
    error=DIA_Init(&appContext->diaSettings);
    if (error) 
        goto OnError;
    
OnError:
    return error;
}

static Err AppCommonFree(AppContext* appContext);

#pragma segment Segment2

static Err InitThesaurus(AppContext* appContext)
{
    Err error=errNone;

    error=AppCommonInit(appContext);
    if (error) 
        goto OnError;
    
    error=AppNotifyInit(appContext);
    if (error)
        goto OnErrorCommonFree;

OnError:
    return error;
    
OnErrorCommonFree:
    AppCommonFree(appContext);
    goto OnError;    
}

#pragma segment Segment1

static Boolean DictInit(AppContext* appContext, AbstractFile *file)
{
    if ( !FsFileOpen( appContext, file ) )
        return false;

    if ( !dictNew(file) )
        return false;

    appContext->wordsCount = dictGetWordsCount(file);
    appContext->currentWord = 0;
    appContext->listItemOffset = 0;
    LogV1("DictInit(%s) ok", file->fileName );
    return true;
}

static void DictCurrentFree(AppContext* appContext)
{
    AbstractFile* file;
#if 0
    int i;
    // UNDONE: need to fix that to be dictionary-specific instead of global
    for (i = 0; i < appContext->recordsCount; i++)
    {
        if (0 != appContext->recsInfo[i].lockCount)
        {
            Assert(0);
        }
    }
#endif
    file=GetCurrentFile(appContext);
    if ( NULL != file)
    {
        dictDelete(file);
        FsFileClose(file);
        SetCurrentFile(appContext, NULL);
    }
}

Err AppCommonFree(AppContext* appContext)
{
    Err error=errNone;

    error=DIA_Free(&appContext->diaSettings);
    Assert(!error);

    DictCurrentFree(appContext);
    FreeDicts(appContext);
    FreeInfoData(appContext);
    FreeHistory(appContext);

    dcDelCacheDb();

    if ( NULL != appContext->prefs.lastDbUsedName )
        new_free( appContext->prefs.lastDbUsedName );
    
    if (appContext->wmpCacheDb)
        CloseMatchingPatternDB(appContext);

    if (appContext->bookmarksDb)
    	CloseBookmarksDB(appContext);

    FsDeinit(&appContext->fsSettings);
    return error;
}

#pragma segment Segment2

static void StopThesaurus(AppContext* appContext)
{
    Err error=errNone;
    SavePreferencesThes(appContext);
    bfFreePTR(appContext);

    error=AppCommonFree(appContext);
    Assert(!error);
    
    FrmSaveAllForms();
    FrmCloseAllForms();

    error=AppNotifyFree(true);    
    Assert(!error);
}

void DisplayAbout(AppContext* appContext)
{
    UInt16 currentY=0;
    if (GetOsVersion()>=35)
        WinPushDrawState();
    ClearDisplayRectangle(appContext);
    HideScrollbar();
    
    currentY=7;
    FntSetFont(largeFont);
    DrawCenteredString(appContext, "ArsLexis Thesaurus", currentY);
    currentY+=16;
#ifdef DEMO
    DrawCenteredString(appContext, "Ver 1.4 (demo)", currentY);
#else
  #ifdef DEBUG
    DrawCenteredString(appContext, "Ver 1.4 (debug)", currentY);
  #else
    DrawCenteredString(appContext, "Ver 1.4", currentY);
  #endif
#endif
    currentY+=20;
    
    FntSetFont(boldFont);
    DrawCenteredString(appContext, "Copyright (c) ArsLexis", currentY);
    currentY+=24;

    FntSetFont(largeFont);
    DrawCenteredString(appContext, "http://www.arslexis.com", currentY);
    currentY+=40;
    
    FntSetFont(stdFont);
#ifdef DEMO_HANDANGO
    DrawCenteredString(appContext, "Buy at: www.handango.com/purchase", currentY);
    currentY+=14;
    DrawCenteredString(appContext, "        Product ID: 10023", currentY);
#endif
#ifdef DEMO_PALMGEAR
    DrawCenteredString(appContext, "Buy at: www.palmgear.com?7423", currentY);
#endif

    if (GetOsVersion()>=35)
        WinPopDrawState();    
}

static void MainFormReflow(AppContext* appContext, FormType* frm)
{
    UpdateFrmBounds(frm);

    FrmSetObjectPosByID(frm, ctlArrowLeft, -1, appContext->screenHeight-12);
    FrmSetObjectPosByID(frm, ctlArrowRight, -1, appContext->screenHeight-12);
    FrmSetObjectBoundsByID(frm, scrollDef, appContext->screenWidth-8, -1, -1, appContext->screenHeight-18);

    if (appContext->fInResidentMode)
    {
        FrmSetObjectPosByID(frm, bmpClose, appContext->screenWidth-13, appContext->screenHeight-13);
        FrmSetObjectPosByID(frm, buttonClose, appContext->screenWidth-14, appContext->screenHeight-14);
        FrmSetObjectPosByID(frm, bmpFind, appContext->screenWidth-27, appContext->screenHeight-13);
        FrmSetObjectPosByID(frm, buttonFind, appContext->screenWidth-28, appContext->screenHeight-14);
        FrmHideObject(frm,FrmGetObjectIndex(frm, popupHistory));
    }
    else
    {
        FrmSetObjectPosByID(frm, bmpFind, appContext->screenWidth-13, appContext->screenHeight-13);
        FrmSetObjectPosByID(frm, buttonFind, appContext->screenWidth-14, appContext->screenHeight-14);
        FrmSetObjectPosByID(frm, popupHistory, appContext->screenWidth-32, appContext->screenHeight-13);
        FrmSetObjectPosByID(frm, listHistory, appContext->screenWidth-80, appContext->screenHeight-60);

        FrmHideObject(frm,FrmGetObjectIndex(frm, bmpClose));
        FrmHideObject(frm,FrmGetObjectIndex(frm, buttonClose));
    }
    FrmSetObjectBoundsByID(frm, fieldWordMain, -1, appContext->screenHeight-13, appContext->screenWidth-66, -1);
}

static Boolean MainFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Assert( DIA_Supported(&appContext->diaSettings) );
    if ( !DIA_Supported(&appContext->diaSettings))
        return false;

    MainFormReflow(appContext, frm);
    // TODO: optimize, only do when dx screen size has changed
    // HACK: we should have currentWord = -1 and check for that instead
    // (currently currentWord = 0 here)
    if (appContext->currDispInfo)
        SendEvtWithType(evtNewWordSelected);
    RedrawMainScreen(appContext);
    return true;
}

static Boolean MainFormOpen(AppContext *appContext, FormType *frm)
{    
    AbstractFile *  fileToOpen;
    char *          lastDbUsedName;
    int             i;

    MainFormReflow(appContext, frm);

    FrmDrawForm(frm);
    FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWordMain));

    HistoryListInit(appContext, frm);

    RemoveNonexistingDatabases(appContext);
    ScanForDictsThes(appContext, false);

    if (0 == appContext->dictsCount)
    {
        FrmAlert(alertNoDB);
        SendStopEvent();
        return true;
    }

ChooseDatabase:
    fileToOpen = NULL;
    if (1 == appContext->dictsCount )
        fileToOpen = appContext->dicts[0];
    else
    {
        lastDbUsedName = appContext->prefs.lastDbUsedName;
        if ( NULL != lastDbUsedName )
        {
            LogV1("db name from prefs: %s", lastDbUsedName );
        }
        else
        {
            LogG("no db name from prefs" );
        }

        if ( (NULL != lastDbUsedName) && (dbStartupActionLast == appContext->prefs.dbStartupAction) )
            fileToOpen=FindOpenDatabase(appContext, lastDbUsedName);
    }

    if (NULL == fileToOpen)
    {
        // ask the user which database to use
        FrmPopupForm(formSelectDict);
        return true;
    }
    else
    {
        if ( !DictInit(appContext, fileToOpen) )
        {
            // failed to initialize dictionary. If we have more - retry,
            // if not - just quit
            if ( appContext->dictsCount > 1 )
            {
                i = 0;
                while ( fileToOpen != appContext->dicts[i] )
                {
                    ++i;
                    Assert( i<appContext->dictsCount );
                }
                AbstractFileFree( appContext->dicts[i] );
                while ( i<appContext->dictsCount )
                {
                    appContext->dicts[i] = appContext->dicts[i+1];
                    ++i;
                }
                --appContext->dictsCount;
                FrmAlert( alertDbFailed);
                goto ChooseDatabase;
            }
            else
            {
                FrmAlert( alertDbFailed);
                SendStopEvent();
                return true;                    
            }
        }
    }
    WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
    WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);

    if ( appContext->residentWordLookup )
    {
        DoWord(appContext, appContext->residentWordLookup);
        return true;
    }

    if ( startupActionClipboard == appContext->prefs.startupAction )
    {
        if (!FTryClipboard(appContext, false))
            DisplayAbout(appContext);
    }
    else
        DisplayAbout(appContext);

    if ( (startupActionLast == appContext->prefs.startupAction) &&
        appContext->prefs.lastWord[0] )
    {
        DoWord(appContext, (char *)appContext->prefs.lastWord );
    }
    FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWordMain));
    return true;
}

static Boolean MainFormHandleEventThes(EventType * event)
{
    Boolean         handled = false;
    FormType *      frm;
    Short           newValue;
    ListType *      list;
    long            wordNo;
    int             i;
    int             selectedDb;
    AbstractFile *  fileToOpen;
    char *          word;
    AppContext*     appContext=GetAppContext();

    Assert(appContext);

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case winEnterEvent:
            // workaround for probable Sony CLIE's bug that causes only part of screen to be repainted on return from PopUps
            if (DIA_HasSonySilkLib(&appContext->diaSettings) && frm==(void*)((SysEventType*)event)->data.winEnter.enterWindow)
                RedrawMainScreen(appContext);
            break;

        case winDisplayChangedEvent:
            handled= MainFormDisplayChanged(appContext, frm);
            break;
            
        case frmUpdateEvent:
            LogG("mainFrm - frmUpdateEvent" );
            RedrawMainScreen(appContext);
            handled = true;
            break;

        case frmOpenEvent:
            handled = MainFormOpen(appContext, frm);
            break;

        case popSelectEvent:
            switch (event->data.popSelect.listID)
            {
                case listHistory:
                    word = appContext->wordHistory[event->data.popSelect.selection];
                    wordNo = dictGetFirstMatching(GetCurrentFile(appContext), word);
                    if (wordNo != appContext->currentWord)
                    {
                        appContext->currentWord = wordNo;
                        Assert(wordNo < appContext->wordsCount);
                        DrawDescription(appContext, appContext->currentWord);
                        appContext->penUpsToConsume = 1;
                    }
                    break;
                default:
                    Assert(0);
                    break;
            }
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case ctlArrowLeft:
                    if (appContext->currentWord > 0)
                    {
                        --appContext->currentWord;
                        RedisplayWord(appContext);
                    }
                    handled = true;
                    break;
                case ctlArrowRight:
                    if (appContext->currentWord < appContext->wordsCount - 1)
                    {
                        ++appContext->currentWord;                        
                        RedisplayWord(appContext);
                    }
                    handled = true;
                    break;
                case buttonFind:
                    GoToFindWordForm(appContext,frm);
                    handled = true;
                    break;
                case buttonClose:
                    SendStopEvent();
                    handled=true;
                    break;
                case popupHistory:
                    // need to propagate the event down to popus
                    handled = false;
                    break;
                default:
                    Assert(0);
                    break;
            }
            break;

        case evtNewWordSelected:
            word = dictGetWord(GetCurrentFile(appContext), appContext->currentWord);
            FldClearInsert(frm, fieldWordMain, word);

            AddToHistory(appContext, appContext->currentWord);
            HistoryListSetState(appContext, frm);

            WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
            WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);
            DrawDescription(appContext, appContext->currentWord);
            appContext->penUpsToConsume = 1;
            handled = true;
            break;

        case evtNewDatabaseSelected:
            selectedDb = EvtGetInt( event );
            fileToOpen = appContext->dicts[selectedDb];
            if ( GetCurrentFile(appContext) != fileToOpen )
            {
                DictCurrentFree(appContext);
                if ( !DictInit(appContext, fileToOpen) )
                {
                    // failed to initialize dictionary. If we have more - retry,
                    // if not - just quit
                    if ( appContext->dictsCount > 1 )
                    {
                        i = 0;
                        while ( fileToOpen != appContext->dicts[i] )
                        {
                            ++i;
                            Assert( i<appContext->dictsCount );
                        }
                        AbstractFileFree( appContext->dicts[i] );
                        while ( i<appContext->dictsCount )
                        {
                            appContext->dicts[i] = appContext->dicts[i+1];
                            ++i;
                        }
                        --appContext->dictsCount;
                        list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
                        LstSetListChoices(list, NULL, appContext->dictsCount);
                        if ( appContext->dictsCount > 1 )
                        {
                            FrmAlert( alertDbFailedGetAnother );
                            FrmPopupForm( formSelectDict );
                        }
                        else
                        {
                            /* only one dictionary left - try this one */
                            FrmAlert( alertDbFailed );
                            SendNewDatabaseSelected(0);
                        }
                        return true;
                    }
                    else
                    {
                        FrmAlert( alertDbFailed);
                        SendStopEvent();
                        return true;                    
                    }
                }
            }

            ClearMatchingPatternDB(appContext);

            RedrawMainScreen(appContext);

            if ( startupActionClipboard == appContext->prefs.startupAction )
            {
                if (!FTryClipboard(appContext, false))
                    DisplayAbout(appContext);
            }
            else
                DisplayAbout(appContext);

            if ( (startupActionLast == appContext->prefs.startupAction) &&
                appContext->prefs.lastWord[0] )
            {
                DoWord(appContext, (char *)appContext->prefs.lastWord );
            }

            handled = true;
            break;

        case keyDownEvent:
            if ( HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) )
            {
                if (FiveWayCenterPressed(appContext, event))
                {
                    FrmPopupForm(formDictFind);
                }
                if (FiveWayDirectionPressed(appContext, event, Left ))
                {
                    if (appContext->currentWord > 0)
                    {
                        --appContext->currentWord;
                        RedisplayWord(appContext);
                    }
                }
                if (FiveWayDirectionPressed(appContext, event, Right ))
                {
                    if (appContext->currentWord < appContext->wordsCount - 1)
                    {
                        ++appContext->currentWord;
                        RedisplayWord(appContext);
                    }
                }
                if (FiveWayDirectionPressed(appContext, event, Up ))
                {
                    DefScrollUp(appContext, appContext->prefs.navButtonScrollType );
                }
                if (FiveWayDirectionPressed(appContext, event, Down ))
                {
                    DefScrollDown(appContext, appContext->prefs.navButtonScrollType );
                }
                return false;
            }
            else if (pageUpChr == event->data.keyDown.chr)
            {
                DefScrollUp(appContext, appContext->prefs.hwButtonScrollType);
            }
            else if (pageDownChr == event->data.keyDown.chr)
            {
                DefScrollDown(appContext, appContext->prefs.hwButtonScrollType);
            }
            else
            {
                // notify ourselves that a text field is (possibly) updated
                SendFieldChanged();
                // mark as unhandled so that text field will (possibly) get updated
                handled = false;
            }
            break;

        case evtFieldChanged:
            // TODO: should compare with the previous ->lastWord and only do
            // stuff if different
            GoToFindWordForm(appContext,frm);
            handled = true;
            break;

        case sclExitEvent:
            newValue = event->data.sclRepeat.newValue;
            if (newValue != appContext->firstDispLine)
            {
                SetGlobalBackColor(appContext);            
                ClearRectangle(DRAW_DI_X, DRAW_DI_Y, appContext->screenWidth-FRM_RSV_W+2, appContext->screenHeight-FRM_RSV_H);
                appContext->firstDispLine = newValue;
                DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
                SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
            }
            handled = true;
            break;

        case penDownEvent:
            if ((NULL == appContext->currDispInfo) || (event->screenX > appContext->screenWidth-FRM_RSV_W) || (event->screenY > appContext->screenHeight-FRM_RSV_H))
            {
                handled = false;
                break;
            }

            cbPenDownEvent(appContext,event->screenX,event->screenY);
            handled = true;
            break;

        case penMoveEvent:
            cbPenMoveEvent(appContext,event->screenX,event->screenY);
            handled = true;
            break;

        case penUpEvent:
            if ((NULL == appContext->currDispInfo) || (event->screenX > appContext->screenWidth-FRM_RSV_W) || (event->screenY > appContext->screenHeight-FRM_RSV_H))
            {
                handled = false;
                break;
            }

            Assert( appContext->penUpsToConsume >= 0);

            if (appContext->penUpsToConsume >0)
            {
                --appContext->penUpsToConsume;
                handled = true;
                //break;
            }

            if(cbPenUpEvent(appContext,event->screenX,event->screenY))
            {
                handled = true;
                break;
            }

            handled = true;
            break;

        case menuEvent:
            switch (event->data.menu.itemID)
            {
                case menuItemFind:
                    GoToFindWordForm(appContext,frm);
                    break;
                case menuItemAbout:
                    if (NULL != appContext->currDispInfo)
                    {
                        diFree(appContext->currDispInfo);
                        appContext->currDispInfo = NULL;
                        appContext->currentWord = 0;
                    }
                    cbNoSelection(appContext);
                    DisplayAbout(appContext);
                    break;
                case menuItemSelectDB:
                    FrmPopupForm(formSelectDict);
                    break;
                case menuItemCopy:
                    if (NULL != appContext->currDispInfo)
                        diCopyToClipboard(appContext->currDispInfo);
                    break;
                case menuItemLookupClipboard:
                    FTryClipboard(appContext,false);
                    break;
#ifdef DEBUG
                case menuItemStress:
                    // initiate stress i.e. going through all the words
                    // 0 means that stress is not in progress
                    appContext->currentStressWord = -1;
                    // get nilEvents as fast as possible
                    appContext->ticksEventTimeout = 0;
                    break;
#endif
                case menuItemPrefs:
                    FrmPopupForm(formPrefs);
                    break;

                case menuItemDispPrefs:
                    FrmPopupForm(formDisplayPrefs);
                    break;

                case menuItemRandomWord:
                    {
                        wordNo = GenRandomLong(appContext->wordsCount);
                        word = dictGetWord(GetCurrentFile(appContext), wordNo);
                        FldClearInsert(frm, fieldWordMain, word);
                        AddToHistory(appContext, wordNo);
                        HistoryListSetState(appContext, frm);
                        DrawDescription(appContext, wordNo);
                    }
                    break;

                case menuItemFindPattern:
                    FrmPopupForm(formDictFindPattern);
                    break;
                    
                case menuItemBookmarkView:
                    if ( GetBookmarksCount(appContext)>0 )
                        FrmPopupForm(formBookmarks);
                    else
                        FrmAlert(alertNoBookmarks);
                    break;

                case menuItemBookmarkWord:
                    AddBookmark(appContext, dictGetWord(GetCurrentFile(appContext), appContext->currentWord));
                    break;

                case menuItemBookmarkDelete:
                    DeleteBookmark(appContext, dictGetWord(GetCurrentFile(appContext), appContext->currentWord));
                    break;

                case sysEditMenuCopyCmd:
                case sysEditMenuCutCmd:
                case sysEditMenuPasteCmd:
                    // generated by command bar, not sure what to do with those
                    // so just ignore them
                    return false;
                    break;

                default:
                    Assert(0);
                    break;
            }
            handled = true;
            break;
        case nilEvent:
#ifdef DEBUG
            if ( 0 != appContext->currentStressWord )
            {
                if ( -1 == appContext->currentStressWord )
                    appContext->currentStressWord = 0;
                DrawDescription(appContext, appContext->currentStressWord++);
                if (appContext->currentStressWord==dictGetWordsCount(GetCurrentFile(appContext)))
                {
                    // disable running the stress
                    appContext->currentStressWord = 0;
                    // disable getting nilEvent
                    appContext->ticksEventTimeout = evtWaitForever;
                }
            }
#endif
            handled = true;
            break;

        default:
            break;
    }
    return handled;
}

static Boolean FindFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Assert( DIA_Supported(&appContext->diaSettings) );
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(frm);

    SetListHeight(frm, listMatching, appContext->dispLinesCount);
    
    FrmSetObjectPosByID(frm, ctlArrowLeft, -1, appContext->screenHeight-12);
    FrmSetObjectPosByID(frm, ctlArrowRight, -1, appContext->screenHeight-12);
    FrmSetObjectBoundsByID(frm, listMatching, -1, -1, appContext->screenWidth, -1);
    FrmSetObjectBoundsByID(frm, fieldWord, -1, appContext->screenHeight-13, appContext->screenWidth-66, -1);
    FrmSetObjectPosByID(frm, buttonCancel, appContext->screenWidth-40, appContext->screenHeight-14);

    FrmDrawForm(frm);
    return true;
}

static Boolean FindFormHandleEventThes(EventType * event)
{
    Boolean     handled = false;
    char *      word;
    FormPtr     frm;
    FieldPtr    fld;
    ListPtr     list;
    AppContext* appContext=GetAppContext();

    Assert(appContext);
    
    /*if (event->eType == nilEvent)
        return true;*/

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled= FindFormDisplayChanged(appContext, frm);
            break;
            
        case frmOpenEvent:
            fld =(FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
            appContext->prevSelectedWord = 0xffffffff;
            LstSetListChoicesEx( list, NULL, dictGetWordsCount(GetCurrentFile(appContext)));
            LstSetDrawFunction(list, ListDrawFunc);
            appContext->prevTopItem = 0;
            appContext->selectedWord = 0;
            Assert(appContext->selectedWord < appContext->wordsCount);
            word = &(appContext->lastWord[0]);
            /* force updating the field */
            if (0 == StrLen(word) )
            {
                LstSetSelectionEx(appContext, list, appContext->selectedWord);
            }
            else
            {
                FldInsert(fld, word, StrLen(word));
                FldSetInsPtPosition(fld,appContext->fldInsPt);
                // force detecting a change in DoFieldChange()
                appContext->lastWord[0]='\0';
                SendFieldChanged();
            }
            FrmDrawForm(frm);
            FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWord));
            handled = true;
            break;

        case lstSelectEvent:
            /* copy word from text field to a buffer, so next time we
               come back here, we'll come back to the same place */
            //RememberLastWord(appContext, frm, fieldWord);
            /* set the selected word as current word */
            appContext->currentWord = appContext->listItemOffset + (UInt32) event->data.lstSelect.selection;
            /* send a msg to yourself telling that a new word
               have been selected so we need to draw the
               description */
            Assert(appContext->currentWord < appContext->wordsCount);
            SendEvtWithType(evtNewWordSelected);
            handled = true;
            FrmReturnToForm(0);
            break;

        case keyDownEvent: 
         /* ugly trick: there is no event that says that
            text field has changed, so I have to generate it myself
            every time I get keyDownEvent */
            switch (event->data.keyDown.chr)
            {
                case returnChr:
                case linefeedChr:
                    RememberLastWord(appContext, frm,fieldWord);
                    appContext->currentWord = appContext->selectedWord;
                    Assert(appContext->currentWord < appContext->wordsCount);
                    SendEvtWithType(evtNewWordSelected);
                    FrmReturnToForm(0);
                    return true;
                    break;

                case pageUpChr:
                    if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
                    {
                        ScrollWordListByDx( appContext, frm, -(appContext->dispLinesCount-1) );
                        return true;
                    }
                
                case pageDownChr:
                    if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
                    {
                        ScrollWordListByDx( appContext, frm, (appContext->dispLinesCount-1) );
                        return true;
                    }

                default:
                    if ( HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) )
                    {
                        if (FiveWayCenterPressed(appContext, event))
                        {
                            RememberLastWord(appContext, frm,fieldWord);
                            appContext->currentWord = appContext->selectedWord;
                            Assert(appContext->currentWord < appContext->wordsCount);
                            SendEvtWithType(evtNewWordSelected);
                            FrmReturnToForm(0);
                            return true;
                        }
                    
                        if (FiveWayDirectionPressed(appContext, event, Left ))
                        {
                            ScrollWordListByDx( appContext, frm, -(appContext->dispLinesCount-1) );
                            return true;
                        }
                        if (FiveWayDirectionPressed(appContext, event, Right ))
                        {
                            ScrollWordListByDx( appContext, frm, (appContext->dispLinesCount-1) );
                            return true;
                        }
                        if (FiveWayDirectionPressed(appContext, event, Up ))
                        {
                            ScrollWordListByDx( appContext, frm, -1 );
                            return true;
                        }
                        if (FiveWayDirectionPressed(appContext, event, Down ))
                        {
                            ScrollWordListByDx( appContext, frm, 1 );
                            return true;
                        }
                        return false;
                    }
                    break;
            }
            // add a message to the queue to inform about the possible change
            // of text field. We can't do the processing here because text
            // field has not yet been updated
            SendFieldChanged();
            // mark as not handled so that the event will get handled by a text field
            handled = false;
            break;

        case evtFieldChanged:
            DoFieldChanged(appContext);
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    RememberLastWord(appContext, frm, fieldWord);
                    FrmReturnToForm(0);
                    break;
                case ctlArrowLeft:
                    ScrollWordListByDx( appContext, frm, -(appContext->dispLinesCount-1) );
                    break;
                case ctlArrowRight:
                    ScrollWordListByDx( appContext, frm, (appContext->dispLinesCount-1) );
                    break;
                default:
                    Assert(0);
                    break;
            }
            handled = true;
            break;
        default:
            break;
    }
    return handled;
}

static Boolean SelectDictFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Assert( DIA_Supported(&appContext->diaSettings) );
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(frm);
    FrmDrawForm(frm);
    return true;
}

static Boolean SelectDictFormHandleEventThes(EventType * event)
{
    FormPtr     frm;
    ListPtr     list;
    long        selectedDb;
    AppContext* appContext=GetAppContext();
    Assert(appContext);
    
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            return SelectDictFormDisplayChanged(appContext, frm=FrmGetActiveForm());
            break;

        case frmOpenEvent:
            selectedDb = FindCurrentDbIndex(appContext);
            frm = FrmGetActiveForm();
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
            LstSetDrawFunction(list, ListDbDrawFunc);
            LstSetListChoices(list, NULL, appContext->dictsCount);
            LstSetSelection(list, selectedDb);
            LstMakeItemVisible(list, selectedDb);
            if (NULL == GetCurrentFile(appContext))
                FrmHideObject(frm, FrmGetObjectIndex(frm, buttonCancel));
            else
                FrmShowObject(frm, FrmGetObjectIndex(frm, buttonCancel));
            FrmDrawForm(frm);
            return true;

        case lstSelectEvent:
            return true;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonSelect:
                    frm = FrmGetActiveForm();
                    list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
                    selectedDb = LstGetSelection(list);
                    if (selectedDb != noListSelection)
                        SendNewDatabaseSelected(selectedDb);
                    FrmReturnToForm(0);
                    return true;

                case buttonCancel:
                    Assert( NULL != GetCurrentFile(appContext) );
                    FrmReturnToForm(0);
                    return true;

                case buttonRescanDicts:
                    DictCurrentFree(appContext);
                    FreeDicts(appContext);
                    // force scanning external memory for databases
                    ScanForDictsThes(appContext, true);
                    frm = FrmGetActiveForm();
                    list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
                    LstSetListChoices(list, NULL, appContext->dictsCount);
                    if (0==appContext->dictsCount)
                    {
                        FrmHideObject(frm, FrmGetObjectIndex(frm, buttonSelect));
                        FrmShowObject(frm, FrmGetObjectIndex(frm, buttonCancel));
                    }
                    else
                    {
                        FrmShowObject(frm, FrmGetObjectIndex(frm, buttonSelect));
                        if (NULL == GetCurrentFile(appContext))
                            FrmHideObject(frm, FrmGetObjectIndex(frm, buttonCancel));
                        else
                            FrmShowObject(frm, FrmGetObjectIndex(frm, buttonCancel));
                        LstSetSelection(list, 0);
                        LstMakeItemVisible(list, 0);
                    }
                    FrmUpdateForm(FrmGetFormId(frm), frmRedrawUpdateCode);
                    return true;
                default:
                    Assert(0);
                    break;
            }
            break;
        default:
            break;
        }
    return false;
}

static void PrefsToGUI(AppPrefs* prefs, FormType * frm)
{
    Assert(frm);
    SetPopupLabel(frm, listStartupAction, popupStartupAction, prefs->startupAction);
    SetPopupLabel(frm, listStartupDB, popupStartupDB, prefs->dbStartupAction);
    SetPopupLabel(frm, listhwButtonsAction, popuphwButtonsAction, prefs->hwButtonScrollType);
    SetPopupLabel(frm, listNavButtonsAction, popupNavButtonsAction, prefs->navButtonScrollType);
    CtlSetValue( (ControlType*)FrmGetObjectPtr( frm, FrmGetObjectIndex(frm, checkResidentMode) ), (Int16)prefs->fResidentModeEnabled );
}

static Boolean PrefFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Assert( DIA_Supported(&appContext->diaSettings) );
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(frm);

    FrmSetObjectPosByID(frm, buttonOk, -1, appContext->screenHeight-13-14);
    FrmSetObjectPosByID(frm, buttonCancel, -1, appContext->screenHeight-13-14);

    FrmDrawForm(frm);
    return true;
}

static Boolean PrefFormHandleEventThes(EventType * event)
{
    Boolean     handled = false;
    FormType *  frm;
    ListType *  list;
    char *      listTxt;
    AppContext* appContext=GetAppContext();

    Assert(appContext);

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled=PrefFormDisplayChanged(appContext, frm);
            break;

        case frmOpenEvent:
            PrefsToGUI(&(appContext->prefs), frm);
            appContext->tmpPrefs = appContext->prefs;
            FrmDrawForm(frm);
            handled = true;
            break;

        case popSelectEvent:
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,event->data.popSelect.listID));
            listTxt = LstGetSelectionText(list, event->data.popSelect.selection);
            switch (event->data.popSelect.listID)
            {
                case listStartupAction:
                    appContext->tmpPrefs.startupAction = (StartupAction) event->data.popSelect.selection;
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, popupStartupAction)), listTxt);
                    break;
                case listStartupDB:
                    appContext->tmpPrefs.dbStartupAction = (DatabaseStartupAction)event->data.popSelect.selection;
                    CtlSetLabel( (ControlType*)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupStartupDB)), listTxt);
                    break;
                case listhwButtonsAction:
                    appContext->tmpPrefs.hwButtonScrollType = (ScrollType) event->data.popSelect.selection;
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, popuphwButtonsAction)), listTxt);
                    break;
                case listNavButtonsAction:
                    appContext->tmpPrefs.navButtonScrollType = (ScrollType) event->data.popSelect.selection;
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, popupNavButtonsAction)), listTxt);
                    break;
                default:
                    Assert(0);
                    break;
            }
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case checkResidentMode:
                    appContext->tmpPrefs.fResidentModeEnabled = (Boolean) event->data.ctlSelect.on;
                    handled = true;
                    break;
                case popupStartupAction:
                case popupStartupDB:
                case popuphwButtonsAction:
                case listNavButtonsAction:
                    // need to propagate the event down to popus
                    handled = false;
                    break;
                case buttonOk:
                    appContext->prefs = appContext->tmpPrefs;
                    SavePreferencesThes(appContext);
                    // pass through
                case buttonCancel:
                    FrmReturnToForm(0);
                    handled = true;
                    break;
                default:
                    Assert(0);
                    break;
            }
            break;
        default:
            handled = false;
            break;
    }
    return handled;
}

static Boolean HandleEventThes(AppContext* appContext, EventType * event)
{
    FormPtr frm;
    UInt16 formId;
    Err error=errNone;
    Boolean handled = false;

    if (event->eType == frmLoadEvent)
    {
        formId = event->data.frmLoad.formID;
        frm = FrmInitForm(formId);
        FrmSetActiveForm(frm);
        error=DefaultFormInit(appContext, frm);

        switch (formId)
        {
        case formDictMain:
            FrmSetEventHandler(frm, MainFormHandleEventThes);
            handled = true;
            break;
        case formDictFind:
            FrmSetEventHandler(frm, FindFormHandleEventThes);
            handled = true;
            break;
        case formSelectDict:
            FrmSetEventHandler(frm, SelectDictFormHandleEventThes);
            handled = true;
            break;
        case formPrefs:
            FrmSetEventHandler(frm, PrefFormHandleEventThes);
            handled = true;
            break;
        case formDisplayPrefs:
            FrmSetEventHandler(frm, DisplayPrefFormHandleEvent);
            handled = true;
            break;
        case formDictFindPattern:
            FrmSetEventHandler(frm, FindPatternFormHandleEvent);
            handled = true;
            break;
        case formBookmarks:
            FrmSetEventHandler(frm, BookmarksFormHandleEvent);
            return true;
        default:
            Assert(0);
            handled = false;
            break;
        }
    }
    else if (event->eType == winDisplayChangedEvent)
        SyncScreenSize(appContext);
    
    Assert(!error); // so that we get breakpoint if something unexpected happens
    return handled;
}

static void EventLoopThes(AppContext* appContext)
{
    EventType event;
    Word error;

    event.eType = (eventsEnum) 0;
    while (event.eType != appStopEvent)
    {
        EvtGetEvent(&event, appContext->ticksEventTimeout);
        if (SysHandleEvent(&event))
            continue;
        if (MenuHandleEvent(0, &event, &error))
            continue;
        if (HandleEventThes(appContext, &event))
            continue;
        FrmDispatchEvent(&event);
    }
}

#pragma segment Segment1

Err AppPerformResidentLookup(Char* term)
{
    Err             error;
    AbstractFile *  chosenDb;
    AppContext *    appContext=(AppContext*)MemPtrNew(sizeof(AppContext));

    if (!appContext)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    error=AppCommonInit(appContext);
    RemoveNonexistingDatabases(appContext);
    ScanForDictsThes(appContext, false);
    if (0 == appContext->dictsCount)
        FrmAlert(alertNoDB);
    else
    {
        if (1 == appContext->dictsCount )
            chosenDb = appContext->dicts[0];
        else
        {
            // because we can't start resident mode without previously gracefully exiting at least one time
            Assert(appContext->prefs.lastDbUsedName); 
            chosenDb=FindOpenDatabase(appContext, appContext->prefs.lastDbUsedName);
        }            
        if (!chosenDb || !DictInit(appContext, chosenDb))
            FrmAlert(alertDbFailed);
        else 
        {
            if (term)
            {
                appContext->currentWord=dictGetFirstMatching(chosenDb, term);
                error=PopupResidentLookupForm(appContext);
            }                
            else
            {
                appContext->currentWord=-1;
                error=PopupResidentBrowseForm(appContext);
                if (!error && appContext->currentWord!=-1)
                    error=PopupResidentLookupForm(appContext);
            }                    
        }          
    }
    AppCommonFree(appContext);
OnError:
    if (appContext)
        MemPtrFree(appContext);
    return error;
}

#pragma segment Segment2

static Err AppLaunch(char *cmdPBP) 
{
    Err error=errNone;
    AppContext* appContext=(AppContext*)MemPtrNew(sizeof(AppContext));
    if (!appContext)
    {
        error=!errNone;
        goto OnError;
    }     
    error=InitThesaurus(appContext);
    if (error) 
        goto OnError;

    if (cmdPBP)
        ParseResidentWord(appContext, cmdPBP);

    FrmGotoForm(formDictMain);
    EventLoopThes(appContext);
    StopThesaurus(appContext);
OnError: 
    if (appContext) 
        MemPtrFree(appContext);
    return error;
}

#pragma segment Segment1

DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Err err=errNone;
    switch (cmd)
    {
    case sysAppLaunchCmdNormalLaunch:
        err=AppLaunch((char*)cmdPBP);
        break;
        
    case sysAppLaunchCmdNotify:
        AppHandleSysNotify((SysNotifyParamType*)cmdPBP);
        break;
    }

    return err;
}

