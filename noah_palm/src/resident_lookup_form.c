/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

/**
 * @file resident_lookup_form.c
 * Implementation of <code>formResidentLookup</code>.
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#include "resident_lookup_form.h"
#include "resident_browse_form.h"
#include "five_way_nav.h"

static void ResidentLookupFormFindPressed(AppContext* appContext) 
{
    Err     error;
    char *  word;
    long    currentWord=appContext->currentWord;

    RememberFieldWordMain(appContext, FrmGetActiveForm() );

    error=PopupResidentBrowseForm(appContext);
    if (!error) 
    {
        if (appContext->currentWord!=currentWord)
        {
            word = dictGetWord(GetCurrentFile(appContext), appContext->currentWord);
            FldClearInsert(FrmGetActiveForm(), fieldWordMain, word);
            DrawDescription(appContext, appContext->currentWord);
        }
    }
    else 
        Assert(false);
}

/**
 * Handles navigation to previous or next word.
 * If previous or next word is available displays centered <code>SEARCH_TXT</code>
 * fetches word's definition and draws it.
 * @param appContext context variable.
 * @param next if <code>true</code>, then navigate to next word, else to previous one.
 * @return <code>true</code>, so that it could be easily used as event handler.
 */
static void ResidentLookupFormWordNavigate(AppContext* appContext, Boolean next)
{
    char *      word;
    long        prevWord;

    prevWord = appContext->currentWord;

    if (next && appContext->currentWord < appContext->wordsCount - 1)
        appContext->currentWord++;
    else if (!next && appContext->currentWord > 0)
        appContext->currentWord--;

    if (prevWord != appContext->currentWord)
    {
        word = dictGetWord(GetCurrentFile(appContext), appContext->currentWord + 1);
        FldClearInsert(FrmGetActiveForm(), fieldWordMain, word);
        DrawDescription(appContext, appContext->currentWord);
    }
}

/**
 * <code>keyDownEvent</code> handler function for <code>formResidentLookup</code>.
 * Reacts to five way navigator keys (scroll up/down, previous/next word) and hardware keys 
 * (scroll up/down).
 * @param appContext context variable.
 * @param form pointer to <code>formResidentLookup</code> object.
 * @param event actual event object.
 * @return <code>true</code> if event was handled.
 */
static Boolean ResidentLookupFormKeyDown(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled=false;
    if ( HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) )
    {
        if (FiveWayDirectionPressed(appContext, event, Up ))
        {
            DefScrollUp(appContext, appContext->prefs.navButtonScrollType );
            handled=true;
        }
        else if (FiveWayDirectionPressed(appContext, event, Down ))
        {
            DefScrollDown(appContext, appContext->prefs.navButtonScrollType );
            handled=true;
        }
        else if (FiveWayDirectionPressed(appContext, event, Left))
        {
            ResidentLookupFormWordNavigate(appContext, false);
            handled=true;
        }            
        else if (FiveWayDirectionPressed(appContext, event, Right))
        {
            ResidentLookupFormWordNavigate(appContext, true);
            handled=true;
        }            
    }
    else if (pageUpChr == event->data.keyDown.chr)
    {
        DefScrollUp(appContext, appContext->prefs.hwButtonScrollType);
        handled=true;
    }
    else if (pageDownChr == event->data.keyDown.chr)
    {
        DefScrollDown(appContext, appContext->prefs.hwButtonScrollType);
        handled=true;
    }
    /*else if (((event->data.keyDown.chr >= 'a')  && (event->data.keyDown.chr <= 'z'))
             || ((event->data.keyDown.chr >= 'A') && (event->data.keyDown.chr <= 'Z'))
             || ((event->data.keyDown.chr >= '0') && (event->data.keyDown.chr <= '9')))
    {
        appContext->lastWord[0] = event->data.keyDown.chr;
        appContext->lastWord[1] = 0;
        ResidentLookupFormFindPressed(appContext);
    }*/    
    else
    {
        // notify ourselves that a text field is (possibly) updated
        SendFieldChanged();
        // mark as unhandled so that text field will (possibly) get updated
        handled = false;
    }
    return handled;
}

/**
 * <code>winDisplayChangedEvent</code> handler function for <code>formResidentLookup</code>.
 * Resizes form and repositions controls contained by it, enqueuing <code>frmUpdateEvent</code>
 * afterwards.
 * @param appContext context variable.
 * @param form pointer to <code>formResidentLookup</code> object.
 * @return <code>true</code> if event was handled (only if DIA functionality is supported).
 */
