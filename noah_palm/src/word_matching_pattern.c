/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Owner: Lukasz Szweda (qkiss@wp.pl)

  support for Find Word Matching Pattern
*/

#include "word_matching_pattern.h"
#include "word_compress.h"
#include "five_way_nav.h"

// type and name of the database with cached results of matching the pattern
#define WORD_MATCHING_PATTERN_TYPE  'womp'
#define WORD_MATCHING_PATTERN_DB    "Noah_wmp_cache"

// limit on the number of words matched. User has to scroll manually through
// the list so it doesn't make sense giving him thousands of results
#define MATCHED_WORDS_LIMIT 200

static Boolean TappedInRect(RectangleType * rect)
{
    EventType ev;

    if (EvtEventAvail())
    {
        EvtGetEvent(&ev, 1);
        if (ev.eType == penDownEvent && RctPtInRectangle(ev.screenX, ev.screenY ,rect))
            return true;
    }
    return false;
}

Err OpenMatchingPatternDB(AppContext* appContext)
{
    Err     err;

    if (appContext->wmpCacheDb)
        return errNone;

    appContext->wmpCacheDb = OpenDbByNameCreatorType(WORD_MATCHING_PATTERN_DB, APP_CREATOR, WORD_MATCHING_PATTERN_TYPE);
    if (appContext->wmpCacheDb)
        return errNone;

    err = DmCreateDatabase(0, WORD_MATCHING_PATTERN_DB, APP_CREATOR,  WORD_MATCHING_PATTERN_TYPE, false);
    if ( errNone != err)
        return err;

    appContext->wmpCacheDb = OpenDbByNameCreatorType(WORD_MATCHING_PATTERN_DB, APP_CREATOR, WORD_MATCHING_PATTERN_TYPE);
    if (!appContext->wmpCacheDb)
        return dmErrCantOpen;

    return errNone;
}

Err CloseMatchingPatternDB(AppContext* appContext)
{
    if (appContext->wmpCacheDb != NULL)
    {
        DmCloseDatabase(appContext->wmpCacheDb);
        appContext->wmpCacheDb = NULL;
    }
    return errNone;
}

Err ClearMatchingPatternDB(AppContext* appContext)
{
    Err err;

    while (DmNumRecords(appContext->wmpCacheDb))
    {
        err = DmRemoveRecord(appContext->wmpCacheDb, 0);
        if (err) 
            return err;
    }
    return errNone;
}

Err ReadPattern(AppContext* appContext, char * pattern)
{
    MemHandle rh;
    char * rp;

    rh = DmQueryRecord(appContext->wmpCacheDb, 0);
    if (!rh) 
    {
        pattern[0] = 0;
        return dmErrCantFind;
    }
    rp = MemHandleLock(rh);
    StrCopy(pattern, rp);
    return MemHandleUnlock(rh);
}

// Writes a pattern into the DB at record no 0
Err WritePattern(AppContext* appContext, char * pattern)
{
    Err err;
    MemHandle rh;
    UInt16 len, pos, num;
    char * rp;

    len = StrLen(pattern);
    num = DmNumRecords(appContext->wmpCacheDb);
    if (num > 0)
        rh = DmGetRecord(appContext->wmpCacheDb, 0);
    else 
    {
        pos = dmMaxRecordIndex;
        rh = DmNewRecord(appContext->wmpCacheDb, &pos, len + 1);
    }
    if (!rh) 
        return DmGetLastErr();
    rp = MemHandleLock(rh);
    err = DmWrite(rp, 0, pattern, len + 1);
    MemHandleUnlock(rh);
    DmReleaseRecord(appContext->wmpCacheDb, 0, true);
    return err;
}

Err ReadMatchingPatternRecord(AppContext* appContext, long pos, long * elem)
{
    MemHandle rh;
    long *rp;

    rh = DmQueryRecord(appContext->wmpCacheDb, pos / WMP_REC_PACK_COUNT + 1);
    if (!rh) 
        return DmGetLastErr();
    rp = MemHandleLock(rh);
    *elem = rp[pos % WMP_REC_PACK_COUNT];
    return MemHandleUnlock(rh);
}

