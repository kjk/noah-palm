/*
  Copyright (C) 2000, 2001, 2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)

  All the common code shared between my other apps.
 */

#ifdef NOAH_PRO
#include "noah_pro.h"
#include "noah_pro_rcp.h"
#endif

#ifdef NOAH_LITE
#include "noah_lite.h"
#include "noah_lite_rcp.h"
#endif

#ifdef THESAURUS
#include "thes.h"
#include "thes_rcp.h"
#endif

extern GlobalData gd;

#include <PalmCompatibility.h>
#include "common.h"
#include "extensible_buffer.h"
#include "fs.h"

#include "roget_support.h"

#ifdef DEBUG
LogInfo g_Log;
char    g_logBuf[512];
#endif

/* return a copy of the string. Caller needs to free the memory */
char *strdup( char *str )
{
    int     strLen;
    char    *newStr;

    Assert(str);
    strLen = strlen( str );
    newStr = new_malloc_zero( strLen+1 );
    if (newStr)
        MemMove( newStr, str, strLen );
    return newStr;
}


/* those functions are not available for Noah Lite */
#ifndef NOAH_LITE

void HistoryListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
#if 0
    char *str;
    /* max width of the string in the list selection window */
    Int16 stringWidthP = 64;
    Int16 stringLenP;
    Boolean truncatedP = false;

    Assert((itemNum >= 0) && (itemNum <= gd.dbPrefs.historyCount));

    str = dictGetWord(gd.dbPrefs.wordHistory[itemNum]);
    stringLenP = StrLen(str);

    FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
    WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
#endif
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

void AddToHistory(UInt32 wordNo)
{
#if 0
    MemMove(&(gd.dbPrefs.wordHistory[1]), &(gd.dbPrefs.wordHistory[0]), ((HISTORY_ITEMS - 1) * sizeof(gd.dbPrefs.wordHistory[0])));
    gd.dbPrefs.wordHistory[0] = wordNo;

    if (gd.dbPrefs.historyCount < HISTORY_ITEMS)
    {
        ++gd.dbPrefs.historyCount;
    }
#endif
}

void SetPopupLabel(FormType * frm, UInt16 listID, UInt16 popupID, Int16 txtIdx, char *txtBuf)
{
    ListType *list = NULL;
    char *listTxt;

    Assert(frm);
    Assert(txtBuf);

    list = (ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listID));
    LstSetSelection(list, txtIdx);
    listTxt = LstGetSelectionText(list, txtIdx);
    MemMove(txtBuf, listTxt, StrLen(listTxt) + 1);
    CtlSetLabel((ControlType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, popupID)), txtBuf);
}
#endif

/*
Detect an Palm OS version and return it in a way easy for later
examination (20 for 2.0, 35 for 3.5 etc.)
*/
int GetOsVersion(void)
{
    UInt32      osVersionPart;
    static int  osVersion=0;

    if ( osVersion==0 )
    {
        FtrGet( sysFtrCreator, sysFtrNumROMVersion, &osVersionPart );
        osVersion = sysGetROMVerMajor(osVersionPart)*10;
        if ( sysGetROMVerMinor(osVersionPart) < 10 )
            osVersion += sysGetROMVerMinor(osVersionPart);
    }
    return osVersion;
}

/*
Find out and return the max screen depth supported by the given os.
Number of supported colors is 2^screen_depth
OS less than 30 only supports 1 (2 colors).
*/
int GetMaxScreenDepth()
{
    UInt32    supportedDepths;
    UInt32    i;
    int       maxDepth;

    maxDepth = 1;
    if ( GetOsVersion() >= 35 )
    {
        WinScreenMode( winScreenModeGetSupportedDepths, NULL, NULL, &supportedDepths, NULL );
        for ( i = 1; supportedDepths; i++ )
        {
            if ( ( supportedDepths & 0x01 ) == 0x01 )
                maxDepth = i;
            supportedDepths >>= 1;
        }
    }
    return maxDepth;
}

