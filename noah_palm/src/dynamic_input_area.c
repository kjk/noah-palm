/**
 * @file dynamic_input_area.c
 * Implementation of Dynamic Input Area feature.
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#include "dynamic_input_area.h"
#include <SonyCLIE.h>
#include "common.h"

/**
 * Performs detection and initialization of Sony CLIE-specific features.
 * @param diaSettings structure used to store detected configuration.
 * @return error code.
 */
static Err SonySilkLibInit(DIA_Settings* diaSettings) 
{
    UInt32  vskVersion=0;
    Err     error=SysLibFind(sonySysLibNameSilk, &diaSettings->sonySilkLibRefNum);

    if (sysErrLibNotFound==error) {
        error=SysLibLoad('libr', sonySysFileCSilkLib, &diaSettings->sonySilkLibRefNum);
        if (!error) DIA_SetFlag(diaSettings, diaLoadedSonySilkLib);
    }
    if (!error) {
        DIA_SetFlag(diaSettings, diaHasSonySilkLib);
        error = FtrGet(sonySysFtrCreator, sonySysFtrNumVskVersion, &vskVersion);
        if (error) {
            error=SilkLibOpen(diaSettings->sonySilkLibRefNum);
            if (error) goto OnError;
            SilkLibEnableResize(diaSettings->sonySilkLibRefNum);
        }
        else if (vskVersionNum2==vskVersion) {
            error=VskOpen(diaSettings->sonySilkLibRefNum);
            if (error) goto OnError;
            VskSetState(diaSettings->sonySilkLibRefNum, vskStateEnable, vskResizeVertically);			
        }
        else {
            error=VskOpen(diaSettings->sonySilkLibRefNum);
            if (error) goto OnError;
            VskSetState(diaSettings->sonySilkLibRefNum, vskStateEnable, vskResizeHorizontally);			
        }
    }
    else error=errNone;
OnError:
    return error;
}

/**
 * Performs cleanup of Sony-CLIE specific features.
 */
static Err SonySilkLibFree(DIA_Settings* diaSettings) 
{
    UInt32  vskVersion=0;
    Err     error;

    Assert(diaSettings);
    Assert(DIA_HasSonySilkLib(diaSettings));
    Assert(diaSettings->sonySilkLibRefNum);

    error=FtrGet(sonySysFtrCreator, sonySysFtrNumVskVersion, &vskVersion);
    if (error)
    {
        SilkLibDisableResize(diaSettings->sonySilkLibRefNum);
        error=SilkLibClose(diaSettings->sonySilkLibRefNum);
        if (silkLibErrStillOpen==error) error=errNone;
        if (error) goto OnErrorFreeLib;
    }
    else
    {
        error=VskClose(diaSettings->sonySilkLibRefNum);
        if (vskErrStillOpen==error) error=errNone;
        if (error) goto OnErrorFreeLib;
    }

OnErrorFreeLib:
    if (DIA_TestFlag(diaSettings, diaLoadedSonySilkLib))
    {
        Err tmpErr=SysLibRemove(diaSettings->sonySilkLibRefNum);
        if (tmpErr) error=tmpErr;
    }
    diaSettings->sonySilkLibRefNum=0;
    return error;
}

