/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)

  All the common code shared between my other apps.
*/

#include "common.h"
#include "five_way_nav.h"
#include "menu_support_database.h"

#ifndef NOAH_LITE
void SetPopupLabel(FormType * frm, UInt16 listID, UInt16 popupID, Int16 txtIdx)
{
    ListType *list = NULL;
    char* listTxt;
    Assert(frm);

    list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listID));
    LstSetSelection(list, txtIdx);
    listTxt = LstGetSelectionText(list, txtIdx);
    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, popupID)), listTxt);
}
#endif //NOAH_LITE

#ifndef I_NOAH

/* those functions are not available for Noah Lite */
#ifndef NOAH_LITE

void HistoryListInit(AppContext* appContext, FormType *frm)
{
    ListType *  list;

    list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
    LstSetListChoices(list, NULL, appContext->historyCount);
    LstSetDrawFunction(list, HistoryListDrawFunc);
    HistoryListSetState(appContext, frm);
}

void HistoryListSetState(AppContext* appContext, FormType *frm)
{
    ListType *  list;

    list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,  listHistory));
    if (0 == appContext->historyCount)
        CtlHideControlEx(frm,popupHistory);
    else
    {
        LstSetListChoices(list, NULL, appContext->historyCount);
        CtlShowControlEx(frm,popupHistory);
    }
}

void HistoryListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    char *  str;
    AppContext* appContext=GetAppContext();
    /* max width of the string in the list selection window */
    Int16   stringWidthP = HISTORY_LIST_DX;
    Int16   stringLenP;
    Boolean truncatedP = false;
    Assert(appContext);
    Assert((itemNum >= 0) && (itemNum <= appContext->historyCount));

    str = appContext->wordHistory[itemNum];
    stringLenP = StrLen(str);

    FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
    WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
}

void strtolower(char *txt)
{
    while (*txt)
    {
        if ((*txt > 'A') && (*txt < 'Z'))
        {
            *txt = *txt - 'A' + 'a';
        }
        ++txt;
    }
}

void AddToHistory(AppContext* appContext, UInt32 wordNo)
{
    char *  word;
    int     i;

    word = dictGetWord(GetCurrentFile(appContext), wordNo);

    // don't add duplicates
    for (i=0; i<appContext->historyCount; i++)
    {
        if ( 0 == StrCompare(word, appContext->wordHistory[i]) )
            return;
    }

    word = strdup(word);
    if (!word)
        return;

    if (appContext->historyCount==HISTORY_ITEMS) 
        new_free(appContext->wordHistory[HISTORY_ITEMS-1]);
    MemMove(&(appContext->wordHistory[1]), &(appContext->wordHistory[0]), ((HISTORY_ITEMS - 1) * sizeof(appContext->wordHistory[0])));
    appContext->wordHistory[0] = word;

    if (appContext->historyCount < HISTORY_ITEMS)
        ++appContext->historyCount;
}

void FreeHistory(AppContext* appContext)
{
    int i;
    for (i=0;i<appContext->historyCount;i++)
    {
        new_free(appContext->wordHistory[i]);
    }
    appContext->historyCount = 0;
}

// return false if didn't find anything in clipboard, true if 
// got word from clipboard
Boolean FTryClipboard(AppContext* appContext)
{
    MemHandle   clipItemHandle;
    char        txt[WORD_MAX_LEN+1];
    char *      clipTxt;
    char *      word;
    UInt32      wordNo;
    int         idx;
    UInt16      itemLen;

    Assert(GetCurrentFile(appContext));

    clipItemHandle = ClipboardGetItem(clipboardText, &itemLen);
    if (!clipItemHandle || (0==itemLen))
            return false;

    clipTxt = (char *) MemHandleLock(clipItemHandle);

    if (!clipTxt)
    {
        MemHandleUnlock( clipItemHandle );
        return false;
    }

    MemSet(txt, sizeof(txt), 0);
    itemLen = (itemLen < sizeof(txt)-1) ? itemLen : sizeof(txt)-1;
    MemMove(txt, clipTxt, itemLen);

    strtolower(txt);
    RemoveWhiteSpaces( txt );
    idx = 0;
    while (txt[idx] && (txt[idx] == ' '))
        ++idx;

    MemHandleUnlock(clipItemHandle);

    wordNo = dictGetFirstMatching(GetCurrentFile(appContext), txt);
    word = dictGetWord(GetCurrentFile(appContext), wordNo);

    if (0 == StrNCaselessCompare(&(txt[idx]), word,  ((UInt16) StrLen(word) <  itemLen) ? StrLen(word) : itemLen))
    {
        DrawDescription(appContext, wordNo);
        return true;
    }
    return false;
}


#endif

void FreeDicts(AppContext* appContext)
{
    // make sure to call DictCurrentFree() before calling here
    Assert( NULL == GetCurrentFile(appContext) );
    while(appContext->dictsCount>0)
    {
        AbstractFileFree( appContext->dicts[--appContext->dictsCount] );
    }
}

#endif // I_NOAH

/*
Detect an Palm OS version and return it in a way easy for later
examination (20 for 2.0, 35 for 3.5 etc.)
*/
int GetOsVersion(AppContext* appContext)
{
    UInt32      osVersionPart;
    if ( appContext->osVersion==0 )
    {
        FtrGet( sysFtrCreator, sysFtrNumROMVersion, &osVersionPart );
        appContext->osVersion = sysGetROMVerMajor(osVersionPart)*10;
        if ( sysGetROMVerMinor(osVersionPart) < 10 )
            appContext->osVersion += sysGetROMVerMinor(osVersionPart);
    }
    return appContext->osVersion;
}

/*
Find out and return the max screen depth supported by the given os.
Number of supported colors is 2^screen_depth
OS less than 30 only supports 1 (2 colors).
*/
int GetMaxScreenDepth(AppContext* appContext)
{
    UInt32    supportedDepths;
    UInt32    i;

    if (!appContext->maxScreenDepth) {
        appContext->maxScreenDepth = 1;
        if ( GetOsVersion(appContext) >= 35 )
        {
            WinScreenMode( winScreenModeGetSupportedDepths, NULL, NULL, &supportedDepths, NULL );
            for ( i = 1; supportedDepths; i++ )
            {
                if ( ( supportedDepths & 0x01 ) == 0x01 )
                    appContext->maxScreenDepth = i;
                supportedDepths >>= 1;
            }
        }
    }      
    return appContext->maxScreenDepth;
}

/*
Find out and return current depth of the screen.
*/
int GetCurrentScreenDepth(AppContext* appContext)
{
    UInt32    depth;
    int       osVersion = GetOsVersion(appContext);

    if ( osVersion >= 35 )
    {
        WinScreenMode( winScreenModeGet, NULL, NULL, &depth, NULL );
        return (int)depth;
    }
    return 1;
}

/*
Check if color is supported on this device
*/
Boolean IsColorSupported(AppContext* appContext)
{
    Boolean     fSupported;

    if ( GetOsVersion(appContext) >= 35 )
    {
        WinScreenMode( winScreenModeGet, NULL, NULL, NULL, &fSupported);
        return fSupported;
    }
    return false;
}

void SetTextColor(AppContext* appContext, RGBColorType *color)
{
    if ( GetOsVersion(appContext) >= 40 )
    {
        WinSetTextColorRGB (color, NULL);
        WinSetForeColorRGB (color, NULL);
    }
}

void SetTextColorBlack(AppContext* appContext)
{
    RGBColorType rgb_color;
    rgb_color.index = 0;
    rgb_color.r = 0;
    rgb_color.g = 0;
    rgb_color.b = 0;
    SetTextColor( appContext, &rgb_color );
}
void SetTextColorRed(AppContext* appContext)
{
    RGBColorType rgb_color;
    rgb_color.r = 255;
    rgb_color.g = 0;
    rgb_color.b = 0;
    SetTextColor( appContext, &rgb_color );

}
void SetBackColor(AppContext* appContext,RGBColorType *color)
{
    if ( GetOsVersion(appContext) >= 40 )
    {
        WinSetBackColorRGB (color, NULL);
    }
}
void SetBackColorWhite(AppContext* appContext)
{
    RGBColorType rgb_color;

    rgb_color.index = 0;
    rgb_color.r = 255;
    rgb_color.g = 255;
    rgb_color.b = 255;
    SetBackColor( appContext, &rgb_color );
}
void SetBackColorRGB(int r, int g, int b,AppContext* appContext)
{
    RGBColorType rgb_color;

    rgb_color.r = r;
    rgb_color.g = g;
    rgb_color.b = b;
    SetBackColor( appContext, &rgb_color );
}
void SetTextColorRGB(int r, int g, int b,AppContext* appContext)
{
    RGBColorType rgb_color;

    rgb_color.r = r;
    rgb_color.g = g;
    rgb_color.b = b;
    SetTextColor(appContext, &rgb_color );
}
void SetGlobalBackColor(AppContext* appContext)
{
    RGBColorType rgb_color;

    rgb_color.r = appContext->prefs.displayPrefs.bgcolR;
    rgb_color.g = appContext->prefs.displayPrefs.bgcolG;
    rgb_color.b = appContext->prefs.displayPrefs.bgcolB;
    SetBackColor( appContext, &rgb_color);
}

void HideScrollbar(void)
{
    FormType *frm;
    frm = FrmGetActiveForm();
    SclSetScrollBar((ScrollBarType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, scrollDef)), 0,  0, 0, 1);
}

