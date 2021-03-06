/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk
*/

/**
 * @file noah_pro_2nd_segment.c
 * Implementation of Noah Pro functions that don't need to reside in 1st segment, because they are used only
 * during normal application runs and are not called in ResidentMode.
 */
 
#include "noah_pro_1st_segment.h"
#include "noah_pro_2nd_segment.h"
#include "five_way_nav.h"
#include "word_matching_pattern.h"
#include "bookmarks.h"
#include "better_formatting.h"
#include "PrefsStore.hpp"

// Create a blob containing serialized preferences.
// Devnote: caller needs to free memory returned.
void* SerializePreferencesNoahPro(AppContext* appContext, long *pBlobSize)
{
    char *          prefsBlob;
    long            blobSize;
    long            blobSizePhaseOne;
    int             phase;
    NoahPrefs *     prefs;
    UInt32          prefRecordId = AppPrefId;
    AbstractFile *  currFile;
    unsigned char   currFilePos;
    int             i;

    Assert( pBlobSize );

    LogG( "SerializePreferencesNoahPro()" );

    prefs = &appContext->prefs;
    /* phase one: calculate the size of the blob */
    /* phase two: create the blob */
    prefsBlob = NULL;
    for( phase=1; phase<=2; phase++)
    {
        blobSize = 0;
        Assert( 4 == sizeof(prefRecordId) );
        /* 1. preferences */
        serData( (char*)&prefRecordId, (long)sizeof(prefRecordId), prefsBlob, &blobSize );
        serByte( prefs->startupAction, prefsBlob, &blobSize );
        serByte( prefs->hwButtonScrollType, prefsBlob, &blobSize );
        serByte( prefs->navButtonScrollType, prefsBlob, &blobSize );
        serByte( prefs->dbStartupAction, prefsBlob, &blobSize );
        serByte( prefs->bookmarksSortType, prefsBlob, &blobSize );
        serByte( prefs->fResidentModeEnabled, prefsBlob, &blobSize );

        /* 2. number of databases found */
        serByte( appContext->dictsCount, prefsBlob, &blobSize );

        /* 3. currently used database */
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

        /* 4. list of databases */
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

        /* 5. last word */
        serString( (char*)appContext->prefs.lastWord, prefsBlob, &blobSize);

        /* 6. number of words in the history */
        serInt( appContext->historyCount, prefsBlob, &blobSize );

        /* 7. all words in the history */
        for (i=0; i<appContext->historyCount; i++)
        {
            serString(appContext->wordHistory[i], prefsBlob, &blobSize);
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

void SavePreferencesNoahPro(AppContext* appContext)
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


    prefsBlob = SerializePreferencesNoahPro( appContext, &blobSize );
    if ( NULL == prefsBlob ) 
        return;

    db = DmOpenDatabaseByTypeCreator(NOAH_PREF_TYPE, NOAH_PRO_CREATOR, dmModeReadWrite);
    if (!db)
    {
        err = DmCreateDatabase(0, "Noah Prefs", NOAH_PRO_CREATOR,  NOAH_PREF_TYPE, false);
        if ( errNone != err)
        {
            appContext->err = ERR_NO_PREF_DB_CREATE;
            return;
        }

        db = DmOpenDatabaseByTypeCreator(NOAH_PREF_TYPE, NOAH_PRO_CREATOR, dmModeReadWrite);
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
        if ( IsValidPrefRecord(recData) )
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

Err InitNoahPro(AppContext* appContext)
{
    Err         error;

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

void StopNoahPro(AppContext* appContext)
{
    SavePreferencesNoahPro(appContext);
    bfFreePTR(appContext);
    
    Err error = AppCommonFree(appContext);
    Assert(!error);
    
    FrmSaveAllForms();
    FrmCloseAllForms();

    error = AppNotifyFree(true);    
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
    DrawCenteredString(appContext, "Noah Pro", currentY);
    currentY+=16;

#ifdef DEMO
    DrawCenteredString(appContext, "Ver 3.5 (demo)", currentY);
#else
  #ifdef DEBUG
    DrawCenteredString(appContext, "Ver 3.5 (debug)", currentY);
  #else
    DrawCenteredString(appContext, "Ver 3.5", currentY);
  #endif
#endif
    currentY+=20;
    
    FntSetFont(boldFont);
    DrawCenteredString(appContext, "Copyright (c) ArsLexis", currentY);
    currentY+=24;

    FntSetFont(largeFont);
    DrawCenteredString(appContext, "http://www.arslexis.com", currentY);
    
    currentY+=20;
    FntSetFont(boldFont);
//    DrawCenteredString(appContext, (appContext->armIsPresent==true)?"ARM supported":"ARM not present", currentY);
    
    currentY+=20;
    
//    currentY+=40;
    
    FntSetFont(stdFont);
#ifdef DEMO_HANDANGO
    DrawCenteredString(appContext, "Buy at: www.handango.com/purchase", currentY);
    currentY+=14;
    DrawCenteredString(appContext, "        Product ID: 10421", currentY);
#endif
#ifdef DEMO_PALMGEAR
    DrawCenteredString(appContext, "Buy at: www.palmgear.com?6923", currentY);
#endif

    if (GetOsVersion()>=35)
        WinPopDrawState();    
}

static void MainFormReflow(AppContext* appContext, FormType* frm)
{
    // a hack. proper version: don't even get here if it's not os <3.5
    if (GetOsVersion()<=35)
        return;

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

// Handle winDisplayChangedEvent
static Boolean MainFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Assert( DIA_Supported(&appContext->diaSettings) );
    if ( !DIA_Supported(&appContext->diaSettings))
        return false;

    MainFormReflow(appContext, frm);

    // For some reason on 5.3 sim I also get it just by opening the menu
    // which sucks (we re-get and re-display the word here) so I'm dispabling
    // this. doesn't seem to have negative impact
#if 0
    // TODO: optimize, only do when dx screen size has changed
    // HACK: we should have currentWord = -1 and check for that instead
    // (currently currentWord = 0 here)
    if (appContext->currDispInfo)
        SendEvtWithType(evtNewWordSelected);
#endif

    RedrawMainScreen(appContext);
    return true;
}

static Boolean MainFormOpen(AppContext* appContext, FormType* frm, EventType* event)
{
    AbstractFile *  fileToOpen;
    char *          lastDbUsedName;
    int             i;

    MainFormReflow(appContext, frm);

    FrmDrawForm(frm);
    FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWordMain));

    HistoryListInit(appContext, frm);

    RemoveNonexistingDatabases(appContext);
    ScanForDictsNoahPro(appContext, false);

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

        if ( (NULL != lastDbUsedName) &&
            (dbStartupActionLast == appContext->prefs.dbStartupAction) )
        {
            for( i=0; i<appContext->dictsCount; i++)
            {
                if (0==StrCompare( lastDbUsedName, appContext->dicts[i]->fileName ) )
                {
                    fileToOpen = appContext->dicts[i];
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

    WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
    WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);

    if ( appContext->residentWordLookup )
    {
        if (0==StrLen(appContext->residentWordLookup))
        {
            // we were launched in resident mode but couldn't be able to retrieve
            // the word from the field, so we try to lookup the clipboard
            if (!FTryClipboard(appContext, false))
                DoWord(appContext, appContext->residentWordLookup);
        }
        else
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
        DoWord(appContext, (char *)appContext->prefs.lastWord);
    }
    return true;
}

static Boolean MainFormMenuCommand(AppContext* appContext, FormType* frm, EventType* event)
{
    Boolean handled = true;

    switch (event->data.menu.itemID)
    {
        case menuItemFind:
            GoToFindWordForm(appContext,frm);
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
        case menuItemShowPronunciation:
            pronDisplayHelp(appContext,"");
            break;
        case menuItemDispPrefs:
            FrmPopupForm(formDisplayPrefs);
            break;
        case menuItemRandomWord:
            {
                long wordNo = GenRandomLong(appContext->wordsCount);
                char *word = dictGetWord(GetCurrentFile(appContext), wordNo);
                FldClearInsert(frm, fieldWordMain, word);
                AddToHistory(appContext, wordNo);
                HistoryListSetState(appContext, frm);
                DrawDescription(appContext, wordNo);
            }
            break;
        case menuItemCopy:
            if (NULL != appContext->currDispInfo)
                diCopyToClipboard(appContext->currDispInfo);
            break;
        case menuItemLookupClipboard:
            FTryClipboard(appContext, false);
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

        case sysEditMenuCopyCmd:
        case sysEditMenuCutCmd:
        case sysEditMenuPasteCmd:
            // generated by command bar, not sure what to do with those
            // so just ignore them
            handled = false;
            break;
            
        default:
            Assert(0);
            break;
    }
    return handled;
}

static Boolean MainFormNewDatabaseSelected(AppContext* appContext, FormType* frm, EventType *event)
{
    ListType *      list;
    int             i;

    int selectedDb = EvtGetInt( event );
    AbstractFile* fileToOpen = appContext->dicts[selectedDb];
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
    
    ClearMatchingPatternDB(appContext);
    
    RedrawMainScreen(appContext);
    
    if ( startupActionClipboard == appContext->prefs.startupAction )
    {
        if (!FTryClipboard(appContext,false))
            DisplayAbout(appContext);
    }
    else
        DisplayAbout(appContext);
    
    if ( (startupActionLast == appContext->prefs.startupAction) &&
        appContext->prefs.lastWord[0] )
    {
        DoWord( appContext, (char *)appContext->prefs.lastWord );
    }
    
    // TODO: replace with HistoryListInit(frm) ? 
    //if (appContext->historyCount > 0)
    {
        list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
        LstSetListChoices(list, NULL, appContext->historyCount);
        LstSetDrawFunction(list, HistoryListDrawFunc);
        CtlShowControlEx(frm, popupHistory);
    }
    // TODO: remove when decided it's final
    /*else
    {
        CtlHideControlEx(frm, popupHistory);
    }*/
    return true;
}

static Boolean MainFormHandleEventNoahPro(EventType * event)
{
    Boolean         handled = false;
    FormType *      frm;
    Short           newValue;
    long            wordNo;
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
            RedrawMainScreen(appContext);
            handled = true;
            break;

        case frmOpenEvent:
            handled = MainFormOpen(appContext, frm, event);
            break;

        case popSelectEvent:
            switch (event->data.popSelect.listID)
            {
                case listHistory:
                    word = (char*)appContext->wordHistory[event->data.popSelect.selection];
                    wordNo = dictGetFirstMatching(GetCurrentFile(appContext), word);
                    if (wordNo != appContext->currentWord)
                    {
                        Assert(wordNo < appContext->wordsCount);
                        DrawDescription(appContext, wordNo);
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
                    break;
                case ctlArrowRight:
                    if (appContext->currentWord < appContext->wordsCount - 1)
                    {
                        ++appContext->currentWord;
                        RedisplayWord(appContext);
                    }
                    break;
                case buttonFind:
                    GoToFindWordForm(appContext,frm);
                    break;
                case buttonClose:
                    Assert(appContext->fInResidentMode);
                    SendStopEvent();
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

#ifdef I_NOAH
        case evtReformatLastResponse:
            ReformatLastResponse(appContext);
            handled = true;
            break;
#endif

        case evtNewWordSelected:
            word = dictGetWord(GetCurrentFile(appContext), appContext->currentWord);
            FldClearInsert(frm, fieldWordMain, word);

            AddToHistory(appContext, appContext->currentWord);
            HistoryListSetState(appContext, frm);
            // WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
            // WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);
            DrawDescription(appContext, appContext->currentWord);
            appContext->penUpsToConsume = 1;

            handled = true;
            break;

        case evtNewDatabaseSelected:
            handled = MainFormNewDatabaseSelected(appContext,frm,event);
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
            if (appContext->penUpsToConsume > 0)
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
            handled = MainFormMenuCommand(appContext,frm,event);
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
    if ( !DIA_Supported(&appContext->diaSettings))
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

// Event handler proc for the find word dialog
static Boolean FindFormHandleEventNoahPro(EventType * event)
{
    Boolean     handled = false;
    char *      word;
    FormPtr     frm;
    FieldPtr    fld;
    ListPtr     list;
    AppContext* appContext=GetAppContext();

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled= FindFormDisplayChanged(appContext, frm);
            break;

        case frmOpenEvent:
            fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
            list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
            appContext->prevSelectedWord = 0xffffffff;
            LstSetListChoicesEx(list, NULL, dictGetWordsCount(GetCurrentFile(appContext)));
            LstSetDrawFunction(list, ListDrawFunc);
            appContext->prevTopItem = 0;
            appContext->selectedWord = 0;
            word = &(appContext->lastWord[0]);
            /* force updating the field */
            if (0 == StrLen(word))
            {
                // no word so focus on first word
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
            return true;

        case lstSelectEvent:
            /* copy word from text field to a buffer, so next time we
               come back here, we'll come back to the same place */
            // RememberLastWord(appContext, frm, fieldWord);
            /* set the selected word as current word */
            appContext->currentWord = appContext->listItemOffset + (UInt32) event->data.lstSelect.selection;
            /* send a msg to yourself telling that a new word
               have been selected so we need to draw the
               description */
            Assert(appContext->currentWord < appContext->wordsCount);
            SendEvtWithType(evtNewWordSelected);
            FrmReturnToForm(0);
            return true;

        case keyDownEvent:

            /* ugly trick: there is no event that says that
               text field has changed, so I have to generate it myself
               every time I get keyDownEvent */
            switch (event->data.keyDown.chr)
            {
                case returnChr:
                case linefeedChr:
                    RememberLastWord(appContext, frm, fieldWord);
                    appContext->currentWord = appContext->selectedWord;
                    Assert(appContext->currentWord < appContext->wordsCount);
                    SendEvtWithType(evtNewWordSelected);
                    FrmReturnToForm(0);
                    return true;

                case pageUpChr:
                    if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
                    {
                        ScrollWordListByDx(appContext, frm, -(appContext->dispLinesCount-1) );
                        return true;
                    }

                case pageDownChr:
                    if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
                    {
                        ScrollWordListByDx(appContext, frm, (appContext->dispLinesCount-1) );
                        return true;
                    }

                default:
                    if ( HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) )
                    {
                        if (FiveWayCenterPressed(appContext, event))
                        {
                            RememberLastWord(appContext, frm, fieldWord);
                            appContext->currentWord = appContext->selectedWord;
                            Assert(appContext->currentWord < appContext->wordsCount);
                            SendEvtWithType(evtNewWordSelected);
                            FrmReturnToForm(0);
                            return true;
                        }
                    
                        if (FiveWayDirectionPressed(appContext, event, Left ))
                        {
                            ScrollWordListByDx(appContext, frm, -(appContext->dispLinesCount-1) );
                            return true;
                        }
                        if (FiveWayDirectionPressed(appContext, event, Right ))
                        {
                            ScrollWordListByDx(appContext, frm, (appContext->dispLinesCount-1) );
                            return true;
                        }
                        if (FiveWayDirectionPressed(appContext, event, Up ))
                        {
                            ScrollWordListByDx(appContext, frm, -1 );
                            return true;
                        }
                        if (FiveWayDirectionPressed(appContext, event, Down ))
                        {
                            ScrollWordListByDx(appContext, frm, 1 );
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
            return false;

        case evtFieldChanged:
            DoFieldChanged(appContext);
            return true;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    RememberLastWord(appContext, frm, fieldWord);
                    FrmReturnToForm(0);
                    break;
                case ctlArrowLeft:
                    ScrollWordListByDx(appContext, frm, -(appContext->dispLinesCount-1) );
                    break;
                case ctlArrowRight:
                    ScrollWordListByDx(appContext, frm, (appContext->dispLinesCount-1) );
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

static Boolean SelectDictFormHandleEventNoahPro(EventType * event)
{
    FormPtr frm;
    ListPtr list;
    long    selectedDb;
    AppContext* appContext=GetAppContext();

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            return SelectDictFormDisplayChanged(appContext, frm);
            break;

        case frmOpenEvent:
            selectedDb = FindCurrentDbIndex(appContext);
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
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
                    list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listOfDicts));
                    selectedDb = LstGetSelection(list);
                    if (selectedDb != noListSelection)
                        SendNewDatabaseSelected( selectedDb );
                    FrmReturnToForm(0);
                    break;
                case buttonCancel:
                    Assert( NULL != GetCurrentFile(appContext) );
                    FrmReturnToForm(0);
                    break;
                case buttonRescanDicts:
                    DictCurrentFree(appContext);
                    FreeDicts(appContext);
                    // force scanning external memory for databases
                    ScanForDictsNoahPro(appContext, true);
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

static void PrefsToGUI(AppPrefs *prefs, FormType * frm)
{
    Assert(frm);
    SetPopupLabel(frm, listStartupAction, popupStartupAction, prefs->startupAction);
    SetPopupLabel(frm, listStartupDB, popupStartupDB, prefs->dbStartupAction);
    SetPopupLabel(frm, listhwButtonsAction, popuphwButtonsAction, prefs->hwButtonScrollType);
    SetPopupLabel(frm, listNavButtonsAction, popupNavButtonsAction, prefs->navButtonScrollType);
    CtlSetValue( (ControlType*)FrmGetObjectPtr( frm, FrmGetObjectIndex(frm, checkResidentMode) ),(Int16) prefs->fResidentModeEnabled );
}

static Boolean PrefFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Assert( DIA_Supported(&appContext->diaSettings) );
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(frm);

    FrmSetObjectPosByID(frm, buttonOk,      -1, appContext->screenHeight-14);
    FrmSetObjectPosByID(frm, buttonCancel,  -1, appContext->screenHeight-14);
        
    FrmDrawForm(frm);
    return true;
}

static Boolean PrefFormHandleEventNoahPro(EventType * event)
{
    Boolean     handled = true;
    FormType *  frm;
    ListType *  list;
    char *      listTxt;
    AppContext* appContext=GetAppContext();

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled=PrefFormDisplayChanged(appContext, frm);
            break;

        case frmOpenEvent:
            FrmDrawForm(frm);
            PrefsToGUI(&(appContext->prefs), frm);
            appContext->tmpPrefs = appContext->prefs;
            return true;

        case popSelectEvent:
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, event->data.popSelect.listID));
            listTxt = LstGetSelectionText(list, event->data.popSelect.selection);
            switch (event->data.popSelect.listID)
            {
                case listStartupAction:
                    appContext->tmpPrefs.startupAction = (StartupAction) event->data.popSelect.selection;
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupStartupAction)), listTxt);
                    break;
                case listStartupDB:
                    appContext->tmpPrefs.dbStartupAction = (DatabaseStartupAction) event->data.popSelect.selection;
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, popupStartupDB)), listTxt);
                    break;
                case listhwButtonsAction:
                    appContext->tmpPrefs.hwButtonScrollType = (ScrollType) event->data.popSelect.selection;
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popuphwButtonsAction)), listTxt);
                    break;
                case listNavButtonsAction:
                    appContext->tmpPrefs.navButtonScrollType = (ScrollType) event->data.popSelect.selection;
                    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupNavButtonsAction)), listTxt);
                    break;
                default:
                    Assert(0);
                    break;
            }
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
                case popupNavButtonsAction:
                    // need to propagate the event down to popus
                    handled = false;
                    break;
                case buttonOk:
                    appContext->prefs = appContext->tmpPrefs;
                    SavePreferencesNoahPro(appContext);
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

