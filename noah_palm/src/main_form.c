#include "main_form.h"
#include "inet_word_lookup.h"
#include "inet_definition_format.h"

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
    DrawCenteredString(appContext, "Ver 1.0 (demo)", currentY);
#else
  #ifdef DEBUG
    DrawCenteredString(appContext, "Ver 1.0 (Debug)", currentY);
  #else
    DrawCenteredString(appContext, "Ver 1.0", currentY);
  #endif
#endif
    currentY+=20;
    
    FntSetFont(boldFont);
    DrawCenteredString(appContext, "(C) 2000-2003 ArsLexis", currentY);
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
        DrawWord(ebufGetDataPointer(&appContext->currentWord), appContext->screenHeight-FONT_DY);
        SetBackColorWhite(appContext);
        ClearDisplayRectangle(appContext);
        DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
        SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
        WinPopDrawState();
    }        
}

static Boolean MainFormOpen(AppContext* appContext, FormType* form, EventType* event)
{
    Err error=errNone;
    MainFormDraw(appContext, form);
    return true;
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
    {
        FrmDeleteForm(form);
    }        
OnError:
    return error;    
}


