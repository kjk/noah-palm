/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#include "common.h"
#include "five_way_nav.h"
#include "main_form.h"
#include "preferences_form.h"
#include "inet_connection.h"
#include "bookmarks.h"
#include "words_list_form.h"

static const UInt32 ourMinVersion = sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0);
static const UInt32 kPalmOS20Version = sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0);

#include "PrefsStore.hpp"

#define PREFS_DB_NAME "iNoah Prefs"

#define startupAction_id         0
#define hwButtonScrolType_id     1
#define navButtonScrollType_id   2
#define lastWord_id              3
#define bookmarksSortType_id     4
#define cookie_id                5
#define fShowPronunciation_id    6
#define dpListStyle_id           7

// General idea is to load all the preferences setting from the database,
// set a default value if setting is not in the database
static void LoadPreferencesInoah(AppContext* appContext)
{
    Err               err;
    AppPrefs *        prefs = &(appContext->prefs);
    PrefsStoreReader  store(PREFS_DB_NAME, APP_CREATOR, APP_PREF_TYPE);
    UInt16            tmpUInt16;
    char *            tmpStr;

    // general pattern: set a given setting, try to read it from the database
    // ignore errors (they could be due to database not being there (first run)
    // or a given setting not being there (pref settings changed between program
    // versions)
    tmpUInt16 = startupActionNone;
    err = store.ErrGetUInt16(startupAction_id, &tmpUInt16);
    prefs->startupAction = (StartupAction)tmpUInt16;

    tmpUInt16 = scrollPage;
    err = store.ErrGetUInt16(hwButtonScrolType_id, &tmpUInt16);
    prefs->hwButtonScrollType = (ScrollType)tmpUInt16;

    tmpUInt16 = scrollPage;
    err = store.ErrGetUInt16(navButtonScrollType_id, &tmpUInt16);
    prefs->navButtonScrollType = (ScrollType)tmpUInt16;

    tmpUInt16 = bkmSortByTime;
    err = store.ErrGetUInt16(bookmarksSortType_id, &tmpUInt16);
    prefs->bookmarksSortType  = (BookmarkSortType)bkmSortByTime;

    MemSet(prefs->lastWord, sizeof(prefs->lastWord), 0);
    err = store.ErrGetStr(lastWord_id, &tmpStr);
    if (!err)
    {
        Assert(StrLen(tmpStr)<sizeof(prefs->lastWord));
        SafeStrNCopy(prefs->lastWord, sizeof(prefs->lastWord), tmpStr, -1);
    }

    MemSet(prefs->cookie, sizeof(prefs->cookie), 0);
    err = store.ErrGetStr(cookie_id, &tmpStr);
    if (!err)
    {
        Assert(StrLen(tmpStr)<sizeof(prefs->cookie));
        SafeStrNCopy(prefs->cookie, sizeof(prefs->cookie), tmpStr, -1);
    }

    prefs->fShowPronunciation = true;
    err = store.ErrGetBool(fShowPronunciation_id, &prefs->fShowPronunciation);

    prefs->displayPrefs.listStyle = 2;
    err = store.ErrGetUInt16(dpListStyle_id, (UInt16*)&prefs->displayPrefs.listStyle);

    // TODO: display prefs
    SetDefaultDisplayParam(&appContext->prefs.displayPrefs, false, false);

}

static void SavePreferencesInoah(AppContext* appContext)
{
    Err                 err;
    AppPrefs *          prefs = &(appContext->prefs);
    PrefsStoreWriter    store(PREFS_DB_NAME, APP_CREATOR, APP_PREF_TYPE);

    // ignore all errors, we can't do much about them anyway
    err = store.ErrSetUInt16(startupAction_id, (UInt16)prefs->startupAction );
    Assert(!err);
    err = store.ErrSetUInt16(hwButtonScrolType_id, (UInt16)prefs->hwButtonScrollType);
    Assert(!err);
    err = store.ErrSetUInt16(navButtonScrollType_id, (UInt16)prefs->navButtonScrollType);
    Assert(!err);
    err = store.ErrSetUInt16(bookmarksSortType_id, (UInt16)prefs->bookmarksSortType);
    Assert(!err);
    err = store.ErrSetStr(lastWord_id,(char*)prefs->lastWord);
    Assert(!err);
    err = store.ErrSetStr(cookie_id,(char*)prefs->cookie);
    Assert(!err);
    err = store.ErrSetBool(fShowPronunciation_id,prefs->fShowPronunciation);
    Assert(!err);
    err = store.ErrSetUInt16(dpListStyle_id,(UInt16)prefs->displayPrefs.listStyle);
    Assert(!err);
    err = store.ErrSavePreferences();
    Assert(!err);

    // TODO: display prefs
};

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

#ifdef DEBUG
static void TestRGBStandardColors()
{
    // those are defined in display_format.h
    // don't forget to update this list if new colors are added to display_format.h
    Assert(WHITE_Packed == PackRGB(0xff,0xff,0xff)); 
    Assert(BLACK_Packed == PackRGB(0x00,0x00,0x00)); 
    Assert(RED_Packed   == PackRGB(0xff,0x00,0x00)); 
    Assert(BLUE_Packed  == PackRGB(0x00,0x00,0xff)); 
}