/*
Find out and return current depth of the screen.
*/
int GetCurrentScreenDepth()
{
    UInt32    depth;
    int       osVersion = GetOsVersion();

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
Boolean IsColorSupported()
{
    Boolean     fSupported;

    if ( GetOsVersion() >= 35 )
    {
        WinScreenMode( winScreenModeGet, NULL, NULL, NULL, &fSupported);
        return fSupported;
    }
    return false;
}

static RGBColorType rgb_color;

static void SetTextColor(RGBColorType *color)
{
    if ( GetOsVersion() >= 40 )
    {
        WinSetTextColorRGB (color, NULL);
    }
}

void SetTextColorBlack(void)
{
    rgb_color.index = 0;
    rgb_color.r = 0;
    rgb_color.g = 0;
    rgb_color.b = 0;
    SetTextColor( &rgb_color );
}
void SetTextColorRed(void)
{
    rgb_color.r = 255;
    rgb_color.g = 0;
    rgb_color.b = 0;
    SetTextColor( &rgb_color );

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

    if (maxLines > diGetLinesCount(di))
    {
        HideScrollbar();
        return;
    }

    if (maxLines + firstLine > diGetLinesCount(di))
    {
        /* ??? */
        return;
    }

    min = 0;
    page_size = maxLines;
    max = diGetLinesCount(di) - page_size;
    value = firstLine;

    frm = FrmGetActiveForm();
    SclSetScrollBar((ScrollBarType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, scrollDef)), value, min, max, page_size);
}

void DisplayHelp(void)
{
    char *rawTxt;

    if (NULL == gd.currDispInfo)
    {
        gd.currDispInfo = diNew();
        if (NULL == gd.currDispInfo)
        {
            /* TODO: we should rather exit, since this means totally
               out of ram */
            return;
        }
    }
    rawTxt = ebufGetDataPointer(gd.helpDipsBuf);
    diSetRawTxt(gd.currDispInfo, rawTxt);

    gd.firstDispLine = 0;
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
    DrawDisplayInfo(gd.currDispInfo, 0, DRAW_DI_X, DRAW_DI_Y, DRAW_DI_LINES);
    SetScrollbarState(gd.currDispInfo, DRAW_DI_LINES, gd.firstDispLine);
}


void DrawDescription(UInt32 wordNo)
{
    char *word = NULL;
#ifndef NOAH_LITE
    Int16 wordLen = 0;
#endif
    Err err;

    Assert(wordNo < gd.wordsCount);

    /* if currDispInfo has not been yet allocated, allocate it now.
       It'll be reused */
    if (NULL == gd.currDispInfo)
    {
        gd.currDispInfo = diNew();
        if (NULL == gd.currDispInfo)
        {
            /* TODO: we should rather exit, since this means totally
               out of ram */
            return;
        }
    }

    DrawCentered(SEARCH_TXT);

    err = dictGetDisplayInfo(wordNo, 120, gd.currDispInfo);
    if (err != 0)
    {
        return;
    }

    gd.currentWord = wordNo;
    gd.firstDispLine = 0;

    /* write the word at the bottom */
    word = dictGetWord(wordNo);

#ifndef NOAH_LITE
    wordLen = StrLen(word);
    MemSet(gd.prefs.lastWord, 32, 0);
    MemMove(gd.prefs.lastWord, word, wordLen < 31 ? wordLen : 31);
#endif

    DrawWord(word, 149);
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
    SetScrollbarState(gd.currDispInfo, DRAW_DI_LINES, gd.firstDispLine);
    DrawDisplayInfo(gd.currDispInfo, gd.firstDispLine, DRAW_DI_X, DRAW_DI_Y, DRAW_DI_LINES);
}

#ifdef DEBUG
void MyPause(long mult)
{
    long i;
    long j;
    long res;

    for (i = 0; i < 5 * mult; i++)
    {
        for (j = 0; j < 500000; j++)
        {
            res = 3 * i + j;
        }
    }
}