Err DIA_Init(DIA_Settings* diaSettings) 
{
    UInt16      cardNo;
    LocalID     localId;
    UInt32      value;
    Err         error;
    Err         tmpErr;

    Assert(diaSettings);
    MemSet(diaSettings, sizeof(*diaSettings), 0);
    
    error=SonySilkLibInit(diaSettings);
    if (error) 
        goto OnError;

    if (!DIA_HasSonySilkLib(diaSettings))
    {
        if ( 0==FtrGet(pinCreator, pinFtrAPIVersion, &value) )
        {
            if ( 0!=value )
                DIA_SetFlag(diaSettings, diaHasPenInputMgr);
        }
    }
    if (!FtrGet(sysFtrCreator, sysFtrNumNotifyMgrVersion, &value) && value)
    {
        error=SysCurAppDatabase(&cardNo, &localId);
        if (error)
            goto OnError;

        if (DIA_HasPenInputMgr(diaSettings)) 
        {
            error=SysNotifyRegister(cardNo, localId, sysNotifyDisplayResizedEvent, NULL, sysNotifyNormalPriority, NULL);
            if (sysNotifyErrDuplicateEntry==error) 
                error=errNone;
            if (error)
                goto OnError;
            DIA_SetFlag(diaSettings, diaRegisteredDisplayResizeNotify);
        }

        if (DIA_HasSonySilkLib(diaSettings))
        {
            error=SysNotifyRegister(cardNo, localId, sysNotifyDisplayChangeEvent, NULL, sysNotifyNormalPriority, NULL);
            if (sysNotifyErrDuplicateEntry==error) 
                error=errNone;
            if (error)
                goto OnError;
            DIA_SetFlag(diaSettings, diaRegisteredDisplayChangeNotify);
        }

    }
Exit:
    return error;
OnError:
    if (DIA_HasSonySilkLib(diaSettings))
    {
        tmpErr=SonySilkLibFree(diaSettings);
        if (tmpErr) error=tmpErr;
    }
    goto Exit;
}

Err DIA_Free(DIA_Settings* diaSettings) 
{
    UInt16  cardNo;
    LocalID localId;
    Err     error=errNone;
    Err     tmpErr;

    Assert(diaSettings);
    if (DIA_TestFlag(diaSettings, diaRegisteredDisplayResizeNotify) || DIA_TestFlag(diaSettings, diaRegisteredDisplayChangeNotify)) {
        error=SysCurAppDatabase(&cardNo, &localId);
        if (error) goto OnErrorFreeLib;

        if (DIA_TestFlag(diaSettings, diaRegisteredDisplayResizeNotify)) {
            error=SysNotifyUnregister(cardNo, localId, sysNotifyDisplayResizedEvent, sysNotifyNormalPriority);
            if (!error) DIA_ResetFlag(diaSettings, diaRegisteredDisplayResizeNotify);
            else goto OnErrorFreeLib;
        }
        if (DIA_TestFlag(diaSettings, diaRegisteredDisplayChangeNotify)) {
            error=SysNotifyUnregister(cardNo, localId, sysNotifyDisplayChangeEvent, sysNotifyNormalPriority);
            if (!error) DIA_ResetFlag(diaSettings, diaRegisteredDisplayChangeNotify);
            else goto OnErrorFreeLib;
        }

    }
OnErrorFreeLib:
    if (DIA_HasSonySilkLib(diaSettings)) {
        tmpErr=SonySilkLibFree(diaSettings);
        if (tmpErr) error=tmpErr;
    }
OnError:
    return error;
}

void DIA_HandleResizeEvent() 
{
    EventType event;
    RectangleType* bounds;
    Coord x, y;
    MemSet(&event, sizeof(event), 0);
    event.eType=(eventsEnum)winDisplayChangedEvent;
    bounds=&event.data.winDisplayChanged.newBounds;
    WinGetDisplayExtent(&x, &y);
    bounds->extent.x=x;
    bounds->extent.y=y;
    EvtAddUniqueEventToQueue(&event, 0, true);
}

Err DIA_FrmEnableDIA(const DIA_Settings* diaSettings, FormType* form, Coord minH, Coord prefH, Coord maxH, Coord minW, Coord prefW, Coord maxW) 
{
    Err         error;
    WinHandle   wh;

    Assert(diaSettings);
    Assert(form);

    if (DIA_HasSonySilkLib(diaSettings))
        DIA_HandleResizeEvent();

    if (!DIA_HasPenInputMgr(diaSettings))
        return errNone;

    error=FrmSetDIAPolicyAttr(form, frmDIAPolicyCustom);
    if (error) 
        goto OnError;

    wh=FrmGetWindowHandle(form);
    Assert(wh);

    error=PINSetInputTriggerState(pinInputTriggerEnabled);
    error=WinSetConstraintsSize(wh, minH, prefH, maxH, minW, prefW, maxW);
    error=PINSetInputAreaState(pinInputAreaUser);
    error = errNone;
    
OnError:
    if (error==pinErrNoSoftInputArea) 
        error=errNone;
    return error;
}