static void TestRGBColorPacking(int r,int g, int b)
{
    PackedRGB   rgb = PackRGB(r,g,b);
    int   newR = RGBGetR(rgb);
    int   newG = RGBGetG(rgb);
    int   newB = RGBGetB(rgb);
    Assert(newG==g);
    Assert(newR==r);
    Assert(newB==b);

/*    rgb = PackRGB_f(r,g,b);
    newR = RGBGetR(rgb);
    newG = RGBGetG(rgb);
    newB = RGBGetB(rgb);
    Assert(newG==g);
    Assert(newR==r);
    Assert(newB==b);*/
}
#endif

static Err AppInit(AppContext* appContext)
{
    Err     error;

    MemSet(appContext, sizeof(*appContext), 0);
    error=FtrSet(APP_CREATOR, appFtrContext, (UInt32)appContext);
    if (error) 
        goto OnError;
    error=FtrSet(APP_CREATOR, appFtrLeaksFile, 0);
    if (error) 
        goto OnError;
    
    LogInit( appContext, "c:\\inoah_log.txt" );
    InitFiveWay(appContext);
    
    ebufInit(&appContext->currentDefinition, 0);
    ebufInit(&appContext->currentWordBuf, 0);
    ebufInit(&appContext->lastResponse, 0);
    ebufInit(&appContext->lastMessage, 0);

    appContext->currBookmarkDbType = bkmInvalid; // has anybody idea why it isn't 0?

    appContext->currDispInfo=diNew();
    if (!appContext->currDispInfo)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }

#ifdef DEBUG
    TestRGBStandardColors();
    TestRGBColorPacking(0xff,0xff,0xff);
    TestRGBColorPacking(0xff,0xfe,0xfd);
    TestRGBColorPacking(0xff,0x7f,0x00);
    TestRGBColorPacking(0x00,0x7f,0xff);
    TestRGBColorPacking(0x00,0x00,0x00);
    TestRGBColorPacking(0x7f,0x80,0x7e);
    TestRGBColorPacking(0xfe,0x34,0x04);
#endif
    
    LoadPreferencesInoah(appContext);

    // we don't store this in preferences database
    appContext->prefs.fEnablePronunciation = false;

    SyncScreenSize(appContext);
    
    error=DIA_Init(&appContext->diaSettings);
    if (error) 
        goto OnError;
    
OnError:
    return error;
}

static void AppDispose(AppContext* appContext)
{
    Err error=errNone;
    if (ebufGetDataSize(&appContext->currentWordBuf))
    {
        StrNCopy(appContext->prefs.lastWord, ebufGetDataPointer(&appContext->currentWordBuf), WORD_MAX_LEN-1);
        appContext->prefs.lastWord[WORD_MAX_LEN-1]=chrNull;
    }
    SavePreferencesInoah(appContext);
    FrmCloseAllForms();
    error=DIA_Free(&appContext->diaSettings);
    Assert(!error);
    
    if (ConnectionInProgress(appContext))
        AbortCurrentConnection(appContext, false);
    
    if (appContext->currDispInfo)
    {
        diFree(appContext->currDispInfo);
        appContext->currDispInfo=NULL;
    }
    ebufFreeData(&appContext->currentWordBuf);
    ebufFreeData(&appContext->currentDefinition);
    ebufFreeData(&appContext->lastResponse);
    ebufFreeData(&appContext->lastMessage);
    
    if (appContext->wordsList)
    {
        while (appContext->wordsInListCount)
        {
            Char* word=appContext->wordsList[--appContext->wordsInListCount];
            if (word)
                new_free(word);
        }
        new_free(appContext->wordsList);
        appContext->wordsList=NULL;
    }
}

static void AppLoadForm(AppContext* appContext, const EventType* event)
{
    Err error=errNone;
    switch (event->data.frmLoad.formID)
    {
        case formDictMain:
            error=MainFormLoad(appContext);
            break;
            
        case formPrefs:
            error=PreferencesFormLoad(appContext);
            break;
            
        case formDisplayPrefs:
            error=DisplayPrefsFormLoad(appContext);
            break;

        case formBookmarks:
            error=BookmarksFormLoad(appContext);
            break;
            
        case formWordsList:
            error=WordsListFormLoad(appContext);
            break;
        
        default:
            Assert(false);
    }
    Assert(!error);
    if (error)
    {
        // impossible has happen and we have to respect that
        // if we don't quit we might get a blank screen due
        // to form not being there (as in MainFormLoad() we
        // delete the form if there's an error)
        SendStopEvent();
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
        case winDisplayChangedEvent:
            SyncScreenSize(appContext);
            break;
    }
    return handled;
}

static void AppEventLoop(AppContext* appContext)
{
    Err error=errNone;
    EventType event;
    do 
    {
        Int32 timeout=evtWaitForever;
        if (ConnectionInProgress(appContext)) 
            timeout=0;
        EvtGetEvent(&event, timeout);
        if (appStopEvent!=event.eType)
        {
            if (nilEvent==event.eType && ConnectionInProgress(appContext))
                PerformConnectionTask(appContext);
            else 
            {                
                if (!SysHandleEvent(&event))
                    if (!MenuHandleEvent(0, &event, &error))
                        if (!AppHandleEvent(appContext, &event))
                            FrmDispatchEvent(&event);
            }
        }                                        
    } while (appStopEvent!=event.eType);
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
    if (error)
        return error;

    switch (cmd)
    {
        case sysAppLaunchCmdNormalLaunch:
            error=AppLaunch();
            break;
            
        case sysAppLaunchCmdNotify:
            error=AppHandleSysNotify((SysNotifyParamType*)cmdPBP);
            break;
    }
    return error;
}