void DrawDebugScrollArea(void)
{
    RectangleType srcRect;

    srcRect.topLeft.x = 0;
    srcRect.topLeft.y = 0;
    srcRect.extent.x = 159;
    srcRect.extent.y = 159 - 9;
    WinCopyRectangle(NULL, NULL, &srcRect, 0, 9, winPaint);
    ClearRectangle(0, 0, 160, 9);
}

void DrawDebugAt(char *txt, Int16 dx)
{
    Int16 len;
    len = StrLen(txt);
    WinDrawChars(txt, len, dx, 0);
}

void DrawDebugNum(UInt32 num)
{
    char buf[40];
    StrIToA(buf, num);
    DrawDebug(buf);
}

void DrawDebug2Num(char *txt, UInt32 num)
{
    char buf[40];
    StrIToA(buf, num);
    DrawDebug2(txt, buf);
}

void DrawDebug(char *txt)
{
    DrawDebugScrollArea();
    DrawDebugAt(txt, 0);
}

void DrawDebug2(char *txt1, char *txt2)
{
    Int16 len;
    Int16 strDx;

    DrawDebugScrollArea();

    DrawDebugAt(txt1, 0);
    len = StrLen(txt1);
    strDx = FntCharsWidth(txt1, len);
    DrawDebugAt(txt2, strDx + 8);
}

#endif

void ClearRectangle(Int16 sx, Int16 sy, Int16 ex, Int16 ey)
{
    RectangleType r;
    r.topLeft.x = sx;
    r.topLeft.y = sy;
    r.extent.x = ex;
    r.extent.y = ey;
    WinEraseRectangle(&r, 0);
}

void DrawCentered(char *txt)
{
    FontID prev_font;

    ClearRectangle(0, 0, 152, 144);
    prev_font = FntGetFont();
    FntSetFont((FontID) 1);
    WinDrawChars(txt, StrLen(txt), 46, (160 - 20) / 2);
    FntSetFont(prev_font);
}

void DrawCenteredString(const char *str, int dy)
{
    Int16 strDx;
    int strLen;

    Assert(str);
    strLen = StrLen(str);
    strDx = FntCharsWidth(str, strLen);
    WinDrawChars(str, strLen, 80 - strDx / 2, dy);
}

#define WAIT_CACHE_TXT "Caching rec:  "

void DrawCacheRec(int recNum)
{
    char buf[30];
    StrCopy(buf, WAIT_CACHE_TXT);
    StrIToA(buf + sizeof(WAIT_CACHE_TXT) - 2, recNum);
    DrawWord(buf, 149 );
    /*DrawCentered(buf);*/
}

static int curr_y;
static FontID curr_font;
static FontID original_font;

void dh_set_current_y(int y)
{
    curr_y = y;
}

void dh_save_font(void)
{
    original_font = FntGetFont();
    curr_font = original_font;
}

void dh_restore_font(void)
{
    FntSetFont(original_font);
}

void dh_display_string(const char *str, int font, int dy)
{
    if (curr_font != (FontID) font)
    {
        curr_font = (FontID) font;
        FntSetFont(curr_font);
    }

    DrawCenteredString(str, curr_y);
    curr_y += dy;
}

/* display maxLines from DisplayInfo, starting with first line,
starting at x,y  coordinates */
void DrawDisplayInfo(DisplayInfo * di, int firstLine, Int16 x, Int16 y,
                int maxLines)
{
    int totalLines;
    int i;
    char *line;
    Int16 curY;
    Int16 fontDY;
    FontID prev_font;

    Assert(firstLine >= 0);
    Assert(maxLines > 0);

    SetTextColorRed();

    fontDY = FntCharHeight();

    curY = y;
    prev_font = FntGetFont();
    totalLines = diGetLinesCount(di) - firstLine;
    if (totalLines > maxLines)
    {
        totalLines = maxLines;
    }
    for (i = 0; i < totalLines; i++)
    {
        line = diGetLine(di, i + firstLine);
        if (diLineBoldP(di, i + firstLine) && (prev_font != 1))
        {
            FntSetFont((FontID) 1);
            prev_font = (FontID) 1;
            SetTextColorRed();
        }
        else if (prev_font != 0)
        {
            FntSetFont((FontID) 0);
            prev_font = (FontID) 0;
            SetTextColorRed();
        }
        WinDrawChars(line, StrLen(line), x, curY);
        curY += fontDY;
    }
    FntSetFont(prev_font);
}

