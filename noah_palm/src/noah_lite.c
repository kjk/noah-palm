/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
 
#include "common.h"
#include "five_way_nav.h"
#include "word_matching_pattern.h"

#define MAX_WORD_LABEL_LEN 18


#if 0
static char wait_str[] = "Searching...";
#endif

static void DictCurrentFree(AppContext* appContext)
{
    // TODO: SavePreferences();
    AbstractFile* file=GetCurrentFile(appContext);
    if ( NULL != file )
    {
        dictDelete(file);
        FsFileClose( file );
        SetCurrentFile(appContext, NULL);
    }
}

static Boolean DictInit(AppContext* appContext, AbstractFile *file)
{
    if ( !FsFileOpen(appContext, file ) )
    {
        LogV1( "DictInit(%s), FsFileOpen() failed", file->fileName );
        return false;
    }

    if ( !dictNew(file) )
    {
        LogV1( "DictInit(%s), FsFileOpen() failed", file->fileName );
        return false;
    }

    // TODO: save last used database in preferences
    appContext->wordsCount = dictGetWordsCount(file);
    appContext->currentWord = 0;
    appContext->listItemOffset = 0;
    LogV1( "DictInit(%s) ok", file->fileName );
    return true;
}

static void StopNoahLite(AppContext* appContext)
{
    Err error=errNone;
    DictCurrentFree(appContext);
    FreeDicts(appContext);
    FreeInfoData(appContext);
    bfFreePTR(appContext);
    
    error=DIA_Free(&appContext->diaSettings);
    Assert(!error);

    CloseMatchingPatternDB(appContext);

    FrmSaveAllForms();
    FrmCloseAllForms();
    FsDeinit(&appContext->fsSettings);
}

static Err InitNoahLite(AppContext* appContext)
{
    Err error=errNone;
    UInt32 value=0;
    Boolean res=false;
    MemSet(appContext, sizeof(*appContext), 0);
    error=FtrSet(APP_CREATOR, appFtrContext, (UInt32)appContext);
    if (error) 
        goto OnError;
    error=FtrSet(APP_CREATOR, appFtrLeaksFile, 0);
    if (error) 
        goto OnError;
    
    LogInit( appContext, "c:\\noah_pro_log.txt" );
    InitFiveWay(appContext);

    appContext->penUpsToConsume = 0;
    appContext->selectedWord = 0;
    appContext->prevSelectedWord = 0xfffff;
    // disable getting nilEvent
    appContext->ticksEventTimeout = evtWaitForever;
#ifdef DEBUG
    appContext->currentStressWord = 0;
#endif

    // fill out the default display preferences
    appContext->prefs.displayPrefs.listStyle = 0;
    SetDefaultDisplayParam(&appContext->prefs.displayPrefs,false,false);
    appContext->ptrOldDisplayPrefs = NULL;
    
    SyncScreenSize(appContext);
    FsInit(&appContext->fsSettings);

    // UNDONE: LoadPreferences()

    res=CreateHelpData(appContext);
    Assert(res);

// define _DONT_DO_HANDLE_DYNAMIC_INPUT_ to disable Pen Input Manager operations
#ifndef _DONT_DO_HANDLE_DYNAMIC_INPUT_
    error=DIA_Init(&appContext->diaSettings);
#endif  

OnError:
    return error;
}

void DisplayAbout(AppContext* appContext)
{
    UInt16 currentY=0;
    WinPushDrawState();
    ClearDisplayRectangle(appContext);
    HideScrollbar();
    
    currentY=7;
    FntSetFont(largeFont);
    DrawCenteredString(appContext, "Noah Lite", currentY);
    currentY+=16;
    
#ifdef DEBUG
    DrawCenteredString(appContext, "Ver 1.0 (debug)", currentY);
#else
    DrawCenteredString(appContext, "Ver 1.0", currentY);
#endif
    currentY+=20;
    
    FntSetFont(boldFont);
    DrawCenteredString(appContext, "(C) 2003-2004 ArsLexis", currentY);
    currentY+=24;
    
    FntSetFont(largeFont);
    DrawCenteredString(appContext, "get Noah Pro at:", currentY);
    currentY+=20;
    DrawCenteredString(appContext, "http://www.arslexis.com", currentY);

    WinPopDrawState();    
}

static void DictFoundCBNoahLite(void * context, AbstractFile *file )
{
    AppContext* appContext=(AppContext*)context;
    Assert(appContext);
    Assert( file );
    Assert(NOAH_LITE_CREATOR == file->creator);
    Assert(WORDNET_LITE_TYPE == file->type);

    if (appContext->dictsCount>=MAX_DICTS)
    {
        AbstractFileFree(file);
        return;
    }

    appContext->dicts[appContext->dictsCount++] = file;
}

