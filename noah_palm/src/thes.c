/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include "cw_defs.h"

#include "thes.h"
#include "display_info.h"
#include "roget_support.h"

#ifdef DEMO
char helpText[] =
    " This is a demo version. It's fully\n functional but has only 10% of\n the thesaurus data.\n"
    \
    " Go to www.arslexis.com to get the\n full version and find out about\n latest developments.";
#endif

#ifndef DEMO
char helpText[] =
    "Instructions:\n\255 to lookup a definition of a word\n  press the spyglass icon in the right\n  lower corner and select the word\n\255 you can scroll the definition using\n  hardware buttons, tapping on the\n  screen or using a scrollbar\n\255 left/right arrow moves to next\n  or previous word\n\255 for more information go to\n  WWW.ARSLEXIS.COM";
#endif

GlobalData gd;

Boolean FIsThesPrefRecord(void *recData)
{
    long    sig;
    Assert( recData );
    sig = ((long*)recData)[0];
    return (Thes11Pref == sig) ? true : false;
}


/* Create a blob containing 
caller needs to free memory returned
*/
void *GetSerializedPreferencesThes(long *pBlobSize)
{
    void *      prefsBlob;
    long        blobSize;
    long        blobSizePhaseOne;
    int         phase;
    ThesPrefs   *prefs;
    UInt32      prefRecordId = Thes11Pref;
    int         wordLen;
    int         i;

    Assert( pBlobSize );

    LogG( "GetSerializedPreferencesThes()" );

    prefs = &gd.prefs;
    /* phase one: calculate the size of the blob */
    /* phase two: create the blob */
    prefsBlob = NULL;
    for( phase=1; phase<=2; phase++)
    {
        blobSize = 0;
        Assert( 4 == sizeof(prefRecordId) );
        serData( (char*)&prefRecordId, sizeof(prefRecordId), prefsBlob, &blobSize );
        serByte( prefs->fDelVfsCacheOnExit, prefsBlob, &blobSize );
        serByte( prefs->startupAction, prefsBlob, &blobSize );
        serByte( prefs->tapScrollType, prefsBlob, &blobSize );
        serByte( prefs->hwButtonScrollType, prefsBlob, &blobSize );
        serByte( prefs->dbStartupAction, prefsBlob, &blobSize );
        wordLen = strlen( (const char*) gd.prefs.lastWord)+1;
        Assert( wordLen <= WORD_MAX_LEN );
        serInt( wordLen, prefsBlob, &blobSize );
        serData( (char*)gd.prefs.lastWord, wordLen, prefsBlob, &blobSize );
        LogV1( "GetSerializedPreferencesThes(), lastWord=%s", gd.prefs.lastWord );

        serInt( gd.historyCount, prefsBlob, &blobSize );
        for (i=0; i<gd.historyCount; i++)
        {
            serLong( gd.wordHistory[i], prefsBlob, &blobSize );
        }
        if ( 1 == phase )
        {
            Assert( blobSize > 0 );
            blobSizePhaseOne = blobSize;
            prefsBlob = new_malloc( blobSize );
            if (NULL == prefsBlob)
            {
                LogG("GetSerializedPreferencesThes(): prefsBlob==NULL");
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


/* Given a blob containing serialized prefrences deserilize the blob
and set the preferences accordingly.
*/
void DeserilizePreferencesThes(unsigned char *prefsBlob, long blobSize)
{
    ThesPrefs *     prefs;
    int             wordLen;
    int             i;

    Assert( prefsBlob );
    Assert( blobSize > 8 );

    LogG( "DeserilizePreferencesThes()" );

    prefs = &gd.prefs;
    /* skip the 4-byte signature of the preferences record */
    Assert( FIsThesPrefRecord( prefsBlob ) );
    prefsBlob += 4;
    blobSize -= 4;

    prefs->fDelVfsCacheOnExit = (Boolean) deserByte( &prefsBlob, &blobSize );
    prefs->startupAction = (StartupAction) deserByte( &prefsBlob, &blobSize );
    prefs->tapScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->hwButtonScrollType = (ScrollType) deserByte( &prefsBlob, &blobSize );
    prefs->dbStartupAction = (DatabaseStartupAction) deserByte( &prefsBlob, &blobSize );

    Assert( blobSize > 0);
    wordLen = deserInt( &prefsBlob, &blobSize );
    Assert( wordLen <= WORD_MAX_LEN );
    Assert( 0 == prefsBlob[wordLen-1] );
    deserData( (unsigned char*)gd.prefs.lastWord, wordLen, &prefsBlob, &blobSize );
    LogV1( "DeserilizePreferencesThes(), lastWord=%s", gd.prefs.lastWord );

    gd.historyCount = deserInt( &prefsBlob, &blobSize );
    for (i=0; i<gd.historyCount; i++)
    {
        gd.wordHistory[i] = deserLong( &prefsBlob, &blobSize );
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

    prefsBlob = GetSerializedPreferencesThes( &blobSize );
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

#define PREF_REC_MIN_SIZE 4

void LoadPreferencesThes()
{
    DmOpenRef    db;
    UInt         recNo;
    void *       recData;
    MemHandle    recHandle;
    UInt         recsCount;
    Boolean      fRecFound = false;

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
            DeserilizePreferencesThes((unsigned char*)recData, MemHandleSize(recHandle) );
        }
        MemPtrUnlock(recData);
    }
    DmCloseDatabase(db);
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

/* called for every file on the external card */
void VfsFindCbThes( AbstractFile *file )
{
    AbstractFile *fileCopy;

    /* TODO: update progress dialog with a number of files processed */
    if ( THES_CREATOR != file->creator )
        return;

    if ( ROGET_TYPE != file->type )
        return;

    fileCopy = AbstractFileNewFull( file->fsType, file->creator, file->type, file->fileName );
    if ( NULL == fileCopy ) return;
    fileCopy->volRef = file->volRef;
    DictFoundCBThes( fileCopy );
}

void ScanForDictsThes(void)
{
    FsMemFindDb( THES_CREATOR, ROGET_TYPE, NULL, &DictFoundCBThes );

    /* TODO: show a progress dialog with a number of files processed so far */
    FsVfsFindDb( &VfsFindCbThes );
}

Err InitThesaurus(void)
{
    MemSet((void *) &gd, sizeof(GlobalData), 0);
    LogInit( &g_Log, "c:\\thes_log.txt" );

    SetCurrentFile( NULL );

/*     gd.current_timeout = -1; */
    gd.prevSelectedWord = 0xfffff;

    gd.firstDispLine = -1;
    gd.prevSelectedWord = -1;

    /* fill out the default values for Noah preferences
       and try to load them from pref database */
    gd.prefs.startupAction = startupActionClipboard;
    gd.prefs.tapScrollType = scrollLine;
    gd.prefs.hwButtonScrollType = scrollPage;
    gd.prefs.dbStartupAction = dbStartupActionAsk;

    FsInit();

    LoadPreferencesThes();

    if (!CreateInfoData())
        return !errNone;
    return errNone;
}

Boolean DictInit(AbstractFile *file)
{
    if ( !FsFileOpen( file ) )
        return false;

    if ( !dictNew() )
    {
        return false;
    }

    LogG( "DictInit() after dictNew()" );
    // TODO: save last used database in preferences
    gd.wordsCount = dictGetWordsCount();
    LogG( "DictInit() after dictGetWordsCount()" );
    gd.currentWord = 0;
    gd.listItemOffset = 0;
    return true;
}

void DictCurrentFree(void)
{
    if ( NULL == GetCurrentFile() ) return;

#if 0
    int i;
    // TODO: need to fix that to be dictionary-specific instead of global
    for (i = 0; i < gd.recordsCount; i++)
    {
        if (0 != gd.recsInfo[i].lockCount)
        {
            Assert(0);
        }
    }
#endif

    dictDelete();
    FsFileClose( GetCurrentFile() );
    if (gd.currDispInfo)
    {
        diFree(gd.currDispInfo);
        gd.currDispInfo = NULL;
    }

}

void StopThesaurus()
{
    SavePreferencesThes();
    DictCurrentFree();
    FreeDicts();
    FreeInfoData();
    FrmSaveAllForms();
    FrmCloseAllForms();
    FsDeinit();
}

void DisplayAboutThes(void)
{
    FontID prev_font;
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
    HideScrollbar();

    prev_font = FntGetFont();
    FntSetFont((FontID) 2);
    DrawCenteredString("ArsLexis Thesaurus" , 16 + 8 - 17);
#ifdef DEMO
    DrawCenteredString( "DEMO", 28 + 12 - 17 );
#else
  #ifdef DEBUG
    DrawCenteredString("Ver 1.1 DEBUG", 28 + 12 - 17);
  #else
    DrawCenteredString("Ver 1.1", 28 + 12 - 17);
  #endif
#endif
    FntSetFont((FontID) 1);
    DrawCenteredString( "(C) ArsLexis", 58 + 8 - 17);
    DrawCenteredString( "arslexis@pobox.com", 76 + 8 - 17);
    FntSetFont((FontID) 2);
    DrawCenteredString( "http://www.arslexis.com", 96 + 8 - 17);

    FntSetFont(prev_font);
}

/* return 0 if didn't find anything in clipboard, 1 if 
   got word from clipboard */
int TryClipboard(void)
{
    MemHandle   clipItemHandle = 0;
    UInt16      itemLen;
    char        txt[30];
    char        *clipTxt;
    char        *word;
    long        wordNo;
    int         idx;

    if (startupActionNone == gd.prefs.startupAction)
        return 0;

    if ((startupActionLast == gd.prefs.startupAction) && gd.prefs.lastWord[0])
    {
        clipTxt = gd.prefs.lastWord;
    }
    else
    {
        clipItemHandle = ClipboardGetItem(clipboardText, &itemLen);
        if (!clipItemHandle)
            return 0;
        if (0 == itemLen)
            return 0;
        clipTxt = (char *) MemHandleLock(clipItemHandle);
        if (!clipTxt)
            return 0;
    }

    MemSet(txt, 30, 0);
    MemMove(txt, clipTxt, (itemLen < 28) ? itemLen : 28);

    strtolower(txt);
    RemoveWhiteSpaces( txt );

    idx = 0;
    while (txt[idx] && (txt[idx] == ' '))
    {
        ++idx;
    }

    if (clipItemHandle)
        MemHandleUnlock(clipItemHandle);

    wordNo = dictGetFirstMatching(&(txt[idx]));
    word = dictGetWord(wordNo);

    if (0 == StrNCaselessCompare(&(txt[idx]), word,  ((UInt16) StrLen(word) <  itemLen) ? StrLen(word) : itemLen))
    {
        DrawDescription(wordNo);
        return 1;
    }
    return 0;
}

int LastUsedDatabase(void)
{
    #if 0
    int i;
    for (i = 0; i < gd.dictsCount; i++)
    {
        if (0 == StrCompare(gd.prefs.lastDbName, gd.foundDbs[i].dbName))
        {
            return i;
        }
    }
#endif
    return -1;
}

#define FONT_DY  11
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
    EventType       newEvent;
    AbstractFile *  fileToOpen;
    int             i;
    int             selectedDb;
/*      RectangleType r; */
/*      static     UInt16 start_x; */
/*      static     UInt16 start_y; */
/*      int        line; */
/*      int        charPos; */

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

        DisplayAboutThes();
        ScanForDictsThes();

        if (0 == gd.dictsCount)
        {
            FrmAlert(alertNoDB);
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = appStopEvent;
            EvtAddEventToQueue(&newEvent);
            return true;
        }

ChooseDatabase:
        fileToOpen = NULL;
        if (1 == gd.dictsCount )
        {
            fileToOpen = gd.dicts[0];
        }
        else
        {
            if (dbStartupActionLast == gd.prefs.dbStartupAction)
            {
#if 0
                // TODO: use last used database
                fileToOpen = LastUsedDatabase();
#endif
                fileToOpen = NULL;
            }
        }
        if (NULL == fileToOpen)
        {
            /* ask user which database to use */
            FrmPopupForm(formSelectDict);
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
                    MemSet(&newEvent, sizeof(EventType), 0);
                    newEvent.eType = appStopEvent;
                    EvtAddEventToQueue(&newEvent);
                    return true;                    
                }
            }
            if (!TryClipboard())
            {
                DisplayAboutThes();
                /* start the timer, so we'll switch to info
                   text after a few seconds */
/*            gd.start_seconds_count = TimGetSeconds();
            gd.current_timeout = 50; */
            }
        }

        list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listHistory));
        LstSetListChoices(list, NULL, gd.historyCount);
        LstSetDrawFunction(list, HistoryListDrawFunc);
        if (0 == gd.historyCount)
        {
            CtlHideControlEx(frm,popupHistory);
        }

        WinDrawLine(0, 145, 160, 145);
        WinDrawLine(0, 144, 160, 144);

        handled = true;
        break;

    case popSelectEvent:
        switch (event->data.popSelect.listID)
        {
        case listHistory:
            wordNo = gd.wordHistory[event->data.popSelect.selection];
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
            handled = false;
            break;
        default:
            Assert(0);
            break;
        }
        break;

    case evtNewWordSelected:
        AddToHistory(gd.currentWord);
        if (1 == gd.historyCount)
        {
            CtlShowControlEx(frm,popupHistory);
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
            LstSetListChoices(list, NULL, gd.historyCount);
            LstSetDrawFunction(list, HistoryListDrawFunc);
        }
        if (gd.historyCount < 6)
        {
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
            LstSetListChoices(list, NULL, gd.historyCount);
        }
        DrawDescription(gd.currentWord);
        WinDrawLine(0, 145, 160, 145);
        WinDrawLine(0, 144, 160, 144);
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
                        MemSet(&newEvent, sizeof(EventType), 0);
                        newEvent.eType = (eventsEnum) evtNewDatabaseSelected;
                        EvtSetInt( &newEvent, 0 );
                        EvtAddEventToQueue(&newEvent);
                    }
                    return true;
                }
                else
                {
                    FrmAlert( alertDbFailed);
                    MemSet(&newEvent, sizeof(EventType), 0);
                    newEvent.eType = appStopEvent;
                    EvtAddEventToQueue(&newEvent);
                    return true;                    
                }
            }
        }

        WinDrawLine(0, 145, 160, 145);
        WinDrawLine(0, 144, 160, 144);
        if (!TryClipboard())
        {
            DisplayAboutThes();
            /* start the timer, so we'll switch to info
               text after a few seconds */
/*           gd.start_seconds_count = TimGetSeconds();
           gd.current_timeout = 50; */
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
                 || ((event->data.keyDown.chr >= 'A') && (event->data.keyDown.chr <= 'Z')))
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
            DisplayAboutThes();
            break;