/* max width of the word displayed at the bottom */
#define MAX_WORD_DX        100

void DrawWord(char *word, int pos_y)
{
    Int16 wordLen = 0;
    Int16 word_dx = MAX_WORD_DX;
    Boolean truncatedP = false;
    FontID prev_font;

    Assert(NULL != word);

    wordLen = StrLen(word);
    ClearRectangle(21, pos_y - 1, MAX_WORD_DX, 11);
    prev_font = FntGetFont();
    FntSetFont((FontID) 1);
    FntCharsInWidth(word, &word_dx, &wordLen, &truncatedP);
    WinDrawChars(word, wordLen, 22, pos_y - 2);
    FntSetFont(prev_font);
}

/*
  Given entry number find out entry's definition (record,
  offset within the record and the definitions' length)
 */
Boolean get_defs_record(long entry_no, int first_record_with_defs_len, int defs_len_rec_count, int first_record_with_defs,
                int *record_out, long *offset_out, long *len_out)
{
    SynsetDef synset_def = { 0 };

    Assert(entry_no >= 0);
    Assert(first_record_with_defs_len >= 0);
    Assert(defs_len_rec_count > 0);
    Assert(first_record_with_defs >= 0);
    Assert(record_out);
    Assert(offset_out);
    Assert(len_out);

    synset_def.synsetNo = entry_no;
    if ( !get_defs_records(1, first_record_with_defs_len, defs_len_rec_count, first_record_with_defs, &synset_def) )
    {
        return false;
    }

    *record_out = synset_def.record;
    *offset_out = synset_def.offset;
    *len_out = synset_def.dataSize;
    return true;
}

