/*
  Copyright (C) 2000,2001, 2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include "noah_pro.h"
#include "noah_pro_rcp.h"
#include "extensible_buffer.h"

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

#define evtFieldChanged          (firstUserEvent+1)
#define evtNewWordSelected       (firstUserEvent+2)
#define evtNewDatabaseSelected   (firstUserEvent+3)

char helpText[] = "Instructions:\n\255 to lookup a definition of a word \npress the find icon in the right \nlower corner and select the word \n\255 you can scroll the definition using \nhardware buttons, tapping on the \nscreen or using a scrollbar \n\255 left/right arrow moves to next \nor previous word\n";

// #define WAIT_TXT     "Please wait..."

GlobalData gd;

void SavePreferences()
{
#if 0    
    DmOpenRef      db = NULL;
    UInt           recNo;
    UInt           recsCount;
    Boolean        recFoundP = false;
    Err            err;
    NoahDBPrefs    *pref = NULL;
    long           recSize = 0;
    MemHandle      recHandle = 0;

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
    recFoundP = 0;
    recsCount = DmNumRecords(db);
    while (recNo < recsCount)
    {
        recHandle = DmGetRecord(db, recNo);
        pref = (NoahDBPrefs *) MemHandleLock(recHandle);
        recSize = MemHandleSize(recHandle);
        /* records must have matching id and size */
        if ((pref->recordId == data->recordId) && (dataLen == recSize))
        {
            /* and for per-db preferences, the name has to match the name
               of the database */
            recFoundP = true;
            if (NoahDB10Pref == pref->recordId)
            {
                if (0 == StrCompare((char *) pref->dbName, (char *) data->dbName))
                    recFoundP = true;
                else
                    recFoundP = false;
            }
        }
        if (recFoundP)
            break;

        MemPtrUnlock(pref);
        DmReleaseRecord(db, recNo, true);
        ++recNo;
    }

    if (!recFoundP)
    {
        recNo = 0;
        recHandle = DmNewRecord(db, &recNo, dataLen);
        pref = (NoahDBPrefs *) MemHandleLock(recHandle);
        /* ErrFatalDisplayIf(!pref, "M"); */
    }
    DmWrite(pref, 0, data, dataLen);
    MemPtrUnlock(pref);
    DmReleaseRecord(db, recNo, true);
    DmCloseDatabase(db);
#endif    
}

