/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "cw_defs.h"

#include "noah_pro.h"
#include "noah_pro_rcp.h"
#include "extensible_buffer.h"
#include "common.h"

#ifdef WN_PRO_DICT
#include "wn_pro_support.h"
#endif

#ifdef WNLEX_DICT
#include "wn_lite_ex_support.h"
#endif

#ifdef EP_DICT
#include "ep_support.h"
#endif

#ifdef SIMPLE_DICT
#include "simple_support.h"
#endif

#define PREF_REC_MIN_SIZE 4+5
#define FONT_DY  11
#define WORDS_IN_LIST 12

static char sa_txt[20];
static char sdb_txt[10];
static char but_txt[20];
static char tap_txt[20];

GlobalData gd;

inline Boolean FIsPrefRecord(void *recData)
{
    long    sig;
    Assert( recData );
    sig = ((long*)recData)[0];
    if ( Noah21Pref == sig )
        return true;
    else
        return false;
}

void DictFoundCBNoahPro( AbstractFile *file )
{
    Assert( file );
    Assert( NOAH_PRO_CREATOR == file->creator );
    Assert( (WORDNET_PRO_TYPE == file->type) ||
            (WORDNET_LITE_TYPE == file->type) ||
            (SIMPLE_TYPE == file->type) ||
            (ENGPOL_TYPE == file->type));

    if (gd.dictsCount>=MAX_DICTS)
    {
        AbstractFileFree(file);
        return;
    }

    gd.dicts[gd.dictsCount++] = file;
}