Err WriteMatchingPatternRecord(AppContext* appContext, long elem)
{
    Err err;
    MemHandle rh;
    long *rp;
    UInt16 pos, offset;
    Int32 num;

    num = DmNumRecords(appContext->wmpCacheDb);
    if (num > 1)
    {
        pos = num - 1;
        rh = DmGetRecord(appContext->wmpCacheDb, pos);
        offset = MemHandleSize(rh);
        if (offset < WMP_REC_PACK_COUNT * WMP_REC_SIZE)
            DmResizeRecord(appContext->wmpCacheDb, pos, offset + WMP_REC_SIZE);
        else
        {
            offset = 0;
            pos = dmMaxRecordIndex;
            rh = DmNewRecord(appContext->wmpCacheDb, &pos, WMP_REC_SIZE);
        }
    }
    else
    {
        offset = 0;
        pos = dmMaxRecordIndex;
        rh = DmNewRecord(appContext->wmpCacheDb, &pos, WMP_REC_SIZE);
    }
    if (!rh) return DmGetLastErr();
        
    rp = MemHandleLock(rh);
    err = DmWrite(rp, offset, &elem, WMP_REC_SIZE);
    MemHandleUnlock(rh);
    DmReleaseRecord(appContext->wmpCacheDb, pos, true);
    return err;
}

long NumMatchingPatternRecords(AppContext* appContext)
{
    UInt16 num;
    long count;
    MemHandle rh;
    
    num = DmNumRecords(appContext->wmpCacheDb);
    if (num > 1)
    {
        rh = DmQueryRecord(appContext->wmpCacheDb, num - 1);
        // full record packs = all records - first record (pattern's there) - last record (doesn't have to be full)
        // so 2 records are definetely not full
        count = (num - 2) * WMP_REC_PACK_COUNT;
        count += MemHandleSize(rh) / WMP_REC_SIZE;
        return count;
    }
    return 0;
}

// returns 1 if word matches given pattern, 0 otherwise
int WordMatchesPattern(char * word, char * pattern)
{
    char c;

    switch (c = *pattern++)
    {
        case '?':
            if (*word == '\0')
                return 0;
            return WordMatchesPattern(++word, pattern);

        case '*':
            while (*pattern == '*')
                pattern++;
            if (*pattern == '\0')
                return 1;
            while (*++word != '\0')
                if (WordMatchesPattern(word, pattern))
                    return 1;
            return 0;

        default:
            // word case preserved, as the same words with different case exist
            if (c == *word)
            {
                if (c == '\0')
                    return 1;
                return WordMatchesPattern(++word, pattern);
            }
            return 0;
    }
}