void SetScrollbarState(DisplayInfo * di, int maxLines, int firstLine)
{
    FormType    *frm;
    Short        min, max, value, page_size;
    AppContext  *appContext = GetAppContext();

    SetGlobalBackColor(appContext);
    if ((appContext->lastDispLine - firstLine + 1) > diGetLinesCount(di))
    {
        HideScrollbar();
        SetBackColorWhite(appContext); 
        return;
    }

    if ((appContext->lastDispLine + 1) > diGetLinesCount(di))
    {
        /* ??? */
        SetBackColorWhite(appContext); 
        return;
    }

    min = 0;
    page_size = (appContext->lastDispLine - firstLine + 1);
    max = diGetLinesCount(di) - page_size;
    value = firstLine;

    frm = FrmGetActiveForm();
    // not really sure why I need to do it since the problem only shows up in
    // Noah Pro and not lite/thes
    SclDrawScrollBar( (ScrollBarType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, scrollDef)) );
    SclSetScrollBar((ScrollBarType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, scrollDef)), value, min, max, page_size);
    SetBackColorWhite(appContext); 
}

#ifndef I_NOAH

#pragma segment Segment2

#ifndef I_NOAH
void DisplayHelp(AppContext* appContext)
{
    char *rawTxt;

    if (NULL == appContext->currDispInfo)
    {
        appContext->currDispInfo = diNew();
        if (NULL == appContext->currDispInfo)
        {
            /* TODO: we should rather exit, since this means totally
               out of ram */
            return;
        }
    }
    rawTxt = ebufGetDataPointer(appContext->helpDispBuf);
    diSetRawTxt(appContext->currDispInfo, rawTxt);
    appContext->firstDispLine = 0;
    ClearDisplayRectangle(appContext);
    cbNoSelection(appContext);    

    // this is a hack - because of other formatting changes help is displayed
    // in a large font. it has to be small font
    FntSetFont(stdFont);
    DrawDisplayInfo(appContext->currDispInfo, 0, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
    SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
}
#endif

#pragma segment Segment1

extern void DisplayAbout(AppContext* appContext);

static void RedrawWordDefinition(AppContext* appContext)
{
    Int16 wordLen = 0;
    char *word = NULL;

    // might happen if there is no word chosen yet, in which case we
    // display about screen
    if (NULL == appContext->currDispInfo)
    {
        DisplayAbout(appContext);
        return;
    }

    // it can happen that the dictionary has been deleted in the meantime
    // (during rescanning of databases in selecting of new databases)
    // in that case do nothing
    // UNDONE: find a better solution?
    if ( NULL == GetCurrentFile(appContext) )
        return;


    /* write the word at the bottom */
    word = dictGetWord(GetCurrentFile(appContext), appContext->currentWord);

    // TODO: replace with SetWordAsLastWord() ?
    wordLen = StrLen(word);
    MemSet(appContext->prefs.lastWord, WORD_MAX_LEN, 0);
    MemMove(appContext->prefs.lastWord, word, wordLen < WORD_MAX_LEN-1 ? wordLen : WORD_MAX_LEN);

    SetBackColorWhite(appContext);
    DrawWord(word, appContext->screenHeight-FONT_DY);
    ClearDisplayRectangle(appContext);
    DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
    SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);

}

#pragma segment Segment2

void SendNewWordSelected(void)
{
    EventType   newEvent;
    MemSet(&newEvent, sizeof(EventType), 0);
    newEvent.eType = (eventsEnum) evtNewWordSelected;
    EvtAddEventToQueue(&newEvent);
}

#pragma segment Segment1

void RedrawMainScreen(AppContext* appContext)
{
    FrmDrawForm(FrmGetActiveForm());
    WinDrawLine(0, appContext->screenHeight-FRM_RSV_H+1, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H+1);
    WinDrawLine(0, appContext->screenHeight-FRM_RSV_H, appContext->screenWidth, appContext->screenHeight-FRM_RSV_H);
    RedrawWordDefinition(appContext);
}

void DrawDescription(AppContext* appContext, long wordNo)
{
    Err err;

    Assert(wordNo < appContext->wordsCount);

    // if currDispInfo has not been yet allocated, allocate it now.
    // It'll be reused
    if (NULL == appContext->currDispInfo)
    {
        appContext->currDispInfo = diNew();
        if (NULL == appContext->currDispInfo)
        {
            // UNDONE: we should rather exit, since this means totally
            // out of ram
            return;
        }
    }

    DrawCentered(appContext, SEARCH_TXT);

    err = dictGetDisplayInfo(GetCurrentFile(appContext), wordNo, 120, appContext->currDispInfo);
    if (err != 0)
    {
        return;
    }

    appContext->currentWord = wordNo;
    appContext->firstDispLine = 0;

    RedrawWordDefinition(appContext);
}

#endif //I_NOAH

void ClearDisplayRectangle(AppContext* appContext)
{
    SetGlobalBackColor(appContext);
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, appContext->screenWidth - DRAW_DI_X, appContext->screenHeight - DRAW_DI_Y - FRM_RSV_H);
}

void ClearRectangle(Int16 sx, Int16 sy, Int16 ex, Int16 ey)
{
    RectangleType r;
    r.topLeft.x = sx;
    r.topLeft.y = sy;
    r.extent.x = ex;
    r.extent.y = ey;
    WinEraseRectangle(&r, 0);
}

void DrawCentered(AppContext* appContext, char *txt)
{
    FontID prev_font;

    ClearDisplayRectangle(appContext);
    prev_font = FntGetFont();
    FntSetFont((FontID) 1);
    WinDrawChars(txt, StrLen(txt), 46, (appContext->screenHeight - 20) / 2);
    FntSetFont(prev_font);
}

void DrawStringCenteredInRectangle(AppContext* appContext, const char *str, int dy)
{
    RectangleType   rc;
    Int16           strDx;
    int             strLen;

    Assert(str);
    strLen = StrLen(str);
    strDx = FntCharsWidth(str, strLen);

    rc.topLeft.x = ((appContext->screenWidth - strDx) / 2) - 3;
    rc.topLeft.y = dy;
    rc.extent.x  = strDx+6;
    rc.extent.y  = 12;  // TODO: should be font dy?

    WinDrawRectangleFrame(roundFrame, &rc);
    DrawCenteredString(appContext, str, dy);
}

void DrawCenteredString(AppContext* appContext, const char *str, int dy)
{
    Int16 strDx;
    int strLen;

    Assert(str);
    strLen = StrLen(str);
    strDx = FntCharsWidth(str, strLen);
    WinDrawChars(str, strLen, (appContext->screenWidth - strDx) / 2, dy);
}

/* max width of the word displayed at the bottom */
#define MAX_WORD_DX        100

void DrawWord(char *word, int pos_y)
{
    Int16 wordLen = 0;
    Int16 word_dx = MAX_WORD_DX;
    
#ifndef I_NOAH    
    Int16 wordStartX=21;
#else 
    Int16 wordStartX=1;
#endif
        
    Boolean truncatedP = false;
    FontID prev_font;

    Assert(NULL != word);

    wordLen = StrLen(word);
    ClearRectangle(wordStartX, pos_y - 1, MAX_WORD_DX, FONT_DY);
    prev_font = FntGetFont();
    FntSetFont((FontID) 1);
    FntCharsInWidth(word, &word_dx, &wordLen, &truncatedP);
    WinDrawChars(word, wordLen, wordStartX+1, pos_y - 2);
    FntSetFont(prev_font);
}

#ifndef I_NOAH
/**
 *  Add params to defs cache
 *  We keep it in order!
 */
static void AddDefsToCache(DefsCache *cache, long current_entry, int curr_record, long idx_in_rec, long offset)
{
    int     i,j;
    int     insert_position;
    long    diffs[DEFS_CACHE_SIZE+1];
    long    minDiff = 900000;
   
    if(cache->numberOfEntries == DEFS_CACHE_SIZE)
    {
        //find one to delete
        //find diffs between entries
        diffs[0] = cache->current_entry[0];
        for(i = 1; i < DEFS_CACHE_SIZE;i++)
            diffs[i] = cache->current_entry[i]-cache->current_entry[i-1];
        //find smallest diff
        for(i=0;i<DEFS_CACHE_SIZE;i++)
            if(diffs[i] < minDiff)
            {
                minDiff = diffs[i];
                j = i;            
            }
        //position j will be deleted
        for(i = j; i<DEFS_CACHE_SIZE-1;i++)
        {
            cache->current_entry[i] = cache->current_entry[i+1];
            cache->curr_record[i] = cache->curr_record[i+1];
            cache->idx_in_rec[i] = cache->idx_in_rec[i+1];
            cache->offset[i] = cache->offset[i+1];
        }
        cache->numberOfEntries--;
    }
    //find where insert
    i = 0;
    insert_position = -1;
    while(i < cache->numberOfEntries)
    {
        if(cache->current_entry[i] < current_entry)
            insert_position = i;
        i++;
    }
    insert_position++;
    //make room
    for(i = cache->numberOfEntries-1; i >= insert_position; i--)  
    {
        cache->current_entry[i+1] = cache->current_entry[i];
        cache->curr_record[i+1] = cache->curr_record[i];
        cache->idx_in_rec[i+1] = cache->idx_in_rec[i];
        cache->offset[i+1] = cache->offset[i];
    }    
    //insert
    cache->current_entry[insert_position] = current_entry;
    cache->curr_record[insert_position] = curr_record;
    cache->idx_in_rec[insert_position] = idx_in_rec;
    cache->offset[insert_position] = offset;
    cache->numberOfEntries++;
}
/**
 *  Take params from defs cache
 */