// Create a blob containing serialized prefernces.
// Devnote: caller needs to free memory returned.
void *SerializePreferencesNoahPro(long *pBlobSize)
{
    char *          prefsBlob;
    long            blobSize;
    long            blobSizePhaseOne;
    int             phase;
    NoahPrefs *     prefs;
    UInt32          prefRecordId = Noah21Pref;
    AbstractFile *  currFile;
    unsigned char   currFilePos;
    int             i;

    Assert( pBlobSize );

    LogG( "SerializePreferencesNoahPro()" );

    prefs = &gd.prefs;
    /* phase one: calculate the size of the blob */
    /* phase two: create the blob */
    prefsBlob = NULL;
    for( phase=1; phase<=2; phase++)
    {
        blobSize = 0;
        Assert( 4 == sizeof(prefRecordId) );
        /* 1. preferences */
        serData( (char*)&prefRecordId, (long)sizeof(prefRecordId), prefsBlob, &blobSize );
        serByte( prefs->fDelVfsCacheOnExit, prefsBlob, &blobSize );
        serByte( prefs->startupAction, prefsBlob, &blobSize );
        serByte( prefs->tapScrollType, prefsBlob, &blobSize );
        serByte( prefs->hwButtonScrollType, prefsBlob, &blobSize );
        serByte( prefs->dbStartupAction, prefsBlob, &blobSize );

        /* 2. number of databases found */
        serByte( gd.dictsCount, prefsBlob, &blobSize );

        /* 3. currently used database */
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

        /* 4. list of databases */
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

        /* 5. last word */
        serString( (char*)gd.prefs.lastWord, prefsBlob, &blobSize);

        /* 6. number of words in the history */
        serInt( gd.historyCount, prefsBlob, &blobSize );

        /* 7. all words in the history */
        for (i=0; i<gd.historyCount; i++)
        {
            serString(gd.wordHistory[i], prefsBlob, &blobSize);
        }

        if ( 1 == phase )
        {
            Assert( blobSize > 0 );
            blobSizePhaseOne = blobSize;
            prefsBlob = (char*)new_malloc( blobSize );
            if (NULL == prefsBlob)
            {
                LogG("SerializePreferencesNoahPro(): prefsBlob==NULL");
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
void DeserilizePreferencesNoahPro(unsigned char *prefsBlob, long blobSize)
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

    LogG( "DeserilizePreferencesNoahPro()" );

    prefs = &gd.prefs;
    /* skip the 4-byte signature of the preferences record */
    Assert( FIsPrefRecord( prefsBlob ) );
    prefsBlob += 4;
    blobSize -= 4;

    /* 1. preferences */
    prefs->fDelVfsCacheOnExit = (Boolean) deserByte( &prefsBlob, &blobSize );
    prefs->startupAction = (StartupAction) deserByte( &prefsBlob, &blobSize );
    prefs->tapScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->hwButtonScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->dbStartupAction = (DatabaseStartupAction) deserByte( &prefsBlob, &blobSize );

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
            DictFoundCBNoahPro( file );
        }

        if (i==currDb)
            prefs->lastDbUsedName = fileName;
        else
            new_free(fileName);
    }

    /* 5. last word */
    deserStringToBuf( (char*)gd.prefs.lastWord, WORD_MAX_LEN, &prefsBlob, &blobSize );
    LogV1( "DeserilizePreferencesNoahPro(), lastWord=%s", gd.prefs.lastWord );

    /* 6. number of words in the history */
    gd.historyCount = deserInt( &prefsBlob, &blobSize );

    /* 7. all words in the history */
    for (i=0; i<gd.historyCount; i++)
    {
        gd.wordHistory[i] = deserString( &prefsBlob, &blobSize );
    }
}

void SavePreferencesNoahPro()
{
    DmOpenRef      db;
    UInt           recNo;
    UInt           recsCount;
    Boolean        fRecFound = false;
    Err            err;
    void *         recData=NULL;
    long           recSize=0;
    MemHandle      recHandle;
    void *         prefsBlob;
    long           blobSize;
    Boolean        fRecordBusy = false;

    prefsBlob = SerializePreferencesNoahPro( &blobSize );
    if ( NULL == prefsBlob ) return;

    db = DmOpenDatabaseByTypeCreator(NOAH_PREF_TYPE, NOAH_PRO_CREATOR, dmModeReadWrite);
    if (!db)
    {
        err = DmCreateDatabase(0, "Noah Prefs", NOAH_PRO_CREATOR,  NOAH_PREF_TYPE, false);
        if ( errNone != err)
        {
            gd.err = ERR_NO_PREF_DB_CREATE;
            return;
        }

        db = DmOpenDatabaseByTypeCreator(NOAH_PREF_TYPE, NOAH_PRO_CREATOR, dmModeReadWrite);
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
        if ( FIsPrefRecord(recData) )
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

void LoadPreferencesNoahPro()
{
    DmOpenRef    db;
    UInt         recNo;
    void *       recData;
    MemHandle    recHandle;
    UInt         recsCount;
    Boolean      fRecFound = false;

    gd.fFirstRun = true;
    db = DmOpenDatabaseByTypeCreator(NOAH_PREF_TYPE, NOAH_PRO_CREATOR, dmModeReadWrite);
    if (!db) return;
    recsCount = DmNumRecords(db);
    for (recNo = 0; (recNo < recsCount) && !fRecFound; recNo++)
    {
        recHandle = DmQueryRecord(db, recNo);
        recData = MemHandleLock(recHandle);
        if ( (MemHandleSize(recHandle)>=PREF_REC_MIN_SIZE) && FIsPrefRecord( recData ) )
        {
            LogG( "LoadPreferencesNoahPro(), found prefs record" );
            fRecFound = true;
            gd.fFirstRun = false;
            DeserilizePreferencesNoahPro((unsigned char*)recData, MemHandleSize(recHandle) );
        }
        MemPtrUnlock(recData);
    }
    DmCloseDatabase(db);
}

Boolean FNoahProDatabase( UInt32 creator, UInt32 type )
{
    if ( NOAH_PRO_CREATOR != creator )
        return false;

    if ( (WORDNET_PRO_TYPE != type) &&
        (WORDNET_LITE_TYPE != type) &&
        (SIMPLE_TYPE != type) &&
        (ENGPOL_TYPE != type))
    {
        return false;
    }
    return true;
}

/* called for every file on the external card */
void VfsFindCbNoahPro( AbstractFile *file )
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
    DictFoundCBNoahPro( fileCopy );
}

Boolean FDatabaseExists(AbstractFile *file)
{
    PdbHeader   hdr;

    Assert( eFS_VFS == file->fsType );

    if ( !FVfsPresent() )
        return false;

    if ( !ReadPdbHeader(file->volRef, file->fileName, &hdr ) )
        return false;
    
    if ( FNoahProDatabase( hdr.creator, hdr.type ) )
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
    
void ScanForDictsNoahPro(Boolean fAlwaysScanExternal)
{
    FsMemFindDb(NOAH_PRO_CREATOR, WORDNET_PRO_TYPE, NULL, &DictFoundCBNoahPro);
    FsMemFindDb(NOAH_PRO_CREATOR, SIMPLE_TYPE, NULL, &DictFoundCBNoahPro);

    /* TODO: show a progress dialog with a number of files processed so far */
    
    // only scan external memory card (slow) if this is the first
    // time we run (don't have the list of databases cached in
    // preferences) or we didn't find any databases at all
    // (unless it was over-written by fAlwaysScanExternal flag
    if (fAlwaysScanExternal || gd.fFirstRun || (0==gd.dictsCount))
        FsVfsFindDb( &VfsFindCbNoahPro );
    LogV1( "ScanForDictsNoahPro(), found %d dicts", gd.dictsCount );
}

Err InitNoahPro(void)
{
    MemSet((void *) &gd, sizeof(GlobalData), 0);
    LogInit( &g_Log, "c:\\noah_pro_log.txt" );

    InitFiveWay();

    SetCurrentFile( NULL );

    gd.err = ERR_NONE;
    gd.prevSelectedWord = 0xfffff;

    gd.firstDispLine = -1;
    gd.prevSelectedWord = -1;
    // disable getting nilEvent
    gd.ticksEventTimeout = evtWaitForever;
#ifdef DEBUG
    gd.currentStressWord = 0;
#endif

    // fill out the default values for Noah preferences
    // and try to load them from pref database
    gd.prefs.fDelVfsCacheOnExit = true;
    gd.prefs.startupAction = startupActionNone;
    gd.prefs.tapScrollType = scrollLine;
    gd.prefs.hwButtonScrollType = scrollPage;
    gd.prefs.dbStartupAction = dbStartupActionAsk;
    gd.prefs.lastDbUsedName = NULL;

    FsInit();

    LoadPreferencesNoahPro();

    if (!CreateHelpData())
        return !errNone;

    return errNone;
}

void DictCurrentFree(void)
{
    if ( NULL != GetCurrentFile() )
    {
        dictDelete();
        FsFileClose( GetCurrentFile() );
        SetCurrentFile(NULL);
    }
}

Boolean DictInit(AbstractFile *file)
{
    if ( !FsFileOpen( file ) )
    {
        LogV1( "DictInit(%s), FsFileOpen() failed", file->fileName );
        return false;
    }

    if ( !dictNew() )
    {
        LogV1( "DictInit(%s), FsFileOpen() failed", file->fileName );
        return false;
    }

    gd.wordsCount = dictGetWordsCount();
    gd.currentWord = 0;
    gd.listItemOffset = 0;
    LogV1( "DictInit(%s) ok", file->fileName );
    return true;
}

void StopNoahPro(void)
{
    SavePreferencesNoahPro();
    DictCurrentFree();
    FreeDicts();
    FreeInfoData();
    FreeHistory();

//    decided to be always true
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
    dh_display_string("Noah Pro", 2, 16);

#ifdef DEMO
    dh_display_string("Ver 2.1 (demo)", 2, 20);
#else
  #ifdef DEBUG
    dh_display_string("Ver 2.1 (debug)", 2, 20);
  #else
    dh_display_string("Ver 2.1", 2, 20);
  #endif
#endif
    dh_display_string("(C) 2000-2003 ArsLexis", 1, 24);
    dh_display_string("http://www.arslexis.com", 2, 40); 
#ifdef DEMO_HANDANGO
    dh_display_string("Buy at: www.handango.com/purchase", 0, 14);
    dh_display_string("        Product ID: 10421", 0, 0);
#endif
#ifdef DEMO_PALMGEAR
    dh_display_string("Buy at: www.palmgear.com?7775", 2, 0);
#endif
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

Boolean MainFormHandleEventNoahPro(EventType * event)
{
    Boolean         handled = false;
    FormType *      frm;
    Short           newValue;
    ListType *      list;
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
            ScanForDictsNoahPro(false);

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
                        Assert(wordNo < gd.wordsCount);
                        DrawDescription(wordNo);
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
                    {
                        DrawDescription(gd.currentWord - 1);
                    }
                    break;
                case ctlArrowRight:
                    if (gd.currentWord < gd.wordsCount - 1)
                    {
                        DrawDescription(gd.currentWord + 1);
                    }
                    break;
                case buttonFind:
                    FrmPopupForm(formDictFind);
                    break;
                case popupHistory:
                    // need to propagate the event down to popus
                    return false;
                    break;
                default:
                    Assert(0);
                    break;
            }
            handled = true;
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
                            SendNewDatabaseSelected( 0 );
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

            // TODO: replace with HistoryListInit(frm) ? 
            if (gd.historyCount > 0)
            {
                list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
                LstSetListChoices(list, NULL, gd.historyCount);
                LstSetDrawFunction(list, HistoryListDrawFunc);
                CtlShowControlEx(frm, popupHistory);
            }
            else
            {
                CtlHideControlEx(frm, popupHistory);
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

#if 0
            r.topLeft.x = event->screenX-5;
            r.topLeft.y = event->screenY-5;
            r.extent.x = 10;
            r.extent.y = 10;
            start_x = event->screenX;
            start_y = event->screenY;
            if (GetCharBounds(event->screenX,event->screenY,&r,&line,&charPos))
            {
                 WinInvertRectangle(&r,0);
            }
#endif
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

#if 0
            r.topLeft.x = event->data.penUp.start.x-5;
            r.topLeft.y = event->data.penUp.start.y-5;
            r.extent.x = 10;
            r.extent.y = 10;
            WinInvertRectangle(&r,0);
            if (GetCharBounds(event->data.penUp.start.x, event->data.penUp.start.y,
                     &r, &line, &charPos))
            {
                WinInvertRectangle(&r,0);
            }

            r.topLeft.x = event->data.penUp.end.x-5;
            r.topLeft.y = event->data.penUp.end.y-5;
            r.extent.x = 10;
            r.extent.y = 10;
            WinInvertRectangle(&r,0);
#endif

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
                case menuItemHelp:
                    DisplayHelp();
                    break;
                case menuItemSelectDB:
                    FrmPopupForm(formSelectDict);
                    break;
                case menuItemDispPrefs:
                    FrmPopupForm(formDisplayPrefs);
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

#if 0
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
                        /* we don't need evtNil events anymore */
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

// Event handler proc for the find word dialog
Boolean FindFormHandleEventNoahPro(EventType * event)
{
    Boolean     handled = false;
    char *      word;
    FormPtr     frm;
    FieldPtr    fld;
    ListPtr     list;

    if (event->eType == nilEvent)
        return true;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case frmOpenEvent:
            fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
            list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
            gd.prevSelectedWord = 0xffffffff;
            LstSetListChoicesEx(list, NULL, dictGetWordsCount());
            LstSetDrawFunction(list, ListDrawFunc);
            gd.prevTopItem = 0;
            gd.selectedWord = 0;
            word = &(gd.lastWord[0]);
            /* force updating the field */
            if (0 == StrLen(word))
            {
                // no word so focus on first word
                LstSetSelectionEx(list, gd.selectedWord);
            }
            else
            {
                FldInsert(fld, word, StrLen(word));
                SendFieldChanged();
            }
            FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWord));
            FrmDrawForm(frm);
            return true;

        case lstSelectEvent:
            /* copy word from text field to a buffer, so next time we
               come back here, we'll come back to the same place */
            RememberLastWord(frm);
            /* set the selected word as current word */
            gd.currentWord = gd.listItemOffset + (UInt32) event->data.lstSelect.selection;
            /* send a msg to yourself telling that a new word
               have been selected so we need to draw the
               description */
            Assert(gd.currentWord < gd.wordsCount);
            SendNewWordSelected();
            FrmReturnToForm(0);
            return true;

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
                    RememberLastWord(frm);
                    gd.currentWord = gd.selectedWord;
                    Assert(gd.currentWord < gd.wordsCount);
                    SendNewWordSelected();
                    FrmReturnToForm(0);
                    return true;

                case pageUpChr:
                    if ( ! (HaveFiveWay() && EvtKeydownIsVirtual(event) && IsFiveWayEvent(event) ) )
                    {
                        ScrollWordListByDx( frm, -WORDS_IN_LIST );
                        return true;
                    }

                case pageDownChr:
                    if ( ! (HaveFiveWay() && EvtKeydownIsVirtual(event) && IsFiveWayEvent(event) ) )
                    {
                        ScrollWordListByDx( frm, WORDS_IN_LIST );
                        return true;
                    }

                default:
                    if ( HaveFiveWay() && EvtKeydownIsVirtual(event) && IsFiveWayEvent(event) )
                    {
                        if (FiveWayCenterPressed(event))
                        {
                            RememberLastWord(frm);
                            gd.currentWord = gd.selectedWord;
                            Assert(gd.currentWord < gd.wordsCount);
                            SendNewWordSelected();
                            FrmReturnToForm(0);
                            return true;
                        }
                    
                        if (FiveWayDirectionPressed( event, Left ))
                        {
                            ScrollWordListByDx( frm, -WORDS_IN_LIST );
                            return true;
                        }
                        if (FiveWayDirectionPressed( event, Right ))
                        {
                            ScrollWordListByDx( frm, WORDS_IN_LIST );
                            return true;
                        }
                        if (FiveWayDirectionPressed( event, Up ))
                        {
                            ScrollWordListByDx( frm, -1 );
                            return true;
                        }
                        if (FiveWayDirectionPressed( event, Down ))
                        {
                            ScrollWordListByDx( frm, 1 );
                            return true;
                        }
                        return false;
                    }
                    break;
            }
            SendFieldChanged();
            return false;

        case fldChangedEvent:
        case evtFieldChanged:
            DoFieldChanged();
            return true;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    RememberLastWord(frm);
                    FrmReturnToForm(0);
                    return true;

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

Boolean SelectDictFormHandleEventNoahPro(EventType * event)
{
    FormPtr frm;
    ListPtr list;
    long    selectedDb;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case frmOpenEvent:
            selectedDb = FindCurrentDbIndex();
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
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
                    list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
                    selectedDb = LstGetSelection(list);
                    if (selectedDb != noListSelection)
                        SendNewDatabaseSelected( selectedDb );
                    FrmReturnToForm(0);
                    break;
                case buttonCancel:
                    Assert( NULL != GetCurrentFile() );
                    FrmReturnToForm(0);
                    break;
                case buttonRescanDicts:
                    DictCurrentFree();
                    FreeDicts();
                    // force scanning external memory for databases
                    ScanForDictsNoahPro(true);
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
                    break;
                default:
                    Assert(0);
                    break;
            }
            return true;
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
#if 0
    CtlSetValue((ControlType *)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, checkDeleteVfs)), gd.prefs.fDelVfsCacheOnExit );
#endif
}