Boolean get_defs_records(long entry_count, int first_record_with_defs_len,  int defs_len_rec_count, int first_record_with_defs,
                 SynsetDef * synsets)
{
    long offset = 0;
    long curr_len;
    long idx_in_rec;
    int curr_record = 0;
    long curr_rec_size = 0;
    unsigned char *def_lens = NULL;
    long current_entry;
    int entry_index;
    Boolean loop_end_p;

    Assert(synsets);
    Assert(entry_count >= 0);
    Assert(first_record_with_defs_len >= 0);
    Assert(defs_len_rec_count > 0);
    Assert(first_record_with_defs >= 0);

    curr_record = first_record_with_defs_len;
    def_lens = (unsigned char *) CurrFileLockRecord(curr_record);
    if (NULL == def_lens)
    {
        return false;
    }
    curr_rec_size = CurrFileGetRecordSize(curr_record);

    /* calculate the length and offset of all entries */
    idx_in_rec = 0;
    current_entry = 0;
    entry_index = 0;
    do
    {
        curr_len = (UInt32) def_lens[idx_in_rec++];
        if (255 == curr_len)
        {
            curr_len = (UInt32) ((UInt32) def_lens[idx_in_rec] * 256 + def_lens[idx_in_rec + 1]);
            idx_in_rec += 2;
        }

        if (current_entry == synsets[entry_index].synsetNo)
        {
            synsets[entry_index].offset = offset;
            synsets[entry_index].dataSize = curr_len;
            ++entry_index;
            if (entry_index == entry_count)
            {
                break;
            }
        }

        offset += curr_len;
        Assert( idx_in_rec <= curr_rec_size );
        if (idx_in_rec == curr_rec_size)
        {
            CurrFileUnlockRecord(curr_record);
            ++curr_record;
            Assert(curr_record < first_record_with_defs_len + defs_len_rec_count);
            def_lens = (unsigned char *) CurrFileLockRecord(curr_record);
            curr_rec_size = CurrFileGetRecordSize(curr_record);
            idx_in_rec = 0;
        }
        ++current_entry;
    }
    while (true);

    CurrFileUnlockRecord(curr_record);

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
                if (synsets[entry_index].offset >= CurrFileGetRecordSize(curr_record))
                {
                    synsets[entry_index].offset -= CurrFileGetRecordSize(curr_record);
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

char * GetWnPosTxt(int pos)
{
    return GetNthTxt(pos, "(noun) \0(verb) \0(adj.) \0(adv.) \0");
}

#ifdef STRESS
void stress(long step)
{
    long wordNo;
    long wordsCount;

    wordsCount = dictGetWordsCount();
    for (wordNo = 0; wordNo < wordsCount; wordNo += step)
    {
        DrawDescription(wordNo);
    }
}
#endif

/* return the dictionary type for the current file */
UInt32 CurrFileDictType(void)
{
    AbstractFile *file = GetCurrentFile();
    return file->type;
}

Boolean dictNew(void)
{
    AbstractFile *file;

    file = GetCurrentFile();

    switch (CurrFileDictType())
    {
        case ROGET_TYPE:
            file->dictData.roget = RogetNew();
            if (NULL == file->dictData.roget)
            {
                LogG( "dictNew(): RogetNew() failed" );
                return false;
            }
            break;
#if 0
        case WORDNET_PRO_TYPE:
            file->dictData.wn = wn_new();
            break;
#endif
        default:
            LogG( "dictNew(): assert" );
            Assert(0);
            break;
    }
    LogG( "dictNew() ok" );
    return true;
}

/* free all resources allocated by dictionary */
void dictDelete(void)
{
    AbstractFile *file = GetCurrentFile();
    if ( NULL == file ) return;
    switch (CurrFileDictType())
    {
        case ROGET_TYPE:
            RogetDelete( file->dictData.roget );
            file->dictData.roget = NULL;
            break;
        case WORDNET_PRO_TYPE:
#if 0
            wn_delete( file->dictData.wn );
            file->dictData.wn = NULL;
#endif
            break;
        default:
            Assert(0);
            break;
    }
}

long dictGetWordsCount(void)
{
    AbstractFile *file = GetCurrentFile();
    switch (CurrFileDictType())
    {
        case ROGET_TYPE:
            return RogetGetWordsCount( file->dictData.roget );
        case WORDNET_PRO_TYPE:
            Assert(0);
            return 0;
        default:
            Assert(0);
            return 0;
    }
}

long dictGetFirstMatching(char *word)
{
    AbstractFile *file = GetCurrentFile();
    switch (CurrFileDictType())
    {
        case ROGET_TYPE:
            return RogetGetFirstMatching( file->dictData.roget, word );
        case WORDNET_PRO_TYPE:
            Assert(0);
            return 0;
        default:
            Assert(0);
            return 0;
    }
}

char *dictGetWord(long wordNo)
{
    AbstractFile *file = GetCurrentFile();
    switch (CurrFileDictType())
    {
        case ROGET_TYPE:
            return RogetGetWord( file->dictData.roget, wordNo );
        case WORDNET_PRO_TYPE:
            Assert(0);
            return 0;
        default:
            Assert(0);
            return NULL;
    }
}

Err dictGetDisplayInfo(long wordNo, int dx, DisplayInfo * di)
{
    AbstractFile *file = GetCurrentFile();
    switch (CurrFileDictType())
    {
        case ROGET_TYPE:
            return RogetGetDisplayInfo( file->dictData.roget, wordNo, dx, di );
        case WORDNET_PRO_TYPE:
            Assert(0);
            return 0;
        default:
            Assert(0);
            return 0;
    }
}

extern char helpText[];

Boolean CreateInfoData(void)
{
    gd.helpDipsBuf = ebufNew();
    ebufAddStr(gd.helpDipsBuf, helpText);
    ebufWrapBigLines(gd.helpDipsBuf);
    return true;
}

void FreeInfoData(void)
{
    if (gd.helpDipsBuf)
    {
        ebufDelete(gd.helpDipsBuf);
    }
    if (gd.currDispInfo)
    {
        diFree(gd.currDispInfo);
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
   nly Int so list can only handle 2^15 items) */
long GetMaxListItems()
{
    if (GetOsVersion()>35)
        return 20000;
    return 65000;
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

void DefScrollUp(ScrollType scroll_type)
{
    DisplayInfo *di = NULL;
    int to_scroll = 0;

    di = gd.currDispInfo;
    if ((scrollNone == scroll_type) || (NULL == di) || (0 == gd.firstDispLine))
        return;

    switch (scroll_type)
    {
    case scrollLine:
        to_scroll = 1;
        break;
    case scrollHalfPage:
        to_scroll = DRAW_DI_LINES / 2;
        break;
    case scrollPage:
        to_scroll = DRAW_DI_LINES;
        break;
    default:
        Assert(0);
        break;
    }

    if (gd.firstDispLine >= to_scroll)
        gd.firstDispLine -= to_scroll;
    else
        gd.firstDispLine = 0;

    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
    DrawDisplayInfo(di, gd.firstDispLine, DRAW_DI_X, DRAW_DI_Y, DRAW_DI_LINES);
    SetScrollbarState(di, DRAW_DI_LINES, gd.firstDispLine);
}

void DefScrollDown(ScrollType scroll_type)
{
    DisplayInfo *di;
    int invisible_lines = 0;
    int to_scroll = 0;

    di = gd.currDispInfo;
    invisible_lines = diGetLinesCount(di) - gd.firstDispLine - DRAW_DI_LINES;

    if ((scrollNone == scroll_type) || (NULL == di) || (invisible_lines <= 0))
        return;

    switch (scroll_type)
    {
    case scrollLine:
        to_scroll = 1;
        break;
    case scrollHalfPage:
        to_scroll = DRAW_DI_LINES / 2;
        break;
    case scrollPage:
        to_scroll = DRAW_DI_LINES;
        break;
    default:
        Assert(0);
        break;
    }

    if (invisible_lines >= to_scroll)
        gd.firstDispLine += to_scroll;
    else
        gd.firstDispLine += invisible_lines;
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
    DrawDisplayInfo(di, gd.firstDispLine, DRAW_DI_X, DRAW_DI_Y, DRAW_DI_LINES);
    SetScrollbarState(di, DRAW_DI_LINES, gd.firstDispLine);
}

char *GetDatabaseName(int dictNo)
{
    Assert((dictNo >= 0) && (dictNo < gd.dictsCount));
    return gd.dicts[dictNo]->fileName;
}

void LstSetListChoicesEx(ListType * list, char **itemText, long itemsCount)
{
    Assert(list);

    if (itemsCount > GetMaxListItems())
        itemsCount = GetMaxListItems();

    LstSetListChoices(list, itemText, itemsCount);
}

void LstSetSelectionEx(ListType * list, long itemNo)
{
    Assert(list);
    gd.listItemOffset = CalcListOffset(gd.wordsCount, itemNo);
    Assert(gd.listItemOffset <= itemNo);
    gd.prevSelectedWord = itemNo;
    LstSetSelection(list, itemNo - gd.listItemOffset);
}

void LstMakeItemVisibleEx(ListType * list, long itemNo)
{
    Assert(list);
    gd.listItemOffset = CalcListOffset(itemNo, gd.wordsCount);
    Assert(gd.listItemOffset <= itemNo);
    LstMakeItemVisible(list, itemNo - gd.listItemOffset);
}

void LstSetSelectionMakeVisibleEx(ListType * list, long itemNo)
{
    long    newTopItem;

    Assert(list);

    if (itemNo == gd.prevSelectedWord)
        return;

    gd.prevSelectedWord = itemNo;

    if (GetOsVersion() < 35)
    {
        /* special case when we don't exceed palm's list limit:
           do it the easy way */
        if (gd.wordsCount < GetMaxListItems())
        {
            gd.listItemOffset = 0;
            LstSetSelection(list, itemNo);
            return;
        }

        newTopItem = CalcListOffset(gd.wordsCount, itemNo);
        if ((gd.listItemOffset == newTopItem) && (newTopItem != (gd.wordsCount - GetMaxListItems())) && (itemNo > 30))
        {
            gd.listItemOffset = newTopItem + 30;
        }
        else
        {
            gd.listItemOffset = newTopItem;
        }
        LstMakeItemVisible(list, itemNo - gd.listItemOffset);
        LstSetSelection(list, itemNo - gd.listItemOffset);
        //CtlShowControlEx( FrmGetActiveForm(), listMatching );
        LstEraseList(list);
        LstDrawList(list);
        return;
    }

    // more convoluted version for PalmOS versions that only support
    // 15 bits for number of items
    gd.listItemOffset = CalcListOffset(gd.wordsCount, itemNo);
    Assert(gd.listItemOffset <= itemNo);
    newTopItem = itemNo - gd.listItemOffset;
    if (gd.prevTopItem == newTopItem)
    {
        if (gd.listItemOffset > 14)
        {
            newTopItem += 13;
            gd.listItemOffset -= 13;
        }
        else
        {
            newTopItem -= 13;
            gd.listItemOffset += 13;
        }
    }
    if (newTopItem > gd.prevTopItem)
    {
        LstScrollList(list, winDown, newTopItem - gd.prevTopItem);
    }
    else
    {
        LstScrollList(list, winUp, gd.prevTopItem - newTopItem);
    }
    LstSetSelection(list, itemNo - gd.listItemOffset);
    gd.prevTopItem = newTopItem;
    //CtlShowControlEx( FrmGetActiveForm(), listMatching );
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
    Int16       stringWidthP = 80; /* max width of the string in the list selection window */
    Int16       stringLenP;
    Boolean     truncatedP = false;
    long        realItemNo;

    if ( gd.listDisabledP )
        return;
    Assert(itemNum >= 0);
    realItemNo = gd.listItemOffset + itemNum;
    Assert((realItemNo >= 0) && (realItemNo < gd.wordsCount));
    if (realItemNo >= gd.wordsCount)
    {
        return;
    }
    str = dictGetWord(realItemNo);
    stringLenP = StrLen(str);

    FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
    WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
}

void ListDbDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    char    *str;
    Int16   strDx = 120;
    Int16   strLen;
    Boolean truncatedP = false;

    Assert(itemNum >= 0);
    str = GetDatabaseName(itemNum);
    strLen = StrLen(str);

    FntCharsInWidth(str, &strDx, &strLen, &truncatedP);
    WinDrawChars(str, strLen, bounds->topLeft.x, bounds->topLeft.y);
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
    char    **newStrings;

    // increase the "stack size" if necessary
    if ( 0 == ss->freeSlots )
    {
        // increase of 40 should serve our domain well
        newSlots = ss->stackedCount + 40;
        newStrings = new_malloc( newSlots * sizeof(char*) );
        if ( NULL == newStrings )
        {
            gd.err = ERR_NO_MEM;
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

void LogInitFile(LogInfo *logInfo)
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

void LogInit( LogInfo *logInfo, char *fileName )
{
    logInfo->fCreated = false;
    logInfo->fileName = fileName;
}

void Log(LogInfo *logInfo, char *txt)
{
    HostFILE        *hf = NULL;

    LogInitFile(logInfo);
    hf = HostFOpen(logInfo->fileName, "a");
    if (hf)
    {
        HostFPrintF(hf, "%s\n", txt );
        HostFClose(hf);
    }
}