static Boolean GetDefsFromCache(DefsCache *cache, long wanted_entry, long *current_entry, int *curr_record, long *idx_in_rec, long *offset)
{
    int     i,j;

    if(cache->numberOfEntries == 0)
        return false;
    
    j = -1;
    for(i = 0; i < cache->numberOfEntries; i++)
        if(cache->current_entry[i] < wanted_entry)
            j = i;
    
    if(j < 0)
        return false;
    
    *current_entry = cache->current_entry[j];
    *curr_record = cache->curr_record[j];
    *idx_in_rec = cache->idx_in_rec[j];
    *offset = cache->offset[j];
    return true;
}

/**
 *  Faster version of function get_defs_records - 
 *  diffrence: "long entry_count" is set to 1. 
 */
static Boolean get_defs_records_oneEntryCount(AbstractFile* file, int first_record_with_defs_len,  int defs_len_rec_count, int first_record_with_defs, SynsetDef * synsets, WcInfo *wcInfo)
{
    long offset = 0;
    long curr_len;
    int curr_record = 0;
    long curr_rec_size = 0;
    long idx_in_rec = 0;
    unsigned char *def_lens = NULL;
    unsigned char *def_lens_fast = NULL;
    unsigned char *def_lens_fast_end = NULL;
    long current_entry = 0;
    Boolean loop_end_p;
    DefsCache *cache = &wcInfo->defsCache;
        
    Assert(cache);
    Assert(synsets);
    Assert(entry_count >= 0);
    Assert(first_record_with_defs_len >= 0);
    Assert(defs_len_rec_count > 0);
    Assert(first_record_with_defs >= 0);

    if(!GetDefsFromCache(cache, synsets[0].synsetNo, &current_entry, &curr_record, &idx_in_rec, &offset))
    {
        curr_record = first_record_with_defs_len;
    }
    def_lens = (unsigned char *) fsLockRecord(file, curr_record);
    if (NULL == def_lens)
    {
        return false;
    }
    curr_rec_size = fsGetRecordSize(file, curr_record);

    /* calculate the length and offset of all entries */
    def_lens_fast = def_lens;
    def_lens_fast += idx_in_rec;
    def_lens_fast_end = def_lens + curr_rec_size;
    
    do
    {
        curr_len = (UInt32) ((unsigned char)def_lens_fast[0]);
        def_lens_fast++;
        if (255 == curr_len)
        {
            curr_len = (UInt32)((unsigned char)(def_lens_fast[0])) * 256;
            def_lens_fast++;
            curr_len += (UInt32)(unsigned char)(def_lens_fast[0]);
            def_lens_fast++;
        }

        if (current_entry == synsets[0].synsetNo)
        {
            synsets[0].offset = offset;
            synsets[0].dataSize = curr_len;
            idx_in_rec = (long)(def_lens_fast-def_lens);
            if(curr_len >= 255)
                idx_in_rec-=2;
            idx_in_rec--;    
            AddDefsToCache(cache, current_entry, curr_record, idx_in_rec, offset);
            break;
        }

        offset += curr_len;
        Assert(def_lens_fast_end <= def_lens_fast);
        if(def_lens_fast_end == def_lens_fast)
        {
            fsUnlockRecord(file, curr_record);
            ++curr_record;
            Assert(curr_record < first_record_with_defs_len + defs_len_rec_count);
            def_lens = (unsigned char *) fsLockRecord(file, curr_record);
            def_lens_fast = def_lens;
            curr_rec_size = fsGetRecordSize(file, curr_record);
            def_lens_fast_end = def_lens + curr_rec_size;
        }
        ++current_entry;
    }
    while (true);

    fsUnlockRecord(file, curr_record);

    /* -1 means that we haven't yet found record for this one */
    synsets[0].record = -1;

    /* find out the record it is in and the offset in that record */
    curr_record = first_record_with_defs;
    do
    {
        loop_end_p = true;
        if (-1 == synsets[0].record)
        {
            /* didn't find the record yet for this one */
            if (synsets[0].offset >= fsGetRecordSize(file, curr_record))
            {
                synsets[0].offset -= fsGetRecordSize(file, curr_record);
                loop_end_p = false;
            }
            else
            {
                synsets[0].record = curr_record;
                Assert(synsets[0].offset < 65000);
            }
        }
        ++curr_record;
    }
    while (!loop_end_p);
    return true;
}

/*
  Given entry number find out entry's definition (record,
  offset within the record and the definitions' length)
 */
 
Boolean get_defs_record(AbstractFile* file, long entry_no, int first_record_with_defs_len, int defs_len_rec_count, int first_record_with_defs, int *record_out, long *offset_out, long *len_out, WcInfo *wcInfo)
{
    SynsetDef synset_def;
    MemSet(&synset_def, sizeof(synset_def), 0);

    Assert(entry_no >= 0);
    Assert(first_record_with_defs_len >= 0);
    Assert(defs_len_rec_count > 0);
    Assert(first_record_with_defs >= 0);
    Assert(record_out);
    Assert(offset_out);
    Assert(len_out);

    synset_def.synsetNo = entry_no;
    /*old version - slow!!! new version knows that the 1 is constant */
    //if ( !get_defs_records(file, 1, first_record_with_defs_len, defs_len_rec_count, first_record_with_defs, &synset_def) )
    if ( !get_defs_records_oneEntryCount(file, first_record_with_defs_len, defs_len_rec_count, first_record_with_defs, &synset_def, wcInfo) )
    {
        return false;
    }

    *record_out = synset_def.record;
    *offset_out = synset_def.offset;
    *len_out = synset_def.dataSize;
    return true;
}

Boolean get_defs_records(AbstractFile* file, long entry_count, int first_record_with_defs_len,  int defs_len_rec_count, int first_record_with_defs, SynsetDef * synsets)
{
    long offset = 0;
    long curr_len;
    long idx_in_rec = 0;
    int curr_record = 0;
    long curr_rec_size = 0;
    unsigned char *def_lens = NULL;
    long current_entry = 0;
    long current_entry_test;
    int entry_index = 0;
    Boolean loop_end_p;

    Assert(synsets);
    Assert(entry_count >= 0);
    Assert(first_record_with_defs_len >= 0);
    Assert(defs_len_rec_count > 0);
    Assert(first_record_with_defs >= 0);

    curr_record = first_record_with_defs_len;
    def_lens = (unsigned char *) fsLockRecord(file, curr_record);
    if (NULL == def_lens)
    {
        return false;
    }
    curr_rec_size = fsGetRecordSize(file, curr_record);

    /* calculate the length and offset of all entries */
    idx_in_rec = 0;
    current_entry = 0;
    entry_index = 0;

    current_entry_test = synsets[entry_index].synsetNo;
    do
    {
        curr_len = (UInt32) def_lens[idx_in_rec++];
        if (255 == curr_len)
        {
            curr_len = (UInt32) ((UInt32) def_lens[idx_in_rec] * 256 + def_lens[idx_in_rec + 1]);
            idx_in_rec += 2;
        }

        if (current_entry == current_entry_test)        
        {

            synsets[entry_index].offset = offset;
            synsets[entry_index].dataSize = curr_len;
            
            ++entry_index;
            if (entry_index == entry_count) 
            {
                break;
            }
            current_entry_test = synsets[entry_index].synsetNo;
        }

        offset += curr_len;
        Assert( idx_in_rec <= curr_rec_size );
        if (idx_in_rec == curr_rec_size)
        {
            fsUnlockRecord(file, curr_record);
            ++curr_record;
            Assert(curr_record < first_record_with_defs_len + defs_len_rec_count);
            def_lens = (unsigned char *) fsLockRecord(file, curr_record);
            curr_rec_size = fsGetRecordSize(file, curr_record);
            idx_in_rec = 0;
        }
        ++current_entry;
    }
    while (true);

    fsUnlockRecord(file, curr_record);

    /* -1 means that we haven't yet found record for this one */
    for (entry_index = 0; entry_index < entry_count; ++entry_index)
    {
        synsets[entry_index].record = -1;
    }

    /* find out the record it is in and the offset in that record */
    curr_record = first_record_with_defs;
    entry_index = 0;
    do
    {
        loop_end_p = true;
        for (entry_index = 0; entry_index < entry_count; ++entry_index)
        {
            if (-1 == synsets[entry_index].record)
            {
                /* didn't find the record yet for this one */
                if (synsets[entry_index].offset >= fsGetRecordSize(file, curr_record))
                {
                    synsets[entry_index].offset -= fsGetRecordSize(file, curr_record);
                    loop_end_p = false;
                }
                else
                {
                    synsets[entry_index].record = curr_record;
                    Assert(synsets[entry_index].offset < 65000);
                }
            }
        }
        ++curr_record;
    }
    while (!loop_end_p);

    return true;
}

char *GetNthTxt(int n, char *txt)
{
    while (n > 0)
    {
        while (*txt)
        {
            ++txt;
        }
        ++txt;
        --n;
    }
    return txt;
}

char* GetWnPosTxt(int pos)
{
    return GetNthTxt(pos, "(noun) \0(verb) \0(adj.) \0(adv.) \0");
}

#define GetFileType(file) ((file)->type)

