/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include <PalmCompatibility.h>
#include "noah_lite.h"
#include "display_info.h"
#include "wn_lite_ex_support.h"

#define MAX_WORD_LABEL_LEN 18


#if 0
static char wait_str[] = "Searching...";
#endif

GlobalData gd;

void DictCurrentFree(void)
{
    // TODO: SavePreferences();
    dictDelete();
    if ( NULL != GetCurrentFile() )
    {
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

    // TODO: save last used database in preferences
    gd.wordsCount = dictGetWordsCount();
    gd.currentWord = 0;
    gd.listItemOffset = 0;
    LogV1( "DictInit(%s) ok", file->fileName );
    return true;
}

void StopNoahLite()
{
    Err error=errNone;
    DictCurrentFree();
    FreeDicts();
    FreeInfoData();

// 2003-11-28 andrzejc DynamicInputArea
    error=DIA_Free(&gd.diaSettings);
    Assert(!error);

    CloseMatchingPatternDB();

    FrmSaveAllForms();
    FrmCloseAllForms();
    FsDeinit();
}

Err InitNoahLite(void)
{
    Err error=errNone;
    UInt32 value=0;
    MemSet((void *) &gd, sizeof(GlobalData), 0);
    LogInit( &g_Log, "c:\\noah_lite_log.txt" );

    InitFiveWay();

    SetCurrentFile(NULL);

    gd.penUpsToConsume = 0;
    gd.selectedWord = 0;
    gd.prevSelectedWord = 0xfffff;
    // disable getting nilEvent
    gd.ticksEventTimeout = evtWaitForever;
#ifdef DEBUG
    gd.currentStressWord = 0;
#endif

// 2003-11-26 andrzejc DynamicInputArea
    SyncScreenSize();
// define _DONT_DO_HANDLE_DYNAMIC_INPUT_ to disable Pen Input Manager operations
#ifndef _DONT_DO_HANDLE_DYNAMIC_INPUT_
    error=DIA_Init(&gd.diaSettings);
    if (error) return error;
#endif  

    FsInit();

    // UNDONE: LoadPreferences()

    if (!CreateHelpData())
        return !errNone;

    return errNone;
}

void DisplayAbout(void)
{
    ClearDisplayRectangle();
    HideScrollbar();

    dh_set_current_y(7);

    dh_save_font();
    dh_display_string("Noah Lite", 2, 16);
#ifdef DEBUG
    dh_display_string("Ver 1.0 (debug)", 2, 20);
#else
    dh_display_string("Ver 1.0", 2, 20);
#endif
    dh_display_string("(C) 2000-2003 ArsLexis", 1, 24);
    dh_display_string("get Noah Pro at:", 2, 20);
    dh_display_string("http://www.arslexis.com", 2, 0);

    dh_restore_font();
}

void DictFoundCBNoahLite( AbstractFile *file )
{
    Assert( file );
    Assert(NOAH_LITE_CREATOR == file->creator);
    Assert(WORDNET_LITE_TYPE == file->type);

    if (gd.dictsCount>=MAX_DICTS)
    {
        AbstractFileFree(file);
        return;
    }

    gd.dicts[gd.dictsCount++] = file;
}

/* called for every file on the external card */
void VfsFindCbNoahLite( AbstractFile *file )
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
    DictFoundCBNoahLite( fileCopy );
}


void ScanForDictsNoahLite(void)
{
    FsMemFindDb( NOAH_LITE_CREATOR, WORDNET_LITE_TYPE, NULL, &DictFoundCBNoahLite );
    /* TODO: optimize by just looking in a few specific places like "/", "/Palm",
    "/Palm/Launcher", "xx/msfiles/xx" ? */
    FsVfsFindDb( &VfsFindCbNoahLite );
}

static Boolean MainFormDisplayChanged(FormType* frm) 
{
    Boolean handled=false;
    if (DIA_Supported(&gd.diaSettings))
    {
        UInt16 index=0;
        RectangleType newBounds;
        WinGetBounds(WinGetDisplayWindow(), &newBounds);
        WinSetBounds(FrmGetWindowHandle(frm), &newBounds);
        
        FrmSetObjectBoundsByID(frm, ctlArrowLeft, 0, gd.screenHeight-12, 8, 11);
        FrmSetObjectBoundsByID(frm, ctlArrowRight, 8, gd.screenHeight-12, 10, 11);
        FrmSetObjectBoundsByID(frm, scrollDef, gd.screenWidth-8, 1, 7, gd.screenHeight-18);
        FrmSetObjectBoundsByID(frm, bmpFind, gd.screenWidth-13, gd.screenHeight-13, 13, 13);
        FrmSetObjectBoundsByID(frm, buttonFind, gd.screenWidth-14, gd.screenHeight-14, 14, 14);

        RedrawMainScreen();
        handled=true;
    }
    return handled;
}

