/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk (krzysztofk@pobox.com)
  Author: Andrzej Ciarkowski

  Implements handling of the main form for iNoah.
*/

#include "main_form.h"
#include "inet_word_lookup.h"
#include "five_way_nav.h"
#include "bookmarks.h"

typedef enum MainFormUpdateCode_ 
{
    redrawAll=frmRedrawUpdateCode,
    redrawLookupStatusBar,
} MainFormUpdateCode;

const UInt16 lookupStatusBarHeight=14;

void MainFormPressFindButton(const FormType* form) 
{
    Assert(form);
    Assert(formDictMain==FrmGetFormId(form));
    UInt16 index=FrmGetObjectIndex(form, buttonFind);
    Assert(frmInvalidObjectId!=index);
    const ControlType* findButton=static_cast<const ControlType*>(FrmGetObjectPtr(form, index));
    Assert(findButton);
    CtlHitControl(findButton);
}

static void MainFormDisplayAbout(AppContext* appContext)
{
    UInt16 currentY=appContext->lookupStatusBarVisible?lookupStatusBarHeight+1:0;
    WinPushDrawState();
    SetGlobalBackColor(appContext);
    ClearRectangle(0, currentY, appContext->screenWidth, appContext->screenHeight - currentY - FRM_RSV_H);
    HideScrollbar();
    
    currentY+=7;
    FntSetFont(largeFont);
    DrawCenteredString(appContext, "ArsLexis iNoah", currentY);
    currentY+=16;
#ifdef DEMO
    DrawCenteredString(appContext, "Ver 0.5 (demo)", currentY);
#else
  #ifdef DEBUG
    DrawCenteredString(appContext, "Ver 0.5 (debug)", currentY);
  #else
    DrawCenteredString(appContext, "Ver 0.5", currentY);
  #endif
#endif
    currentY+=20;
    
    FntSetFont(boldFont);
    DrawCenteredString(appContext, "Copyright (c) ArsLexis", currentY);
    currentY+=24;

    FntSetFont(largeFont);
    DrawCenteredString(appContext, "http://www.arslexis.com", currentY);
    currentY+=40;
    
    WinPopDrawState();    
}

static void MainFormDrawLookupStatus(AppContext* appContext, FormType* form)
{
    WinPushDrawState();
    SetGlobalBackColor(appContext);
    ClearRectangle(0, 0, appContext->screenWidth, lookupStatusBarHeight);
    WinDrawLine(0, lookupStatusBarHeight, appContext->screenWidth, lookupStatusBarHeight);
    const Char* text=GetLookupStatusText(appContext);
    Assert(text);
    UInt16 textLen=StrLen(text);    
    WinDrawTruncChars(text, textLen, 1, 1, appContext->screenWidth - 14);
    WinPopDrawState();
}

static void MainFormDraw(AppContext* appContext, FormType* form, UInt16 updateCode=redrawAll)
{
    switch (updateCode)
    {
        case redrawAll:
            FrmDrawForm(form);
            WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
            WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);
            if (!appContext->currDispInfo)
                MainFormDisplayAbout(appContext);
            else 
            {
                UInt16 top=appContext->lookupStatusBarVisible?lookupStatusBarHeight+1:0;
                UInt16 linesCount=(appContext->screenHeight-FRM_RSV_H-top)/FONT_DY;
                WinPushDrawState();
                SetBackColorWhite(appContext);
                ClearRectangle(0, top, appContext->screenWidth, appContext->screenHeight - top - FRM_RSV_H);
                DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, 0, top, linesCount);
                SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
                WinPopDrawState();
            } 
            if (!appContext->lookupStatusBarVisible) 
                break; // fall through in case we should redraw status bar
                
        case redrawLookupStatusBar:
            MainFormDrawLookupStatus(appContext, form);
            break;
            
        default:            
            Assert(false);
    }            
}

