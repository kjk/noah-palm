#include "preferences_form.h"

static Boolean PreferencesFormOpen(AppContext* appContext, FormType* form, EventType* event)
{
    const AppPrefs& prefs=appContext->prefs;
    SetPopupLabel(form, listStartupAction, popupStartupAction, prefs.startupAction);
    SetPopupLabel(form, listhwButtonsAction, popuphwButtonsAction, prefs.hwButtonScrollType);
    SetPopupLabel(form, listNavButtonsAction, popupNavButtonsAction, prefs.navButtonScrollType);
    FrmUpdateForm(formPrefs, frmRedrawUpdateCode);
    return true;
}

static void PreferencesFormSaveSettings(AppPrefs& prefs, FormType* form)
{
    prefs.startupAction=static_cast<StartupAction>(LstGetSelectionByListID(form, listStartupAction));
    prefs.hwButtonScrollType=static_cast<ScrollType>(LstGetSelectionByListID(form, listhwButtonsAction));
    prefs.navButtonScrollType=static_cast<ScrollType>(LstGetSelectionByListID(form, listNavButtonsAction));
}

static Boolean PreferencesFormControlSelected(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled=false;
    switch (event->data.ctlSelect.controlID)
    {
        case buttonOk:
            Assert(appContext);
            AppPrefs& prefs=appContext->prefs;
            PreferencesFormSaveSettings(prefs, form);
            // fall through the next case
                    
        case buttonCancel:
            FrmReturnToForm(0);
            handled=true;
            break;
        
    }
    return handled;
}

static Boolean PreferencesFormDisplayChanged(AppContext* appContext, FormType* form) 
{
    Boolean handled=false;
    if (DIA_Supported(&appContext->diaSettings))
    {
        UInt16 index=0;
        RectangleType bounds;
        WinGetBounds(WinGetDisplayWindow(), &bounds);
        bounds.topLeft.y+=2;
        bounds.topLeft.x+=2;
        bounds.extent.y-=4;
        bounds.extent.x-=4;
        WinSetBounds(FrmGetWindowHandle(form), &bounds);
        
        index=FrmGetObjectIndex(form, buttonOk);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(form, index, &bounds);
        bounds.topLeft.y=appContext->screenHeight-13-bounds.extent.y;
        FrmSetObjectBounds(form, index, &bounds);

        index=FrmGetObjectIndex(form, buttonCancel);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(form, index, &bounds);
        bounds.topLeft.y=appContext->screenHeight-13-bounds.extent.y;
        FrmSetObjectBounds(form, index, &bounds);

        FrmUpdateForm(formPrefs, frmRedrawUpdateCode);        
        handled=true;
    }
    return handled;
}


static Boolean PreferencesFormHandleEvent(EventType* event) 
{
    AppContext* appContext=GetAppContext();
    Assert(appContext);
    FormType* form=FrmGetFormPtr(formPrefs);
    Boolean handled=false;
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled=PreferencesFormDisplayChanged(appContext, form);
            break;
            
        case frmOpenEvent:
            handled=PreferencesFormOpen(appContext, form, event);
            break;
            
        case ctlSelectEvent:
            handled=PreferencesFormControlSelected(appContext, form, event);
            break;
    
    }
    return handled;
}


Err PreferencesFormLoad(AppContext* appContext)
{
    Err error=errNone;
    FormType* form=FrmInitForm(formPrefs);
    if (!form)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    error=DefaultFormInit(appContext, form);
    if (!error)
    {
        FrmSetActiveForm(form);
        FrmSetEventHandler(form, PreferencesFormHandleEvent);
    }
    else 
        FrmDeleteForm(form);
OnError:
    return error;    
}
