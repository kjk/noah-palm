/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk (krzysztofk@pobox.com)
  Author: Andrzej Ciarkowski

  Implements handling of the main form for iNoah.
*/

#include "main_form.h"
#include "inet_word_lookup.h"
#include "inet_connection.h"
#include "five_way_nav.h"
#include "bookmarks.h"
#include "inet_registration.h"

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
    UInt16 currentY=0;
    WinPushDrawState();
    SetGlobalBackColor(appContext);
    ClearRectangle(0, currentY, appContext->screenWidth, appContext->screenHeight - currentY - FRM_RSV_H);
    HideScrollbar();
    
    currentY+=7;
    FntSetFont(largeFont);
    DrawCenteredString(appContext, "ArsLexis iNoah", currentY);
    currentY+=16;
#ifdef DEMO
    DrawCenteredString(appContext, "Ver 0.6 (demo)", currentY);
#else
  #ifdef DEBUG
    DrawCenteredString(appContext, "Ver 0.6 (beta)", currentY);
  #else
    DrawCenteredString(appContext, "Ver 0.6", currentY);
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
    UInt16 statusBarStartY=appContext->screenHeight-lookupStatusBarHeight;
    ClearRectangle(0, statusBarStartY, appContext->screenWidth, lookupStatusBarHeight);
    WinDrawLine(0, statusBarStartY, appContext->screenWidth, statusBarStartY);
    const char* text=GetConnectionStatusText(appContext);
    Assert(text);
    UInt16 textLen=StrLen(text);    
    WinDrawTruncChars(text, textLen, 1, statusBarStartY+1, appContext->screenWidth - 14);
    WinPopDrawState();
}

static void MainFormDrawCurrentDisplayInfo(AppContext* appContext)
{
    WinPushDrawState();
    SetBackColorRGB(appContext, WHITE_Packed);
    ClearRectangle(0, 0, appContext->screenWidth, appContext->screenHeight - FRM_RSV_H);
    DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, 0, 0, appContext->dispLinesCount);
    SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
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
            switch (appContext->mainFormContent)
            {
                case mainFormShowsAbout:
                    MainFormDisplayAbout(appContext);
                    break;
                
                case mainFormShowsMessage:
                case mainFormShowsDefinition:
                    MainFormDrawCurrentDisplayInfo(appContext);
                    break;
                    
                default:
                    Assert(false);
            }
            if (!(appContext->lookupStatusBarVisible && ConnectionInProgress(appContext))) 
                break; // fall through in case we should redraw status bar
                
        case redrawLookupStatusBar:
            MainFormDrawLookupStatus(appContext, form);
            break;
            
        default:            
            Assert(false);
    }            
}

static void MainFormSelectWholeInputText(const FormType* form)
{
    UInt16 index=FrmGetObjectIndex(form, fieldWordInput);
    Assert(frmInvalidObjectId!=index);
    FieldType* field=static_cast<FieldType*>(FrmGetObjectPtr(form, index));
    Assert(field);
    FldSelectAllText(field);
}

static void MainFormHandleConnectionProgress(AppContext* appContext, FormType* form, EventType* event)
{
    Assert(event);
    ConnectionProgressEventData* data=reinterpret_cast<ConnectionProgressEventData*>(&event->data);
    if (connectionProgress==data->flag) 
        FrmUpdateForm(formDictMain, redrawLookupStatusBar);
    else
    {
        UInt16 index=FrmGetObjectIndex(form, buttonAbortLookup);
        Assert(frmInvalidObjectId!=index);
        
        if (connectionFinished==data->flag)
        {
            appContext->lookupStatusBarVisible=false;
            FrmHideObject(form, index);
            index=FrmGetObjectIndex(form, buttonFind);
            Assert(frmInvalidObjectId!=index);
            FrmShowObject(form, index);
            MainFormSelectWholeInputText(form);
        }
        else 
        {
            Assert(connectionStarted==data->flag);
            appContext->lookupStatusBarVisible=true;
            FrmShowObject(form, index);
            index=FrmGetObjectIndex(form, buttonFind);
            Assert(frmInvalidObjectId!=index);
            FrmHideObject(form, index);
        }
        FrmUpdateForm(formDictMain, redrawAll); 
    }        
}

static void MainFormFindButtonPressed(AppContext* appContext, FormType* form)
{
    const char* newWord=NULL;
    UInt16      index=FrmGetObjectIndex(form, fieldWordInput);

    Assert(frmInvalidObjectId!=index);

    FieldType* field=static_cast<FieldType*>(FrmGetObjectPtr(form, index));
    const char* prevWord=ebufGetDataPointer(&appContext->currentWordBuf);
    Assert(field);
    newWord=FldGetTextPtr(field);
    if (newWord && (StrLen(newWord)>0) && (!prevWord || 0!=StrCompare(newWord, prevWord)))
        StartWordLookup(appContext, newWord);
    else if (mainFormShowsDefinition!=appContext->mainFormContent)
    {
        const char* currentDefinition=ebufGetDataPointer(&appContext->currentDefinition);
        if ( NULL != currentDefinition )
        {
            // it can be NULL if we didn't have a definition and pressed "GO"
            // with no word in text field
            cbNoSelection(appContext);
            appContext->mainFormContent=mainFormShowsDefinition;

            diSetRawTxt(appContext->currDispInfo, const_cast<char*>(currentDefinition));
            FrmUpdateForm(formDictMain, redrawAll);
        }
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
            if (ConnectionInProgress(appContext))
                AbortCurrentConnection(appContext, true);
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
        const char* text=static_cast<const char*>(MemHandleLock(handle));
        if (text)
        {
            ebufAddStrN(&buffer, const_cast<char*>(text), length);
            MemHandleUnlock(handle);
        }
    }
    if (ebufGetDataSize(&buffer))
    {
        ebufAddChar(&buffer, chrNull);
        StartWordLookup(appContext, ebufGetDataPointer(&buffer));
    }
    ebufFreeData(&buffer);
}