Boolean dictNew(AbstractFile* file)
{
    switch (GetFileType(file))
    {
#ifdef THESAURUS
        case ROGET_TYPE:
            file->dictData.roget = RogetNew(file);
            if (NULL == file->dictData.roget)
            {
                LogG("dictNew(): RogetNew() failed" );
                return false;
            }
            break;
#endif
#ifdef WN_PRO_DICT
        case WORDNET_PRO_TYPE:
            file->dictData.wn = (struct _WnInfo*) wn_new(file);
            if (NULL == file->dictData.wn)
            {
                LogG( "dictNew(): wn_new() failed" );
                return false;
            }
            break;
#endif
#ifdef WNLEX_DICT
        case WORDNET_LITE_TYPE:
            file->dictData.wnLite = (struct _WnLiteInfo*) wnlex_new(file);
            if (NULL == file->dictData.wnLite)
            {
                LogG( "dictNew(): wnlex_new() failed" );
                return false;
            }
            break;
#endif
#ifdef SIMPLE_DICT
        case SIMPLE_TYPE:
            file->dictData.simple = (struct _SimpleInfo*)simple_new(file);
            if ( NULL == file->dictData.simple )
            {
                LogG( "dictNew(): simple_new() failed" );
                return false;
            }
            break;
#endif
#ifdef EP_DICT
        case ENGPOL_TYPE:
            file->dictData.engpol = (EngPolInfo*)epNew(file);
            if ( NULL == file->dictData.engpol )
            {
                LogG( "dictNew(): epNew() failed" );
                return false;
            }
            break;
#endif
        default:
            LogG( "dictNew(): assert" );
            Assert(0);
            break;
    }
    LogV1( "dictNew(%s) ok", file->fileName );
    return true;
}

/* free all resources allocated by dictionary */
void dictDelete(AbstractFile* file)
{
    if ( NULL == file ) return;
    switch (GetFileType(file))
    {
#ifdef THESAURUS
        case ROGET_TYPE:
            RogetDelete( file->dictData.roget );
            file->dictData.roget = NULL;
            break;
#endif
#ifdef WN_PRO_DICT
        case WORDNET_PRO_TYPE:
            wn_delete( file->dictData.wn );
            file->dictData.wn = NULL;
            break;
#endif
#ifdef WNLEX_DICT
        case WORDNET_LITE_TYPE:
            wnlex_delete( file->dictData.wnLite );
            file->dictData.wnLite = NULL;
            break;
#endif
#ifdef SIMPLE_DICT
        case SIMPLE_TYPE:
            simple_delete( file->dictData.simple );
            file->dictData.simple = NULL;
            break;
#endif
#ifdef EP_DICT
        case ENGPOL_TYPE:
            epDelete( file->dictData.engpol );
            file->dictData.engpol = NULL;
            break;
#endif
        default:
            Assert(0);
            break;
    }
}

long dictGetWordsCount(AbstractFile* file)
{
    switch (GetFileType(file))
    {
#ifdef THESAURUS
        case ROGET_TYPE:
            return RogetGetWordsCount( file->dictData.roget );
#endif
#ifdef WN_PRO_DICT
        case WORDNET_PRO_TYPE:
            return wn_get_words_count( file->dictData.wn );
#endif
#ifdef WNLEX_DICT
        case WORDNET_LITE_TYPE:
            return wnlex_get_words_count( file->dictData.wnLite );
#endif
#ifdef SIMPLE_DICT
        case SIMPLE_TYPE:
            return simple_get_words_count( file->dictData.simple );
        break;
#endif
#ifdef EP_DICT
        case ENGPOL_TYPE:
            return epGetWordsCount( file->dictData.engpol );
#endif
        default:
            Assert(0);
            return 0;
    }
    return 0;
}

long dictGetFirstMatching(AbstractFile* file, char *word)
{
    switch (GetFileType(file))
    {
#ifdef THESAURUS
        case ROGET_TYPE:
            return RogetGetFirstMatching( file->dictData.roget, word );
#endif
#ifdef WN_PRO_DICT
        case WORDNET_PRO_TYPE:
            return wn_get_first_matching( file->dictData.wn, word );
#endif
#ifdef WNLEX_DICT
        case WORDNET_LITE_TYPE:
            return wnlex_get_first_matching( file->dictData.wnLite, word );
#endif
#ifdef SIMPLE_DICT
        case SIMPLE_TYPE:
            return simple_get_first_matching( file->dictData.simple, word );
#endif
#ifdef EP_DICT
        case ENGPOL_TYPE:
            return epGetFirstMatching( file->dictData.engpol, word );
#endif
        default:
            Assert(0);
            return 0;
    }
    return 0;
}

char* dictGetWord(AbstractFile* file, long wordNo)
{
    switch (GetFileType(file))
    {
#ifdef THESAURUS
        case ROGET_TYPE:
            return RogetGetWord( file->dictData.roget, wordNo );
#endif
#ifdef WN_PRO_DICT
        case WORDNET_PRO_TYPE:
            return wn_get_word( file->dictData.wn, wordNo );
#endif
#ifdef WNLEX_DICT
        case WORDNET_LITE_TYPE:
            return wnlex_get_word( file->dictData.wnLite, wordNo );
#endif
#ifdef SIMPLE_DICT
        case SIMPLE_TYPE:
            return simple_get_word( file->dictData.simple, wordNo );
#endif
#ifdef EP_DICT
        case ENGPOL_TYPE:
            return epGetWord( file->dictData.engpol, wordNo );
#endif
        default:
            Assert(0);
            return NULL;
    }
    return NULL;
}

#ifdef _RECORD_SPEED_
/**
    Starts the timer and remembers the description.
*/
void StartTiming(AppContext* appContext, char * description)
{
    appContext->recordSpeedDescription = description;
    appContext->recordSpeedTicksCount = TimGetTicks();
}

/**
    Stops the timer, adds measured time to the description and saves it in memopad.
*/
void StopTiming(AppContext* appContext)
{
    ExtensibleBuffer    * buf;
    char                * tmpString;

    appContext->recordSpeedTicksCount = TimGetTicks() - appContext->recordSpeedTicksCount;    // now we know how much time it took to lookup the word
    buf = ebufNew();
    Assert(buf);
    ebufAddStr(buf, appContext->recordSpeedDescription);
    tmpString = new_malloc(maxStrIToALen);
    StrPrintF(tmpString, "%ld", appContext->recordSpeedTicksCount);
    ebufAddStr(buf, tmpString);
    new_free(tmpString);
    ebufAddChar(buf, '\0');
    // we can reuse this variable after freeing to get extensible buffer data
    tmpString = ebufGetDataPointer(buf);
    CreateNewMemo(tmpString, -1);
    ebufDelete(buf);
}
#endif

Err dictGetDisplayInfo(AbstractFile* file, long wordNo, int dx, DisplayInfo * di)
{
    Err              err;
#ifdef _RECORD_SPEED_
    char             * word;
    ExtensibleBuffer * buf;
#endif

    switch (GetFileType(file))
    {
#ifdef THESAURUS
        case ROGET_TYPE:
            err = RogetGetDisplayInfo( file->dictData.roget, wordNo, dx, di );
            return err;
#endif
#ifdef WN_PRO_DICT
        case WORDNET_PRO_TYPE:
#ifdef _RECORD_SPEED_
            buf = ebufNew();
            Assert(buf);
            ebufAddStr(buf, "wn_gdi non-ARM; '");
            word = dictGetWord(file, wordNo);
            Assert(word);
            ebufAddStr(buf, word);
            ebufAddStr(buf, "'; ticks: ");
            ebufAddChar(buf, '\0');
            StartTiming(GetAppContext(), ebufGetDataPointer(buf));
#endif
            err = wn_get_display_info( file->dictData.wn, wordNo, dx, di );
#ifdef _RECORD_SPEED_
            StopTiming(GetAppContext());
            ebufDelete(buf);
#endif
            return err;
            break;
#endif
#ifdef WNLEX_DICT
        case WORDNET_LITE_TYPE:
#ifdef _RECORD_SPEED_
            buf = ebufNew();
            Assert(buf);
            ebufAddStr(buf, "wnlex_gdi non-ARM; '");
            word = dictGetWord(file, wordNo);
            Assert(word);
            ebufAddStr(buf, word);
            ebufAddStr(buf, "'; ticks: ");
            ebufAddChar(buf, '\0');
            StartTiming(GetAppContext(), ebufGetDataPointer(buf));
#endif
            err = wnlex_get_display_info( file->dictData.wnLite, wordNo, dx, di );
#ifdef _RECORD_SPEED_
            ebufDelete(buf);
            StopTiming(GetAppContext());
#endif
            return err;
            break;
#endif
#ifdef SIMPLE_DICT
        case SIMPLE_TYPE:
            return simple_get_display_info( file->dictData.simple, wordNo, dx, di );
#endif
#ifdef EP_DICT
        case ENGPOL_TYPE:
            return epGetDisplayInfo( file->dictData.engpol, wordNo, dx, di );
#endif
        default:
            Assert(0);
            return 0;
    }
    return 0;
}

#define helpText \
    " Instructions:\n" \
    "\255 to lookup a definition of a word\n" \
    "  press the find button in the right\n" \
    "  lower corner and select the word\n" \
    "\255 you can scroll the definition using\n" \
    "  hardware buttons, tapping on the\n" \
    "  screen or using a scrollbar\n" \
    "\255 left/right arrow moves to next\n" \
    "  or previous word\n" \
    "\n" \
    " For more information go to\n" \
    " www.arslexis.com\n"

Boolean CreateHelpData(AppContext* appContext)
{
    Assert(!appContext->helpDispBuf);
    appContext->helpDispBuf = ebufNew();
    ebufAddStr(appContext->helpDispBuf, (char*)helpText);
    ebufAddChar(appContext->helpDispBuf, '\0');
    ebufWrapBigLines(appContext->helpDispBuf,false);
    return true;
}

