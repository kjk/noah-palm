/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#include <PalmCompatibility.h>
#include "noah_lite.h"
#include "display_info.h"
#include "wn_lite_ex_support.h"

#define MAX_WORD_LABEL_LEN 18

#define evtFieldChanged          (firstUserEvent+1)
#define evtNewWordSelected       (firstUserEvent+2)
#define evtNewDatabaseSelected   (firstUserEvent+3)

void ClearDisplayRectangle()
{
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 160 - DRAW_DI_X - 7,
                    160 - DRAW_DI_Y);
}

char helpText[] =
    "Instructions:\n\255 to lookup a definition of a word\n  press the find button in the right\n  upper corner and select the word\n\255 you can scroll the definition using\n  hardware buttons, tapping on the\n  screen or using a scrollbar\n\255 left/right arrow moves to next\n  or previous word\n";

static char wait_str[] = "Searching...";

GlobalData gd;


// TODO: share those functions with noah_pro.c
NoahErrors GetMemVfs(void)
{
    NoahErrors err;
 
    if (gd.memInitedP)
    {
        if (gd.memWorksP)
        {
            gd.currVfs = &gd.memVfs;
            return ERR_NONE;
        }
        else
            return ERR_GENERIC;
    }
    else
    {
        gd.memVfs.vfsData = (void*) &gd.memVfsData;
        setVfsToMem( &gd.memVfs );
        gd.currVfs = &gd.memVfs;
        err = vfsInit();
        if ( ERR_NONE != err )
        {
           gd.currVfs = NULL;
           return err;
        }
        gd.memInitedP = true;
        gd.memWorksP = true;
        return ERR_NONE;
    }
}

void DeinitMemVfs(void)
{
    if ( ERR_NONE == GetMemVfs() )
        vfsDeinit();
}

// TODO: share this with noah_pro.c
/* Add a db on which a vfs is currently positioned to the list of
found databases */
void AddDbToFoundList( VFSDBType vfsDbType )
{
    if (gd.dbsCount >= MAX_DBS)
    {
        gd.err = ERR_TOO_MANY_DBS;
        return;
    }
    gd.foundDbs[gd.dbsCount].vfsDbType = vfsDbType;
    StrCopy(gd.foundDbs[gd.dbsCount].dbName, vfsGetDbName() );
    StrCopy(gd.foundDbs[gd.dbsCount].dbFullPath, vfsGetFullDbPath() );
    gd.foundDbs[gd.dbsCount].historyCount = 0;
    gd.foundDbs[gd.dbsCount].lastWord[0] = 0;
    ++gd.dbsCount;
}
Boolean ProgramInitNL(void)
{
    MemSet((void *) &gd, sizeof(GlobalData), 0);

/*     gd.current_timeout = -1; */

    gd.currentDb = -1;
    gd.penUpsToConsume = 0;
    gd.selectedWord = 0;
    gd.prevSelectedWord = 0xfffff;

#ifdef NEVER
    gd.help_disp_info = diNew();
    Assert(gd.help_disp_info);
    tmp = (char *) new_malloc(StrLen(helpText) + 1);
    MemMove(tmp, helpText, StrLen(helpText) + 1);
    convertRawTextToDisplayInfo(gd.help_disp_info, tmp);
#endif

    if ( ERR_NONE != GetMemVfs() )
        return false;

    //setCurrentVfsToMem(); 
#if 0
    if (false == vfsInit())
    {
        DrawDebug("Palm db init failed");
        return false;
    }
    DrawDebug("Palm db iniialized");
#endif

    vfsIterateRestart();
    while (true == vfsFindDb(NOAH_LITE_CREATOR, WORDNET_LITE_TYPE, NULL))
    {
        //DrawDebug2("WN file found:", vfsGetDbName());
        AddDbToFoundList(DB_SIMPLE_DM );
#if 0
        gd.foundDbs[gd.dbsCount].dbType = vfsGetDbType();
        gd.foundDbs[gd.dbsCount].dbCreator = vfsGetDbCreator();
        gd.foundDbs[gd.dbsCount].vfsDbType = DB_LEX_DM;
        StrCopy(gd.foundDbs[gd.dbsCount].name, vfsGetDbName());
        ++gd.dbsCount;
#endif
    }

    if (gd.dbsCount == 0)
    {
        DrawDebug("WN pro file not found");
        return false;
    }

    if (!CreateInfoData())
        return false;

    return true;
}