void FillMatchingPatternDB(AppContext* appContext, char * pattern)
{
    char            tmp[10];
    RectangleType   rc, rcStop;
    long            i, num;
    long            matchedWords = 0;
    char *          str;
    AbstractFile *  currFile=GetCurrentFile(appContext);

    // usually we would do just rc = { {30,40}, {40,100} } but that causes
    // using global variables and we have to avoid that
    rc.topLeft.x = 30;
    rc.topLeft.y = 40;
    rc.extent.x = 100;
    rc.extent.y = 60;
    rcStop.topLeft.x = 60;
    rcStop.topLeft.y = 80;
    rcStop.extent.x = 40;
    rcStop.extent.y = 12;

    num = dictGetWordsCount(currFile);
    WinEraseRectangle(&rc, 7);
    WinDrawRectangleFrame(roundFrame, &rc);
    DrawCenteredString(appContext, SEARCH_TXT, rc.topLeft.y + 5);
    WinDrawRectangleFrame(roundFrame, &rcStop);
    DrawCenteredString(appContext, "Stop", rcStop.topLeft.y);

    if (pattern[0] != '*' && pattern[0] != '?')
    {
        // special case: we can optimize the searching

        UInt16  len, tmpLen;
        char *  tmpPattern;  // prefix of the pattern (beginning of the pattern until '*' or '?')
        long    pos;
        long    wordCount;

        len = StrLen(pattern);
        tmpPattern = (char *) new_malloc(len + 1);
        for (i = 0; i < len; i++)
        {
            if (pattern[i] != '*' && pattern[i] != '?')
                tmpPattern[i] = pattern[i];
            else
                break;
        }
        tmpPattern[i] = 0;
        tmpLen = StrLen(tmpPattern);
        pos = dictGetFirstMatching(currFile, tmpPattern);
        str = dictGetWord(currFile, pos);
        wordCount = dictGetWordsCount(currFile);
        while (true)
        {
            // word prefix has changed and is not fitting to pattern, we quit
            if (StrNCaselessCompare(str, tmpPattern, tmpLen))
                break;
            if (WordMatchesPattern(str, pattern))
            {
                ++matchedWords;
                if (matchedWords > MATCHED_WORDS_LIMIT)
                    break;
                WriteMatchingPatternRecord(appContext, pos);
            }
            StrPrintF(tmp, "%d%%", (int)((long)(pos * 100) / (long)num));
            DrawCenteredString(appContext, tmp, rc.topLeft.y + 20);
            if (TappedInRect(&rcStop)) 
                break;
            if (++pos >= wordCount) 
                break;
            str = dictGetWord(currFile, pos);
        }
        new_free(tmpPattern);
    }
    else
    {
        // we need to iterate through the whole database
        
        for (i = 0; i < num; i++)
        {
            str = dictGetWord(currFile, i);
            if (WordMatchesPattern(str, pattern))
            {
                ++matchedWords;
                if (matchedWords > MATCHED_WORDS_LIMIT)
                    break;
                WriteMatchingPatternRecord(appContext, i);
            }
            StrPrintF(tmp, "%d%%", (int)((long)(i * 100) / (long)num));
            DrawCenteredString(appContext, tmp, rc.topLeft.y + 20);
            if (TappedInRect(&rcStop))
                break;
        }
    }
}

void PatternListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    char        *str;
    Int16       stringWidthP = 160; /* max width of the string in the list selection window */
    Int16       stringLenP;
    Boolean     truncatedP = false;
    long        realItemNo;
    AppContext* appContext = GetAppContext();

    Assert(itemNum >= 0);

    if (ReadMatchingPatternRecord(appContext, itemNum, &realItemNo) == errNone)
    {
        str = dictGetWord(GetCurrentFile(appContext), realItemNo);
        stringLenP = StrLen(str);

        FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
        WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
    }
}

// Set topLeft.y coordinate of object objID on form frm to topLeftY.
// Leave all aother coordinates unchanged
static void SetObjectTopLeftY(FormType* frm, UInt16 objID, Coord topLeftY)
{
    RectangleType  newBounds;
    UInt16         index=FrmGetObjectIndex(frm, objID);

    Assert(index!=frmInvalidObjectId);
    FrmGetObjectBounds(frm, index, &newBounds);
    newBounds.topLeft.y= topLeftY;
    FrmSetObjectBounds(frm, index, &newBounds);
}

static Boolean FindPatternFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Boolean handled=false;
    if (DIA_Supported(&appContext->diaSettings))
    {
        UInt16 index=0;
        ListType* list=0;
        RectangleType newBounds;

        Coord screenHeightMinus14 = appContext->screenHeight-14;

        WinGetBounds(WinGetDisplayWindow(), &newBounds);
        WinSetBounds(FrmGetWindowHandle(frm), &newBounds);

        index=FrmGetObjectIndex(frm, listMatching);
        Assert(index!=frmInvalidObjectId);
        list=(ListType*)FrmGetObjectPtr(frm, index);
        Assert(list);
        LstSetHeight(list, appContext->dispLinesCount);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.extent.x=appContext->screenWidth;
        FrmSetObjectBounds(frm, index, &newBounds);

        SetObjectTopLeftY(frm, fieldWord, screenHeightMinus14);
        SetObjectTopLeftY(frm, buttonFind, screenHeightMinus14);
        SetObjectTopLeftY(frm, buttonCancel, screenHeightMinus14);
        SetObjectTopLeftY(frm, buttonOneChar, screenHeightMinus14);
        SetObjectTopLeftY(frm, buttonAnyChar, screenHeightMinus14);

        FrmDrawForm(frm);
        handled=true;
    }
    return handled;
}