#ifdef DEBUG
        case menuItemStress:
            stress(1);
            break;
#endif
        case menuItemSelectDB:
            FrmPopupForm(formSelectDict);
            break;
        case menuItemHelp:
            DisplayHelp();
            break;
        case menuItemPrefs:
            FrmPopupForm(formPrefs);
            break;
        default:
            Assert(0);
            break;
        }
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

#define WORDS_IN_LIST 15

void RememberLastWord(FormType * frm)
{
    char *word = NULL;
    int wordLen;
    FieldType *fld;

    Assert(frm);

    fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
#if 0
    MemSet((void *) &(gd.prefs.lastWord[0]), wordHistory, 0);
#endif
    word = FldGetTextPtr(fld);
    wordLen = FldGetTextLength(fld);
#if 0
    if (wordLen >= (wordHistory - 1))
    {
        wordLen = wordHistory - 1;
    }
#endif
    MemMove((void *) &(gd.lastWord[0]), word, wordLen);
    FldDelete(fld, 0, wordLen - 1);
}

Boolean FindFormHandleEventThes(EventType * event)
{
    Boolean handled = false;
    char *word;
    FormPtr frm;
    FieldPtr fld;
    ListPtr list;
    EventType newEvent;
    long newSelectedWord;

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
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = evtFieldChanged;
            EvtAddEventToQueue(&newEvent);
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
        MemSet(&newEvent, sizeof(EventType), 0);
        newEvent.eType = evtFieldChanged;
        EvtAddEventToQueue(&newEvent);
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
    static int  selectedDb = 0;
    EventType   newEvent;

    switch (event->eType)
    {
    case frmOpenEvent:
        frm = FrmGetActiveForm();
        list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
        LstSetDrawFunction(list, ListDbDrawFunc);
        LstSetListChoices(list, NULL, gd.dictsCount);
        LstSetSelection(list, selectedDb);
        LstMakeItemVisible(list, selectedDb);
        if (NULL == GetCurrentFile())
        {
            LogG( "SelDictFH(): current file doesn't exist" );
            FrmHideObject(frm, FrmGetObjectIndex(frm, buttonCancel));
        }
        else
        {
            LogG( "SelDictFH(): current file exists" );
            FrmShowObject(frm, FrmGetObjectIndex(frm, buttonCancel));
        }
        FrmDrawForm(frm);
        return true;
        break;

    case lstSelectEvent:
        selectedDb = event->data.lstSelect.selection;
        return true;
        break;

    case ctlSelectEvent:
        switch (event->data.ctlSelect.controlID)
        {
        case buttonSelect:
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = (eventsEnum) evtNewDatabaseSelected;
            EvtSetInt( &newEvent, selectedDb );
            selectedDb = 0;
            EvtAddEventToQueue(&newEvent);
            FrmReturnToForm(0);
            return true;
        case buttonCancel:
            Assert( NULL != GetCurrentFile() );
            FrmReturnToForm(0);
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

static char sa_txt[20];
static char but_txt[20];
static char tap_txt[20];

void PrefsToGUI(FormType * frm)
{
    Assert(frm);
    SetPopupLabel(frm, listStartupAction, popupStartupAction, gd.prefs.startupAction, (char *) sa_txt);
    SetPopupLabel(frm, listhwButtonsAction, popuphwButtonsAction, gd.prefs.hwButtonScrollType, (char *) but_txt);
    SetPopupLabel(frm, listTapAction, popupTapAction, gd.prefs.tapScrollType, (char *) tap_txt);
}

Boolean PrefFormHandleEventThes(EventType * event)
{
    Boolean handled = false;
    FormType *frm = NULL;
    ListType *list = NULL;
    char *listTxt = NULL;

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
/*       case listStartupDB: */
/*            gd.prefs.dbStartupAction = event->data.popSelect.selection; */
/*            MemMove(sdb_txt,listTxt,StrLen(listTxt)+1); */
/*            CtlSetLabel(FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupStartupDB)),sdb_txt); */
/*            break; */
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
/*       case popupStartupDB: */
        case popuphwButtonsAction:
        case popupTapAction:
            handled = false;
            break;
        case buttonOk:
            SavePreferencesThes();
            // pass through
        case buttonCancel:
            Assert( NULL != GetCurrentFile() );
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
/*      EvtGetEvent(&event, gd.current_timeout); */
        EvtGetEvent(&event, -1);
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

