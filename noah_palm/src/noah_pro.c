/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "noah_pro_1st_segment.h"
#include "noah_pro_2nd_segment.h"
#include "five_way_nav.h"
#include "resident_lookup_form.h"
#include "resident_browse_form.h"
#include "word_matching_pattern.h"
#include "bookmarks.h"
#include "PrefsStore.hpp"

static void DictFoundCBNoahPro(void* context, AbstractFile *file)
{
    AppContext* appContext=(AppContext*)context;
    Assert(appContext);
    Assert( file );
    Assert( NOAH_PRO_CREATOR == file->creator );
    Assert( (WORDNET_PRO_TYPE == file->type) ||
            (WORDNET_LITE_TYPE == file->type) ||
            (SIMPLE_TYPE == file->type) ||
            (ENGPOL_TYPE == file->type));

    if (appContext->dictsCount>=MAX_DICTS)
    {
        AbstractFileFree(file);
        return;
    }
    appContext->dicts[appContext->dictsCount++] = file;
}

// Given a blob containing serialized prefrences deserilize the blob
// and set the preferences accordingly.
static void DeserializePreferencesNoahPro(AppContext* appContext, unsigned char *prefsBlob, long blobSize)
{
    NoahPrefs *     prefs;
    eFsType         fsType;
    char *          fileName;
    UInt32          creator;
    UInt32          type;
    int             i;
    unsigned char   dbCount;
    unsigned char   currDb;
    AbstractFile *  file;

    Assert( prefsBlob );
    Assert( blobSize > 8 );

    prefs = &appContext->prefs;
    /* skip the 4-byte signature of the preferences record */
    Assert( IsValidPrefRecord( prefsBlob ) );
    prefsBlob += 4;
    blobSize -= 4;

    /* 1. preferences */
    prefs->startupAction = (StartupAction) deserByte( &prefsBlob, &blobSize );
    prefs->hwButtonScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->navButtonScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->dbStartupAction = (DatabaseStartupAction) deserByte( &prefsBlob, &blobSize );
    prefs->bookmarksSortType = (BookmarkSortType) deserByte( &prefsBlob, &blobSize );
    prefs->fResidentModeEnabled = (Boolean) deserByte( &prefsBlob, &blobSize );

    Assert( blobSize > 0);

    /* 2. number of databases detected */
    dbCount = deserByte( &prefsBlob, &blobSize );

    /* 3. currently used database */
    currDb = deserByte( &prefsBlob, &blobSize );

    /* 4. list of databases */
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
            DictFoundCBNoahPro( appContext, file );
        }

        if (i==currDb)
            prefs->lastDbUsedName = fileName;
        else
            new_free(fileName);
    }

    /* 5. last word */
    deserStringToBuf( (char*)appContext->prefs.lastWord, WORD_MAX_LEN, &prefsBlob, &blobSize );

    /* 6. number of words in the history */
    appContext->historyCount = deserInt( &prefsBlob, &blobSize );

    /* 7. all words in the history */
    for (i=0; i<appContext->historyCount; i++)
    {
        appContext->wordHistory[i] = deserString( &prefsBlob, &blobSize );
    }
    /* 8. better formatting data*/
    deserData( (unsigned char*)&appContext->prefs.displayPrefs, (long)sizeof(appContext->prefs.displayPrefs), &prefsBlob, &blobSize );
    
}

static void LoadPreferencesNoahPro(AppContext* appContext)
{
    DmOpenRef    db;
    UInt         recNo;
    void *       recData;
    MemHandle    recHandle;
    UInt         recsCount;
    Boolean      fRecFound = false;

    appContext->fFirstRun = true;
    db = DmOpenDatabaseByTypeCreator(NOAH_PREF_TYPE, NOAH_PRO_CREATOR, dmModeReadWrite);
    if (!db) return;
    recsCount = DmNumRecords(db);
    for (recNo = 0; (recNo < recsCount) && !fRecFound; recNo++)
    {
        recHandle = DmQueryRecord(db, recNo);
        recData = MemHandleLock(recHandle);
        if ( (MemHandleSize(recHandle)>=PREF_REC_MIN_SIZE) && IsValidPrefRecord( recData ) )
        {
            LogG( "LoadPreferencesNoahPro(), found prefs record" );
            fRecFound = true;
            appContext->fFirstRun = false;
            DeserializePreferencesNoahPro(appContext, (unsigned char*)recData, MemHandleSize(recHandle) );
        }
        MemPtrUnlock(recData);
    }
    DmCloseDatabase(db);
}

static Boolean FNoahProDatabase( UInt32 creator, UInt32 type )
{
    if ( NOAH_PRO_CREATOR != creator )
        return false;

#ifdef DEMO
    if ( WORDNET_PRO_TYPE == type )
        return true;
#else
    if ( (WORDNET_PRO_TYPE == type) ||
        (WORDNET_LITE_TYPE == type) ||
        (SIMPLE_TYPE == type) ||
        (ENGPOL_TYPE == type))
    {
        return true;
    }
#endif
    return false;
}

/* called for every file on the external card */
static void VfsFindCbNoahPro( void* context, AbstractFile *file )
{
    AbstractFile *fileCopy;

    /* UNDONE: update progress dialog with a number of files processed */

    if ( !FNoahProDatabase( file->creator, file->type ) )
        return;

    // "PPrs" is (supposedly) name of a db created by Palm Reader Pro 2.2.2
    // that has matching creator/type of Noah's database. We need to filter
    // those out
    // note: this would not be right if the file was on external card but I don't
    // think this is the case
    if ( 0==StrCompare(file->fileName,"PPrs") )
        return;

    fileCopy = AbstractFileNewFull( file->fsType, file->creator, file->type, file->fileName );
    if ( NULL == fileCopy ) return;
    fileCopy->volRef = file->volRef;
    DictFoundCBNoahPro( context, fileCopy );
}

