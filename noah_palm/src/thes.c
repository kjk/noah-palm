/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include "cw_defs.h"

#include "thes.h"
#include "display_info.h"
#include "roget_support.h"

#define PREF_REC_MIN_SIZE 4
#define FONT_DY  11
#define WORDS_IN_LIST 15

#ifdef NEVER
char helpText[] =
    " This is a demo version. It's fully\n functional but has only 10% of\n the thesaurus data.\n" \
    " Go to www.arslexis.com to get the\n full version and find out about\n latest developments.\n";
#endif

static char sa_txt[20];
static char sdb_txt[10];
static char but_txt[20];
static char tap_txt[20];

GlobalData gd;

inline Boolean FIsThesPrefRecord(void *recData)
{
    long    sig;
    Assert( recData );
    sig = ((long*)recData)[0];
    return (Thes11Pref == sig) ? true : false;
}

void DictFoundCBThes( AbstractFile *file )
{
    Assert( file );
    Assert( THES_CREATOR == file->creator );
    Assert( ROGET_TYPE == file->type );
    if (gd.dictsCount>=MAX_DICTS)
    {
        AbstractFileFree(file);
        return;
    }

    gd.dicts[gd.dictsCount++] = file;
}

// Create a blob containing serialized preferences.
// Caller needs to free returned memory
void *SerializePreferencesThes(long *pBlobSize)
{
    char *          prefsBlob;
    long            blobSize;
    long            blobSizePhaseOne;
    int             phase;
    ThesPrefs *     prefs;
    UInt32          prefRecordId = Thes11Pref;
    int             i;
    unsigned char   currFilePos;
    AbstractFile *  currFile;

    Assert( pBlobSize );

    LogG( "SerializePreferencesThes()" );

    prefs = &gd.prefs;
    /* phase one: calculate the size of the blob */
    /* phase two: create the blob */
    prefsBlob = NULL;
    for( phase=1; phase<=2; phase++)
    {
        blobSize = 0;
        Assert( 4 == sizeof(prefRecordId) );
        // 1. preferences
        serData( (char*)&prefRecordId, (long)sizeof(prefRecordId), prefsBlob, &blobSize );
        serByte( prefs->fDelVfsCacheOnExit, prefsBlob, &blobSize );
        serByte( prefs->startupAction, prefsBlob, &blobSize );
        serByte( prefs->tapScrollType, prefsBlob, &blobSize );
        serByte( prefs->hwButtonScrollType, prefsBlob, &blobSize );
        serByte( prefs->dbStartupAction, prefsBlob, &blobSize );

        // 2. number of databases found      
        serByte( gd.dictsCount, prefsBlob, &blobSize );

        // 3. currently used database
        currFilePos = 0xff;
        for (i=0; i<gd.dictsCount; i++)
        {
            if( GetCurrentFile() == gd.dicts[i] )
            {
                currFilePos = (unsigned char)i;
                break;
            }
        }
        serByte( currFilePos, prefsBlob, &blobSize );

        // 4. list of databases
        // note: we don't really need to store databases from eFS_MEM because
        // we rescan them anyway, but this is easier to code
        for(i=0; i<gd.dictsCount; i++)
        {
            currFile = gd.dicts[i];
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
        serString( (char*)gd.prefs.lastWord, prefsBlob, &blobSize );

        // 6. number of words in the history
        serInt( gd.historyCount, prefsBlob, &blobSize );

        // 7. all words in the history
        for (i=0; i<gd.historyCount; i++)
        {
            serString( gd.wordHistory[i], prefsBlob, &blobSize );
        }

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


// Given a blob containing serialized prefrences deserilize the blob
// and set the preferences accordingly.
void DeserilizePreferencesThes(unsigned char *prefsBlob, long blobSize)
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

    LogG( "DeserilizePreferencesThes()" );

    prefs = &gd.prefs;
    /* skip the 4-byte signature of the preferences record */
    Assert( FIsThesPrefRecord( prefsBlob ) );
    prefsBlob += 4;
    blobSize -= 4;

    // 1. preferences
    prefs->fDelVfsCacheOnExit = (Boolean) deserByte( &prefsBlob, &blobSize );
    prefs->startupAction = (StartupAction) deserByte( &prefsBlob, &blobSize );
    prefs->tapScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->hwButtonScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->dbStartupAction = (DatabaseStartupAction) deserByte( &prefsBlob, &blobSize );

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
            DictFoundCBThes( file );
        }

        if (i==currDb)
            prefs->lastDbUsedName = fileName;
        else
            new_free(fileName);
    }

    /// 5. last word
    deserStringToBuf( (char*)gd.prefs.lastWord, WORD_MAX_LEN, &prefsBlob, &blobSize );
    LogV1( "DeserilizePreferencesThes(), lastWord=%s", gd.prefs.lastWord );

    // 6. number of words in the history
    gd.historyCount = deserInt( &prefsBlob, &blobSize );

    // 7. all words in the history
    for (i=0; i<gd.historyCount; i++)
    {
        gd.wordHistory[i] = deserString( &prefsBlob, &blobSize );
    }
}

