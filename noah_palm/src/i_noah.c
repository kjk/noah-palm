#include "common.h"
#include "five_way_nav.h"
#include "inet_word_lookup.h"

static const UInt32 ourMinVersion = sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0);
static const UInt32 kPalmOS20Version = sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0);

static const UInt16 defaultEventTimeout=50; // In milliseconds

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
    
    LogInit( appContext, "c:\\thes_log.txt" );
    InitFiveWay(appContext);
    appContext->ticksEventTimeout=(1000.0*(float)SysTicksPerSecond())/((float)defaultEventTimeout);

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

/*    
    error=INetInit(&appContext->inetSettings);
    if (error) 
    {
        FrmAlert(alertNoWirelessInternetLibrary);
        DIA_Free(&appContext->diaSettings);
    }    
*/   
    
OnError:
    return error;
}

static void AppFree(AppContext* appContext)
{
    Err error=errNone;
    error=DIA_Free(&appContext->diaSettings);
    Assert(!error);
}

static void AppEventLoop(AppContext* appContext)
{
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
//    FrmGotoForm(formDictMain);
//    AppEventLoop(appContext);
    error=INetWordLookup("word", &appContext->serverIpAddress, NULL, NULL);

    AppFree(appContext);
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