void FreeInfoData(AppContext* appContext)
{
    if (appContext->helpDispBuf)
    {
        ebufDelete(appContext->helpDispBuf);
        appContext->helpDispBuf = NULL;
    }
    if (appContext->currDispInfo)
    {
        diFree(appContext->currDispInfo);
        appContext->currDispInfo = NULL;
    }
}

/* given a string, remove white spaces from the beginning and end of the 
string (in place) */
void RemoveWhiteSpaces( char *src )
{
    char    *dst = src;
    char    *last_space_pos = NULL;

    while ( *src && ( (' ' == *src) || ('\n' == *src) || ('\t' == *src) ) )
        ++src;

    while( *src )
    {
        if ( (*src == ' ') && (NULL == last_space_pos) )
        {
            last_space_pos = dst;
        }
        *dst++ = *src++;
    }

    if ( NULL != last_space_pos )
        *last_space_pos = '\0';
    else
        *dst = '\0';
}

/* a hack necessary for different OS versions. In <35
   the pointer to a list item was UInt, in >=35 it's
   only Int so list can only handle 2^15 items)
   For safety we declare the limit as a little less */
#define MAX_ITEMS_IN_LIST 32000 
long GetMaxListItems()
{
#if 0
	// doesn't seem to work on OS 35 (e.g. Treo 300).
	// I'm puzzled since I think it did work
    if (GetOsVersion()>35)
        return 20000;
    return 65000;
#endif
	return MAX_ITEMS_IN_LIST;
}

long CalcListOffset(long itemsCount, long itemNo)
{
    long next_offset;

    if (itemNo < GetMaxListItems())
        return 0;
    if (itemNo > (itemsCount - GetMaxListItems()))
        return itemsCount - GetMaxListItems();

    next_offset = itemNo - (GetMaxListItems() / 2);
    return next_offset;
} 
#endif //I_NOAH

void DefScrollUp(AppContext* appContext, ScrollType scroll_type)
{
    DisplayInfo *di = NULL;
    int to_scroll = 0;

    di = appContext->currDispInfo;
    if ((scrollNone == scroll_type) || (NULL == di) || (0 == appContext->firstDispLine))
        return;

    to_scroll = appContext->lastDispLine - appContext->firstDispLine;

    switch (scroll_type)
    {
    case scrollLine:
        to_scroll = 1;
        break;
    case scrollHalfPage:
        to_scroll = to_scroll / 2;
        break;
    case scrollPage:
        to_scroll = to_scroll;
        break;
    default:
        Assert(0);
        break;
    }

    if (appContext->firstDispLine >= to_scroll)
        appContext->firstDispLine -= to_scroll;
    else
        appContext->firstDispLine = 0;

    SetGlobalBackColor(appContext); //global back color
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, appContext->screenWidth-FRM_RSV_W+2, appContext->screenHeight-FRM_RSV_H);
    DrawDisplayInfo(di, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
    SetScrollbarState(di, appContext->dispLinesCount, appContext->firstDispLine);
}

void DefScrollDown(AppContext* appContext, ScrollType scroll_type)
{
    DisplayInfo *di;
    int invisible_lines = 0;
    int to_scroll = 0;

    di = appContext->currDispInfo;

    if ((scrollNone == scroll_type) || (NULL == di))
        return;

    invisible_lines = diGetLinesCount(di) - appContext->lastDispLine - 1;
    if (invisible_lines <= 0)
        return;

    to_scroll = appContext->lastDispLine - appContext->firstDispLine;

    switch (scroll_type)
    {
    case scrollLine:
        to_scroll = 1;
        break;
    case scrollHalfPage:
        to_scroll = to_scroll / 2;
        break;
    case scrollPage:
        to_scroll = to_scroll;
        break;
    default:
        Assert(0);
        break;
    }

    if (invisible_lines >= to_scroll)
        appContext->firstDispLine += to_scroll;
    else
        appContext->firstDispLine += invisible_lines;

    SetGlobalBackColor(appContext); //global back color
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, appContext->screenWidth-FRM_RSV_W+2, appContext->screenHeight-FRM_RSV_H);
    DrawDisplayInfo(di, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
    SetScrollbarState(di, appContext->dispLinesCount, appContext->firstDispLine);
}

#ifndef I_NOAH
// appContext->selectedWord is currently highlighted word, highlight word+dx (dx can
// be negative in which case we highlight previous words). Used for scrolling
// by page up/down and 5-way scroll
void ScrollWordListByDx(AppContext* appContext, FormType *frm, int dx)
{
    ListType *  lst;

    if ( (appContext->selectedWord + dx < appContext->wordsCount) && (appContext->selectedWord + dx >= 0) )
    {
        appContext->selectedWord += dx;
        lst = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
        LstSetSelectionMakeVisibleEx(appContext, lst, appContext->selectedWord);
    }
}

char *GetDatabaseName(AppContext* appContext, int dictNo)
{
    Assert((dictNo >= 0) && (dictNo < appContext->dictsCount));
    return appContext->dicts[dictNo]->fileName;
}

void LstSetListChoicesEx(ListType * list, char **itemText, long itemsCount)
{
    Assert(list);

    if (itemsCount > GetMaxListItems())
        itemsCount = GetMaxListItems();

    LstSetListChoices(list, itemText, itemsCount);
}

void LstSetSelectionEx(AppContext* appContext, ListType * list, long itemNo)
{
    Assert(list);
    appContext->listItemOffset = CalcListOffset(appContext->wordsCount, itemNo);
    Assert(appContext->listItemOffset <= itemNo);
    appContext->prevSelectedWord = itemNo;
    LstSetSelection(list, itemNo - appContext->listItemOffset);
}

void LstMakeItemVisibleEx(AppContext* appContext, ListType * list, long itemNo)
{
    Assert(list);
    appContext->listItemOffset = CalcListOffset(itemNo, appContext->wordsCount);
    Assert(appContext->listItemOffset <= itemNo);
    LstMakeItemVisible(list, itemNo - appContext->listItemOffset);
}

void LstSetSelectionMakeVisibleEx(AppContext* appContext, ListType * list, long itemNo)
{
    long    newTopItem;

    Assert(list);

    if (itemNo == appContext->prevSelectedWord)
        return;

    appContext->prevSelectedWord = itemNo;

    if (GetOsVersion(appContext) < 35)
    {
        /* special case when we don't exceed palm's list limit:
           do it the easy way */
        if (appContext->wordsCount < GetMaxListItems())
        {
            appContext->listItemOffset = 0;
            LstSetSelection(list, itemNo);
            return;
        }

        newTopItem = CalcListOffset(appContext->wordsCount, itemNo);
        if ((appContext->listItemOffset == newTopItem) && (newTopItem != (appContext->wordsCount - GetMaxListItems())) && (itemNo > 30))
        {
            appContext->listItemOffset = newTopItem + 30;
        }
        else
        {
            appContext->listItemOffset = newTopItem;
        }
        LstMakeItemVisible(list, itemNo - appContext->listItemOffset);
        LstSetSelection(list, itemNo - appContext->listItemOffset);
        LstEraseList(list);
        LstDrawList(list);
        return;
    }

    // more convoluted version for PalmOS versions that only support
    // 15 bits for number of items
    appContext->listItemOffset = CalcListOffset(appContext->wordsCount, itemNo);
    Assert(appContext->listItemOffset <= itemNo);
    newTopItem = itemNo - appContext->listItemOffset;
    if (appContext->prevTopItem == newTopItem)
    {
        if (appContext->listItemOffset > 14)
        {
            newTopItem += 13;
            appContext->listItemOffset -= 13;
        }
        else
        {
            newTopItem -= 13;
            appContext->listItemOffset += 13;
        }
    }
    if (newTopItem > appContext->prevTopItem)
    {
        LstScrollList(list, winDown, newTopItem - appContext->prevTopItem);
    }
    else
    {
        LstScrollList(list, winUp, appContext->prevTopItem - newTopItem);
    }
    LstSetSelection(list, itemNo - appContext->listItemOffset);
    appContext->prevTopItem = newTopItem;
}

void CtlShowControlEx( FormType *frm, UInt16 objID)
{
    CtlShowControl((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, objID)));

}
void CtlHideControlEx( FormType *frm, UInt16 objID)
{
    CtlHideControl((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, objID)));
}

void ListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    char        *str;
    AppContext* appContext=GetAppContext();
    Int16       stringWidthP;
    Int16       stringLenP;
    Boolean     truncatedP = false;
    long        realItemNo;
    Assert(appContext);
    stringWidthP= appContext->screenWidth; /* max width of the string in the list selection window */
    Assert(itemNum >= 0);
    realItemNo = appContext->listItemOffset + itemNum;
    Assert((realItemNo >= 0) && (realItemNo < appContext->wordsCount));
    if (realItemNo >= appContext->wordsCount)
    {
        return;
    }
    str = dictGetWord(GetCurrentFile(appContext), realItemNo);
    stringLenP = StrLen(str);

    FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
    WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
}

// return a position of the last occurence of character c in string str
// return NULL if no c in str
static char *StrFindLastCharPos(char *str, char c)
{
    char    *lastPos = NULL;
    while( *str )
    {
        if (c==*str)
            lastPos = str;
        ++str;
    }
    return lastPos;
}

