/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

/**
 * @file resident_browse_form.c
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 * @date 2003-12-13
 */
 
#include "resident_browse_form.h"
#include "five_way_nav.h"

static Boolean ResidentBrowseFormDisplayChanged(AppContext* appContext, FormType* form) 
{
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(form);

    SetListHeight(form, listMatching, appContext->dispLinesCount);
    FrmSetObjectBoundsByID(form, listMatching, -1, -1, appContext->screenWidth, -1);

    FrmSetObjectPosByID(form, ctlArrowLeft,  -1, appContext->screenHeight-12);
    FrmSetObjectPosByID(form, ctlArrowRight, -1, appContext->screenHeight-12);

    FrmSetObjectBoundsByID(form, fieldWord, -1, appContext->screenHeight-13, appContext->screenWidth-66, -1);
    FrmSetObjectPosByID(form, buttonCancel, appContext->screenWidth-40, appContext->screenHeight-14);

    FrmUpdateForm(formResidentBrowse, frmRedrawUpdateCode);

    return true;
}

static void ResidentBrowseFormChooseWord(AppContext* appContext, FormType* form, UInt32 wordNo)
{
    ControlType* cancelButton=NULL;
    UInt16 index=FrmGetObjectIndex(form, buttonCancel);
    Assert(index!=frmInvalidObjectId);
    appContext->currentWord = wordNo;
    Assert(appContext->currentWord < appContext->wordsCount);
    cancelButton=(ControlType*)FrmGetObjectPtr(form, index);
    Assert(cancelButton);
    CtlHitControl(cancelButton);
}

static Boolean ResidentBrowseFormListItemSelected(AppContext* appContext, FormType* form, const EventType* event)
{
    ResidentBrowseFormChooseWord(appContext, form, appContext->listItemOffset + (UInt32) event->data.lstSelect.selection);
    return true;
}

static Boolean ResidentBrowseFormControlSelected(AppContext* appContext, FormType* form, const EventType* event)
{
    Boolean handled=false;
    Err error=errNone;
    UInt16 controlId = event->data.ctlSelect.controlID;
    switch (controlId)
    {
        case ctlArrowLeft:
            ScrollWordListByDx(appContext, form, -(appContext->dispLinesCount-1));
            handled=true;
            break;                
        
        case ctlArrowRight:
            ScrollWordListByDx(appContext, form, (appContext->dispLinesCount-1));
            handled=true;
            break;      
            
        case buttonCancel:
            RememberLastWord(appContext, form, fieldWord);
            break;
            
        default:
            Assert(false);
            
    }
    return handled;
}

static Boolean ResidentBrowseFormWinEnter(AppContext* appContext, FormType* form, EventType* event)
{
    struct _WinEnterEventType* winEnter=(struct _WinEnterEventType*)&event->data;
    if (winEnter->enterWindow==(void*)form) // this means we're entering this window for the first time and we have to do initial list drawing
    {
        FieldType* field=NULL;
        ListType* list=NULL;
        UInt16 index=FrmGetObjectIndex(form, fieldWord);
        Assert(index!=frmInvalidObjectId);
        field=(FieldType*)FrmGetObjectPtr(form, index);
        index=FrmGetObjectIndex(form, listMatching);
        Assert(index!=frmInvalidObjectId);
        list=(ListType*)FrmGetObjectPtr(form, index);
        appContext->prevSelectedWord = 0xffffffff;
        LstSetListChoicesEx(list, NULL, dictGetWordsCount(GetCurrentFile(appContext)));
        LstSetDrawFunction(list, ListDrawFunc);
        appContext->prevTopItem = 0;
        Assert(appContext->selectedWord < appContext->wordsCount);
        if (*appContext->lastWord)
        {
            appContext->selectedWord = 0;
            FldInsert(field, appContext->lastWord, StrLen(appContext->lastWord));
            FldSendChangeNotification(field);
        }
        else
        {
            if (-1==appContext->currentWord)
                appContext->selectedWord=0;
            else
                appContext->selectedWord=appContext->currentWord;             
            LstSetSelectionEx(appContext, list, appContext->selectedWord);
        }            
        index=FrmGetObjectIndex(form, fieldWord);
        Assert(index!=frmInvalidObjectId);
        FrmSetFocus(form, index);
        FrmUpdateForm(formResidentBrowse, frmRedrawUpdateCode);
    }
    return false;
}