Boolean MainFormHandleEventNoahLite(EventType * event)
{
    Boolean         handled = false;
    FormType        *frm;
    Short           newValue;
    AbstractFile    *fileToOpen;
    int             dbsInInternalMemory;
    int             dbsOnExternalCard;
    int             i;
    EventType       newEvent;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case winEnterEvent:
            // workaround for probable Sony CLIE's bug that causes only part of screen to be repainted on return from PopUps
            if (DIA_HasSonySilkLib(&gd.diaSettings) && frm==(void*)((SysEventType*)event)->data.winEnter.enterWindow)
                RedrawMainScreen();
            break;

        case winDisplayChangedEvent:
            handled= MainFormDisplayChanged(frm);
            break;
            
        case frmUpdateEvent:
            LogG( "mainFrm - frmUpdateEvent" );
            RedrawMainScreen();
            return true;

        case frmOpenEvent:
            FrmDrawForm(frm);

// 2003-11-28 andrzejc DynamicInputArea  
//            WinDrawLine(0, 145, 160, 145);
//            WinDrawLine(0, 144, 160, 144);
            WinDrawLine(0, gd.screenHeight-FRM_RSV_H+1, gd.screenWidth, gd.screenHeight-FRM_RSV_H+1);
            WinDrawLine(0, gd.screenHeight-FRM_RSV_H, gd.screenWidth, gd.screenHeight-FRM_RSV_H);

            ScanForDictsNoahLite();
            if ( 0 == gd.dictsCount )
            {
    NoDatabaseFound:
                FrmAlert(alertNoDB);
    ExitProgram:
                MemSet(&newEvent, sizeof(EventType), 0);
                newEvent.eType = appStopEvent;
                EvtAddEventToQueue(&newEvent);
                return true;
            }

            dbsInInternalMemory = 0;
            dbsOnExternalCard = 0;
            for (i=0; i<gd.dictsCount; i++)
            {
                if ( eFS_MEM == gd.dicts[i]->fsType )
                {
                    ++dbsInInternalMemory;
                }
                else
                {
                    Assert( eFS_VFS == gd.dicts[i]->fsType );
                    ++dbsOnExternalCard;
                }
            }
            if (0==dbsInInternalMemory)
            {
                Assert( dbsOnExternalCard > 0 );
                Assert( dbsOnExternalCard + dbsInInternalMemory == gd.dictsCount );
                FrmAlert(alertNoInternalDB);
                goto ExitProgram;
            }

            // TODO: select database if > 1 (but we want to only allow one anyway)
            fileToOpen = NULL;
            i = 0;
            while ( (i<gd.dictsCount) && (fileToOpen == NULL) )
            {
                if ( eFS_MEM == gd.dicts[i]->fsType )
                    fileToOpen = gd.dicts[i];
                ++i;
            }

            Assert( NULL != fileToOpen );
            if ( NULL == fileToOpen )
                goto NoDatabaseFound;

            if ( !DictInit(fileToOpen) )
            {
                FrmAlert(alertDbFailed);
                goto ExitProgram;
            }

            DisplayAbout();
#if 0
            /* start the timer, so we'll switch to info
               text after a few seconds */
            gd.start_seconds_count = TimGetSeconds();
            gd.current_timeout = 50;
#endif
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case ctlArrowLeft:
                    if (gd.currentWord > 0)
                        DrawDescription(gd.currentWord - 1);
                    break;
                case ctlArrowRight:
                    if (gd.currentWord < gd.wordsCount - 1)
                        DrawDescription(gd.currentWord + 1);
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
            DrawDescription(gd.currentWord);
            gd.penUpsToConsume = 1;
            handled = true;
            break;

        case evtNewDatabaseSelected:
            DisplayAbout();
            /* start the timer, so we'll switch to info
               text after a few seconds */
    /*      gd.start_seconds_count = TimGetSeconds();
          gd.current_       timeout = 50; */
            handled = true;
            break;      

        case keyDownEvent:
            if ( HaveFiveWay() && EvtKeydownIsVirtual(event) && IsFiveWayEvent(event) )
            {
                if (FiveWayCenterPressed(event))
                {
                    FrmPopupForm(formDictFind);
                }
                if (FiveWayDirectionPressed( event, Left ))
                {
                    if (gd.currentWord > 0)
                        DrawDescription(gd.currentWord - 1);
                }
                if (FiveWayDirectionPressed( event, Right ))
                {
                    if (gd.currentWord < gd.wordsCount - 1)
                        DrawDescription(gd.currentWord + 1);
                }
                if (FiveWayDirectionPressed( event, Up ))
                {
                    DefScrollUp( scrollLine );
                }
                if (FiveWayDirectionPressed( event, Down ))
                {
                    DefScrollDown( scrollLine );
                }
                return false;
            }
            else if (pageUpChr == event->data.keyDown.chr)
            {
                DefScrollUp( scrollLine );
            }
            else if (pageDownChr == event->data.keyDown.chr)
            {
                DefScrollDown( scrollLine );
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
                ClearDisplayRectangle();
                gd.firstDispLine = newValue;
                DrawDisplayInfo(gd.currDispInfo, gd.firstDispLine, DRAW_DI_X, DRAW_DI_Y, gd.dispLinesCount);
            }
            handled = true;
            break;

        case penUpEvent:
            if (0 != gd.penUpsToConsume > 0)
            {
                --gd.penUpsToConsume;
                handled = true;
                break;
            }
// 2003-11-28 andrzejc DynamicInputArea
//            if ((NULL == gd.currDispInfo) || (event->screenX > 150) || (event->screenY > 144))
            if ((NULL == gd.currDispInfo) || (event->screenX > gd.screenWidth-FRM_RSV_W) || (event->screenY > gd.screenHeight-FRM_RSV_H))
            {
                handled = true;
                break;
            }

            if (event->screenY > 87)
            {
                DefScrollDown( scrollLine );
            }
            else
            {
                DefScrollUp( scrollLine );
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
                    if (NULL != gd.currDispInfo)
                    {
                        diFree(gd.currDispInfo);
                        gd.currDispInfo = NULL;
                        gd.currentWord = 0;
                    }
                    DisplayAbout();
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
                case menuItemHelp:
                    DisplayHelp();
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

//#define WORDS_IN_LIST 12

static Boolean FindFormDisplayChanged(FormType* frm) 
{
    Boolean handled=false;
    if (DIA_Supported(&gd.diaSettings))
    {
        UInt16 index=0;
        ListType* list=0;
        RectangleType newBounds;
        WinGetBounds(WinGetDisplayWindow(), &newBounds);
        WinSetBounds(FrmGetWindowHandle(frm), &newBounds);

        FrmSetObjectBoundsByID(frm, ctlArrowLeft, 0, gd.screenHeight-12, 8, 11);
        FrmSetObjectBoundsByID(frm, ctlArrowRight, 8, gd.screenHeight-12, 10, 11);
        
        index=FrmGetObjectIndex(frm, listMatching);
        Assert(index!=frmInvalidObjectId);
        list=(ListType*)FrmGetObjectPtr(frm, index);
        Assert(list);
        LstSetHeight(list, gd.dispLinesCount);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.extent.x=gd.screenWidth;
        FrmSetObjectBounds(frm, index, &newBounds);
        
        index=FrmGetObjectIndex(frm, fieldWord);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=gd.screenHeight-13;
        newBounds.extent.x=gd.screenWidth-66;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonCancel);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.x=gd.screenWidth-40;
        newBounds.topLeft.y=gd.screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        FrmDrawForm(frm);
        handled=true;
    }
    return handled;
}


Boolean FindFormHandleEventNoahLite(EventType * event)
{
    Boolean     handled = false;
    char *      word;
    FormPtr     frm;
    FieldPtr    fld;
    ListPtr     list;

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled= FindFormDisplayChanged(frm);
            break;
                        
        case frmOpenEvent:
            fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
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
            handled = true;
            break;

        case lstSelectEvent:
            /* copy word from text field to a buffer, so next time we
               come back here, we'll come back to the same place */
            RememberLastWord(frm);
            /* set the selected word as current word */
            gd.currentWord = gd.listItemOffset + (long) event->data.lstSelect.selection;
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
                    RememberLastWord(frm);
                    gd.currentWord = gd.selectedWord;
                    Assert(gd.currentWord < gd.wordsCount);
                    SendNewWordSelected();
                    FrmReturnToForm(0);
                    return true;

                case pageUpChr:
                    if ( ! (HaveFiveWay() && EvtKeydownIsVirtual(event) && IsFiveWayEvent(event) ) )
                    {
                        ScrollWordListByDx( frm, -(gd.dispLinesCount-1) );
                        return true;
                    }
                
                case pageDownChr:
                    if ( ! (HaveFiveWay() && EvtKeydownIsVirtual(event) && IsFiveWayEvent(event) ) )
                    {
                        ScrollWordListByDx( frm, (gd.dispLinesCount-1) );
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
                            ScrollWordListByDx( frm, -(gd.dispLinesCount-1) );
                            return true;
                        }
                        if (FiveWayDirectionPressed( event, Right ))
                        {
                            ScrollWordListByDx( frm, (gd.dispLinesCount-1) );
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
                        }                        return false;
                    }
                    break;
            }
            SendFieldChanged();
            handled = false;
            break;

        case evtFieldChanged:
            DoFieldChanged();
            return true;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    RememberLastWord(frm);
                    FrmReturnToForm(0);
                    break;
                case ctlArrowLeft:
                    ScrollWordListByDx( frm, -(gd.dispLinesCount-1) );
                    break;
                case ctlArrowRight:
                    ScrollWordListByDx( frm, (gd.dispLinesCount-1) );
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
static Boolean FindPatternFormDisplayChanged(FormType* frm) 
{
    Boolean handled=false;
    if (DIA_Supported(&gd.diaSettings))
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
        LstSetHeight(list, gd.dispLinesCount);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.extent.x=gd.screenWidth;
        FrmSetObjectBounds(frm, index, &newBounds);
        
        index=FrmGetObjectIndex(frm, fieldWord);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=gd.screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonFind);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=gd.screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonCancel);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=gd.screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonOneChar);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=gd.screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonAnyChar);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=gd.screenHeight-14;
        FrmSetObjectBounds(frm, index, &newBounds);

        FrmDrawForm(frm);
        handled=true;
    }
    return handled;
}