void SavePreferencesThes()
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

    prefsBlob = SerializePreferencesThes( &blobSize );
    if ( NULL == prefsBlob ) return;

    db = DmOpenDatabaseByTypeCreator(THES_PREF_TYPE, THES_CREATOR, dmModeReadWrite);
    if (!db)
    {
        err = DmCreateDatabase(0, "Thes Prefs", THES_CREATOR,  THES_PREF_TYPE, false);
        if ( errNone != err)
        {
            gd.err = ERR_NO_PREF_DB_CREATE;
            return;
        }

        db = DmOpenDatabaseByTypeCreator(THES_PREF_TYPE, THES_CREATOR, dmModeReadWrite);
        if (!db)
        {
            gd.err = ERR_NO_PREF_DB_OPEN;
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
        if ( FIsThesPrefRecord(recData) )
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

void LoadPreferencesThes()
{
    DmOpenRef    db;
    UInt         recNo;
    void *       recData;
    MemHandle    recHandle;
    UInt         recsCount;
    Boolean      fRecFound = false;

    gd.fFirstRun = true;
    db = DmOpenDatabaseByTypeCreator(THES_PREF_TYPE, THES_CREATOR, dmModeReadWrite);
    if (!db) return;
    recsCount = DmNumRecords(db);
    for (recNo = 0; (recNo < recsCount) && !fRecFound; recNo++)
    {
        recHandle = DmQueryRecord(db, recNo);
        recData = MemHandleLock(recHandle);
        if ( (MemHandleSize(recHandle)>=PREF_REC_MIN_SIZE) && FIsThesPrefRecord( recData ) )
        {
            LogG( "LoadPreferencesThes(), found prefs record" );
            fRecFound = true;
            gd.fFirstRun = false;
            DeserilizePreferencesThes((unsigned char*)recData, MemHandleSize(recHandle) );
        }
        MemPtrUnlock(recData);
    }
    DmCloseDatabase(db);
}

Boolean FThesDatabase( UInt32 creator, UInt32 type )
{
    if ( THES_CREATOR != creator )
        return false;

    if ( ROGET_TYPE != type )
        return false;

    return true;
}

/* called for every file on the external card */
void VfsFindCbThes( AbstractFile *file )
{
    AbstractFile *fileCopy;

    /* UNDONE: update progress dialog with a number of files processed */

    if ( !FThesDatabase(file->creator, file->type) )
        return;

    fileCopy = AbstractFileNewFull( file->fsType, file->creator, file->type, file->fileName );
    if ( NULL == fileCopy ) return;
    fileCopy->volRef = file->volRef;
    DictFoundCBThes( fileCopy );
}

Boolean FDatabaseExists(AbstractFile *file)
{
    PdbHeader   hdr;

    Assert( eFS_VFS == file->fsType );

    if ( !FVfsPresent() )
        return false;

    if ( !ReadPdbHeader(file->volRef, file->fileName, &hdr ) )
        return false;
    
    if ( FThesDatabase( hdr.creator, hdr.type ) )
        return true;
    else
        return false;
}

void RemoveNonexistingDatabases(void)
{
    int i;
    // if we got a list of databases on eFS_VFS from preferences we need
    // to verify that those databases still exist
    for(i=0; i<gd.dictsCount; i++)
    {
        if ( !FDatabaseExists(gd.dicts[i]) )
        {
            AbstractFileFree( gd.dicts[i] );
            MemMove( &(gd.dicts[i]), &(gd.dicts[i+1]), (gd.dictsCount-i-1)*sizeof(gd.dicts[0]) );
            --gd.dictsCount;
        }
    }
}

void ScanForDictsThes(Boolean fAlwaysScanExternal)
{
    FsMemFindDb( THES_CREATOR, ROGET_TYPE, NULL, &DictFoundCBThes );

    /* TODO: show a progress dialog with a number of files processed so far */

    // only scan external memory card (slow) if this is the first
    // time we run (don't have the list of databases cached in
    // preferences) or we didn't find any databases at all
    // (unless it was over-written by fAlwaysScanExternal flag
    if (fAlwaysScanExternal || gd.fFirstRun || (0==gd.dictsCount))
        FsVfsFindDb( &VfsFindCbThes );
}

Err InitThesaurus(void)
{
    MemSet((void *) &gd, sizeof(GlobalData), 0);
    LogInit( &g_Log, "c:\\thes_log.txt" );

    SetCurrentFile( NULL );

    // disable getting nilEvent
    gd.ticksEventTimeout = evtWaitForever;
#ifdef DEBUG
    gd.currentStressWord = 0;
#endif
    gd.prevSelectedWord = 0xfffff;

    gd.firstDispLine = -1;
    gd.prevSelectedWord = -1;

    // fill out the default values for Thesaurus preferences
    // and try to load them from pref database
    gd.prefs.fDelVfsCacheOnExit = true;
    gd.prefs.startupAction = startupActionNone;
    gd.prefs.tapScrollType = scrollLine;
    gd.prefs.hwButtonScrollType = scrollPage;
    gd.prefs.dbStartupAction = dbStartupActionAsk;
    gd.prefs.lastDbUsedName = NULL;

    FsInit();

    LoadPreferencesThes();

    if (!CreateHelpData())
        return !errNone;

    return errNone;
}

Boolean DictInit(AbstractFile *file)
{
    if ( !FsFileOpen( file ) )
        return false;

    if ( !dictNew() )
        return false;

    gd.wordsCount = dictGetWordsCount();
    gd.currentWord = 0;
    gd.listItemOffset = 0;
    LogV1( "DictInit(%s) ok", file->fileName );
    return true;
}

void DictCurrentFree(void)
{

#if 0
    int i;
    // UNDONE: need to fix that to be dictionary-specific instead of global
    for (i = 0; i < gd.recordsCount; i++)
    {
        if (0 != gd.recsInfo[i].lockCount)
        {
            Assert(0);
        }
    }
#endif

    if ( NULL != GetCurrentFile() )
    {
        dictDelete();
        FsFileClose( GetCurrentFile() );
        SetCurrentFile(NULL);
    }
}

void StopThesaurus()
{
    SavePreferencesThes();
    DictCurrentFree();
    FreeDicts();
    FreeInfoData();
    FreeHistory();

//  decided to make it always true
//    if ( gd.prefs.fDelVfsCacheOnExit)
        dcDelCacheDb();

    if ( NULL != gd.prefs.lastDbUsedName )
        new_free( gd.prefs.lastDbUsedName );

    FrmSaveAllForms();
    FrmCloseAllForms();
    FsDeinit();
}

void DisplayAbout(void)
{
    ClearDisplayRectangle();
    HideScrollbar();

    dh_set_current_y(7);

    dh_save_font();
    dh_display_string("ArsLexis Thesaurus", 2, 16);
#ifdef DEMO
    dh_display_string("Ver 1.2 DEMO", 2, 20);
#else
  #ifdef DEBUG
    dh_display_string("Ver 1.2 (debug)", 2, 20);
  #else
    dh_display_string("Ver 1.2 (beta)", 2, 20);
  #endif
#endif
    dh_display_string("(C) 2000-2003 ArsLexis", 1, 24);
    dh_display_string("http://www.arslexis.com", 2, 0); 
    
    dh_restore_font();
}

void DoWord(char *word)
{
    long wordNo;

    LogV1( "DoWord(%s)", word );
    Assert( word );
    wordNo = dictGetFirstMatching(word);
    DrawDescription(wordNo);
}

/*
Given a point on the screen calculate the bounds of the character that 
this point belongs to and also line & position in the line of this character.
Returns true if there is a char that falls within, false otherwise.
*/
Boolean GetCharBounds(UInt16 x, UInt16 y, RectangleType * r, int *line, int *charPos)
{
    DisplayInfo *di = NULL;
    int lineOnScreen;

    Assert(r);
    Assert(line);
    Assert(charPos);

    di = gd.currDispInfo;
    if (NULL == di)
        return false;

    lineOnScreen = y / FONT_DY;    /* should be font height */
    r->topLeft.x = 0;
    r->topLeft.y = lineOnScreen * FONT_DY;
    r->extent.x = 120;
    r->extent.y = FONT_DY;

    *line = lineOnScreen;
    *charPos = 0;

    return true;
}

Boolean MainFormHandleEventThes(EventType * event)
{
    Boolean         handled = false;
    FormType *      frm;
    Short           newValue;
    ListType *      list=NULL;
    long            wordNo;
    int             i;
    int             selectedDb;
    AbstractFile *  fileToOpen;
    char *          lastDbUsedName;
    char *          word;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case frmUpdateEvent:
            LogG( "mainFrm - frmUpdateEvent" );
            RedrawMainScreen();
            handled = true;
            break;

        case frmOpenEvent:
            FrmDrawForm(frm);
            HistoryListInit(frm);

            RemoveNonexistingDatabases();
            ScanForDictsThes(false);

            if (0 == gd.dictsCount)
            {
                FrmAlert(alertNoDB);
                SendStopEvent();
                return true;
            }

ChooseDatabase:
            fileToOpen = NULL;
            if (1 == gd.dictsCount )
                fileToOpen = gd.dicts[0];
            else
            {
                lastDbUsedName = gd.prefs.lastDbUsedName;
                if ( NULL != lastDbUsedName )
                {
                    LogV1( "db name from prefs: %s", lastDbUsedName );
                }
                else
                {
                    LogG( "no db name from prefs" );
                }

                if ( (NULL != lastDbUsedName) &&
                     (dbStartupActionLast == gd.prefs.dbStartupAction) )
                {
                    for( i=0; i<gd.dictsCount; i++)
                    {
                        if (0==StrCompare( lastDbUsedName, gd.dicts[i]->fileName ) )
                        {
                            fileToOpen = gd.dicts[i];
                            LogV1( "found db=%s", fileToOpen->fileName );
                            break;
                        }
                    }
                }
            }

            if (NULL == fileToOpen)
            {
                // ask the user which database to use
                FrmPopupForm(formSelectDict);
                return true;
            }
            else
            {
                if ( !DictInit(fileToOpen) )
                {
                    // failed to initialize dictionary. If we have more - retry,
                    // if not - just quit
                    if ( gd.dictsCount > 1 )
                    {
                        i = 0;
                        while ( fileToOpen != gd.dicts[i] )
                        {
                            ++i;
                            Assert( i<gd.dictsCount );
                        }
                        AbstractFileFree( gd.dicts[i] );
                        while ( i<gd.dictsCount )
                        {
                            gd.dicts[i] = gd.dicts[i+1];
                            ++i;
                        }
                        --gd.dictsCount;
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
            WinDrawLine(0, 145, 160, 145);
            WinDrawLine(0, 144, 160, 144);

            if ( startupActionClipboard == gd.prefs.startupAction )
            {
                if (!FTryClipboard())
                    DisplayAbout();
            }
            else
                DisplayAbout();

            if ( (startupActionLast == gd.prefs.startupAction) &&
                gd.prefs.lastWord[0] )
            {
                DoWord( (char *)gd.prefs.lastWord );
            }
            handled = true;
            break;

        case popSelectEvent:
            switch (event->data.popSelect.listID)
            {
                case listHistory:
                    word = gd.wordHistory[event->data.popSelect.selection];
                    wordNo = dictGetFirstMatching(word);
                    if (wordNo != gd.currentWord)
                    {
                        gd.currentWord = wordNo;
                        Assert(wordNo < gd.wordsCount);
                        DrawDescription(gd.currentWord);
                        gd.penUpsToConsume = 1;
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
                    if (gd.currentWord > 0)
                        DrawDescription(gd.currentWord - 1);
                    handled = true;
                    break;
                case ctlArrowRight:
                    if (gd.currentWord < gd.wordsCount - 1)
                        DrawDescription(gd.currentWord + 1);
                    handled = true;
                    break;
                case buttonFind:
                    FrmPopupForm(formDictFind);
                    handled = true;
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
            AddToHistory(gd.currentWord);
            HistoryListSetState(frm);

            WinDrawLine(0, 145, 160, 145);
            WinDrawLine(0, 144, 160, 144);
            DrawDescription(gd.currentWord);
            gd.penUpsToConsume = 1;
            handled = true;
            break;

        case evtNewDatabaseSelected:
            selectedDb = EvtGetInt( event );
            fileToOpen = gd.dicts[selectedDb];
            if ( GetCurrentFile() != fileToOpen )
            {
                DictCurrentFree();
                if ( !DictInit(fileToOpen) )
                {
                    // failed to initialize dictionary. If we have more - retry,
                    // if not - just quit
                    if ( gd.dictsCount > 1 )
                    {
                        i = 0;
                        while ( fileToOpen != gd.dicts[i] )
                        {
                            ++i;
                            Assert( i<gd.dictsCount );
                        }
                        AbstractFileFree( gd.dicts[i] );
                        while ( i<gd.dictsCount )
                        {
                            gd.dicts[i] = gd.dicts[i+1];
                            ++i;
                        }
                        --gd.dictsCount;
                        list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
                        LstSetListChoices(list, NULL, gd.dictsCount);
                        if ( gd.dictsCount > 1 )
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

            RedrawMainScreen();

            if ( startupActionClipboard == gd.prefs.startupAction )
            {
                if (!FTryClipboard())
                    DisplayAbout();
            }
            else
                DisplayAbout();

            if ( (startupActionLast == gd.prefs.startupAction) &&
                gd.prefs.lastWord[0] )
            {
                DoWord( (char *)gd.prefs.lastWord );
            }

            handled = true;
            break;

        case keyDownEvent:
            if (pageUpChr == event->data.keyDown.chr)
            {
                DefScrollUp(gd.prefs.hwButtonScrollType);
            }
            else if (pageDownChr == event->data.keyDown.chr)
            {
                DefScrollDown(gd.prefs.hwButtonScrollType);
            }
            else if (((event->data.keyDown.chr >= 'a')  && (event->data.keyDown.chr <= 'z'))
                     || ((event->data.keyDown.chr >= 'A') && (event->data.keyDown.chr <= 'Z'))
                     || ((event->data.keyDown.chr >= '0') && (event->data.keyDown.chr <= '9')))
            {
                gd.lastWord[0] = event->data.keyDown.chr;
                gd.lastWord[1] = 0;
                FrmPopupForm(formDictFind);
            }
            handled = true;
            break;

        case sclExitEvent:
            newValue = event->data.sclRepeat.newValue;
            if (newValue != gd.firstDispLine)
            {
                ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
                gd.firstDispLine = newValue;
                DrawDisplayInfo(gd.currDispInfo, gd.firstDispLine, DRAW_DI_X, DRAW_DI_Y, DRAW_DI_LINES);
            }
            handled = true;
            break;

        case penDownEvent:
            if ((NULL == gd.currDispInfo) || (event->screenX > 150) || (event->screenY > 144))
            {
                handled = false;
                break;
            }

    /*        r.topLeft.x = event->screenX-5; */
    /*        r.topLeft.y = event->screenY-5; */
    /*        r.extent.x = 10; */
    /*        r.extent.y = 10; */
    /*       start_x = event->screenX; */
    /*       start_y = event->screenY; */
    /*       if (GetCharBounds(event->screenX,event->screenY,&r,&line,&charPos)) */
    /*       { */
    /*            WinInvertRectangle(&r,0); */
    /*       } */

            handled = true;
            break;

        case penMoveEvent:

            handled = true;
            break;

        case penUpEvent:
            if ((NULL == gd.currDispInfo) || (event->screenX > 150) || (event->screenY > 144))
            {
                handled = false;
                break;
            }

    /*        r.topLeft.x = event->data.penUp.start.x-5; */
    /*        r.topLeft.y = event->data.penUp.start.y-5; */
    /*        r.extent.x = 10; */
    /*        r.extent.y = 10; */
    /*        WinInvertRectangle(&r,0); */
    /*       if (GetCharBounds(event->data.penUp.start.x, event->data.penUp.start.y, */
    /*                 &r, &line, &charPos)) */
    /*       { */
    /*            WinInvertRectangle(&r,0); */
    /*       } */

    /*        r.topLeft.x = event->data.penUp.end.x-5; */
    /*        r.topLeft.y = event->data.penUp.end.y-5; */
    /*        r.extent.x = 10; */
    /*        r.extent.y = 10; */
    /*        WinInvertRectangle(&r,0); */

            if (0 != gd.penUpsToConsume)
            {
                --gd.penUpsToConsume;
                handled = true;
                break;
            }

            if (event->screenY > (144 / 2))
            {
                DefScrollDown(gd.prefs.tapScrollType);
            }
            else
            {
                DefScrollUp(gd.prefs.tapScrollType);
            }
            handled = true;
            break;

        case menuEvent:
            switch (event->data.menu.itemID)
            {
                case menuItemFind:
                    FrmPopupForm(formDictFind);
                    break;
                case menuItemAbout:
                    if (NULL != gd.currDispInfo)
                    {
                        diFree(gd.currDispInfo);
                        gd.currDispInfo = NULL;
                        gd.currentWord = 0;
                    }
                    DisplayAbout();
                    break;
                case menuItemSelectDB:
                    FrmPopupForm(formSelectDict);
                    break;
                case menuItemHelp:
                    DisplayHelp();
                    break;
                case menuItemCopy:
                    if (NULL != gd.currDispInfo)
                        diCopyToClipboard(gd.currDispInfo);
                    break;
                case menuItemTranslate:
                    FTryClipboard();
                    break;
#ifdef DEBUG
                case menuItemStress:
                    // initiate stress i.e. going through all the words
                    // 0 means that stress is not in progress
                    gd.currentStressWord = -1;
                    // get nilEvents as fast as possible
                    gd.ticksEventTimeout = 0;
                    break;
#endif
                case menuItemPrefs:
                    FrmPopupForm(formPrefs);
                    break;
                default:
                    Assert(0);
                    break;
            }
            handled = true;
            break;
        case nilEvent:
#ifdef DEBUG
            if ( 0 != gd.currentStressWord )
            {
                if ( -1 == gd.currentStressWord )
                    gd.currentStressWord = 0;
                DrawDescription(gd.currentStressWord++);
                if (gd.currentStressWord==dictGetWordsCount())
                {
                    // disable running the stress
                    gd.currentStressWord = 0;
                    // disable getting nilEvent
                    gd.ticksEventTimeout = evtWaitForever;
                }
            }
#endif
            handled = true;
            break;

#ifdef NEVER
        case nilEvent:
            if (-1 != gd.start_seconds_count)
            {
                /* we're still displaying About info, check
                   if it's time to switch to info */
                Assert(gd.start_seconds_count <= TimGetSeconds());
                if (NULL == gd.currDispInfo)
                {
                    if (TimGetSeconds() - gd.start_seconds_count > 5)
                    {
                        DisplayHelp();
                        /* we don't nid evtNil events anymore */
                        gd.start_seconds_count = -1;
                        gd.current_timeout = -1;
                    }
                }
                else
                {
                    gd.start_seconds_count = -1;
                    gd.current_timeout = -1;
                }
            }
            handled = true;
            break;
#endif
        default:
            break;
    }
    return handled;
}


Boolean FindFormHandleEventThes(EventType * event)
{
    Boolean     handled = false;
    char *      word;
    FormPtr     frm;
    FieldPtr    fld;
    ListPtr     list;
    long        newSelectedWord;

    if (event->eType == nilEvent)
        return true;

    switch (event->eType)
    {
        case frmOpenEvent:
            frm = FrmGetActiveForm();
            fld =(FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
            gd.prevSelectedWord = 0xffffffff;
            LstSetListChoicesEx(list, NULL, dictGetWordsCount());
            LstSetDrawFunction(list, ListDrawFunc);
            gd.prevTopItem = 0;
            gd.selectedWord = 0;
            Assert(gd.selectedWord < gd.wordsCount);
            word = &(gd.lastWord[0]);
            /* force updating the field */
            if (word[0])
            {
                FldInsert(fld, word, StrLen(word));
                SendFieldChanged();
            }
            else
            {
                LstSetSelectionEx(list, gd.selectedWord);
            }
            FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWord));
            FrmDrawForm(frm);
            handled = true;
            break;

        case lstSelectEvent:
            /* copy word from text field to a buffer, so next time we
               come back here, we'll come back to the same place */
            RememberLastWord(FrmGetActiveForm());
            /* set the selected word as current word */
            gd.currentWord = gd.listItemOffset + (UInt32) event->data.lstSelect.selection;
            /* send a msg to yourself telling that a new word
               have been selected so we need to draw the
               description */
            Assert(gd.currentWord < gd.wordsCount);
            SendNewWordSelected();
            handled = true;
            FrmReturnToForm(0);
            break;

        case keyDownEvent:
            /* kind of ugly trick: there is no event that says, that
               text field has changed, so I generate this event myself
               every time I get keyDownEvent (since I assume, that it
               comes when field has focus). To be more correct I should
               store the old content of field and compare it with a new
               content and do stuff only when they differ */
            switch (event->data.keyDown.chr)
            {
                case returnChr:
                case linefeedChr:
                    RememberLastWord(FrmGetActiveForm());
                    gd.currentWord = gd.selectedWord;
                    Assert(gd.currentWord < gd.wordsCount);
                    SendNewWordSelected();
                    FrmReturnToForm(0);
                    return true;
                    break;
                case pageUpChr:
                    frm = FrmGetActiveForm();
                    fld =(FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,fieldWord));
                    list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,listMatching));
                    if (gd.selectedWord > WORDS_IN_LIST)
                    {
                        gd.selectedWord -= WORDS_IN_LIST;
                        LstSetSelectionMakeVisibleEx(list, gd.selectedWord);
                        return true;
                    }
                    break;
                case pageDownChr:
                    frm = FrmGetActiveForm();
                    fld =(FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,fieldWord));
                    list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
                    if (gd.selectedWord + WORDS_IN_LIST < gd.wordsCount)
                    {
                        gd.selectedWord += WORDS_IN_LIST;
                        LstSetSelectionMakeVisibleEx(list, gd.selectedWord);
                        return true;
                    }
                    break;
                default:
                    break;
            }
            SendFieldChanged();
            handled = false;
            break;
        case fldChangedEvent:
        case evtFieldChanged:
            frm = FrmGetActiveForm();
            fld =(FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,listMatching));
            word = FldGetTextPtr(fld);
            newSelectedWord = 0;
            if (word && *word)
            {
                newSelectedWord = dictGetFirstMatching(word);
            }
            if (gd.selectedWord != newSelectedWord)
            {
                gd.selectedWord = newSelectedWord;
                Assert(gd.selectedWord < gd.wordsCount);
                LstSetSelectionMakeVisibleEx(list, gd.selectedWord);
            }
            handled = true;
            break;
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    RememberLastWord(FrmGetActiveForm());
                    FrmReturnToForm(0);
                    handled = true;
                    break;
                default:
                    Assert(0);
                    break;
            }
            break;
        default:
            break;
    }
    return handled;
}

Boolean SelectDictFormHandleEventThes(EventType * event)
{
    FormPtr     frm;
    ListPtr     list;
    long        selectedDb;

    switch (event->eType)
    {
        case frmOpenEvent:
            selectedDb = FindCurrentDbIndex();
            frm = FrmGetActiveForm();
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
            LstSetDrawFunction(list, ListDbDrawFunc);
            LstSetListChoices(list, NULL, gd.dictsCount);
            LstSetSelection(list, selectedDb);
            LstMakeItemVisible(list, selectedDb);
            if (NULL == GetCurrentFile())
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
                    Assert( NULL != GetCurrentFile() );
                    FrmReturnToForm(0);
                    return true;

                case buttonRescanDicts:
                    DictCurrentFree();
                    FreeDicts();
                    // force scanning external memory for databases
                    ScanForDictsThes(true);
                    frm = FrmGetActiveForm();
                    list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
                    LstSetListChoices(list, NULL, gd.dictsCount);
                    if (0==gd.dictsCount)
                    {
                        FrmHideObject(frm, FrmGetObjectIndex(frm, buttonSelect));
                        FrmShowObject(frm, FrmGetObjectIndex(frm, buttonCancel));
                    }
                    else
                    {
                        FrmShowObject(frm, FrmGetObjectIndex(frm, buttonSelect));
                        if (NULL == GetCurrentFile())
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

void PrefsToGUI(FormType * frm)
{
    Assert(frm);
    SetPopupLabel(frm, listStartupAction, popupStartupAction, gd.prefs.startupAction, (char *) sa_txt);
    SetPopupLabel(frm, listStartupDB, popupStartupDB, gd.prefs.dbStartupAction, (char *) sdb_txt);
    SetPopupLabel(frm, listhwButtonsAction, popuphwButtonsAction, gd.prefs.hwButtonScrollType, (char *) but_txt);
    SetPopupLabel(frm, listTapAction, popupTapAction, gd.prefs.tapScrollType, (char *) tap_txt);
//    CtlSetValue((ControlType *)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, checkDeleteVfs)), gd.prefs.fDelVfsCacheOnExit );
}

Boolean PrefFormHandleEventThes(EventType * event)
{
    Boolean     handled = false;
    FormType *  frm = NULL;
    ListType *  list = NULL;
    char *      listTxt = NULL;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case frmOpenEvent:
            PrefsToGUI(frm);
            FrmDrawForm(frm);
            handled = true;
            break;

        case popSelectEvent:
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,event->data.popSelect.listID));
            listTxt = LstGetSelectionText(list, event->data.popSelect.selection);
            switch (event->data.popSelect.listID)
            {
                case listStartupAction:
                    gd.prefs.startupAction = (StartupAction) event->data.popSelect.selection;
                    MemMove(sa_txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, popupStartupAction)), sa_txt);
                    break;
                case listStartupDB:
                    gd.prefs.dbStartupAction = event->data.popSelect.selection;
                    MemMove(sdb_txt,listTxt,StrLen(listTxt)+1);
                    CtlSetLabel(FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupStartupDB)),sdb_txt);
                    break;
                case listhwButtonsAction:
                    gd.prefs.hwButtonScrollType = (ScrollType) event->data.popSelect.selection;
                    MemMove(but_txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, popuphwButtonsAction)), but_txt);
                    break;
                case listTapAction:
                    gd.prefs.tapScrollType = (ScrollType) event->data.popSelect.selection;
                    MemMove(tap_txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, popupTapAction)), tap_txt);
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
                case popupStartupAction:
                case popupStartupDB:
                case popuphwButtonsAction:
                case popupTapAction:
                    // need to propagate the event down to popus
                    handled = false;
                    break;
                case buttonOk:
                    SavePreferencesThes();
                    // pass through
                case buttonCancel:
                    FrmReturnToForm(0);
                    handled = true;
                    break;