/* Send query dict.php?get_random_word to the server */
inline static void LookupRandomWord(AppContext* appContext)
{
    StartRandomWordLookup(appContext);
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
                StartWordLookup(appContext, appContext->prefs.lastWord);
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
        // mark as unhandled so that left/right works in text field for
        // moving the cursor
        return false;
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
            if (appContext->lookupStatusBarVisible)
                handled=true;
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
    return false;
}

inline static void MainFormHandleAbout(AppContext* appContext)
{
    appContext->mainFormContent=mainFormShowsAbout;
    cbNoSelection(appContext);
    FrmUpdateForm(formDictMain, redrawAll);    
}

inline static void MainFormHandleCopy(AppContext* appContext)
{
    if (appContext->currDispInfo)
        diCopyToClipboard(appContext->currDispInfo);
}

static Boolean RegistrationFormHandleEvent(EventType* event)
{
    Boolean     handled=false;
    FormType*   form=FrmGetFormPtr(formRegistration);
    UInt16      index;

    switch (event->eType)
    {
        case winEnterEvent:
            if ((reinterpret_cast<_WinEnterEventType&>(event->data)).enterWindow==(void*)form)
            {
                index=FrmGetObjectIndex(form, fieldSerialNumber);
                Assert(frmInvalidObjectId!=index);
                FrmSetFocus(form, index);
            }
            break;

        case keyDownEvent:
            if (returnChr==event->data.keyDown.chr || linefeedChr==event->data.keyDown.chr)
            {
                index=FrmGetObjectIndex(form, buttonRegister);
                Assert(frmInvalidObjectId!=index);
                const ControlType* regButton=static_cast<const ControlType*>(FrmGetObjectPtr(form, index));
                Assert(regButton);
                CtlHitControl(regButton);
                handled=true;
            }
    }
    return handled;
}

static void MainFormHandleRegister(AppContext* appContext)
{
    FormType* form=FrmInitForm(formRegistration);
    if (form)
    {
        DefaultFormInit(appContext, form);
        FrmSetEventHandler(form, RegistrationFormHandleEvent);
        UInt16 button=FrmDoDialog(form);
        if (buttonRegister==button)
        {
            UInt16 index=FrmGetObjectIndex(form, fieldSerialNumber);
            Assert(frmInvalidObjectId!=index);
            const FieldType* field=static_cast<FieldType*>(FrmGetObjectPtr(form, index));
            Assert(field);
            const char* serialNumber=FldGetTextPtr(field);
            if (serialNumber && StrLen(serialNumber))
                StartRegistration(appContext, serialNumber);
        }
        FrmDeleteForm(form);
    }
    else
        FrmAlert(alertMemError);
}

inline static void MainFormHandleRecentLookups(AppContext* appContext)
{
    StartRecentLookupsRequest(appContext);
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

        case menuItemRandomWord:
            LookupRandomWord(appContext);
            handled=true;
            break;
            
        case menuItemRecentLookups:
            MainFormHandleRecentLookups(appContext);
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

        case menuItemRegister:
            MainFormHandleRegister(appContext);
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

        case menuItemGotoWebsite:
            WebBrowserCommand(false, 0, sysAppLaunchCmdGoToURL, "http://www.arslexis.com",NULL);
            handled=true;
            break;
            
        case sysEditMenuCopyCmd:
        case sysEditMenuCutCmd:
        case sysEditMenuPasteCmd:
            // generated by command bar, not sure what to do with those
            // so just ignore them
            handled = false;
            break;
        default:
            Assert(false);
    }
    return handled;
}

static Boolean MainFormDisplayChanged(AppContext* appContext, FormType* form) 
{
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(form);

    FrmSetObjectPosByID(form, buttonFind, appContext->screenWidth-26, appContext->screenHeight-13);
    FrmSetObjectBoundsByID(form, fieldWordInput, -1, appContext->screenHeight-13, appContext->screenWidth-60, -1);
    FrmSetObjectPosByID(form, labelWord, -1, appContext->screenHeight-13);
    FrmSetObjectBoundsByID(form, scrollDef, appContext->screenWidth-8, -1, -1, appContext->screenHeight-18);
    FrmSetObjectPosByID(form, buttonAbortLookup, appContext->screenWidth-13, appContext->screenHeight-13);

    ReformatLastResponse(appContext);
    FrmUpdateForm(formDictMain, frmRedrawUpdateCode);        

    return true;
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

        case sclRepeatEvent:
//        case sclExitEvent:
            handled=MainFormScrollExit(appContext, form, event);
            break;
            
        case menuEvent:
            handled=MainFormMenuCommand(appContext, form, event);
            break;
            
        case penDownEvent:
            if ((mainFormShowsDefinition != appContext->mainFormContent) || (event->screenX > appContext->screenWidth-FRM_RSV_W) || (event->screenY > appContext->screenHeight-FRM_RSV_H))
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
            
        case connectionProgressEvent:
            MainFormHandleConnectionProgress(appContext, form, event);
            handled=true;
            break;            

        case evtShowMalformedAlert:
            FrmAlert(alertMalformedResponse);
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
    FrmSetActiveForm(form);
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