static Boolean FDatabaseExists(FS_Settings* fsSettings, AbstractFile *file)
{
    PdbHeader   hdr;

    Assert( eFS_VFS == file->fsType );

    if ( !FVfsPresent(fsSettings) )
        return false;

    if ( !ReadPdbHeader(file->volRef, file->fileName, &hdr ) )
        return false;
    
    if ( FNoahProDatabase( hdr.creator, hdr.type ) )
        return true;
    else
        return false;
}

void RemoveNonexistingDatabases(AppContext* appContext)
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
    
void ScanForDictsNoahPro(AppContext* appContext, Boolean fAlwaysScanExternal)
{
#ifdef DEMO
    FsMemFindDb(NOAH_PRO_CREATOR, WORDNET_PRO_TYPE, NULL, &DictFoundCBNoahPro, appContext);
#else
    FsMemFindDb(NOAH_PRO_CREATOR, WORDNET_PRO_TYPE, NULL, &DictFoundCBNoahPro, appContext);
    FsMemFindDb(NOAH_PRO_CREATOR, WORDNET_LITE_TYPE, NULL, &DictFoundCBNoahPro, appContext);
    FsMemFindDb(NOAH_PRO_CREATOR, SIMPLE_TYPE, NULL, &DictFoundCBNoahPro, appContext);
#endif
    /* TODO: show a progress dialog with a number of files processed so far */
    
    // only scan external memory card (slow) if this is the first
    // time we run (don't have the list of databases cached in
    // preferences) or we didn't find any databases at all
    // (unless it was over-written by fAlwaysScanExternal flag
    if (fAlwaysScanExternal || appContext->fFirstRun || (0==appContext->dictsCount))
        FsVfsFindDb( &appContext->fsSettings, &VfsFindCbNoahPro, appContext );
    LogV1( "ScanForDictsNoahPro(), found %d dicts", appContext->dictsCount );
}

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
    
    LogInit( appContext, "c:\\noah_pro_log.txt" );
    InitFiveWay(appContext);

    appContext->err = ERR_NONE;
    appContext->prevSelectedWord = 0xfffff;

    appContext->firstDispLine = -1;
    appContext->currentWord = -1;
    appContext->prevSelectedWord = -1;
    // disable getting nilEvent
    appContext->ticksEventTimeout = evtWaitForever;
#ifdef DEBUG
    appContext->currentStressWord = 0;
#endif

    appContext->residentWordLookup = NULL;
    // fill out the default values for Noah preferences
    // and try to load them from pref database
    appContext->prefs.startupAction      = startupActionNone;
    appContext->prefs.hwButtonScrollType = scrollPage;
    appContext->prefs.navButtonScrollType = scrollPage;
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
    
    LoadPreferencesNoahPro(appContext);
    
    error=DIA_Init(&appContext->diaSettings);
    if (error) 
        goto OnError;
    
OnError:
    return error;
}

void DictCurrentFree(AppContext* appContext)
{
    AbstractFile* file=GetCurrentFile(appContext);

    if ( NULL != file )
    {
        dictDelete(file);
        FsFileClose( file );
        SetCurrentFile(appContext, NULL);
    }
}

Boolean DictInit(AppContext* appContext, AbstractFile *file)
{
    if ( !FsFileOpen(appContext, file) )
    {
        LogV1( "DictInit(%s), FsFileOpen() failed", file->fileName );
        return false;
    }

    if ( !dictNew(file) )
    {
        LogV1( "DictInit(%s), FsFileOpen() failed", file->fileName );
        return false;
    }

    appContext->wordsCount = dictGetWordsCount(file);
    appContext->currentWord = 0;
    appContext->listItemOffset = 0;
    LogV1( "DictInit(%s) ok", file->fileName );
    return true;
}

Err AppCommonFree(AppContext* appContext)
{
    Err error;

    error=DIA_Free(&appContext->diaSettings);
    Assert(!error);

    DictCurrentFree(appContext);
    FreeDicts(appContext);
    FreeInfoData(appContext);
    FreeHistory(appContext);

    dcDelCacheDb();

    if ( NULL != appContext->prefs.lastDbUsedName )
        new_free( appContext->prefs.lastDbUsedName );
    
    if (appContext->wmpCacheDb) // this way it won't be called in resident mode and won't put PalmOS to it's knees
        CloseMatchingPatternDB(appContext);

    if (appContext->bookmarksDb)
        CloseBookmarksDB(appContext);

    FsDeinit(&appContext->fsSettings);

    return error;
}

Err AppPerformResidentLookup(char* term)
{
    Err             error;
    AppContext*     appContext=(AppContext*)MemPtrNew(sizeof(AppContext));
    AbstractFile *  chosenDb;

    if (!appContext)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    error=AppCommonInit(appContext);
    RemoveNonexistingDatabases(appContext);
    ScanForDictsNoahPro(appContext, false);
    if (0 == appContext->dictsCount)
    {
        FrmAlert(alertNoDB);
        goto Exit;
    }
    else

    if (1 == appContext->dictsCount )
        chosenDb = appContext->dicts[0];
    else 
    {
        // because we can't start resident mode without previously gracefully exiting at least one time
        Assert(appContext->prefs.lastDbUsedName); 
        chosenDb=FindOpenDatabase(appContext, appContext->prefs.lastDbUsedName);
    }            

    if (!chosenDb || !DictInit(appContext, chosenDb))
    {
        FrmAlert(alertDbFailed);
        goto Exit;
    }

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
Exit:
    AppCommonFree(appContext);
OnError:
    if (appContext)
        MemPtrFree(appContext);
    return error;
}


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