void DictCurrentFree(void)
{
    dictDelete();

    if (gd.currDispInfo)
    {
        diFree(gd.currDispInfo);
        gd.currDispInfo = NULL;
    }

    gd.wordsCount = -1;
    gd.firstDispLine = -1;
    gd.currentDb = -1;
    gd.dictData = 0;
    gd.currentWord = 0;
    gd.listItemOffset = 0;
    gd.penUpsToConsume = 0;
    gd.selectedWord = 0;
    gd.prevSelectedWord = 0xfffffff;
}


Boolean DictInit(int db_num)
{
    DBInfo *dbInfo = NULL;

    Assert((db_num >= 0) && (db_num < gd.dbsCount));

    gd.currentDb = db_num;
    dbInfo = &(gd.foundDbs[db_num]);
    gd.dictData = NULL;

    switch (dbInfo->vfsDbType)
    {
    case DB_LEX_DM:
        setCurrentVfsToMem();
        setWnlexAsCurrentDict();
        break;
    default:
        Assert(false);
    }
    vfsIterateRestart();
// TODO: do something
#if 0
    if (!vfsFindDb(dbInfo->dbCreator, dbInfo->dbType, dbInfo->name))
    {
        DrawDebug2("Couldn't open db:", dbInfo->name);
    }
#endif
    gd.dictData = dictNew();
    if (NULL == gd.dictData)
        goto Error;

    gd.wordsCount = dictGetWordsCount();
    gd.currentWord = 0;
    gd.listItemOffset = 0;
    return true;
  Error:
    DictCurrentFree();
    return false;
}

void ApplicationStop()
{
    DictCurrentFree();
    FreeInfoData();
}

Err ApplicationStartNL(void)
{
    if (!ProgramInitNL())
    {
        if (0 == gd.dbsCount)
            FrmAlert(alertNoDB);
        return 1;
    }

    FrmGotoForm(formDictMain);

    return 0;
}

void DrawPleaseWait(void)
{
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 160 - DRAW_DI_X, 160 - DRAW_DI_Y);
    dh_save_font();
    dh_set_current_y(80);
    dh_display_string(wait_str, 1, 0);
    dh_restore_font();
}

void DisplayAboutNL(void)
{
    ClearDisplayRectangle();
    HideScrollbar();

    dh_save_font();
    dh_set_current_y(16 + 8);
    dh_display_string("Noah Lite", 2, 12);
    dh_display_string("Ver 0.7 beta", 2, 30);
    dh_display_string("(C) ArsLexis", 1, 18);
    dh_display_string("arslexis@pobox.com", 1, 20);
    dh_display_string("get Noah Pro at:", 2, 20);
    dh_display_string("http://www.arslexis.com", 2, 0);

    dh_restore_font();
}

/* return 0 if didn't find anything in clipboard, 1 if 
   got word from clipboard */
