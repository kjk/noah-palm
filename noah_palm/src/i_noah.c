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
#define hwButtonScrollType_id    1
#define navButtonScrollType_id   2
#define lastWord_id              3
#define bookmarksSortType_id     4
#define cookie_id                5
#define fShowPronunciation_id    6
#define dpListStyle_id           7
#define dpBgCol_id               8
#define dpfEnablePron_id         9
#define dpfEnablePronFont_id    10
#define reg_code_id             11

// each of those marks the beginning of unique ids for storing
// a given DisplayElementPrefs. Currently we only need to store
// 3 elements per DisplayElementPrefs but to make it future-proof
// I'm allocating them 16 ids apart
#define  depFont_off            0
#define  depColor_off           1
#define  depBgCol_off           2

#define depPos_id               0x10 // == 16
#define depWord_id              0x20
#define depDefinition_id        0x30
#define depExample_id           0x40
#define depSynonym_id           0x50
#define depDefList_id           0x60
#define depPosList_id           0x70
#define depPron_id              0x80

static bool fValidFont(int id)
{
    switch (id)
    {
        case stdFont:
        case boldFont:
        case largeFont:
        case largeBoldFont:
            return true;
        default:
            break;
    }
    return false;
}

static void GetDisplayElementPrefs(PrefsStoreReader *store, DisplayElementPrefs *dep, int uniqueIdStart)
{
    int fontId;
    Err err = store->ErrGetInt(uniqueIdStart+depFont_off, &fontId);
    // ugly hack: iNoah < v1.0 had problems of incorrecting saving fontId in
    // prefs. it would get corrupted so when upgrading from <1.0 to 1.0 we
    // might get font that are invalid and screw the display. We guard against
    // that by detecting using a font that is outside of pre-defined fonts
    // and leafing the default font in place
    // this will break if a set of fonts changes (we'll revert user setting)
    // a better way to fix that would be: don't read preferences generated
    // by iNoah < v1.0 but it's hard to detect
    if (fValidFont((FontID)fontId))
        dep->font = (FontID) fontId;
    err = store->ErrGetUInt32(uniqueIdStart+depColor_off, &dep->color);
    err = store->ErrGetUInt32(uniqueIdStart+depBgCol_off, &dep->bgCol);
}