static void MainFormHandleLookupProgress(AppContext* appContext, FormType* form, EventType* event)
{
    Assert(event);
    LookupProgressEventData* data=reinterpret_cast<LookupProgressEventData*>(&event->data);
    if (lookupProgress==data->flag) 
        FrmUpdateForm(formDictMain, redrawLookupStatusBar);
    else
    {
        UInt16 index=FrmGetObjectIndex(form, scrollDef);
        Assert(frmInvalidObjectId!=index);
        RectangleType bounds;
        FrmGetObjectBounds(form, index, &bounds);
        
        if (lookupFinished==data->flag)
        {
            appContext->lookupStatusBarVisible=false;
            
            bounds.topLeft.y-=lookupStatusBarHeight;
            bounds.extent.y+=lookupStatusBarHeight;
            FrmSetObjectBounds(form, index, &bounds);

            index=FrmGetObjectIndex(form, buttonAbortLookup);
            Assert(frmInvalidObjectId!=index);
            FrmHideObject(form, index);

            if (!data->error) 
                appContext->firstDispLine=0;
            index=FrmGetObjectIndex(form, fieldWordInput);
            Assert(frmInvalidObjectId!=index);
            FieldType* field=static_cast<FieldType*>(FrmGetObjectPtr(form, index));
            Assert(field);
            FldSelectAllText(field);
        }
        else 
        {
            Assert(lookupStarted==data->flag);
            appContext->lookupStatusBarVisible=true;
            
            bounds.topLeft.y+=lookupStatusBarHeight;
            bounds.extent.y-=lookupStatusBarHeight;
            FrmSetObjectBounds(form, index, &bounds);

            index=FrmGetObjectIndex(form, buttonAbortLookup);
            Assert(frmInvalidObjectId!=index);
            FrmShowObject(form, index);
        }
        FrmUpdateForm(formDictMain, redrawAll); 
    }        
}

static void MainFormFindButtonPressed(AppContext* appContext, FormType* form)
{
    const Char* newWord=NULL;
    UInt16 index=FrmGetObjectIndex(form, fieldWordInput);
    Assert(frmInvalidObjectId!=index);
    {
        FieldType* field=static_cast<FieldType*>(FrmGetObjectPtr(form, index));
        const Char* prevWord=ebufGetDataPointer(&appContext->currentWordBuf);
        Assert(field);
        newWord=FldGetTextPtr(field);
        if (newWord && (StrLen(newWord)>0) && (!prevWord || 0!=StrCompare(newWord, prevWord)))
            StartLookup(appContext, newWord);            
    }
}

static Boolean MainFormControlSelected(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled=false;
    switch (event->data.ctlSelect.controlID)
    {
        case buttonFind:
            MainFormFindButtonPressed(appContext, form);
            handled=true;
            break;
            
        case buttonAbortLookup:
            if (LookupInProgress(appContext))
                AbortCurrentLookup(appContext, true, netErrUserCancel);
            handled=true;
            break;

        default:
            Assert(false);
    }
    return handled;
}

static void MainFormLookupClipboard(AppContext* appContext)
{
    UInt16 length=0;
    MemHandle handle=ClipboardGetItem(clipboardText, &length);
    ExtensibleBuffer buffer;
    ebufInit(&buffer, 0);
    if (handle) 
    {
        const Char* text=static_cast<const Char*>(MemHandleLock(handle));
        if (text)
        {
            ebufAddStrN(&buffer, const_cast<Char*>(text), length);
            MemHandleUnlock(handle);
        }
    }
    if (ebufGetDataSize(&buffer))
    {
        ebufAddChar(&buffer, chrNull);
        StartLookup(appContext, ebufGetDataPointer(&buffer));
    }
    ebufFreeData(&buffer);
}