int TryClipboard(void)
{
    MemHandle clipItemHandle;
    UInt16 itemLen;
    char txt[30];
    char *clipTxt;
    char *word;
    long wordNo;
    int idx;

    clipItemHandle = ClipboardGetItem(clipboardText, &itemLen);
    if (!clipItemHandle)
        return 0;
    if (0 == itemLen)
        return 0;
    clipTxt = (char *) MemHandleLock(clipItemHandle);
    if (!clipTxt)
        return 0;

    MemSet(txt, 29, 0);
    MemMove(txt, clipTxt, (itemLen < 28) ? itemLen : 28);

    idx = 0;
    while (txt[idx] && (txt[idx] == ' '))
        ++idx;

    MemHandleUnlock(clipItemHandle);

    wordNo = dictGetFirstMatching(&(txt[idx]));
    word = dictGetWord(wordNo);

/*     if (0==StrNCaselessCompare(&(txt[idx]),word,
                ((UInt16)StrLen(word)<itemLen)?StrLen(word):itemLen))
*/
    if (0 == p_instrcmp(&(txt[idx]), word, ((UInt16) StrLen(word) <  itemLen) ? StrLen(word) : itemLen))
    {
        DrawDescription(wordNo);
        return 1;
    }
    return 0;
}

Boolean DictMainFormHandleEvent(EventType * event)
{
    Boolean    handled = false;
    FormPtr    frm;
    Short    newValue;

    switch (event->eType)
    {
    case frmOpenEvent:
        frm = FrmGetActiveForm();
        FrmDrawForm(frm);
        WinDrawLine(0, DRAW_DI_Y - 2, 160, DRAW_DI_Y - 2);
        WinDrawLine(0, DRAW_DI_Y - 1, 160, DRAW_DI_Y - 1);

        if (gd.dbsCount > 1)
        {
            FrmPopupForm(formSelectDict);
        }
        else
        {
            DictInit(0);
            DisplayAboutNL();
            /* start the timer, so we'll switch to info
               text after a few seconds */
/*           gd.start_seconds_count = TimGetSeconds();
           gd.current_timeout = 50; */
        }

        handled = true;
        break;

    case ctlSelectEvent:
        switch (event->data.ctlSelect.controlID)
        {
        case ctlArrowLeft:
            if (gd.currentWord > 0)
                DrawDescription(gd.currentWord - 1);
            break;
        case ctlArrowRight:
            if (gd.currentWord < gd.wordsCount - 1)
                DrawDescription(gd.currentWord + 1);
            break;
        case buttonFind:
            FrmPopupForm(formDictFind);
            break;
        default:
            Assert(0);
            break;
        }
        handled = true;
        break;

    case evtNewWordSelected:
        DrawDescription(gd.currentWord);
        gd.penUpsToConsume = 1;
        handled = true;
        break;

    case evtNewDatabaseSelected:
        DisplayAboutNL();
        /* start the timer, so we'll switch to info
           text after a few seconds */
/*      gd.start_seconds_count = TimGetSeconds();
      gd.current_timeout = 50; */
        handled = true;
        break;

    case keyDownEvent:
        if (11 == event->data.keyDown.chr)
        {
            DefScrollUp( scrollLine );
        }
        else if (12 == event->data.keyDown.chr)
        {
            DefScrollDown( scrollLine );
        }
        handled = true;
        break;

    case sclExitEvent:
        newValue = event->data.sclRepeat.newValue;
        if (newValue != gd.firstDispLine)
        {
            ClearDisplayRectangle();
            gd.firstDispLine = newValue;
            DrawDisplayInfo(gd.currDispInfo, gd.firstDispLine, DRAW_DI_X, DRAW_DI_Y, DRAW_DI_LINES);
        }
        handled = true;
        break;

    case penUpEvent:
        if (0 != gd.penUpsToConsume > 0)
        {
            --gd.penUpsToConsume;
            handled = true;
            break;
        }
        if ((NULL == gd.currDispInfo) ||
            (event->screenX > 150) || (event->screenY < 14))
        {
            handled = true;
            break;
        }

        if (event->screenY > 87)
        {
            DefScrollDown( scrollLine );
        }
        else
        {
            DefScrollUp( scrollLine );
        }
        handled = true;
        break;
    case menuEvent:
        switch (event->data.menu.itemID)
        {
        case menuItemFind:
            FrmPopupForm(formDictFind);
            break;
        case menuItemAbout:
            if (NULL != gd.currDispInfo)
            {
                diFree(gd.currDispInfo);
                gd.currDispInfo = NULL;
                gd.currentWord = 0;
            }
            DisplayAboutNL();
            break;
#ifdef STRESS
        case menuItemStress:
            stress(20);
            break;
#endif
        case menuItemHelp:
            DisplayHelp();
            break;
        case menuItemSelectDB:
            FrmPopupForm(formSelectDict);
            break;
        default:
            Assert(0);
            break;
        }
        handled = true;
        break;
#ifdef NEVER
    case nilEvent:
        if (-1 != gd.start_seconds_count)
        {
            /* we're still displaying About info, check
               if it's time to switch to info */
            Assert(gd.start_seconds_count <= TimGetSeconds());
            if (NULL == gd.currDispInfo)
            {
                if (TimGetSeconds() - gd.start_seconds_count > 5)
                {
                    DisplayHelp();
                    /* we don't nid evtNil events anymore */
                    gd.start_seconds_count = -1;
                    gd.current_timeout = -1;
                }
            }
            else
            {
                gd.start_seconds_count = -1;
                gd.current_timeout = -1;
            }
        }
        handled = true;
        break;
#endif
    default:
        break;
    }
    return handled;
}