static Boolean ResidentBrowseFormFieldChanged(AppContext* appContext, FormType* form, EventType* event)
{
    char *      word;
    UInt32      newSelectedWord=0;
    FieldType * field;
    ListType *  list;
    UInt16      index=FrmGetObjectIndex(form, fieldWord);

    Assert(index!=frmInvalidObjectId);
    field=(FieldType*)FrmGetObjectPtr(form, index);
    Assert(field);
    index=FrmGetObjectIndex(form, listMatching);
    Assert(index!=frmInvalidObjectId);
    list=(ListType*)FrmGetObjectPtr(form, index);
    Assert(list);
    word=FldGetTextPtr(field);
    if (word && *word)
        newSelectedWord = dictGetFirstMatching(GetCurrentFile(appContext), word);
    if (appContext->selectedWord != newSelectedWord)
    {
        appContext->selectedWord = newSelectedWord;
        Assert(appContext->selectedWord < appContext->wordsCount);
        LstSetSelectionMakeVisibleEx(appContext, list, appContext->selectedWord);
    }
    return true;
}

static Boolean ResidentBrowseFormKeyDown(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled=false;
    switch (event->data.keyDown.chr)
    {
        case returnChr:
        case linefeedChr:
            ResidentBrowseFormChooseWord(appContext, form, appContext->selectedWord);
            handled=true;
            break;

        case pageUpChr:
            if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
            {
                ScrollWordListByDx( appContext, form, -(appContext->dispLinesCount-1) );
                handled=true;
            }
            break;
        
        case pageDownChr:
            if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
            {
                ScrollWordListByDx( appContext, form, (appContext->dispLinesCount-1) );
                handled=true;
            }
            break;

        default:
            if (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event))
            {
                if (FiveWayCenterPressed(appContext, event))
                {
                    ResidentBrowseFormChooseWord(appContext, form, appContext->selectedWord);
                    handled=true;
                }
            
                if (FiveWayDirectionPressed(appContext, event, Left ))
                {
                    ScrollWordListByDx(appContext, form, -(appContext->dispLinesCount-1));
                    handled=true;
                }
                if (FiveWayDirectionPressed(appContext, event, Right ))
                {
                    ScrollWordListByDx( appContext, form, (appContext->dispLinesCount-1) );
                    handled=true;
                }
                if (FiveWayDirectionPressed(appContext, event, Up ))
                {
                    ScrollWordListByDx( appContext, form, -1 );
                    handled=true;
                }
                if (FiveWayDirectionPressed(appContext, event, Down ))
                {
                    ScrollWordListByDx( appContext, form, 1 );
                    handled=true;
                }
            }
            else
            {
                FieldType* field=NULL;
                UInt16 index=FrmGetObjectIndex(form, fieldWord);
                Assert(index!=frmInvalidObjectId);
                field=(FieldType*)FrmGetObjectPtr(form, index);
                Assert(field);
                FldSendChangeNotification(field);
            }                
    }
    return handled;
}    

static Boolean ResidentBrowseFormHandleEvent(EventType* event)
{
    Boolean handled=false;
    FormType* form=FrmGetActiveForm();
    AppContext* appContext=GetAppContext();
    Assert(appContext);
    switch (event->eType) 
    {
        case winEnterEvent:
            handled=ResidentBrowseFormWinEnter(appContext, form, event);
            break;                
        
        case winDisplayChangedEvent:
            handled=ResidentBrowseFormDisplayChanged(appContext, form);
            break;
            
        case lstSelectEvent:
            handled=ResidentBrowseFormListItemSelected(appContext, form, event);
            break;
            
        case ctlSelectEvent:
            handled=ResidentBrowseFormControlSelected(appContext, form, event);
            break;
            
        case fldChangedEvent:
            handled=ResidentBrowseFormFieldChanged(appContext, form, event);
            break;           
            
        case keyDownEvent:
            handled=ResidentBrowseFormKeyDown(appContext, form, event);
            break;
                
    }
    return handled;
}

Err PopupResidentBrowseForm(AppContext* appContext)
{
    Err error=errNone;
    FormType* form=FrmInitForm(formResidentBrowse);
    UInt16 buttonId=0;
    Assert(form);
    error=DefaultFormInit(appContext, form);
    if (error) 
        goto OnError;
    FrmSetEventHandler(form, ResidentBrowseFormHandleEvent);
    buttonId=FrmDoDialog(form);
    Assert(!buttonId || buttonCancel==buttonId);
OnError:
    FrmDeleteForm(form);
    return error;
}