/* called for every file on the external card */
static void VfsFindCbNoahLite(void* context, AbstractFile *file )
{
    AbstractFile *fileCopy;

    if ( !(NOAH_LITE_CREATOR == file->creator) )
        return;

    if ( WORDNET_LITE_TYPE != file->type )
        return;

    if ( eFS_MEM == file->fsType )
    {
        if ( 0 != StrCaselessCompare( file->fileName, "Wordnet medium" ) )
        {
            /* Filter out all except wordnet medium */
            LogV1( "VfsFindCbNoahLite(), filtered out %s", file->fileName );
            return;
        }
    }

    fileCopy = AbstractFileNewFull( file->fsType, file->creator, file->type, file->fileName );
    if ( NULL == fileCopy ) return;
    fileCopy->volRef = file->volRef;
    DictFoundCBNoahLite( context, fileCopy );
}


static void ScanForDictsNoahLite(AppContext* appContext)
{
    FsMemFindDb( NOAH_LITE_CREATOR, WORDNET_LITE_TYPE, NULL, &DictFoundCBNoahLite, appContext);
    /* TODO: optimize by just looking in a few specific places like "/", "/Palm",
    "/Palm/Launcher", "xx/msfiles/xx" ? */
    FsVfsFindDb(&appContext->fsSettings, &VfsFindCbNoahLite, appContext);
}

static Boolean MainFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Boolean handled=false;
    if (DIA_Supported(&appContext->diaSettings))
    {
        UInt16 index=0;
        RectangleType newBounds;
        WinGetBounds(WinGetDisplayWindow(), &newBounds);
        WinSetBounds(FrmGetWindowHandle(frm), &newBounds);
        
        FrmSetObjectBoundsByID(frm, ctlArrowLeft, 0, appContext->screenHeight-12, 8, 11);
        FrmSetObjectBoundsByID(frm, ctlArrowRight, 8, appContext->screenHeight-12, 10, 11);
        FrmSetObjectBoundsByID(frm, scrollDef, appContext->screenWidth-8, 1, 7, appContext->screenHeight-18);
        FrmSetObjectBoundsByID(frm, bmpFind, appContext->screenWidth-13, appContext->screenHeight-13, 13, 13);
        FrmSetObjectBoundsByID(frm, buttonFind, appContext->screenWidth-14, appContext->screenHeight-14, 14, 14);

        RedrawMainScreen(appContext);
        handled=true;
    }
    return handled;
}

static Boolean MainFormHandleEventNoahLite(EventType * event)
{
    Boolean         handled = false;
    FormType        *frm;
    Short           newValue;
    AbstractFile    *fileToOpen;
    int             dbsInInternalMemory;
    int             dbsOnExternalCard;
    int             i;
    AppContext* appContext=GetAppContext();

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
            LogG( "mainFrm - frmUpdateEvent" );
            RedrawMainScreen(appContext);
            return true;

        case frmOpenEvent:
            FrmDrawForm(frm);

            WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
            WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);

            ScanForDictsNoahLite(appContext);
            if ( 0 == appContext->dictsCount )
            {
    NoDatabaseFound:
                FrmAlert(alertNoDB);
    ExitProgram:
                SendStopEvent();
                return true;
            }

            dbsInInternalMemory = 0;
            dbsOnExternalCard = 0;
            for (i=0; i<appContext->dictsCount; i++)
            {
                if ( eFS_MEM == appContext->dicts[i]->fsType )
                {
                    ++dbsInInternalMemory;
                }
                else
                {
                    Assert( eFS_VFS == appContext->dicts[i]->fsType );
                    ++dbsOnExternalCard;
                }
            }
            if (0==dbsInInternalMemory)
            {
                Assert( dbsOnExternalCard > 0 );
                Assert( dbsOnExternalCard + dbsInInternalMemory == appContext->dictsCount );
                FrmAlert(alertNoInternalDB);
                goto ExitProgram;
            }

            // TODO: select database if > 1 (but we want to only allow one anyway)
            fileToOpen = NULL;
            i = 0;
            while ( (i<appContext->dictsCount) && (fileToOpen == NULL) )
            {
                if ( eFS_MEM == appContext->dicts[i]->fsType )
                    fileToOpen = appContext->dicts[i];
                ++i;
            }

            Assert( NULL != fileToOpen );
            if ( NULL == fileToOpen )
                goto NoDatabaseFound;

            if ( !DictInit(appContext, fileToOpen) )
            {
                FrmAlert(alertDbFailed);
                goto ExitProgram;
            }

            DisplayAbout(appContext);