#define WORDS_IN_LIST 15

Boolean FindFormHandleEvent(EventType * event)
{
    Boolean handled = false;
    char *word;
    FormPtr frm;
    FieldPtr fld;
    ListPtr list;
    EventType newEvent;

    switch (event->eType)
    {
    case frmOpenEvent:
        frm = FrmGetActiveForm();
        list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
        gd.prevSelectedWord = 0xffffffff;
        LstSetListChoicesEx(list, NULL, dictGetWordsCount());
        LstSetDrawFunction(list, ListDrawFunc);
        Assert(gd.selectedWord < gd.wordsCount);
        gd.selectedWord = 0;
        LstSetSelectionEx(list, 0);
        gd.prevTopItem = 0;
        FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWord));
        FrmDrawForm(frm);
        handled = true;
        break;

    case lstSelectEvent:
        /* set the selected word as current word */
        gd.currentWord = gd.listItemOffset + (long) event->data.lstSelect.selection;
        /* send a msg to yourself telling that a new word
           have been selected so we need to draw the
           description */
        Assert(gd.currentWord < gd.wordsCount);
        MemSet(&newEvent, sizeof(EventType), 0);
        newEvent.eType = (eventsEnum) evtNewWordSelected;
        EvtAddEventToQueue(&newEvent);
        handled = true;
        FrmReturnToForm(0);
        break;

    case keyDownEvent:
        /* kind of ugly trick: there is no event that says, that
           text field has changed, so I generate this event myself
           every time I get keyDownEvent (since I assume, that it
           comes when field has focus). To be more correct I should
           store the old content of field and compare it with a new
           content and do stuff only when they differ */
        switch (event->data.keyDown.chr)
        {
        case returnChr:
        case linefeedChr:
            gd.currentWord = gd.selectedWord;
            Assert(gd.currentWord < gd.wordsCount);
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = (eventsEnum) evtNewWordSelected;
            EvtAddEventToQueue(&newEvent);
            FrmReturnToForm(0);
            return true;
            break;
        case pageUpChr:
            frm = FrmGetActiveForm();
            fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
            if (gd.selectedWord > WORDS_IN_LIST)
            {
                gd.selectedWord -= WORDS_IN_LIST;
                LstSetSelectionMakeVisibleEx(list, gd.selectedWord);
                return true;
            }
            break;
        case pageDownChr:
            frm = FrmGetActiveForm();
            fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
            list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listMatching));
            if (gd.selectedWord + WORDS_IN_LIST < gd.wordsCount)
            {
                gd.selectedWord += WORDS_IN_LIST;
                LstSetSelectionMakeVisibleEx(list, gd.selectedWord);
                return true;
            }
            break;
        default:
            break;
        }
        MemSet(&newEvent, sizeof(EventType), 0);
        newEvent.eType = evtFieldChanged;
        EvtAddEventToQueue(&newEvent);
        handled = false;
        break;

    case evtFieldChanged:
        frm = FrmGetActiveForm();
        fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
        list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
        word = FldGetTextPtr(fld);
        gd.selectedWord = 0;
        if (word && *word)
        {
            gd.selectedWord = dictGetFirstMatching(word);
        }
        Assert(gd.selectedWord < gd.wordsCount);
        LstSetSelectionMakeVisibleEx(list, gd.selectedWord);
        handled = true;
        break;
    case ctlSelectEvent:
        switch (event->data.ctlSelect.controlID)
        {
        case buttonCancel:
            FrmReturnToForm(0);
            handled = true;
            break;
        default:
            Assert(0);
            break;
        }
        break;
    default:
        break;
    }
    return handled;
}