static Boolean HandleEventNoahPro(AppContext* appContext, EventType * event)
{
    FormType *  frm;
    UInt16      formId;
    Err error=errNone;
    
    if (event->eType == frmLoadEvent)
    {
        formId = event->data.frmLoad.formID;
        frm = FrmInitForm(formId);
        FrmSetActiveForm(frm);
        error=DefaultFormInit(appContext, frm);    
        switch (formId)
        {
            case formDictMain:
                FrmSetEventHandler(frm, MainFormHandleEventNoahPro);
                return true;

            case formDictFind:
                FrmSetEventHandler(frm, FindFormHandleEventNoahPro);
                return true;

            case formDictFindPattern:
                FrmSetEventHandler(frm, FindPatternFormHandleEvent);
                return true;

            case formSelectDict:
                FrmSetEventHandler(frm, SelectDictFormHandleEventNoahPro);
                return true;

            case formPrefs:
                FrmSetEventHandler(frm, PrefFormHandleEventNoahPro);
                return true;

            case formDisplayPrefs:
                FrmSetEventHandler(frm, DisplayPrefFormHandleEvent);
                return true;

            case formPronunciation:
                FrmSetEventHandler(frm, PronunciationFormHandleEvent);
                return true;

            case formBookmarks:
                FrmSetEventHandler(frm, BookmarksFormHandleEvent);
                return true;

            default:
                Assert(0);
                break;
        }
        Assert(!error);
    }
    else if (event->eType == winDisplayChangedEvent)
        SyncScreenSize(appContext);
    return false;
}

static void EventLoopNoahPro(AppContext* appContext)
{
    EventType event;
    Word      error;

    event.eType = (eventsEnum) 0;
    while ( (event.eType != appStopEvent) && (ERR_NONE == appContext->err) )
    {
        EvtGetEvent(&event, appContext->ticksEventTimeout);
        if (SysHandleEvent(&event))
            continue;
        // according to docs error is always set to 0
        if (MenuHandleEvent(0, &event, &error))
            continue;
        if (HandleEventNoahPro(appContext, &event))
            continue;
        FrmDispatchEvent(&event);
    }
#if 0
    switch( appContext->err )
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

Err AppLaunch(char *cmdPBP) 
{
    Err error=errNone;
    AppContext* appContext=(AppContext*)MemPtrNew(sizeof(AppContext));
    if (!appContext)
    {
        error=!errNone;
        goto OnError;
    }     

    error=InitNoahPro(appContext);
    if (error) 
        goto OnError;

    if (cmdPBP)
        ParseResidentWord(appContext, cmdPBP);

    FrmGotoForm(formDictMain);
    EventLoopNoahPro(appContext);
    StopNoahPro(appContext);
OnError: 
    if (appContext) 
        MemPtrFree(appContext);
    return error;
}