void LoadPreferences()
{
#if 0
    DmOpenRef       db = NULL;
    UInt            recNo;
    Boolean         recFoundP = false;
    NoahDBPrefs     *pref;
    MemHandle       recHandle;
    long            recSize = 0;
    UInt            recsCount;

    db = DmOpenDatabaseByTypeCreator(NOAH_PREF_TYPE, NOAH_PRO_CREATOR, dmModeReadWrite);
    if (!db)
        return;
    recsCount = DmNumRecords(db);
    for (recNo = 0; (recNo < recsCount) && !recFoundP; recNo++)
    {
        recHandle = DmQueryRecord(db, recNo);
        pref = (NoahDBPrefs *) MemHandleLock(recHandle);
        recSize = MemHandleSize(recHandle);
        if ((pref->recordId == data->recordId) && (dataLen == recSize))
        {
            recFoundP = true;
            if (NoahDB10Pref == pref->recordId)
            {
                if (0 == StrCompare((char *) pref->dbName, (char *) data->dbName)) 
                    recFoundP = true;
                else
                    recFoundP = false;
            }
            if (recFoundP)
                MemMove(data, pref, dataLen);
        }
        MemPtrUnlock(pref);
    }
    DmCloseDatabase(db);
#endif
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

/* called for every file on the external card */
void VfsFindCbNoahPro( AbstractFile *file )
{
    AbstractFile *fileCopy;

    /* TODO: update progress dialog with a number of files processed */
    if ( NOAH_PRO_CREATOR != file->creator )
        return;

    if ( (WORDNET_PRO_TYPE != file->type) &&
        (WORDNET_LITE_TYPE != file->type) &&
        (SIMPLE_TYPE != file->type) &&
        (ENGPOL_TYPE != file->type))
    {
        return;
    }

    fileCopy = AbstractFileNewFull( file->fsType, file->creator, file->type, file->fileName );
    if ( NULL == fileCopy ) return;
    fileCopy->volRef = file->volRef;
    DictFoundCBNoahPro( fileCopy );
}

void ScanForDictsNoahPro(void)
{
    FsMemFindDb( NOAH_PRO_CREATOR, WORDNET_PRO_TYPE, NULL, &DictFoundCBNoahPro );

    /* TODO: show a progress dialog with a number of files processed so far */
    FsVfsFindDb( &VfsFindCbNoahPro );
}

Err InitNoahPro(void)
{
    MemSet((void *) &gd, sizeof(GlobalData), 0);
    LogInit( &g_Log, "c:\\noah_pro_log.txt" );

    SetCurrentFile( NULL );

    gd.err = ERR_NONE;
/*     gd.current_timeout = -1; */
    gd.prevSelectedWord = 0xfffff;

    gd.firstDispLine = -1;
    gd.prevSelectedWord = -1;

    /* fill out the default values for Noah preferences
       and try to load them from pref database */
    gd.prefs.delVfsCacheOnExitP = 0;
    gd.prefs.startupAction = startupActionNone;
    gd.prefs.tapScrollType = scrollLine;
    gd.prefs.hwButtonScrollType = scrollPage;
    gd.prefs.dbStartupAction = dbStartupActionAsk;
    gd.prefs.externalDbsCount = 0;

    FsInit();

/*    LoadPreferences((NoahDBPrefs *) & (gd.prefs), sizeof(NoahPrefs)); */

    if (!CreateInfoData())
        return !errNone;
    return errNone;
}

void DictCurrentFree(void)
{
    SavePreferences();
    dictDelete();
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

void FreeDicts(void)
{
    while(gd.dictsCount>0)
    {
        AbstractFileFree( gd.dicts[--gd.dictsCount] );
    }
}


void StopNoahPro(void)
{
    // TODO: SavePreferences()
    DictCurrentFree();
    FreeDicts();
    FreeInfoData();
    if ( gd.prefs.delVfsCacheOnExitP )
    {
        dcDelCacheDb();
    }
    FrmSaveAllForms();
    FrmCloseAllForms();
    FsDeinit();
}

void DisplayAboutNoahPro(void)
{
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
    HideScrollbar();

    dh_set_current_y(7);
    dh_save_font();
#ifdef DEBUG
    dh_display_string("Noah Pro DEBUG", 2, 16);
#elif DEMO
    dh_display_string("Noah Pro DEMO", 2, 16);
#else
    dh_display_string("Noah Pro", 2, 16);
#endif

    dh_display_string("Ver 2.1 alpha", 2, 20);
    dh_display_string("(C) ArsLexis 2000-2002", 1, 24);
    dh_display_string("arslexis@pobox.com", 2, 20);
    dh_display_string("http://www.arslexis.com", 2, 0);
    dh_restore_font();
}

void SetWordAsLastWord( char *txt, int wordLen )
{
    MemSet((void *) &(gd.lastWord[0]), WORD_MAX_LEN, 0);
    if ( -1 == wordLen )
    {
        wordLen = StrLen( txt );
    }
    if (wordLen >= (WORD_MAX_LEN - 1))
    {
        wordLen = WORD_MAX_LEN - 1;
    }
    MemMove((void *) &(gd.lastWord[0]), txt, wordLen);
}

/* return 0 if didn't find anything in clipboard, 1 if 
   got word from clipboard */
int TryClipboard(void)
{
    MemHandle   clipItemHandle = 0;
    char        txt[30];
    char        *clipTxt;
    char        *word;
    UInt32      wordNo;
    int         idx;
    UInt16      itemLen;

    if ( 0 == clipItemHandle )
        return 0;

    clipItemHandle = ClipboardGetItem(clipboardText, &itemLen);
    if (!clipItemHandle || (0==itemLen))
            return 0;
    clipTxt = (char *) MemHandleLock(clipItemHandle);

    if (!clipTxt)
    {
        MemHandleUnlock( clipItemHandle );
        return 0;
    }

    MemSet(txt, 30, 0);
    itemLen = (itemLen < 28) ? itemLen : 28;
    MemMove(txt, clipTxt, itemLen);

    strtolower(txt);
    RemoveWhiteSpaces( txt );
    idx = 0;
    while (txt[idx] && (txt[idx] == ' '))
    {
        ++idx;
    }

    if (clipItemHandle)
        MemHandleUnlock(clipItemHandle);

    wordNo = dictGetFirstMatching(txt);
    word = dictGetWord(wordNo);

    if (0 == StrNCaselessCompare(&(txt[idx]), word,  ((UInt16) StrLen(word) <  itemLen) ? StrLen(word) : itemLen))
    {
        DrawDescription(wordNo);
        return 1;
    }
    return 0;
#if 0
    /* if found the word exactly equal to the word in the clipboard - just
    display it. If not, switch to find word dialog positioned at the closest
    matching word */
    if (0 == p_istrcmp(txt, word ) )
    {
        DrawDescription(wordNo);
        return 1;
    }
    else
    {
        SetWordAsLastWord( txt, -1 );
        FrmPopupForm(formDictFind);
    }
    return 0;
#endif
}

int LastUsedDatabase(void)
{
/* TODO:
    int i;

    for (i = 0; i < gd.dbsCount; i++)
    {
        if (0 == StrCompare((char *) gd.prefs.lastDbName,  (char *) gd.foundDbs[i].dbName))
        {
            return i;
        }
    }
*/
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


#define WORDS_IN_LIST 15

void RememberLastWord(FormType * frm)
{
    char        *word = NULL;
    int         wordLen;
    FieldType   *fld;

    Assert(frm);

    fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
    word = FldGetTextPtr(fld);
    wordLen = FldGetTextLength(fld);
    SetWordAsLastWord( word, wordLen );
    FldDelete(fld, 0, wordLen - 1);
}

Boolean MainFormHandleEventNoahPro(EventType * event)
{
    Boolean     handled = false;
    FormType    *frm;
    Short       newValue;
    ListType        *list;
#if 0
    long        wordNo;
#endif
    EventType   newEvent;
    char        *defTxt = NULL;
    int         defTxtLen = 0;
    int         i;
    int         linesCount;
    int         selectedDb;
    static ExtensibleBuffer clipboard_buf = { 0 };
    AbstractFile    *fileToOpen;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
    case frmOpenEvent:
        FrmDrawForm(frm);

        ScanForDictsNoahPro();

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
        }
#if 0
        // old code
        if (gd.dbsCount > 1)
        {
            /* ask user which database to use */
            FrmPopupForm(formSelectDict);
        }
        else
        {
            gd.newDb = 0;
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = (eventsEnum) evtNewDatabaseSelected;
            EvtAddEventToQueue(&newEvent);
        }
#endif
        WinDrawLine(0, 145, 160, 145);
        WinDrawLine(0, 144, 160, 144);
        if (!TryClipboard())
        {
            DisplayAboutNoahPro();
            /* start the timer, so we'll switch to info
               text after a few seconds */
/*            gd.start_seconds_count = TimGetSeconds();
        gd.current_timeout = 50; */
        }
        handled = true;
        break;

    case popSelectEvent:
        switch (event->data.popSelect.listID)
        {
        case listHistory:
#if 0
            wordNo = gd.dbPrefs.wordHistory[event->data.popSelect.selection];
            if (wordNo != gd.currentWord)
            {
                gd.currentWord = wordNo;
                Assert(wordNo < gd.wordsCount);
                DrawDescription(gd.currentWord);
                gd.penUpsToConsume = 1;
            }
#endif
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
            // TODO: why false???
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
#if 0
        if (1 == gd.dbPrefs.historyCount)
        {
            CtlShowControlEx(frm, popupHistory);
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
            LstSetListChoices(list, NULL, gd.dbPrefs.historyCount);
            LstSetDrawFunction(list, HistoryListDrawFunc);
        }
        if (gd.dbPrefs.historyCount < 6)
        {
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listHistory));
            LstSetListChoices(list, NULL, gd.dbPrefs.historyCount);
        }
#endif
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

        FrmDrawForm(frm);
        WinDrawLine(0, 145, 160, 145);
        WinDrawLine(0, 144, 160, 144);

        if (!TryClipboard())
        {
            DisplayAboutNoahPro();
            /* start the timer, so we'll switch to info
               text after a few seconds */
/*           gd.start_seconds_count = TimGetSeconds();
           gd.current_timeout = 50; */
        }
        handled = true;

#if 0
        // old code
        if (!DictInit())
        {
            // TODO: should probably do something more intelligent here
            gd.err = ERR_DICT_INIT;
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = appStopEvent;
            EvtAddEventToQueue(&newEvent);
            return true;
        }
        // TODO: needs to trigger showing find word dialog
#if 0
        if ((startupActionLast == gd.prefs.startupAction)
            && gd.prefs.lastWord[0])
        {
        }
#endif
        if ( startupActionClipboard == gd.prefs.startupAction )
        {
            TryClipboard();
        }

#if 0
        if (gd.dbPrefs.historyCount > 0)
        {
            CtlShowControlEx(frm, popupHistory);
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
            LstSetListChoices(list, NULL, gd.dbPrefs.historyCount);
            LstSetDrawFunction(list, HistoryListDrawFunc);
        }
        else
#endif
        {
            CtlHideControlEx(frm, popupHistory);
        }
#endif
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
            DisplayAboutNoahPro();
            break;
        case menuItemScanDbs:
            // TODO: make it work
#if 0
            DictCurrentFree();
            ScanForDictsNoahPro();
            if (0 == gd.dbsCount)
            {
                FrmAlert(alertNoDB);
                MemSet(&newEvent, sizeof(EventType), 0);
                newEvent.eType = appStopEvent;
                EvtAddEventToQueue(&newEvent);
            }
            if (gd.dbsCount > 1)
            {
                /* ask user which database to use */
                FrmPopupForm(formSelectDict);
            }
            else
            {
                gd.newDb = 0;
                MemSet(&newEvent, sizeof(EventType), 0);
                newEvent.eType = (eventsEnum) evtNewDatabaseSelected;
                EvtAddEventToQueue(&newEvent);
            }
#endif
            handled = true;
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
            {
                ebufReset(&clipboard_buf);
                linesCount = diGetLinesCount(gd.currDispInfo);
                for (i = 0; i < linesCount; i++)
                {
                    defTxt = diGetLine(gd.currDispInfo, i);
                    ebufAddStr(&clipboard_buf, defTxt);
                    ebufAddChar(&clipboard_buf, '\n');
                }
                defTxt = ebufGetDataPointer(&clipboard_buf);
                defTxtLen = StrLen(defTxt);

                ClipboardAddItem(clipboardText, defTxt, defTxtLen);
            }
            break;
        case menuItemTranslate:
            TryClipboard();
            break;
#ifdef STRESS
        case menuItemStress:
            stress(2);
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

void DoFieldChanged(void)
{
    char        *word;
    FormPtr     frm;
    FieldPtr    fld;
    ListPtr     list;
    long        newSelectedWord;

    frm = FrmGetActiveForm();
    fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
    list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
    word = FldGetTextPtr(fld);
    /* DrawWord( word, 149 ); */
    gd.listDisabledP = false;
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
}

/*
Event handler proc for the find word dialog
*/
Boolean FindFormHandleEventNoahPro(EventType * event)
{
    Boolean     handled = false;
    char        *word;
    FormPtr     frm;
    FieldPtr    fld;
    ListPtr     list;
    EventType   newEvent;

    if (event->eType == nilEvent)
        return true;

    switch (event->eType)
    {
    case frmOpenEvent:
        frm = FrmGetActiveForm();
        fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
        list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
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
            // DoFieldChanged();
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = (eventsEnum)evtFieldChanged;
            EvtAddEventToQueue(&newEvent);
        }
        else
        {
            LstSetSelectionEx(list, gd.selectedWord);
        }
        FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWord));
        // CtlHideControlEx( frm, listMatching );
        //gd.listDisabledP = true;
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
        MemSet(&newEvent, sizeof(EventType), 0);
        newEvent.eType = (eventsEnum) evtNewWordSelected;
        EvtAddEventToQueue(&newEvent);
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
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = (eventsEnum) evtNewWordSelected;
            EvtAddEventToQueue(&newEvent);
            FrmReturnToForm(0);
            return true;
            break;
        case pageUpChr:
            frm = FrmGetActiveForm();
            fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
            if (gd.selectedWord > WORDS_IN_LIST)
            {
                gd.selectedWord -= WORDS_IN_LIST;
                LstSetSelectionMakeVisibleEx(list, gd.selectedWord);
                return true;
            }
            break;
        case pageDownChr:
            frm = FrmGetActiveForm();
            fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
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
        newEvent.eType = (eventsEnum)evtFieldChanged;
        EvtAddEventToQueue(&newEvent);
        handled = false;
        break;
    case fldChangedEvent:
    case evtFieldChanged:
        DoFieldChanged();

        handled = true;
        break;
    case ctlSelectEvent:
        switch (event->data.ctlSelect.controlID)
        {
        case buttonCancel:
            RememberLastWord(FrmGetActiveForm());
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = (eventsEnum) evtNewWordSelected;
            EvtAddEventToQueue(&newEvent);
            handled = true;
            FrmReturnToForm(0);
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


/*
Event handler proc for the select dict form.
*/
Boolean SelectDictFormHandleEventNoahPro(EventType * event)
{
    FormPtr frm;
    ListPtr list;
    static long selectedDb = 0;
    EventType newEvent;

    switch (event->eType)
    {
    case frmOpenEvent:
        frm = FrmGetActiveForm();
        list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
        LstSetDrawFunction(list, ListDbDrawFunc);
        LstSetListChoices(list, NULL, gd.dictsCount);
        LstSetSelection(list, selectedDb);
        LstMakeItemVisible(list, selectedDb);
        if (NULL == GetCurrentFile())
        {
            FrmHideObject(frm, FrmGetObjectIndex(frm, buttonCancel));
        }
        else
        {
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
static char sdb_txt[10];
static char but_txt[20];
static char tap_txt[20];

void PrefsToGUI(FormType * frm)
{
    Assert(frm);
    SetPopupLabel(frm, listStartupAction, popupStartupAction, gd.prefs.startupAction, (char *) sa_txt);
    SetPopupLabel(frm, listStartupDB, popupStartupDB, gd.prefs.dbStartupAction, (char *) sdb_txt);
    SetPopupLabel(frm, listhwButtonsAction, popuphwButtonsAction, gd.prefs.hwButtonScrollType, (char *) but_txt);
    SetPopupLabel(frm, listTapAction, popupTapAction, gd.prefs.tapScrollType, (char *) tap_txt);
    CtlSetValue((ControlType *)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, checkDeleteVfs)), gd.prefs.delVfsCacheOnExitP );
}

Boolean PrefFormHandleEventNoahPro(EventType * event)
{
    Boolean handled = true;
    FormType *frm = NULL;
    ListType *list = NULL;
    char *listTxt = NULL;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
    case frmOpenEvent:
        PrefsToGUI(frm);
        FrmDrawForm(frm);
        break;

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
            handled = false; /* why ??? */
            break;
        case buttonOk:
            // SavePreferences((NoahDBPrefs *) & (gd.prefs), sizeof(NoahPrefs),0);
            SavePreferences();
        case buttonCancel:
            FrmReturnToForm(0);
            break;
        case checkDeleteVfs:
            gd.prefs.delVfsCacheOnExitP = CtlGetValue((ControlType *)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, checkDeleteVfs)));
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

Boolean DisplayPrefFormHandleEventNoahPro(EventType * event)
{
    Boolean handled_p = true;
    FormType *frm = NULL;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case frmOpenEvent:
            /* PrefsToGUI(frm); */
            FrmDrawForm(frm);
            break;
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonOk:
                    /*SavePreferences((NoahDBPrefs *) & (gd.prefs), sizeof(NoahPrefs),0);*/
                case buttonCancel:
                    FrmReturnToForm(0);
                    break;
            }
            break;
        default:
            handled_p = false;
            break;
    }
    return handled_p;
}

Boolean HandleEventNoahPro(EventType * event)
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
            FrmSetEventHandler(frm, MainFormHandleEventNoahPro);
            handled = true;
            break;
        case formDictFind:
            FrmSetEventHandler(frm, FindFormHandleEventNoahPro);
            handled = true;
            break;
        case formSelectDict:
            FrmSetEventHandler(frm, SelectDictFormHandleEventNoahPro);
            handled = true;
            break;
        case formPrefs:
            FrmSetEventHandler(frm, PrefFormHandleEventNoahPro);
            handled = true;
            break;
        case formDisplayPrefs:
            FrmSetEventHandler(frm, DisplayPrefFormHandleEventNoahPro);
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

void EventLoopNoahPro(void)
{
    EventType event;
    Word      error;

    event.eType = (eventsEnum) 0;
    while ( (event.eType != appStopEvent) && (ERR_NONE == gd.err) )
    {
        EvtGetEvent(&event, -1);
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