// Given a file path in standard form ("/foo/bar/myFile.pdb")
// returns a newly allocated string that presents the file path in a more
// readable way i.e. "myFile.pdb (/foo/bar)".
// Special case: "/myFile.pdb" produces "myFile.pdb" and not "myFile.pdb ()"
// Client must free the memory.
static char *FilePathDisplayFriendly(char *filePath)
{
    char * newPath, *strTmp, *lastSlash;
    int    newPathLen;

    Assert( '/' == filePath[0] );
    newPathLen = StrLen(filePath)  + 3 /* for " ()" */ 
                                   - 1 /* take "/" out */;

    newPath = (char*)new_malloc( newPathLen + 1 ); /* for NULL-termination */
    if ( !newPath )
        return NULL;

    lastSlash = StrFindLastCharPos(filePath, '/');
    strTmp = newPath;

    StrCopy(strTmp,lastSlash+1);
    strTmp += StrLen(lastSlash+1);

    // put path in "()" unless this is "/foo.pdb" scenario
    if ( filePath != lastSlash )
    {
        *strTmp++ = ' ';
        *strTmp++ = '(';
        while( filePath < lastSlash )
            *strTmp++ = *filePath++;
        *strTmp++ = ')';
    }
    *strTmp++ = 0;
    Assert( StrLen(newPath) <= newPathLen);
    return newPath;
}

void ListDbDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    char    *str, *mungedName = NULL;
    Int16   strDx = bounds->extent.x - bounds->topLeft.x;
    Int16   strLen;
    Boolean truncatedP = false;
    AppContext* appContext=GetAppContext();
    Assert(appContext);

    Assert(itemNum >= 0);
    str = GetDatabaseName(appContext, itemNum);
    Assert(NULL != str);
    strLen = StrLen(str);

    /* full path (in case of databases from external memory card) may be too long
    to fit in one line in which case instead of displaying "/path/too/long/file"
    we want to display "file (/path/too/long)". We'll do this munging for
    all files from vfs. */
    if ( '/' == str[0] )
    {
        mungedName = FilePathDisplayFriendly( str );
        if ( !mungedName )
            return;
        str = mungedName;
        strLen = StrLen(mungedName);
    }

    FntCharsInWidth(str, &strDx, &strLen, &truncatedP);
    WinDrawChars(str, strLen, bounds->topLeft.x, bounds->topLeft.y);

    if (mungedName)
        new_free(mungedName);
}

void ssInit( StringStack *ss )
{
    ss->stackedCount = 0;
    ss->freeSlots = 0;
    ss->strings = NULL;
}

void ssDeinit( StringStack *ss )
{
    Assert( 0 == ss->stackedCount );
    if ( ss->strings )
        new_free( ss->strings );
    ssInit( ss );
}

/* Pushes a copy of the string on the stack */
Boolean ssPush( StringStack *ss, char *str )
{
    int     newSlots;
    char ** newStrings;

    // increase the "stack size" if necessary
    if ( 0 == ss->freeSlots )
    {
        // increase of 40 should serve our domain well
        newSlots = ss->stackedCount + 40;
        newStrings = (char**)new_malloc( newSlots * sizeof(char*) );
        if ( NULL == newStrings )
        {
//            appContext->err = ERR_NO_MEM;
            return false;
        }

        // copy old values if needed
        if ( ss->stackedCount > 0 )
        {
            Assert( ss->strings );
            MemMove( newStrings, ss->strings, ss->stackedCount * sizeof(char*) );
            new_free( ss->strings );
        }
        ss->strings = newStrings;
        ss->freeSlots = newSlots - ss->stackedCount;
    }

    ss->strings[ss->stackedCount] = str;
    ss->stackedCount += 1;
    ss->freeSlots -= 1;
    return true;
}

/* Pop a string from the stack. Return NULL if no more strings on the stack.
   Devnote: caller needs to free the memory for the string */
char *ssPop( StringStack *ss )
{
    char   *toReturn;
    int    i;

    if ( 0 == ss->stackedCount )
        return NULL;

    toReturn = ss->strings[0];

    for (i=0; i<ss->stackedCount-1; i++)
    {
        ss->strings[i] = ss->strings[i+1];
    }
    ss->stackedCount -= 1;
    ss->freeSlots += 1;
    return toReturn;
}

#endif // I_NOAH

#ifdef DEBUG
static void LogInitFile(LogInfo* logInfo)
{
    HostFILE        *hf = NULL;

    if (logInfo->fCreated)
        return;

    hf = HostFOpen(logInfo->fileName, "w");
    if (hf)
    {
        HostFClose(hf);
    }
    logInfo->fCreated = true;
}

void LogInit(AppContext* appContext, char *fileName )
{
    appContext->log.fCreated = false;
    appContext->log.fileName = fileName;
}

void Log(AppContext* appContext, char *txt)
{
    HostFILE        *hf = NULL;
    LogInitFile(&appContext->log);
    hf = HostFOpen(appContext->log.fileName, "a");
    if (hf)
    {
        HostFPrintF(hf, "%s\n", txt );
        HostFClose(hf);
    }
}
#endif

#ifndef I_NOAH

void EvtSetInt( EventType *event, int i)
{
    int *pInt = (int*) (&event->data.generic);
    *pInt = i;
}

int EvtGetInt( EventType *event )
{
    int *pInt = (int*) (&event->data.generic);
    return *pInt;
}

void serByte(unsigned char val, char *prefsBlob, long *pCurrBlobSize)
{
    Assert( pCurrBlobSize );
    if ( prefsBlob )
        prefsBlob[*pCurrBlobSize] = val;
    *pCurrBlobSize += 1;
}

void serInt(int val, char *prefsBlob, long *pCurrBlobSize)
{
    int high, low;

    high = val / 256;
    low = val % 256;
    serByte( high, prefsBlob, pCurrBlobSize );
    serByte( low, prefsBlob, pCurrBlobSize );
}

void serLong(long val, char *prefsBlob, long *pCurrBlobSize)
{
    unsigned char * valPtr;

    valPtr = (unsigned char*) &val;
    serByte( valPtr[0], prefsBlob, pCurrBlobSize );
    serByte( valPtr[1], prefsBlob, pCurrBlobSize );
    serByte( valPtr[2], prefsBlob, pCurrBlobSize );
    serByte( valPtr[3], prefsBlob, pCurrBlobSize );
}

unsigned char deserByte(unsigned char **data, long *pBlobSizeLeft)
{
    unsigned char val;
    unsigned char *d = *data;

    Assert( data && *data && pBlobSizeLeft && (*pBlobSizeLeft>=1) );
    val = *d++;
    *data = d;
    *pBlobSizeLeft -= 1;
    return val;
}

static int getInt(unsigned char *data)
{
    int val;
    val = data[0]*256+data[1];
    return val;
}

int deserInt(unsigned char **data, long *pBlobSizeLeft)
{
    int val;
    unsigned char *d = *data;

    Assert( data && *data && pBlobSizeLeft && (*pBlobSizeLeft>=2) );
    val = getInt( d );
    *data = d+2;
    *pBlobSizeLeft -= 2;
    return val;
}

long deserLong(unsigned char **data, long *pBlobSizeLeft)
{
    long val;
    unsigned char * valPtr;
    unsigned char *d = *data;

    valPtr = (unsigned char*) &val;
    valPtr[0] = d[0];
    valPtr[1] = d[1];
    valPtr[2] = d[2];
    valPtr[3] = d[3];
    *data = d+4;
    *pBlobSizeLeft -= 4;
    return val;
}

void serData(char *data, long dataSize, char *prefsBlob, long *pCurrBlobSize)
{
    long i;
    for ( i=0; i<dataSize; i++)
        serByte(data[i],prefsBlob,pCurrBlobSize);
}

void deserData(unsigned char *valOut, int len, unsigned char **data, long *pBlobSizeLeft)
{
    Assert( data && *data && pBlobSizeLeft && (*pBlobSizeLeft>=len) );
    MemMove( valOut, *data, len );
    *data = *data+len;
    *pBlobSizeLeft -= len;
}

void serString(char *str, char *prefsBlob, long *pCurrBlobSize)
{
    int len = StrLen(str)+1;
    serInt(len, prefsBlob, pCurrBlobSize);
    serData(str, len, prefsBlob, pCurrBlobSize);
}

char *deserString(unsigned char **data, long *pCurrBlobSize)
{
    char *  str;
    int     strLen;

    strLen = deserInt( data, pCurrBlobSize );
    Assert( 0 == (*data)[strLen-1] );
    str = (char*)new_malloc( strLen );
    if (NULL==str)
        return NULL;
    deserData( (unsigned char*)str, strLen, data, pCurrBlobSize );
    return str;
}

void deserStringToBuf(char *buf, int bufSize, unsigned char **data, long *pCurrBlobSize)
{
    int     strLen;

    strLen = deserInt( data, pCurrBlobSize );
    Assert( 0 == (*data)[strLen-1] );
    Assert( bufSize >= strLen );
    deserData( (unsigned char*)buf, strLen, data, pCurrBlobSize );
}

static void SetWordAsLastWord(AppContext* appContext, char *txt, int wordLen )
{
    MemSet((void *) &(appContext->lastWord[0]), WORD_MAX_LEN, 0);
    if ( -1 == wordLen )
        wordLen = StrLen( txt );
    if (wordLen >= (WORD_MAX_LEN - 1))
        wordLen = WORD_MAX_LEN - 1;
    MemMove((void *) &(appContext->lastWord[0]), txt, wordLen);
}

void RememberLastWord(AppContext* appContext, FormType * frm)
{
    char *      word = NULL;
    int         wordLen;
    FieldType * fld;

    Assert(frm);

    fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
    word = FldGetTextPtr(fld);
    wordLen = FldGetTextLength(fld);
    SetWordAsLastWord( appContext, word, wordLen );
    FldDelete(fld, 0, wordLen - 1);
}

#pragma segment Segment2