static Boolean ResidentLookupFormDisplayChanged(AppContext* appContext, FormType* form)
{
    if (  !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(form);

    FrmSetObjectPosByID(form, ctlArrowLeft, -1, appContext->screenHeight-12);
    FrmSetObjectPosByID(form, ctlArrowRight, -1, appContext->screenHeight-12);
    FrmSetObjectBoundsByID(form, scrollDef, appContext->screenWidth-8, -1, -1, appContext->screenHeight-18);
    FrmSetObjectPosByID(form, bmpClose, appContext->screenWidth-13, appContext->screenHeight-13);
    FrmSetObjectPosByID(form, buttonClose, appContext->screenWidth-14, appContext->screenHeight-14);
    FrmSetObjectPosByID(form, bmpFind, appContext->screenWidth-27, appContext->screenHeight-13);
    FrmSetObjectPosByID(form, buttonFind, appContext->screenWidth-28, appContext->screenHeight-14);

    FrmUpdateForm(formResidentLookup, frmRedrawUpdateCode);
    return true;
}

/**
 * <code>penUpEvent</code> handler function for <code>formResidentLookup</code>.
 * Performs scrolling according to current preferences if user taps pen in word definition 
 * area.
 * @param appContext context variable.
 * @param form pointer to <code>formResidentLookup</code> object.
 * @param event actual event object.
 * @return <code>true</code> if event was handled (only if user tapped in word definition area).
 */
static Boolean ResidentLookupFormPenUp(AppContext* appContext, FormType* form, EventType* event) 
{
    Boolean handled=false;
    return handled;      
}

/**
 * <code>sclExitEvent</code> handler function for <code>formResidentLookup</code>.
 * Performs scrolling of definition list area in response to user's activity with scrollbar.
 * @param appContext context variable.
 * @param form pointer to <code>formResidentLookup</code> object.
 * @param event actual event object.
 * @return <code>true</code> if event was handled (what means always).
 */
static Boolean ResidentLookupFormScrollExit(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled=false;
    UInt16  newValue = event->data.sclRepeat.newValue;
    if (newValue != appContext->firstDispLine)
    {
        ClearRectangle(DRAW_DI_X, DRAW_DI_Y, appContext->screenWidth-FRM_RSV_W+2, appContext->screenHeight-FRM_RSV_H);
        appContext->firstDispLine = newValue;
        DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
    }
    handled = true;
    return handled;
}

/**
 * <code>ctlSelectEvent</code> handler function for <code>formResidentLookup</code>.
 * Reacts to user's activity with controls. <code>buttonClose</code> should not be handled here
 * so that it could cause dialog box to close.
 * @param appContext context variable.
 * @param form pointer to <code>formResidentLookup</code> object.
 * @param event actual event object.
 * @return <code>true</code> if event was handled (when user presses navigation buttons). 
 */
static Boolean ResidentLookupFormControlSelected(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled=false;
    Err error=errNone;
    UInt16 controlId = event->data.ctlSelect.controlID;
    switch (controlId)
    {
        case ctlArrowLeft:
            ResidentLookupFormWordNavigate(appContext, false);
            handled=true;
            break;                
        
        case ctlArrowRight:
            ResidentLookupFormWordNavigate(appContext, true);
            handled=true;
            break;      
            
        case buttonFind:
            ResidentLookupFormFindPressed(appContext);
            handled=true;
            break;
        
        case buttonClose:
            break;
            
        default:
            Assert(false);
    }
    return handled;
}

/**
 * <code>winEnterEvent</code> handler function for <code>formResidentLookup</code>.
 * Performs initial drawing of the form, because when we call FrmDoDialog() it doesn't receive
 * <code>frmOpenEvent</code> and none of custom drawing is done.
 * @param appContext context variable.
 * @param form pointer to <code>formResidentLookup</code> object.
 * @param event actual event object.
 * @return <code>false</code>, because we want this event to propagate further.
 */
static Boolean ResidentLookupFormWinEnter(AppContext* appContext, FormType* form, EventType* event)
{
    struct _WinEnterEventType* winEnter=(struct _WinEnterEventType*)&event->data;
    if (winEnter->enterWindow==(void*)form && !appContext->currDispInfo) // this means we're entering this window for the first time and we have to do initial word lookup
    {
        WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
        WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);
        DrawDescription(appContext, appContext->currentWord);
    }
    return false;
}

/**
 * Form event handler function for <code>formResidentLookup</code>.
 * Dispatches events to appropriate handlers according to event type.
 * @param event event to handle.
 * @return <code>true</code> if event was handled and we don't want it to propagate.
 */
static Boolean ResidentLookupFormHandleEvent(EventType* event)
{
    Boolean handled=false;
    AppContext* appContext=GetAppContext();
    FormType* form=FrmGetActiveForm();
    Assert(appContext);
    switch (event->eType)
    {
        case winEnterEvent:
            handled=ResidentLookupFormWinEnter(appContext, form, event);
            break;                

        case winDisplayChangedEvent:
            handled=ResidentLookupFormDisplayChanged(appContext, form);
            break;

        case frmUpdateEvent:
            RedrawMainScreen(appContext);
            handled=true;
            break;
            
        case keyDownEvent:
            handled=ResidentLookupFormKeyDown(appContext, form, event);
            break;
            
        case penUpEvent:
            handled=ResidentLookupFormPenUp(appContext, form, event);
            break;                       
            
        case sclExitEvent:
            handled=ResidentLookupFormScrollExit(appContext, form, event);
            break;
            
        case ctlSelectEvent:
            handled=ResidentLookupFormControlSelected(appContext, form, event);
            break;
        case evtFieldChanged:
            RememberFieldWordMain(appContext, form);
            ResidentLookupFormFindPressed(appContext);
            handled = true;
            break;
    }
    return handled;
}

Err PopupResidentLookupForm(AppContext* appContext) 
{
    Err         error;
    FormType *  form=FrmInitForm(formResidentLookup);
    UInt16      buttonId;
    char *      word;

    Assert(form);
    error=DefaultFormInit(appContext, form);
    if (error) 
        goto OnError;
    FrmSetEventHandler(form, ResidentLookupFormHandleEvent);

    word = dictGetWord(GetCurrentFile(appContext), appContext->currentWord);
    FldClearInsert(form, fieldWordMain, word);                

    buttonId=FrmDoDialog(form);
    Assert(!buttonId || buttonClose==buttonId);
OnError:
    FrmDeleteForm(form);
    return error;
}