static Boolean MainFormOpen(AppContext* appContext, FormType* form, EventType* event)
{

    FrmUpdateForm(formDictMain, redrawAll);

    UInt16 index=FrmGetObjectIndex(form, fieldWordInput);
    FrmSetFocus(form, index);

    switch (appContext->prefs.startupAction)
    {
        case startupActionClipboard:
            MainFormLookupClipboard(appContext);
            break;
        
        case startupActionLast:
            if (StrLen(appContext->prefs.lastWord))
                StartLookup(appContext, appContext->prefs.lastWord);
            break;
    }
    
    return true;
}

static Boolean MainFormKeyDown(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled = false;

    if ( HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) )
    {
        if (FiveWayCenterPressed(appContext, event))
            MainFormPressFindButton(form);
        if (FiveWayDirectionPressed(appContext, event, Up ))
            DefScrollUp(appContext, appContext->prefs.navButtonScrollType );
        if (FiveWayDirectionPressed(appContext, event, Down ))
            DefScrollDown(appContext, appContext->prefs.navButtonScrollType );
        return true;
    }

    switch (event->data.keyDown.chr)
    {
        case returnChr:
        case linefeedChr:
            MainFormPressFindButton(form);
            handled = true;
            break;

        case pageUpChr:
            if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
            {
                DefScrollUp(appContext, appContext->prefs.hwButtonScrollType);
                handled = true;
                break;
            }

        case pageDownChr:
            if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
            {
                DefScrollUp(appContext, appContext->prefs.hwButtonScrollType);
                handled = true;
                break;
            }

        default:
            break;
    }
    return handled;
}

static Boolean MainFormScrollExit(AppContext* appContext, FormType* form, EventType* event)
{
    Int16 newValue = event->data.sclRepeat.newValue;
    if (newValue != appContext->firstDispLine)
    {
        appContext->firstDispLine = newValue;
        FrmUpdateForm(formDictMain, frmRedrawUpdateCode);
    }
    return true;
}

inline static void MainFormHandleAbout(AppContext* appContext)
{
    if (appContext->currDispInfo)
    {
        diFree(appContext->currDispInfo);
        appContext->currDispInfo=NULL;
        cbNoSelection(appContext);
        ebufFreeData(&appContext->currentWordBuf);
        FrmUpdateForm(formDictMain, redrawAll);    
    }
}

inline static void MainFormHandleCopy(AppContext* appContext)
{
    if (appContext->currDispInfo)
        diCopyToClipboard(appContext->currDispInfo);
}

static Boolean MainFormMenuCommand(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled=false;
    switch (event->data.menu.itemID)
    {
        case menuItemAbout: 
            MainFormHandleAbout(appContext);
            handled=true;
            break;
            
        case menuItemCopy:
            MainFormHandleCopy(appContext);
            handled=true;
            break;
            
        case menuItemLookupClipboard:
            MainFormLookupClipboard(appContext);
            handled=true;
            break;
            
        case menuItemPrefs:
            FrmPopupForm(formPrefs);
            handled=true;
            break;            

        case menuItemDispPrefs:
            FrmPopupForm(formDisplayPrefs);
            handled=true;
            break;        
            
        case menuItemBookmarkView:
            if (GetBookmarksCount(appContext)>0)
                FrmPopupForm(formBookmarks);
            else
                FrmAlert(alertNoBookmarks);
            handled=true;
            break;

        case menuItemBookmarkWord:
            if (ebufGetDataSize(&appContext->currentWordBuf))
                AddBookmark(appContext, ebufGetDataPointer(&appContext->currentWordBuf));
            handled=true;
            break;

        case menuItemBookmarkDelete:
            if (ebufGetDataSize(&appContext->currentWordBuf))
                DeleteBookmark(appContext, ebufGetDataPointer(&appContext->currentWordBuf));
            handled=true;
            break;
            
        case menuItemHelp:
            ebufFreeData(&appContext->currentWordBuf);
            DisplayHelp(appContext);
            handled=true;
            break;
            
        default:
            Assert(false);
    }
    return handled;
}