#if 0
            /* start the timer, so we'll switch to info
               text after a few seconds */
            appContext->start_seconds_count = TimGetSeconds();
            appContext->current_timeout = 50;
#endif
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case ctlArrowLeft:
                    if (appContext->currentWord > 0)
                        DrawDescription(appContext, appContext->currentWord - 1);
                    break;
                case ctlArrowRight:
                    if (appContext->currentWord < appContext->wordsCount - 1)
                        DrawDescription(appContext, appContext->currentWord + 1);
                    break;
                case buttonFind:
                    FrmPopupForm(formDictFind);
                    break;
                default:
                    Assert(0);
                    break;
            }
            handled = true;
            break;

        case evtNewWordSelected:
            DrawDescription(appContext, appContext->currentWord);
            appContext->penUpsToConsume = 1;
            handled = true;
            break;

        case evtNewDatabaseSelected:
            DisplayAbout(appContext);
            /* start the timer, so we'll switch to info
               text after a few seconds */
    /*      appContext->start_seconds_count = TimGetSeconds();
          appContext->current_       timeout = 50; */
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
                        DrawDescription(appContext, appContext->currentWord - 1);
                }
                if (FiveWayDirectionPressed(appContext, event, Right ))
                {
                    if (appContext->currentWord < appContext->wordsCount - 1)
                        DrawDescription(appContext, appContext->currentWord + 1);
                }
                if (FiveWayDirectionPressed(appContext, event, Up ))
                {
                    DefScrollUp(appContext, scrollLine );
                }
                if (FiveWayDirectionPressed(appContext, event, Down ))
                {
                    DefScrollDown(appContext, scrollLine );
                }
                return false;
            }
            else if (pageUpChr == event->data.keyDown.chr)
            {
                DefScrollUp(appContext, scrollLine );
            }
            else if (pageDownChr == event->data.keyDown.chr)
            {
                DefScrollDown(appContext, scrollLine );
            }
            else if (((event->data.keyDown.chr >= 'a')  && (event->data.keyDown.chr <= 'z'))
                     || ((event->data.keyDown.chr >= 'A') && (event->data.keyDown.chr <= 'Z'))
                     || ((event->data.keyDown.chr >= '0') && (event->data.keyDown.chr <= '9')))
            {
                appContext->lastWord[0] = event->data.keyDown.chr;
                appContext->lastWord[1] = 0;
                FrmPopupForm(formDictFind);
            }        
            handled = true;
            break;

        case sclExitEvent:
            newValue = event->data.sclRepeat.newValue;
            if (newValue != appContext->firstDispLine)
            {
                SetGlobalBackColor(appContext);
                ClearDisplayRectangle(appContext);
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
            if (0 != appContext->penUpsToConsume > 0)
            {
                --appContext->penUpsToConsume;
                handled = true;
                break;
            }
            if ((NULL == appContext->currDispInfo) || (event->screenX > appContext->screenWidth-FRM_RSV_W) || (event->screenY > appContext->screenHeight-FRM_RSV_H))
            {
                handled = true;
                break;
            }

            if(cbPenUpEvent(appContext,event->screenX,event->screenY))
            {
                handled = true;
                break;
            }

            if (event->screenY > (appContext->screenHeight-FRM_RSV_H)/2)
            {
                DefScrollDown(appContext, scrollLine );
            }
            else
            {
                DefScrollUp(appContext, scrollLine );
            }
            handled = true;
            break;
        case menuEvent:
            switch (event->data.menu.itemID)
            {
                case menuItemFind:
                    FrmPopupForm(formDictFind);
                    break;
                case menuItemFindPattern:
                    FrmPopupForm(formDictFindPattern);
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
#ifdef DEBUG
                case menuItemStress:
                    // initiate stress i.e. going through all the words
                    // 0 means that stress is not in progress
                    appContext->currentStressWord = -1;
                    // get nilEvents as fast as possible
                    appContext->ticksEventTimeout = 0;
                    break;
#endif
                case menuItemHelp:
                    DisplayHelp(appContext);
                    break;
#if 0
                case menuItemSelectDB:
                    FrmPopupForm(formSelectDict);
                    break;
#endif
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

#ifdef NEVER
        case nilEvent:
            if (-1 != appContext->start_seconds_count)
            {
                /* we're still displaying About info, check
                   if it's time to switch to info */
                Assert(appContext->start_seconds_count <= TimGetSeconds());
                if (NULL == appContext->currDispInfo)
                {
                    if (TimGetSeconds() - appContext->start_seconds_count > 5)
                    {
                        DisplayHelp();
                        /* we don't nid evtNil events anymore */
                        appContext->start_seconds_count = -1;
                        appContext->current_timeout = -1;
                    }
                }
                else
                {
                    appContext->start_seconds_count = -1;
                    appContext->current_timeout = -1;
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

static Boolean FindFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Boolean handled=false;
    if (DIA_Supported(&appContext->diaSettings))
    {
        UInt16 index=0;
        ListType* list=0;
        RectangleType newBounds;
        WinGetBounds(WinGetDisplayWindow(), &newBounds);
        WinSetBounds(FrmGetWindowHandle(frm), &newBounds);

        FrmSetObjectBoundsByID(frm, ctlArrowLeft, 0, appContext->screenHeight-12, 8, 11);
        FrmSetObjectBoundsByID(frm, ctlArrowRight, 8, appContext->screenHeight-12, 10, 11);
        
        index=FrmGetObjectIndex(frm, listMatching);
        Assert(index!=frmInvalidObjectId);
        list=(ListType*)FrmGetObjectPtr(frm, index);
        Assert(list);
        LstSetHeight(list, appContext->dispLinesCount);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.extent.x=appContext->screenWidth;
        FrmSetObjectBounds(frm, index, &newBounds);
        
        index=FrmGetObjectIndex(frm, fieldWord);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-13;
        newBounds.extent.x=appContext->screenWidth-66;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonCancel);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.x=appContext->screenWidth-40;
        newBounds.topLeft.y=appContext->screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        FrmDrawForm(frm);
        handled=true;
    }
    return handled;
}


static Boolean FindFormHandleEventNoahLite(EventType * event)
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
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
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
                SendFieldChanged();
            }
            FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWord));
            FrmDrawForm(frm);
            handled = true;
            break;

        case lstSelectEvent:
            /* copy word from text field to a buffer, so next time we
               come back here, we'll come back to the same place */
            RememberLastWord(appContext, frm);
            /* set the selected word as current word */
            appContext->currentWord = appContext->listItemOffset + (long) event->data.lstSelect.selection;
            /* send a msg to yourself telling that a new word
               have been selected so we need to draw the
               description */
            Assert(appContext->currentWord < appContext->wordsCount);
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
                    RememberLastWord(appContext, frm);
                    appContext->currentWord = appContext->selectedWord;
                    Assert(appContext->currentWord < appContext->wordsCount);
                    SendNewWordSelected();
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
                            RememberLastWord(appContext, frm);
                            appContext->currentWord = appContext->selectedWord;
                            Assert(appContext->currentWord < appContext->wordsCount);
                            SendNewWordSelected();
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
            SendFieldChanged();
            handled = false;
            break;

        case evtFieldChanged:
            DoFieldChanged(appContext);
            return true;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    RememberLastWord(appContext, frm);
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