Boolean PrefFormHandleEventNoahPro(EventType * event)
{
    Boolean     handled = true;
    FormType *  frm = NULL;
    ListType *  list = NULL;
    char *      listTxt = NULL;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case frmOpenEvent:
            FrmDrawForm(frm);
            PrefsToGUI(frm);
            return true;

        case popSelectEvent:
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, event->data.popSelect.listID));
            listTxt = LstGetSelectionText(list, event->data.popSelect.selection);
            switch (event->data.popSelect.listID)
            {
                case listStartupAction:
                    gd.prefs.startupAction = (StartupAction) event->data.popSelect.selection;
                    MemMove(sa_txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupStartupAction)),sa_txt);
                    break;
                case listStartupDB:
                    gd.prefs.dbStartupAction = (DatabaseStartupAction) event->data.popSelect.selection;
                    MemMove(sdb_txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, popupStartupDB)),sdb_txt);
                    break;
                case listhwButtonsAction:
                    gd.prefs.hwButtonScrollType = (ScrollType) event->data.popSelect.selection;
                    MemMove(but_txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popuphwButtonsAction)),but_txt);
                    break;
                case listTapAction:
                    gd.prefs.tapScrollType = (ScrollType) event->data.popSelect.selection;
                    MemMove(tap_txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, popupTapAction)),tap_txt);
                    break;
                default:
                    Assert(0);
                    break;
            }
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
                    SavePreferencesNoahPro();
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