static Boolean MainFormDisplayChanged(AppContext* appContext, FormType* form) 
{
    Boolean handled=false;
    if (DIA_Supported(&appContext->diaSettings))
    {
        UInt16 index=0;
        RectangleType bounds;
        WinGetBounds(WinGetDisplayWindow(), &bounds);
        WinSetBounds(FrmGetWindowHandle(form), &bounds);
        
        index=FrmGetObjectIndex(form, buttonFind);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(form, index, &bounds);
        bounds.topLeft.y=appContext->screenHeight-13;
        bounds.topLeft.x=appContext->screenWidth-26;
        FrmSetObjectBounds(form, index, &bounds);

        index=FrmGetObjectIndex(form, fieldWordInput);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(form, index, &bounds);
        bounds.topLeft.y=appContext->screenHeight-13;
        bounds.extent.x=appContext->screenWidth-60;
        FrmSetObjectBounds(form, index, &bounds);

        index=FrmGetObjectIndex(form, labelWord);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(form, index, &bounds);
        bounds.topLeft.y=appContext->screenHeight-13;
        FrmSetObjectBounds(form, index, &bounds);

        index=FrmGetObjectIndex(form, scrollDef);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(form, index, &bounds);
        bounds.topLeft.x=appContext->screenWidth-8;
        bounds.extent.y=appContext->screenHeight-18;
        if (appContext->lookupStatusBarVisible)
            bounds.extent.y-=lookupStatusBarHeight;
        FrmSetObjectBounds(form, index, &bounds);

        index=FrmGetObjectIndex(form, buttonAbortLookup);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(form, index, &bounds);
        bounds.topLeft.x=appContext->screenWidth-13;
        FrmSetObjectBounds(form, index, &bounds);

        FrmUpdateForm(formDictMain, frmRedrawUpdateCode);        
        handled=true;
    }
    return handled;
}

static Boolean MainFormHandleEvent(EventType* event)
{
    AppContext* appContext=GetAppContext();
    FormType* form=FrmGetFormPtr(formDictMain);
    Boolean handled=false;
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled=MainFormDisplayChanged(appContext, form);
            break;
            
        case frmOpenEvent:
            handled=MainFormOpen(appContext, form, event);
            break;   
            
        case frmUpdateEvent:
            MainFormDraw(appContext, form, event->data.frmUpdate.updateCode);
            handled=true;
            break; 
            
        case ctlSelectEvent:
            handled=MainFormControlSelected(appContext, form, event);
            break;

        case keyDownEvent:
            handled = MainFormKeyDown(appContext, form, event);
            break;
            
        case sclExitEvent:
            handled=MainFormScrollExit(appContext, form, event);
            break;
            
        case menuEvent:
            handled=MainFormMenuCommand(appContext, form, event);
            break;
            
        case penDownEvent:
            if ((NULL == appContext->currDispInfo) || (event->screenX > appContext->screenWidth-FRM_RSV_W) || (event->screenY > appContext->screenHeight-FRM_RSV_H))
                break;
            cbPenDownEvent(appContext,event->screenX,event->screenY);
            handled = true;
            break;

        case penMoveEvent:
            cbPenMoveEvent(appContext,event->screenX,event->screenY);
            handled = true;
            break;

        case penUpEvent:
            cbPenUpEvent(appContext,event->screenX,event->screenY);
            handled = true;
            break;
            
        case lookupProgressEvent:
            MainFormHandleLookupProgress(appContext, form, event);
            handled=true;
            break;            
    
    }
    return handled;
}

Err MainFormLoad(AppContext* appContext)
{
    Err error=errNone;
    FormType* form=FrmInitForm(formDictMain);
    if (!form)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    error=DefaultFormInit(appContext, form);
    if (!error)
    {
        FrmSetActiveForm(form);
        FrmSetEventHandler(form, MainFormHandleEvent);
    }
    else 
        FrmDeleteForm(form);
OnError:
    return error;    
}