Boolean SelectDictFormHandleEvent(EventType * event)
{
    FormPtr frm;
    ListPtr list;
    static int selectedDb = 0;
    EventType newEvent;

    switch (event->eType)
    {
    case frmOpenEvent:
        frm = FrmGetActiveForm();
        list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listOfDicts));
        LstSetDrawFunction(list, ListDbDrawFunc);
        LstSetListChoices(list, NULL, gd.dbsCount);
        LstSetSelection(list, selectedDb);
        LstMakeItemVisible(list, selectedDb);
        if (-1 == gd.currentDb)
        {
            FrmHideObject(frm, FrmGetObjectIndex(frm, buttonCancel));
        }
        else
        {
            FrmShowObject(frm, FrmGetObjectIndex(frm, buttonCancel));
        }
        FrmDrawForm(frm);
        return true;
        break;

    case lstSelectEvent:
        selectedDb = event->data.lstSelect.selection;
        return true;
        break;

    case ctlSelectEvent:
        switch (event->data.ctlSelect.controlID)
        {
        case buttonSelect:
            if (gd.currentDb != selectedDb)
            {
                if (-1 != gd.currentDb)
                    DictCurrentFree();
                DictInit(selectedDb);
            }
            MemSet(&newEvent, sizeof(EventType), 0);
            newEvent.eType = (eventsEnum) evtNewDatabaseSelected;
            EvtAddEventToQueue(&newEvent);
            FrmReturnToForm(0);
            return true;
        case buttonCancel:
            if (-1 == gd.currentDb)
                Assert(0);
            FrmReturnToForm(0);
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

Boolean ApplicationHandleEvent(EventType * event)
{
    FormPtr frm;
    Int formId;
    Boolean handled = false;

    if (event->eType == frmLoadEvent)
    {
        formId = event->data.frmLoad.formID;
        frm = FrmInitForm(formId);
        FrmSetActiveForm(frm);

        switch (formId)
        {
        case formDictMain:
            FrmSetEventHandler(frm, DictMainFormHandleEvent);
            break;
        case formDictFind:
            FrmSetEventHandler(frm, FindFormHandleEvent);
            break;
        case formSelectDict:
            FrmSetEventHandler(frm, SelectDictFormHandleEvent);
            break;
        default:
            Assert(0);
            break;
        }
        handled = true;
    }
    return handled;
}

void EventLoop(void)
{
    EventType event;
    Word error;

    event.eType = (eventsEnum) 0;
    while (event.eType != appStopEvent)
    {
/*      EvtGetEvent(&event, gd.current_timeout); */
        EvtGetEvent(&event, -1);
        if (SysHandleEvent(&event))
            continue;
        if (MenuHandleEvent(0, &event, &error))
            continue;
        if (ApplicationHandleEvent(&event))
            continue;
        FrmDispatchEvent(&event);
    }
}

DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Err err;
    switch (cmd)
    {
    case sysAppLaunchCmdNormalLaunch:
        err = ApplicationStartNL();
        if (err)
            return err;
        EventLoop();
        ApplicationStop();
        break;
    default:
        break;
    }
    return 0;
}