// Event handler proc for the find word using pattern dialog
static Boolean FindPatternFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Boolean handled=false;
    if (DIA_Supported(&appContext->diaSettings))
    {
        UInt16 index=0;
        ListType* list=0;
        RectangleType newBounds;
        WinGetBounds(WinGetDisplayWindow(), &newBounds);
        WinSetBounds(FrmGetWindowHandle(frm), &newBounds);

        index=FrmGetObjectIndex(frm, listMatching);
        Assert(index!=frmInvalidObjectId);
        list=(ListType*)FrmGetObjectPtr(frm, index);
        Assert(list);
        LstSetHeight(list, appContext->dispLinesCount);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.extent.x=appContext->screenWidth;
        FrmSetObjectBounds(frm, index, &newBounds);
        
        index=FrmGetObjectIndex(frm, fieldWord);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonFind);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonCancel);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonOneChar);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonAnyChar);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        FrmDrawForm(frm);
        handled=true;
    }
    return handled;
}

static Boolean FindPatternFormHandleEventNoahLite(EventType * event)
{
    Boolean     handled = false;
    FormPtr     frm = FrmGetActiveForm();
    FieldPtr    fld;
    ListPtr     list;
    char *      pattern;
    long        prevMatchWordCount=0;
    AppContext* appContext=GetAppContext();    

    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled = FindPatternFormDisplayChanged(appContext, frm);
            break;
                        
        case frmOpenEvent:
            OpenMatchingPatternDB(appContext);
            list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
            fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
            prevMatchWordCount = NumMatchingPatternRecords(appContext);
            LstSetListChoicesEx(list, NULL, prevMatchWordCount );
            LstSetDrawFunction(list, PatternListDrawFunc);
            FrmDrawForm(frm);
            if (prevMatchWordCount>0)
                LstSetSelectionEx(appContext, list, appContext->wmpLastWordPos);
            pattern = (char *) new_malloc(WORDS_CACHE_SIZE);
            ReadPattern(appContext, pattern);
            if (StrLen(pattern) > 0)
                FldInsert(fld, pattern, StrLen(pattern));
            FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWord));
            new_free(pattern);
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    CloseMatchingPatternDB(appContext);
                    FrmReturnToForm(0);
                    handled = true;
                    break;

                case buttonFind:
                    fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
                    list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
                    pattern = FldGetTextPtr(fld);
                    if (pattern != NULL) {
                        ClearMatchingPatternDB(appContext);
                        WritePattern(appContext, pattern);
                        FillMatchingPatternDB(appContext, pattern);
                        LstSetListChoicesEx(list, NULL, NumMatchingPatternRecords(appContext));
                        LstSetDrawFunction(list, PatternListDrawFunc);
                        LstDrawList(list);
                    }
                    handled = true;
                    break;
                
                case buttonOneChar:
                    fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
                    FldInsert(fld, "?", 1);
                    handled = true;
                    break;

                case buttonAnyChar:
                    fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
                    FldInsert(fld, "*", 1);
                    handled = true;
                    break;

                default:
                    break;
            }
            break;

        case lstSelectEvent:
            appContext->wmpLastWordPos = appContext->listItemOffset + (UInt32) event->data.lstSelect.selection;
            ReadMatchingPatternRecord(appContext, appContext->wmpLastWordPos, &appContext->currentWord);
            SendNewWordSelected();
            FrmReturnToForm(0);
            return true;

        default:
            break;
    }
    
    return handled;
}