static void FindPatternFormFindButtonPressed(AppContext *appContext,FormType *frm)
{
    FieldPtr    fld;
    ListPtr     list;
    char *      pattern;

    fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
    list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
    pattern = FldGetTextPtr(fld);
    if (pattern != NULL)
    {
        ClearMatchingPatternDB(appContext);
        WritePattern(appContext, pattern);
        FillMatchingPatternDB(appContext, pattern);
        LstSetListChoicesEx(list, NULL, NumMatchingPatternRecords(appContext));
        LstSetDrawFunction(list, PatternListDrawFunc);
        LstDrawList(list);
    }
}

static Boolean FindPatternFormKeyDown(AppContext* appContext, FormType* frm, EventType* event)
{
    Boolean     handled = false;

    // TODO: add scrolling of the list with 5-Way Navigator and page up/page down?
    switch (event->data.keyDown.chr)
    {
        case returnChr:
        case linefeedChr:
            FindPatternFormFindButtonPressed(appContext, frm);
            handled = true;
            break;

        default:
            if ( HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) )
            {
                if (FiveWayCenterPressed(appContext, event))
                {
                    FindPatternFormFindButtonPressed(appContext,frm);
                    handled = true;
                    break;
                }
            }
            break;
    }
    return handled;
}

// Event handler proc for the find word using pattern form
Boolean FindPatternFormHandleEvent(EventType* event)
{
    Boolean     handled = false;
    FormPtr     frm = FrmGetActiveForm();
    FieldPtr    fld;
    ListPtr     list;
    long        prevMatchWordCount;
    char *      pattern;
    AppContext* appContext=GetAppContext();

    switch (event->eType)
    {
        case winDisplayChangedEvent:
            handled = FindPatternFormDisplayChanged(appContext, frm);
            break;
                        
        case frmOpenEvent:
            OpenMatchingPatternDB(appContext);
            list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
            fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
            prevMatchWordCount = NumMatchingPatternRecords(appContext);
            LstSetListChoicesEx(list, NULL, prevMatchWordCount);
            LstSetDrawFunction(list, PatternListDrawFunc);
            FrmDrawForm(frm);
            if (prevMatchWordCount>0)
                LstSetSelectionEx(appContext, list, appContext->wmpLastWordPos);
            pattern = (char *) new_malloc(WORDS_CACHE_SIZE);
            ReadPattern(appContext, pattern);
            if (StrLen(pattern) > 0)
                FldInsert(fld, pattern, StrLen(pattern));
            FrmSetFocus(frm, FrmGetObjectIndex(frm, fieldWord));
            new_free(pattern);
            handled = true;
            break;

        case keyDownEvent:
            handled = FindPatternFormKeyDown(appContext, frm, event);
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    CloseMatchingPatternDB(appContext);
                    FrmReturnToForm(0);
                    handled = true;
                    break;

                case buttonFind:
                    FindPatternFormFindButtonPressed(appContext, frm);
                    handled = true;
                    break;
                
                case buttonOneChar:
                    fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
                    FldInsert(fld, "?", 1);
                    handled = true;
                    break;

                case buttonAnyChar:
                    fld = (FieldType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, fieldWord));
                    FldInsert(fld, "*", 1);
                    handled = true;
                    break;

                default:
                    break;
            }
            break;

        case lstSelectEvent:
            appContext->wmpLastWordPos = appContext->listItemOffset + (UInt32) event->data.lstSelect.selection;
            ReadMatchingPatternRecord(appContext, appContext->wmpLastWordPos, &appContext->currentWord);
            SendNewWordSelected();
            FrmReturnToForm(0);
            return true;

        default:
            break;
    }
    
    return handled;
}