void DoFieldChanged(AppContext* appContext)
{
    char        *word;
    FormPtr     frm;
    FieldPtr    fld;
    ListPtr     list;
    long        newSelectedWord;

    frm = FrmGetActiveForm();
    fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
    list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listMatching));
    word = FldGetTextPtr(fld);
    /* DrawWord( word, 149 ); */
    newSelectedWord = 0;
    if (word && *word)
    {
        newSelectedWord = dictGetFirstMatching(GetCurrentFile(appContext), word);
    }
    if (appContext->selectedWord != newSelectedWord)
    {
        appContext->selectedWord = newSelectedWord;
        Assert(appContext->selectedWord < appContext->wordsCount);
        LstSetSelectionMakeVisibleEx(appContext, list, appContext->selectedWord);
    }
}

void SendFieldChanged(void)
{
    EventType   newEvent;
    MemSet(&newEvent, sizeof(EventType), 0);
    newEvent.eType = (eventsEnum)evtFieldChanged;
    EvtAddEventToQueue(&newEvent);
}

void SendNewDatabaseSelected(int db)
{
    EventType   newEvent;
    MemSet(&newEvent, sizeof(EventType), 0);
    newEvent.eType = (eventsEnum) evtNewDatabaseSelected;
    EvtSetInt( &newEvent, db );
    EvtAddEventToQueue(&newEvent);
}

void SendStopEvent(void)
{
    EventType   newEvent;
    MemSet(&newEvent, sizeof(EventType), 0);
    newEvent.eType = appStopEvent;
    EvtAddEventToQueue(&newEvent);
}

#pragma segment Segment1

char *strdup(char *s)
{
    int     len;
    char *  newStr;

    Assert(s);
    len = StrLen(s);
    newStr = (char*)new_malloc_zero(len+1);
    if (newStr)
        StrCopy(newStr,s);
    return newStr;
}

long FindCurrentDbIndex(AppContext* appContext)
{
    long i;
    for(i=0; i<appContext->dictsCount; i++)
    {
        if (appContext->dicts[i] == GetCurrentFile(appContext) )
            return i;
    }
    return 0;
}

#endif // I_NOAH

void FrmSetObjectBoundsByID(FormType* frm, UInt16 objId, Int16 x, Int16 y, Int16 ex, Int16 ey) {
    UInt16 index;
    RectangleType objBounds;
    Assert(frm);
    index=FrmGetObjectIndex(frm, objId);
    Assert(index!=frmInvalidObjectId);
    FrmGetObjectBounds(frm, index, &objBounds);
    objBounds.topLeft.x=x,
    objBounds.topLeft.y=y;
    objBounds.extent.x=ex;
    objBounds.extent.y=ey;
    FrmSetObjectBounds(frm, index, &objBounds);
}

void SyncScreenSize(AppContext* appContext) 
{
    Coord   dx,dy;

    if ( GetOsVersion(appContext) >= 40 )
    {
        WinGetDisplayExtent(&dx,&dy);
        appContext->screenWidth=dx;
        appContext->screenHeight=dy;
    }
    else
    {
        appContext->screenWidth = 160;
        appContext->screenHeight = 160;
    }
    appContext->dispLinesCount=(appContext->screenHeight-FRM_RSV_H)/FONT_DY;
}

Err DefaultFormInit(AppContext* appContext, FormType* frm)
{
    return DIA_FrmEnableDIA(&appContext->diaSettings, frm, FRM_MIN_H, FRM_PREF_H, FRM_MAX_H, FRM_MIN_W, FRM_PREF_W, FRM_MAX_W);
}

#if !(defined(NOAH_LITE) || defined(I_NOAH))

static Err AppHandleResidentLookup()
{
    UInt16 cardNo=0;
    LocalID localId=0;
    FormType* form=FrmGetActiveForm();
    FieldType* field=NULL;
    Err error=SysCurAppDatabase(&cardNo, &localId);
    Char* buffer=NULL;
    UInt16 length=0;
    UInt16 fieldIndex=noFocus;
    Assert(form);
    fieldIndex=FrmGetFocus(form);
    if (noFocus!=fieldIndex)
        field=(FieldType*)FrmGetObjectPtr(form, fieldIndex);
        
    if (error) 
        goto Exit;
    error=SysNotifyUnregister(cardNo, localId, appNotifyResidentLookupEvent, sysNotifyNormalPriority);
    if (sysNotifyErrEntryNotFound==error) 
        Assert(false);
    if (error) 
        goto Exit;
    if (field) 
    {    
        length=FldGetSelectedText(field, NULL, 0);
        if (length) 
        {
            // This MUST be MemPtrNew as we don't have AppContext yet.
            buffer=(Char*)MemPtrNew((++length)*sizeof(Char));
            if (buffer)
            {
                length=FldGetSelectedText(field, buffer, length);
                Assert(length==StrLen(buffer));
                error=AppPerformResidentLookup(buffer);
                MemPtrFree(buffer);
            }
            else
                error=memErrNotEnoughSpace;        
        }
        else
            goto NoWordSelected;
    }
    else 
        goto NoWordSelected;       
Exit:
    return error;
    
NoWordSelected:
    error=AppPerformResidentLookup(NULL);
    goto Exit;
}

static Err AppHandleMenuCmdBarOpen() 
{
    Err error=errNone;
    UInt16 cardNo;
    LocalID localId;
    error=PrepareSupportDatabase(SUPPORT_DATABASE_NAME, bmpMenuBarIcon);
    if (error)
        goto OnError;
    error=MenuCmdBarAddButton(menuCmdBarOnLeft, bmpMenuBarIcon, menuCmdBarResultNotify, appNotifyResidentLookupEvent, APP_NAME);
    if (error) 
        goto OnError;
    error=SysCurAppDatabase(&cardNo, &localId);
    if (error) 
        goto OnError;
    error=SysNotifyRegister(cardNo, localId, appNotifyResidentLookupEvent, NULL, sysNotifyNormalPriority, NULL);
    if (sysNotifyErrDuplicateEntry==error) 
        error=errNone;
OnError:	
    return error;
}

#endif

Err AppHandleSysNotify(SysNotifyParamType* param) 
{
    Err error=errNone;
    AppContext* appContext=GetAppContext();
    Assert(param);
    switch (param->notifyType) 
    {
        case sysNotifyDisplayChangeEvent:
        case sysNotifyDisplayResizedEvent:
            Assert(appContext);
            SyncScreenSize(appContext);
            DIA_HandleResizeEvent();
            break;
            
#if !(defined(NOAH_LITE) || defined(I_NOAH))
        case sysNotifyMenuCmdBarOpenEvent:
            error=AppHandleMenuCmdBarOpen();
            break;

        case appNotifyResidentLookupEvent:
            error=AppHandleResidentLookup();
            param->handled=true;
            break;			
#endif

         default:
            Assert(false); 
    }
    return error;
}

AppContext* GetAppContext() 
{
    AppContext* res=0;
    Err error=FtrGet(APP_CREATOR, appFtrContext, (UInt32*)&res);
    return res;
}

/* Select all text in a given Field */
void FldSelectAllText(FieldType* field)
{
    UInt16 endPos;
    endPos = FldGetTextLength(field);
    FldSetSelection(field,(UInt16)0,endPos);
    FldGrabFocus(field);
}

#ifndef I_NOAH

UInt16 FldGetSelectedText(FieldType* field, Char* buffer, UInt16 bufferSize) 
{
    UInt16 start=0;
    UInt16 len=0;
    Char* text=NULL;
    Assert(field);
    FldGetSelection(field, &start, &len);
    len-=start;
    if (buffer!=NULL)
    {
        text=FldGetTextPtr(field);
        if (len<bufferSize)
        {
            StrNCopy(buffer, text+start, len);
            buffer[len]=chrNull;
        }
        else 
            StrNCopy(buffer, text+start, bufferSize);
    }
    return len;      
}

#pragma segment Segment2

Err AppNotifyInit(AppContext* appContext)
{
    Err error=errNone;
    UInt32 value=0;
    UInt16 cardNo=0;
    LocalID localId=0;
    Assert(appContext);
    if (!FtrGet(sysFtrCreator, sysFtrNumNotifyMgrVersion, &value) && value) 
    {
        AppSetFlag(appContext, appHasNotifyMgr);
#ifndef NOAH_LITE            
        error=SysCurAppDatabase(&cardNo, &localId);
        if (error) 
            goto OnError;
        error=SysNotifyUnregister(cardNo, localId, sysNotifyMenuCmdBarOpenEvent, sysNotifyNormalPriority);
        if (sysNotifyErrEntryNotFound==error) 
            error=errNone;
        if (error) 
            goto OnError;
        error=SysNotifyUnregister(cardNo, localId, appNotifyResidentLookupEvent, sysNotifyNormalPriority);
        if (sysNotifyErrEntryNotFound==error) 
            error=errNone;
        if (error) 
            goto OnError;
#endif            
    }       
OnError:
    return error;    
}

Err AppNotifyFree(AppContext* appContext, Boolean beResident)
{
    Err error=errNone;
#ifndef NOAH_LITE    
    UInt16 cardNo=0;
    LocalID localId=0;
    Err tmpErr=errNone;
    if (AppHasNotifyMgr(appContext)) 
    {
        error=SysCurAppDatabase(&cardNo, &localId);
        if (error) 
            goto OnError;
        // HACK! - disable resident mode in OS <40 since under emulator with
        // 3.5 OS we hang right after displaying word definition and:
        // - we don't know how to fix it
        // - we don't know if it happens on real devices so we disable it just to be sure
        if (40<=GetOsVersion(appContext))
        {
            error=SysNotifyRegister(cardNo, localId, sysNotifyMenuCmdBarOpenEvent, NULL, sysNotifyNormalPriority, NULL);	
            if (error) 
                goto OnError;
        }                
    }
OnError:
    if (!beResident)
    {
        tmpErr=DisposeSupportDatabase(SUPPORT_DATABASE_NAME);
        if (tmpErr)
            Assert(false);
    }
#endif    
    return error;
}

