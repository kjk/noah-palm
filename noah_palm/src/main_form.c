/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk (krzysztofk@pobox.com)
  Author: Andrzej Ciarkowski

  Implements handling of the main form for iNoah.
*/

#include "main_form.h"
#include "inet_word_lookup.h"
#include "inet_definition_format.h"
#include "five_way_nav.h"

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
static void FldSelectAllText(FormType *form, const FieldType* field)
{
    // TODO: obviously.
    UInt16 index;

    index=FrmGetObjectIndex(form, fieldWordInput);
    FrmShowObject(form, index);
    FrmSetFocus(form, index);
}

static void MainFormFindButtonPressed(AppContext* appContext, FormType* form)
{
    const Char* newWord=NULL;
    UInt16 index=FrmGetObjectIndex(form, fieldWordInput);

    Assert(frmInvalidObjectId!=index);

    {
        const FieldType* field=(const FieldType*)FrmGetObjectPtr(form, index);
        const Char* prevWord=ebufGetDataPointer(&appContext->currentWord);
        Assert(field);
        newWord=FldGetTextPtr(field);
        appContext->mainFormMode=mainFormShowingWord;
        if (newWord && (StrLen(newWord)>0) && (!prevWord || 0!=StrCompare(newWord, prevWord)))
        {
            Err error=LookupWord(appContext, newWord);
            FldSelectAllText(form, field);
            if (!error)
                FrmUpdateForm(formDictMain, frmRedrawUpdateCode);
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
    //    Err error=errNone;
    MainFormDraw(appContext, form);

    appContext->mainFormMode=mainFormShowingField;
    index=FrmGetObjectIndex(form, fieldWordInput);
    FrmShowObject(form, index);
    FrmSetFocus(form, index);

//    error=LookupWord(appContext, "art");
//    if (!error)
//        FrmUpdateForm(formDictMain, frmRedrawUpdateCode);
    return true;
}

static Boolean MainFormKeyDown(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled = false;

    switch (event->data.keyDown.chr)
    {
        case returnChr:
        case linefeedChr:
            MainFormFindButtonPressed(appContext,form);
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
                    MainFormFindButtonPressed(appContext,form);
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

static Boolean MainFormHandleEvent(EventType* event)
{
    AppContext* appContext=GetAppContext();
    FormType* form=FrmGetFormPtr(formDictMain);
    Boolean handled=false;
    switch (event->eType)
    {
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


