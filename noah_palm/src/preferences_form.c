#include "preferences_form.h"

static Boolean PreferencesFormOpen(AppContext* appContext, FormType* form, EventType* event)
{
    const AppPrefs& prefs=appContext->prefs;
    SetPopupLabel(form, listStartupAction, popupStartupAction, prefs.startupAction);
    SetPopupLabel(form, listhwButtonsAction, popuphwButtonsAction, prefs.hwButtonScrollType);
    SetPopupLabel(form, listNavButtonsAction, popupNavButtonsAction, prefs.navButtonScrollType);

    if (prefs.fEnablePronunciation)
    {
        UInt16 index=FrmGetObjectIndex(form, cbShowPronunctiation);
        Assert(frmInvalidObjectId!=index);
        FrmSetControlValue(form, index, appContext->prefs.fShowPronunciation);
    }
    
    FrmUpdateForm(formPrefs, frmRedrawUpdateCode);
    return true;
}

static void PreferencesFormSaveSettings(AppPrefs& prefs, FormType* form)
{
    prefs.startupAction=static_cast<StartupAction>(LstGetSelectionByListID(form, listStartupAction));
    prefs.hwButtonScrollType=static_cast<ScrollType>(LstGetSelectionByListID(form, listhwButtonsAction));
    prefs.navButtonScrollType=static_cast<ScrollType>(LstGetSelectionByListID(form, listNavButtonsAction));

    if (prefs.fEnablePronunciation)
    {
        UInt16 index=FrmGetObjectIndex(form, cbShowPronunctiation);
        Assert(frmInvalidObjectId!=index);
        prefs.fShowPronunciation=FrmGetControlValue(form, index);
    }
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
    RectangleType bounds;

    Assert( DIA_Supported(&appContext->diaSettings) );
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    WinGetBounds(WinGetDisplayWindow(), &bounds);
    bounds.topLeft.y+=2;
    bounds.topLeft.x+=2;
    bounds.extent.y-=4;
    bounds.extent.x-=4;
    WinSetBounds(FrmGetWindowHandle(form), &bounds);

    FrmSetObjectPosByID(form, buttonOk,     -1, appContext->screenHeight-13-14);
    FrmSetObjectPosByID(form, buttonCancel, -1, appContext->screenHeight-13-14);

    FrmUpdateForm(formPrefs, frmRedrawUpdateCode);        

    return true;
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