#pragma segment Segment1

AbstractFile* FindOpenDatabase(AppContext* appContext, const Char* name)
{
    int i;
    AbstractFile* foundFile=NULL;
    Assert(name);
    Assert(appContext);
    for( i=0; i<appContext->dictsCount; i++)
    {
        if (0==StrCompare( name, appContext->dicts[i]->fileName ) )
        {
            foundFile = appContext->dicts[i];
            LogV1("found db=%s", foundFile->fileName );
            break;
        }
    }
    return foundFile;
}

#endif // I_NOAH

/* Open a database given its name, creator and type. Return NULL if
   database not found. */
DmOpenRef OpenDbByNameCreatorType(char *dbName, UInt32 creator, UInt32 type)
{
    Err                 err;
    DmSearchStateType   stateInfo;
    UInt16              cardNo = 0;
    LocalID             dbId;
    char                dbNameBuf[dmDBNameLength];

    Assert(StrLen(dbName) < dmDBNameLength);

    err = DmGetNextDatabaseByTypeCreator(true, &stateInfo, type, creator, 0, &cardNo, &dbId);
    while ( errNone == err )
    {
        MemSet(dbNameBuf, sizeof(dbName), 0);
        DmDatabaseInfo(cardNo, dbId, dbNameBuf, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL);

        if ( 0==StrCompare(dbName,dbNameBuf) )
            return DmOpenDatabase(cardNo, dbId, dmModeReadWrite);
        err = DmGetNextDatabaseByTypeCreator(false, &stateInfo, type, creator, 0, &cardNo, &dbId);
    }
    return NULL;
}

UInt16 PercentProgress(Char* buffer, UInt32 current, UInt32 total)
{
    current*=100;
    current/=total;
    Assert(current<=100);
    if (buffer)
    {
        Int16 result=StrPrintF(buffer, "%ld%%", current);
        Assert(2<=result && 5>=result);
    }
    return current;
}

Boolean StrStartsWith(const Char* begin, const Char* end, const Char* subStr)
{
    while (*subStr && begin<end)
    {
        if (*(subStr++)!=*(begin++))
            return false;
    }
    if (begin==end && chrNull!=*subStr)
        return false;
    else
        return true;        
}


const Char* StrFind(const Char* begin, const Char* end, const Char* subStr)
{
    UInt16 subLen=StrLen(subStr);
    const Char* result=NULL;
    while (end-begin>=subLen)
    {
        if (!StrStartsWith(begin, end, subStr))
            begin++;
        else
        {
            result=begin;
            break;
        }            
    }        
    if (!result) result=end;
    return result;         
}

Err StrAToIEx(const Char* begin, const Char* end, Int32* result, UInt16 base)
{
    Err error=errNone;
    Boolean negative=false;
    Int32 res=0;
    const Char* numbers="0123456789abcdefghijklmnopqrstuvwxyz";
    UInt16 numLen=StrLen(numbers);
    Char buffer[2];
    if (begin>=end || base>numLen)
    {    
        error=memErrInvalidParam;
        goto OnError;           
    }
    if (*begin=='-')
    {
        negative=true;
        if (++begin==end)
        {
            error=memErrInvalidParam;
            goto OnError;           
        }
    }           
    buffer[1]=chrNull;
    while (begin!=end) 
    {
        UInt16 num=0;
        buffer[0]=*(begin++);
        StrToLower(buffer, buffer); 
        num=StrFind(numbers, numbers+numLen, buffer)-numbers;
        if (num>=base)
        {   
            error=memErrInvalidParam;
            break;
        }
        else
        {
            res*=base;
            res+=num;
        }
    }
    if (!error)
       *result=res;
OnError:
    return error;    
}

const Char* StrFindOneOf(const Char* begin, const Char* end, const Char* chars)
{
    Char buffer[2];
    const Char* charsEnd=chars+StrLen(chars);
    buffer[1]=chrNull;
    while (begin<end) 
    {
        buffer[0]=*begin;
        if (StrFind(chars, charsEnd, buffer)!=charsEnd)
            break;
        begin++;            
    }
    return begin;
}

void StrTrimTail(const Char* begin, const Char** end)
{
    while (*end>begin && *end!=StrFindOneOf((*end)-1, *end, " \r\n\t"))
        --(*end);
}

#define uriUnreservedCharacters "-_.!~*'()"

inline static void CharToHexString(Char* buffer, Char chr)
{
    const Char* numbers="0123456789ABCDEF";
    buffer[0]=numbers[chr/16];
    buffer[1]=numbers[chr%16];
}

Err StrUrlEncode(const Char* begin, const Char* end, Char** encoded, UInt16* encodedLength)
{
    Err error=errNone;
    if (encoded && encodedLength)
    {
        ExtensibleBuffer result;
        ebufInit(&result, 0);
        while (begin<end)
        {
            Char chr=*begin;
            if (chrNull==chr || (chr>='a' && chr<='z') || (chr>='A' && chr<='Z') || (chr>='0' && chr<='9') || StrFindOneOf(begin, begin+1, uriUnreservedCharacters)==begin)
                ebufAddChar(&result, chr);
            else
            {
                Char buffer[3];
                *buffer='%';
                CharToHexString(buffer+1, chr);
                ebufAddStrN(&result, buffer, 3);
            }
            begin++;
        }
        *encodedLength=ebufGetDataSize(&result);
        *encoded=ebufGetTxtOwnership(&result);
    }
    else
        error=memErrInvalidParam;
    return error;
}


/* Create new MemoPad record with a given header and body of the memo.
   header can be NULL, if bodySize is -1 => calc it from body via StrLen */
static void CreateNewMemoRec(DmOpenRef dbMemo, char *header, char *body, int bodySize)
{
    UInt16      newRecNo;
    MemHandle   newRecHandle;
    void *      newRecData;
    UInt32      newRecSize;
    Char        null = '\0';

    if (-1 == bodySize)
       bodySize = StrLen(body);

    if ( NULL == header )
         header = "";

    newRecSize = StrLen(header) + bodySize + 1;

    newRecHandle = DmNewRecord(dbMemo,&newRecNo,newRecSize);
    if (!newRecHandle)
        return;

    newRecData = MemHandleLock(newRecHandle);

    DmWrite(newRecData,0,header,StrLen(header));
    DmWrite(newRecData,StrLen(header),body,bodySize);
    DmWrite(newRecData,StrLen(header) + bodySize,&null,1);
    MemHandleUnlock(newRecHandle);

    DmReleaseRecord(dbMemo,newRecNo,true);
}

/* Create a new memo with a given memoBody of size of memoBodySize.
   If memoBodySize is -1 => calculate the size via StrLen(memoBody). */
void CreateNewMemo(char *memoBody, int memoBodySize)
{
    LocalID     id;
    UInt16      cardno;
    UInt32      type,creator;

    DmOpenRef   dbMemo;

    // check all the open database and see if memo is currently open
    dbMemo = DmNextOpenDatabase(NULL);
    while (dbMemo)
    {
        DmOpenDatabaseInfo(dbMemo,&id, NULL,NULL,&cardno,NULL);
        DmDatabaseInfo(cardno,id,
            NULL,NULL,NULL, NULL,NULL,NULL,
            NULL,NULL,NULL, &type,&creator);

        if( ('DATA' == type)  && ('memo' == creator) )
            break;
        dbMemo = DmNextOpenDatabase(dbMemo);
    }

    // we either found memo db, in which case dbTmp points to it, or
    // didn't find it, in which case dbTmp is NULL
    if (NULL == dbMemo)
        dbMemo = DmOpenDatabaseByTypeCreator('DATA','memo',dmModeReadWrite);

    if (NULL != dbMemo)
        CreateNewMemoRec(dbMemo, NULL, memoBody, memoBodySize);

    if (dbMemo)
        DmCloseDatabase(dbMemo);
}

#ifdef DEBUG
void LogErrorToMemo_(const Char* message, Err error)
{
    Assert(message);
    Assert(error);    
    ExtensibleBuffer buffer;
    ebufInitWithStr(&buffer, const_cast<Char*>(message));
    UInt16 messageLength=ebufGetDataSize(&buffer);
    ebufAddStr(&buffer, " (0x0000)"); // Provide space for error code
    ebufAddChar(&buffer, chrNull);
    Char* errorCodeBuffer=ebufGetDataPointer(&buffer)+messageLength; // Should now point at start of " (0x0000)" text
    Int16 length=StrPrintF(errorCodeBuffer, " (0x%hx)", (UInt16)error); // Yep, I know it's already UInt16, but that way we won't get buffer overflow if it ever changes
    Assert(9==length);
    CreateNewMemo(errorCodeBuffer-messageLength, messageLength+9);
    ebufFreeData(&buffer);
}
#endif

Int16 LstGetSelectionByListID(const FormType* form, UInt16 listID)
{
    UInt16 index;
    ListType* list=NULL;
    Assert(form);
    index=FrmGetObjectIndex(form, listID);
    Assert(frmInvalidObjectId!=index);
    list=(ListType*)FrmGetObjectPtr(form, index);
    Assert(list);
    return LstGetSelection(list);
}