#if 0
                case checkDeleteVfs:
                    gd.prefs.fDelVfsCacheOnExit = CtlGetValue((ControlType *)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, checkDeleteVfs)));
                    break;
#endif
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

Boolean HandleEventThes(EventType * event)
{
    FormPtr frm;
    UInt16 formId;
    Boolean handled = false;

    if (event->eType == frmLoadEvent)
    {
        formId = event->data.frmLoad.formID;
        frm = FrmInitForm(formId);
        FrmSetActiveForm(frm);

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
        default:
            Assert(0);
            handled = false;
            break;
        }
    }
    return handled;
}

void EventLoopThes(void)
{
    EventType event;
    Word error;

    event.eType = (eventsEnum) 0;
    while (event.eType != appStopEvent)
    {
        EvtGetEvent(&event, gd.ticksEventTimeout);
        if (SysHandleEvent(&event))
            continue;
        if (MenuHandleEvent(0, &event, &error))
            continue;
        if (HandleEventThes(&event))
            continue;
        FrmDispatchEvent(&event);
    }
}

DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Err err;

    switch (cmd)
    {
    case sysAppLaunchCmdNormalLaunch:
        err = InitThesaurus();
        if ( errNone != err )
            return err;
        FrmGotoForm(formDictMain);
        EventLoopThes();
        StopThesaurus();
        break;
    default:
        break;
    }

    return 0;
}