#define GET_ENUM(enumName,varName,defaultValue) \
    tmpInt = (int)(defaultValue); \
    err = store.ErrGetInt(varName##_id, &tmpInt); \
    prefs->varName = (enumName) tmpInt;

// General idea is to load all the preferences setting from the database,
// set a default value if setting is not in the database
static void LoadPreferencesInoah(AppContext* appContext)
{
    Err               err;
    AppPrefs *        prefs = &(appContext->prefs);
    PrefsStoreReader  store(PREFS_DB_NAME, APP_CREATOR, APP_PREF_TYPE);
    const char *      tmpStr;
    int               tmpInt;

    // general pattern: set a given setting, try to read it from the database
    // ignore errors (they could be due to database not being there (first run)
    // or a given setting not being there (pref settings changed between program
    // versions)

    // for enums we have to do tricks because even though we store them as
    // int, CodeWarrior C++ compiler stores them as bytes, so we can't just
    // pass the address of the variable because it might be of wrong size
    // C++ standard, as opposed to C, doesn't require enums to be ints
    // There's a compiler switch but to be save we'll implement it portably

    // evil macros
    GET_ENUM(StartupAction,startupAction,startupActionNone)
    GET_ENUM(ScrollType,hwButtonScrollType,scrollPage)
    GET_ENUM(ScrollType,navButtonScrollType,scrollPage)
    GET_ENUM(BookmarkSortType,bookmarksSortType,bkmSortByTime)

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

    DisplayPrefs *dp=&(prefs->displayPrefs);

    dp->listStyle = 2;
    err = store.ErrGetInt(dpListStyle_id, &dp->listStyle);

    SetDefaultDisplayParam(dp, false, false);

    err = store.ErrGetUInt32(dpBgCol_id, &dp->bgCol);
    err = store.ErrGetBool(dpfEnablePron_id,&dp->fEnablePronunciation);
    err = store.ErrGetBool(dpfEnablePronFont_id,&dp->fEnablePronunciationSpecialFonts);

    MemSet(prefs->regCode, sizeof(prefs->regCode), 0);
    err = store.ErrGetStr(reg_code_id, &tmpStr);
    if (!err)
    {
        Assert(StrLen(tmpStr)<sizeof(prefs->regCode));
        SafeStrNCopy(prefs->regCode, sizeof(prefs->regCode), tmpStr, -1);
    }

    GetDisplayElementPrefs(&store, &dp->pos, depPos_id);
    GetDisplayElementPrefs(&store, &dp->word, depWord_id);
    GetDisplayElementPrefs(&store, &dp->definition, depDefinition_id);
    GetDisplayElementPrefs(&store, &dp->example, depExample_id);
    GetDisplayElementPrefs(&store, &dp->synonym, depSynonym_id);
    GetDisplayElementPrefs(&store, &dp->defList, depDefList_id);
    GetDisplayElementPrefs(&store, &dp->posList, depPosList_id);
    GetDisplayElementPrefs(&store, &dp->pronunciation, depPron_id);
}

// devnote: uniqueIdStart is the first unique id for storing given DisplayElementPrefs
// currently we only need 3 but for to make things future proof, those should
// be at least 0x10 id apart
static void SetDisplayElementPrefs(PrefsStoreWriter *store, DisplayElementPrefs *dep, int uniqueIdStart)
{
    int fontId = (int)dep->font;
    Err err = store->ErrSetInt(uniqueIdStart+depFont_off, fontId);
    Assert(!err);
    err = store->ErrSetUInt32(uniqueIdStart+depColor_off, dep->color);
    Assert(!err);
    err = store->ErrSetUInt32(uniqueIdStart+depBgCol_off, dep->bgCol);
    Assert(!err);
}
 
void SavePreferencesInoah(AppContext* appContext)
{
    AppPrefs *          prefs = &(appContext->prefs);
    PrefsStoreWriter    store(PREFS_DB_NAME, APP_CREATOR, APP_PREF_TYPE);

    // ignore all errors, we can't do much about them anyway
    Err err = store.ErrSetInt(startupAction_id, prefs->startupAction );
    Assert(!err);
    err = store.ErrSetInt(hwButtonScrollType_id, (int)prefs->hwButtonScrollType);
    Assert(!err);
    err = store.ErrSetInt(navButtonScrollType_id, (int)prefs->navButtonScrollType);
    Assert(!err);
    err = store.ErrSetInt(bookmarksSortType_id, (int)prefs->bookmarksSortType);
    Assert(!err);
    err = store.ErrSetStr(lastWord_id,(char*)prefs->lastWord);
    Assert(!err);
    err = store.ErrSetStr(cookie_id,(char*)prefs->cookie);
    Assert(!err);
    err = store.ErrSetBool(fShowPronunciation_id,prefs->fShowPronunciation);
    Assert(!err);

    DisplayPrefs *dp=&(prefs->displayPrefs);
    err = store.ErrSetInt(dpListStyle_id,dp->listStyle);
    Assert(!err);
    err = store.ErrSetUInt32(dpBgCol_id,dp->bgCol);
    Assert(!err);

    err = store.ErrSetBool(dpfEnablePron_id, dp->fEnablePronunciation);
    Assert(!err);
    err = store.ErrSetBool(dpfEnablePronFont_id, dp->fEnablePronunciationSpecialFonts);
    Assert(!err);

    err = store.ErrSetStr(reg_code_id,(char*)prefs->regCode);
    Assert(!err);

    SetDisplayElementPrefs(&store, &dp->pos, depPos_id);
    SetDisplayElementPrefs(&store, &dp->word, depWord_id);
    SetDisplayElementPrefs(&store, &dp->definition, depDefinition_id);
    SetDisplayElementPrefs(&store, &dp->example, depExample_id);
    SetDisplayElementPrefs(&store, &dp->synonym, depSynonym_id);
    SetDisplayElementPrefs(&store, &dp->defList, depDefList_id);
    SetDisplayElementPrefs(&store, &dp->posList, depPosList_id);
    SetDisplayElementPrefs(&store, &dp->pronunciation, depPron_id);

    err = store.ErrSavePreferences();
    Assert(!err);

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
#ifdef DEBUG
    appContext->fInStress = false;
#endif
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
    Err error;
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
            char* word=appContext->wordsList[--appContext->wordsInListCount];
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

#ifdef DEBUG
extern void testExtractLine();
#endif

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

#ifdef DEBUG
    testExtractLine();
#endif

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