Boolean DisplayPrefFormHandleEventNoahPro(EventType * event)
{
    switch (event->eType)
    {
        case frmOpenEvent:
            FrmDrawForm(FrmGetActiveForm());
            return true;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonOk:
                    SavePreferencesNoahPro();
                case buttonCancel:
                    FrmReturnToForm(0);
                    break;
                default:
                    Assert(0);
                    break;
            }
            return true;

        default:
            break;
    }
    return false;
}

Boolean HandleEventNoahPro(EventType * event)
{
    FormType *  frm;
    UInt16      formId;

    if (event->eType == frmLoadEvent)
    {
        formId = event->data.frmLoad.formID;
        frm = FrmInitForm(formId);
        FrmSetActiveForm(frm);

        switch (formId)
        {
            case formDictMain:
                FrmSetEventHandler(frm, MainFormHandleEventNoahPro);
                return true;

            case formDictFind:
                FrmSetEventHandler(frm, FindFormHandleEventNoahPro);
                return true;

            case formSelectDict:
                FrmSetEventHandler(frm, SelectDictFormHandleEventNoahPro);
                return true;

            case formPrefs:
                FrmSetEventHandler(frm, PrefFormHandleEventNoahPro);
                return true;

            case formDisplayPrefs:
                FrmSetEventHandler(frm, DisplayPrefFormHandleEventNoahPro);
                return true;

            default:
                Assert(0);
                break;
        }
    }
    return false;
}

void EventLoopNoahPro(void)
{
    EventType event;
    Word      error;

    event.eType = (eventsEnum) 0;
    while ( (event.eType != appStopEvent) && (ERR_NONE == gd.err) )
    {
        EvtGetEvent(&event, gd.ticksEventTimeout);
        if (SysHandleEvent(&event))
            continue;
        // according to docs error is always set to 0
        if (MenuHandleEvent(0, &event, &error))
            continue;
        if (HandleEventNoahPro(&event))
            continue;
        FrmDispatchEvent(&event);
    }
#if 0
    switch( gd.err )
    {
        case ERR_NONE:
        case ERR_SILENT:
            // do nothing
            break;
        case ERR_NO_MEM:
            FrmAlert( alertMemError );
            break;
        default:
            FrmAlert( alertGenericError );
    }
#endif
}

DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Err     err;

    switch (cmd)
    {
        case sysAppLaunchCmdNormalLaunch:
            err = InitNoahPro();
            if ( errNone != err )
                return err;
            FrmGotoForm(formDictMain);
            EventLoopNoahPro();
            StopNoahPro();
            break;
        default:
            break;
    }
    return 0;
}

