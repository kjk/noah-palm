/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk (krzysztofk@pobox.com)
  Author: Andrzej Ciarkowski

  Implements handling of the main form for iNoah.
*/

#include "main_form.h"
#include "inet_word_lookup.h"
#include "inet_definition_format.h"
#include "five_way_nav.h"

void MainFormPressFindButton(const FormType* form) 
{
    Assert(form);
    Assert(formDictMain==FrmGetFormId(form));
    UInt16 index=FrmGetObjectIndex(form, buttonFind);
    Assert(frmInvalidObjectId!=index);
    const ControlType* findButton=(const ControlType*)FrmGetObjectPtr(form, index);
    Assert(findButton);
    CtlHitControl(findButton);
}


static void MainFormDisplayAbout(AppContext* appContext)
{
    UInt16 currentY=0;
    WinPushDrawState();
    ClearDisplayRectangle(appContext);
    HideScrollbar();
    
    currentY=7;
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
    DrawCenteredString(appContext, "(C) 2003-2004 ArsLexis", currentY);
    currentY+=24;

    FntSetFont(largeFont);
    DrawCenteredString(appContext, "http://www.arslexis.com", currentY);
    currentY+=40;
    
    WinPopDrawState();    
}

static Err LookupWord(AppContext* appContext, const Char* word)
{
    Char* response=NULL;
    UInt16 respSize=0;
    Err error=INetWordLookup(word, &appContext->serverIpAddress, &response, &respSize);
    if (!error)
    {
        Assert(response);
        if (StrFind(response, response+respSize, noDefnitionResponse)==response)
        {
            FrmAlert(alertWordNotFound);
            error=appErrWordNotFound;
        }
        else
            error=PrepareDisplayInfo(appContext, word, response, response+respSize);
        new_free(response);                    
    }
    return error;        
}

static void MainFormDraw(AppContext* appContext, FormType* form)
{
    FrmDrawForm(form);
    WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
    WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);
    if (!appContext->currDispInfo)
        MainFormDisplayAbout(appContext);
    else 
    {
        WinPushDrawState();
        SetBackColorWhite(appContext);
        ClearDisplayRectangle(appContext);
        DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
        SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
        WinPopDrawState();
    }        
}

/* Select all text in a given Field */
static void FldSelectAllText(FieldType* field)
{
    UInt16 endPos;
    endPos = FldGetTextLength(field);
    FldSetSelection(field,(UInt16)0,endPos);
    FldGrabFocus(field);
}

static void MainFormFindButtonPressed(AppContext* appContext, FormType* form)
{
    const Char* newWord=NULL;
    UInt16 index=FrmGetObjectIndex(form, fieldWordInput);

    Assert(frmInvalidObjectId!=index);

    {
        FieldType* field=(FieldType*)FrmGetObjectPtr(form, index);
        const Char* prevWord=ebufGetDataPointer(&appContext->currentWordBuf);
        Assert(field);
        newWord=FldGetTextPtr(field);
        if (newWord && (StrLen(newWord)>0) && (!prevWord || 0!=StrCompare(newWord, prevWord)))
        {
            Err error=LookupWord(appContext, newWord);
            FldSelectAllText(field);
            if (!error)
            {
                appContext->firstDispLine=0;
                FrmUpdateForm(formDictMain, frmRedrawUpdateCode);
            }                
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

        default:
            Assert(false);
    }
    return handled;
}

static Boolean MainFormOpen(AppContext* appContext, FormType* form, EventType* event)
{
    UInt16 index;
    MainFormDraw(appContext, form);

    index=FrmGetObjectIndex(form, fieldWordInput);
    FrmSetFocus(form, index);

    return true;
}

static Boolean MainFormKeyDown(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled = false;

    switch (event->data.keyDown.chr)
    {
        case returnChr:
        case linefeedChr:
//            MainFormFindButtonPressed(appContext,form);
            MainFormPressFindButton(form);
            handled = true;
            break;
/*
        case pageUpChr:
            if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
            {
                ScrollWordListByDx(appContext, frm, -(appContext->dispLinesCount-1) );
                handled = true;
                break;
            }

        case pageDownChr:
            if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
            {
                ScrollWordListByDx(appContext, frm, (appContext->dispLinesCount-1) );
                handled = true;
                break;
            }*/

        default:
            if ( HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) )
            {
                if (FiveWayCenterPressed(appContext, event))
                {
//                    MainFormFindButtonPressed(appContext,form);
                    MainFormPressFindButton(form);
                    handled = true;
                    break;
                }
            
/*                if (FiveWayDirectionPressed(appContext, event, Left ))
                {
                    ScrollWordListByDx(appContext, frm, -(appContext->dispLinesCount-1) );
                    handled = true;
                    break;
                }
                if (FiveWayDirectionPressed(appContext, event, Right ))
                {
                    ScrollWordListByDx(appContext, frm, (appContext->dispLinesCount-1) );
                    handled = true;
                    break;
                }
                if (FiveWayDirectionPressed(appContext, event, Up ))
                {
                    ScrollWordListByDx(appContext, frm, -1 );
                    handled = true;
                    break;
                }
                if (FiveWayDirectionPressed(appContext, event, Down ))
                {
                    ScrollWordListByDx(appContext, frm, 1 );
                    handled = true;
                    break;
                }*/
            }
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

static Boolean MainFormMenuCommand(AppContext* appContext, FormType* form, EventType* event)
{
    return false;
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
            MainFormDraw(appContext, form);
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
/*            
        case winEnterEvent:
            // workaround for probable Sony CLIE's bug that causes only part of screen to be repainted on return from PopUps
            if (DIA_HasSonySilkLib(&appContext->diaSettings) && form==(void*)((SysEventType*)event)->data.winEnter.enterWindow)
                FrmUpdateForm(form, frmRedrawUpdateCode);
            break;
*/
    
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