Boolean FindPatternFormHandleEventNoahLite(EventType * event)
{
    Boolean     handled = false;
    FormPtr     frm = FrmGetActiveForm();
    FieldPtr    fld;
    ListPtr     list;
    static long lastWordPos;
    char *      pattern;
    long        prevMatchWordCount=0;

    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled = FindPatternFormDisplayChanged(frm);
            break;
                        
        case frmOpenEvent:
            OpenMatchingPatternDB();
            list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
            fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
            prevMatchWordCount = NumMatchingPatternRecords();
            LstSetListChoicesEx(list, NULL, prevMatchWordCount );
            LstSetDrawFunction(list, PatternListDrawFunc);
            FrmDrawForm(frm);
            if (prevMatchWordCount>0)
                LstSetSelectionEx(list, lastWordPos);
            pattern = (char *) new_malloc(WORDS_CACHE_SIZE);
            ReadPattern(pattern);
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
                    CloseMatchingPatternDB();
                    FrmReturnToForm(0);
                    handled = true;
                    break;

                case buttonFind:
                    fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
                    list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
                    pattern = FldGetTextPtr(fld);
                    if (pattern != NULL) {
                        ClearMatchingPatternDB();
                        WritePattern(pattern);
                        FillMatchingPatternDB(pattern);
                        LstSetListChoicesEx(list, NULL, NumMatchingPatternRecords());
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
            lastWordPos = gd.listItemOffset + (UInt32) event->data.lstSelect.selection;
            ReadMatchingPatternRecord(lastWordPos, &gd.currentWord);
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
            LstSetListChoices(list, NULL, gd.dictsCount);
            LstSetSelection(list, selectedDb);
            LstMakeItemVisible(list, selectedDb);
            if (-1 == gd.currentDb)
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
                    if (gd.currentDb != selectedDb)
                    {
                        if (-1 != gd.currentDb)
                            DictCurrentFree();
                        DictInit(selectedDb);
                    }
                    MemSet(&newEvent, sizeof(EventType), 0);
                    newEvent.eType = (eventsEnum) evtNewDatabaseSelected;
                    EvtAddEventToQueue(&newEvent);
                    FrmReturnToForm(0);
                    return true;
                case buttonCancel:
                    if (-1 == gd.currentDb)
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

Boolean HandleEventNoahLite(EventType * event)
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
        error=DefaultFormInit(frm);    

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

void EventLoopNoahLite(void)
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
        if (HandleEventNoahLite(&event))
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
            err = InitNoahLite();
            if (err)
                return err;
            FrmGotoForm(formDictMain);
            EventLoopNoahLite();
            StopNoahLite();
            break;

// 2003-11-28 andrzejc DynamicInputArea     
        case sysAppLaunchCmdNotify:
            AppHandleSysNotify((SysNotifyParamType*)cmdPBP);
            break;
        
            
        default:
            break;
    }
    return 0;
}

