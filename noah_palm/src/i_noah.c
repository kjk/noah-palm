/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#include "common.h"
#include "five_way_nav.h"
#include "main_form.h"

#ifdef DEBUG
#include "inet_word_lookup.h"

// This is a full HTTP response we get on device when asking for word "glib"
#pragma warn_a5_access off
unsigned char deviceGlibResponse[] = "HTTP/1.0 200 OK\xd" "\xa" "Date: Thu, 25 Dec 2003 10:44:23 GMT\xd" "\xa" "Server: Apache/1.3.22 (Unix)  (Red-Hat/Linux) mod_ssl/2.8.4 OpenSSL/0.9.6b DAV/1.0.2 PHP/4.1.2 mod_perl/1.24_01\xd" "\xa" "X-Powered-By: PHP/4.1.2\xd" "\xa" "Content-Type: text/html\xd" "\xa" "Connection: close\xd" "\xa" "\xd" "\xa" "!glib\xd" "\xa" "!glib-tongued\xd" "\xa" "!smooth-tongued\xd" "\xa" "$s\xd" "\xa" "@artfully persuasive in speech\xd" "\xa" "#a glib tongue\xd" "\xa" "#a smooth-tongued hypocrite\xd" "\xa" "!glib\xd" "\xa" "!pat\xd" "\xa" "!slick\xd" "\xa" "$s\xd" "\xa" "@having only superficial plausibility\xd" "\xa" "#glib promises\xd" "\xa" "#a slick commercial\xd" "\xa" "!glib\xd" "\xa" "$s\xd" "\xa" "@marked by lack of intellectual depth\xd" "\xa" "#glib generalizations\xd" "\xa" "#a glib response to a complex question\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\xd" "\xa" "\x0";
#endif

static const char *txt = "hello";
static const UInt32 ourMinVersion = sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0);
static const UInt32 kPalmOS20Version = sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0);

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
    UInt32 romVersion;
    Err error=FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    Assert(!error);
    if (romVersion < requiredVersion)
    {
        if ((launchFlags & 
            (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
            (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
        {
            FrmAlert (alertRomIncompatible);
            if (romVersion < kPalmOS20Version)
                AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
        }
        error=sysErrRomIncompatible;
    }
    return error;
}

static Err AppInit(AppContext* appContext)
{
    Err error=errNone;
    UInt32 value=0;
    MemSet(appContext, sizeof(*appContext), 0);
    error=FtrSet(APP_CREATOR, appFtrContext, (UInt32)appContext);
    if (error) 
        goto OnError;
    error=FtrSet(APP_CREATOR, appFtrLeaksFile, 0);
    if (error) 
        goto OnError;
    
    LogInit( appContext, "c:\\iNoah_log.txt" );
    InitFiveWay(appContext);
    
    ebufInit(&appContext->currentDefinition, 0);
    ebufInit(&appContext->currentWord, 0);
    appContext->ticksEventTimeout=evtWaitForever;

    appContext->prefs.displayPrefs.listStyle = 2;
#ifdef DONT_DO_BETTER_FORMATTING
    appContext->prefs.displayPrefs.listStyle = 0;
#endif
    SetDefaultDisplayParam(&appContext->prefs.displayPrefs,false,false);
    SyncScreenSize(appContext);

// define _DONT_DO_HANDLE_DYNAMIC_INPUT_ to disable Pen Input Manager operations
#ifndef _DONT_DO_HANDLE_DYNAMIC_INPUT_
    error=DIA_Init(&appContext->diaSettings);
    if (error) 
        goto OnError;
#endif  

OnError:
    return error;
}

static void AppDispose(AppContext* appContext)
{
    Err error=errNone;
    FrmSaveAllForms();
    FrmCloseAllForms();
    error=DIA_Free(&appContext->diaSettings);
    Assert(!error);
    
    if (appContext->currDispInfo)
    {
        diFree(appContext->currDispInfo);
        appContext->currDispInfo=NULL;
    }
    ebufFreeData(&appContext->currentWord);
    ebufFreeData(&appContext->currentDefinition);
}

static void AppLoadForm(AppContext* appContext, const EventType* event)
{
    Err error=errNone;
    switch (event->data.frmLoad.formID)
    {
        case formDictMain:
            error=MainFormLoad(appContext);
            Assert(!error);
            break;
        
        default:
            Assert(false);
    }
}

static Boolean AppHandleEvent(AppContext* appContext, EventType* event)
{
    Boolean handled=false;
    switch (event->eType)
    {
        case frmLoadEvent:
            AppLoadForm(appContext, event);
            handled=true;
            break;
    }
    return handled;
}

static void AppEventLoop(AppContext* appContext)
{
    Err error=errNone;
    EventType event;
    EvtGetEvent(&event, appContext->ticksEventTimeout);
    while (event.eType != appStopEvent)
    {
        if (!SysHandleEvent(&event))
            if (!MenuHandleEvent(0, &event, &error))
                if (!AppHandleEvent(appContext, &event))
                    FrmDispatchEvent(&event);
        EvtGetEvent(&event, appContext->ticksEventTimeout);
    }
}

static Err AppLaunch() 
{
    Err error=errNone;
    AppContext* appContext=(AppContext*)MemPtrNew(sizeof(AppContext));
    if (!appContext)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }     
    error=AppInit(appContext);
    if (error) 
        goto OnError;
    FrmGotoForm(formDictMain);

#if 0
    testParseResponse((char*)deviceGlibResponse);
#endif

    AppEventLoop(appContext);
    AppDispose(appContext);
OnError: 
    if (appContext) 
        MemPtrFree(appContext);
    return error;
}


UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    Err error=RomVersionCompatible (ourMinVersion, launchFlags);
    if (!error) {
        switch (cmd)
        {
            case sysAppLaunchCmdNormalLaunch:
                error=AppLaunch();
                break;
        }
    }       
    return error;
}