#if 0
Boolean SelectDictFormHandleEventNoahLite(EventType * event)
{
    FormPtr frm;
    ListPtr list;
    static int selectedDb = 0;
    EventType newEvent;

    switch (event->eType)
    {
        case frmOpenEvent:
            frm = FrmGetActiveForm();
            list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listOfDicts));
            LstSetDrawFunction(list, ListDbDrawFunc);
            LstSetListChoices(list, NULL, appContext->dictsCount);
            LstSetSelection(list, selectedDb);
            LstMakeItemVisible(list, selectedDb);
            if (-1 == appContext->currentDb)
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
                    if (appContext->currentDb != selectedDb)
                    {
                        if (-1 != appContext->currentDb)
                            DictCurrentFree();
                        DictInit(selectedDb);
                    }
                    MemSet(&newEvent, sizeof(EventType), 0);
                    newEvent.eType = (eventsEnum) evtNewDatabaseSelected;
                    EvtAddEventToQueue(&newEvent);
                    FrmReturnToForm(0);
                    return true;
                case buttonCancel:
                    if (-1 == appContext->currentDb)
                        Assert(0);
                    FrmReturnToForm(0);
                    return true;
            }
            break;
        default:
            break;
    }
    return false;
}
#endif

static Boolean HandleEventNoahLite(AppContext* appContext, EventType * event)
{
    FormPtr frm;
    Int formId;
    Boolean handled = false;
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
                FrmSetEventHandler(frm, MainFormHandleEventNoahLite);
                break;
            case formDictFind:
                FrmSetEventHandler(frm, FindFormHandleEventNoahLite);
                break;
            case formDictFindPattern:
                FrmSetEventHandler(frm, FindPatternFormHandleEventNoahLite);
                break;
#if 0
            case formSelectDict:
                FrmSetEventHandler(frm, SelectDictFormHandleEventNoahLite);
                break;
#endif
            default:
                Assert(0);
                break;
        }
        handled = true;
        Assert(!error);
    }
    return handled;
}

static void EventLoopNoahLite(AppContext* appContext)
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
        if (HandleEventNoahLite(appContext, &event))
            continue;
        FrmDispatchEvent(&event);
    }
}

static Err AppLaunch() 
{
    Err error=errNone;
    AppContext* appContext=(AppContext*)MemPtrNew(sizeof(AppContext));
    if (!appContext)
    {
        error=!errNone;
        goto OnError;
    }     
    error=InitNoahLite(appContext);
    if (error) 
        goto OnError;
    FrmGotoForm(formDictMain);
    EventLoopNoahLite(appContext);
    StopNoahLite(appContext);
OnError: 
    if (appContext) 
        MemPtrFree(appContext);
    return error;
}


DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Err err;
    switch (cmd)
    {
        case sysAppLaunchCmdNormalLaunch:
            err=AppLaunch();
            break;

        case sysAppLaunchCmdNotify:
            err=AppHandleSysNotify((SysNotifyParamType*)cmdPBP);
            break;
        
            
        default:
            break;
    }
    return err;
}

