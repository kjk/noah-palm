/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)

  All the common code shared between my other apps.
*/

#include "cw_defs.h"

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

#include <PalmCompatibility.h>
#include "extensible_buffer.h"
#include "fs.h"

#ifdef THESAURUS
#include "roget_support.h"
#endif

#ifdef WNLEX_DICT
#include "wn_lite_ex_support.h"
#endif

#ifdef WN_PRO_DICT
#include "wn_pro_support.h"
#endif

#ifdef SIMPLE_DICT
#include "simple_support.h"
#endif

#ifdef EP_DICT
#include "ep_support.h"
#endif

extern GlobalData gd;

#ifdef DEBUG
LogInfo g_Log;
char    g_logBuf[512];
#endif

/* those functions are not available for Noah Lite */
#ifndef NOAH_LITE

void HistoryListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    char *  str;
    /* max width of the string in the list selection window */
    Int16   stringWidthP = 64;
    Int16   stringLenP;
    Boolean truncatedP = false;

    Assert((itemNum >= 0) && (itemNum <= gd.historyCount));

    str = gd.wordHistory[itemNum];
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

void AddToHistory(UInt32 wordNo)
{
    char *  word;

    word = strdup(dictGetWord(wordNo));
    if (!word)
        return;

    MemMove(&(gd.wordHistory[1]), &(gd.wordHistory[0]), ((HISTORY_ITEMS - 1) * sizeof(gd.wordHistory[0])));
    gd.wordHistory[0] = word;

    if (gd.historyCount < HISTORY_ITEMS)
        ++gd.historyCount;
}

void FreeHistory(void)
{
    int i;
    for (i=0;i<gd.historyCount;i++)
    {
        new_free(gd.wordHistory[i]);
    }
    gd.historyCount = 0;
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

void FreeDicts(void)
{
    // make sure to call DictCurrentFree() before calling here
    Assert( NULL == GetCurrentFile() );
    while(gd.dictsCount>0)
    {
        AbstractFileFree( gd.dicts[--gd.dictsCount] );
    }
}


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

extern void DisplayAbout(void);

void RedrawWordDefinition()
{
    Int16 wordLen = 0;
    char *word = NULL;

    // might happen if there is no word chosen yet, in which case we
    // display about screen
    if (NULL == gd.currDispInfo)
    {
        DisplayAbout();
        return;
    }

    // it can happen that the dictionary has been deleted in the meantime
    // (during rescanning of databases in selecting of new databases)
    // in that case do nothing
    // UNDONE: find a better solution?
    if ( NULL == GetCurrentFile() )
        return;


    /* write the word at the bottom */
    word = dictGetWord(gd.currentWord);

    wordLen = StrLen(word);
    MemSet(gd.prefs.lastWord, 32, 0);
    MemMove(gd.prefs.lastWord, word, wordLen < 31 ? wordLen : 31);

    DrawWord(word, 149);
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
    SetScrollbarState(gd.currDispInfo, DRAW_DI_LINES, gd.firstDispLine);
    DrawDisplayInfo(gd.currDispInfo, gd.firstDispLine, DRAW_DI_X, DRAW_DI_Y, DRAW_DI_LINES);

}

void SendNewWordSelected(void)
{
    EventType   newEvent;
    MemSet(&newEvent, sizeof(EventType), 0);
    newEvent.eType = (eventsEnum) evtNewWordSelected;
    EvtAddEventToQueue(&newEvent);
}

void RedrawMainScreen()
{
    FrmDrawForm(FrmGetActiveForm());
    WinDrawLine(0, 145, 160, 145);
    WinDrawLine(0, 144, 160, 144);
    RedrawWordDefinition();
}
void DrawDescription(long wordNo)
{
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

    RedrawWordDefinition();
}

void ClearDisplayRectangle()
{
    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 160 - DRAW_DI_X - 7,
                    160 - DRAW_DI_Y - 16);
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

void DrawCentered(char *txt)
{
    FontID prev_font;

    ClearRectangle(DRAW_DI_X, DRAW_DI_Y, 152, 144);
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

/*    SetTextColorRed(); */

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
/*            SetTextColorRed(); */
        }
        else if (prev_font != 0)
        {
            FntSetFont((FontID) 0);
            prev_font = (FontID) 0;
/*            SetTextColorRed(); */
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
#ifdef THESAURUS
        case ROGET_TYPE:
            file->dictData.roget = RogetNew();
            if (NULL == file->dictData.roget)
            {
                LogG( "dictNew(): RogetNew() failed" );
                return false;
            }
            break;
#endif
#ifdef WN_PRO_DICT
        case WORDNET_PRO_TYPE:
            file->dictData.wn = (struct _WnInfo*) wn_new();
            if (NULL == file->dictData.wn)
            {
                LogG( "dictNew(): wn_new() failed" );
                return false;
            }
            break;
#endif
#ifdef WNLEX_DICT
        case WORDNET_LITE_TYPE:
            file->dictData.wnLite = (struct _WnLiteInfo*) wnlex_new();
            if (NULL == file->dictData.wnLite)
            {
                LogG( "dictNew(): wnlex_new() failed" );
                return false;
            }
            break;
#endif
#ifdef SIMPLE_DICT
        case SIMPLE_TYPE:
            file->dictData.simple = (struct _SimpleInfo*)simple_new();
            if ( NULL == file->dictData.simple )
            {
                LogG( "dictNew(): simple_new() failed" );
                return false;
            }
            break;
#endif
#ifdef EP_DICT
        case ENGPOL_TYPE:
            file->dictData.engpol = (EngPolInfo*)epNew();
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
void dictDelete(void)
{
    AbstractFile *file = GetCurrentFile();
    if ( NULL == file ) return;
    switch (CurrFileDictType())
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

long dictGetWordsCount(void)
{
    AbstractFile *file = GetCurrentFile();
    switch (CurrFileDictType())
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

long dictGetFirstMatching(char *word)
{
    AbstractFile *file = GetCurrentFile();
    switch (CurrFileDictType())
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

char *dictGetWord(long wordNo)
{
    AbstractFile *file = GetCurrentFile();
    switch (CurrFileDictType())
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

Err dictGetDisplayInfo(long wordNo, int dx, DisplayInfo * di)
{
    AbstractFile *file = GetCurrentFile();
    switch (CurrFileDictType())
    {
#ifdef THESAURUS
        case ROGET_TYPE:
            return RogetGetDisplayInfo( file->dictData.roget, wordNo, dx, di );
#endif
#ifdef WN_PRO_DICT
        case WORDNET_PRO_TYPE:
            return wn_get_display_info( file->dictData.wn, wordNo, dx, di );
#endif
#ifdef WNLEX_DICT
        case WORDNET_LITE_TYPE:
            return wnlex_get_display_info( file->dictData.wnLite, wordNo, dx, di );
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
        gd.helpDipsBuf = NULL;
    }
    if (gd.currDispInfo)
    {
        diFree(gd.currDispInfo);
        gd.currDispInfo = NULL;
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
#if 0
	// doesn't seem to work on OS 35 (e.g. Treo 300).
	// I'm puzzled since I think it did work
    if (GetOsVersion()>35)
        return 20000;
    return 65000;
#endif
	return 20000;
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
    Int16       stringWidthP = 160; /* max width of the string in the list selection window */
    Int16       stringLenP;
    Boolean     truncatedP = false;
    long        realItemNo;

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

// return a position of the last occurence of character c in string str
// return NULL if no c in str
char *StrFindLastCharPos(char *str, char c)
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
char *FilePathDisplayFriendly(char *filePath)
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

    Assert(itemNum >= 0);
    str = GetDatabaseName(itemNum);
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

#ifdef DEBUG
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
#endif

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

int getInt(unsigned char *data)
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

void SetWordAsLastWord( char *txt, int wordLen )
{
    MemSet((void *) &(gd.lastWord[0]), WORD_MAX_LEN, 0);
    if ( -1 == wordLen )
        wordLen = StrLen( txt );
    if (wordLen >= (WORD_MAX_LEN - 1))
        wordLen = WORD_MAX_LEN - 1;
    MemMove((void *) &(gd.lastWord[0]), txt, wordLen);
}

void RememberLastWord(FormType * frm)
{
    char *      word = NULL;
    int         wordLen;
    FieldType * fld;

    Assert(frm);

    fld = (FieldType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldWord));
    word = FldGetTextPtr(fld);
    wordLen = FldGetTextLength(fld);
    SetWordAsLastWord( word, wordLen );
    FldDelete(fld, 0, wordLen - 1);
}

void DoFieldChanged(void)
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
        newSelectedWord = dictGetFirstMatching(word);
    }
    if (gd.selectedWord != newSelectedWord)
    {
        gd.selectedWord = newSelectedWord;
        Assert(gd.selectedWord < gd.wordsCount);
        LstSetSelectionMakeVisibleEx(list, gd.selectedWord);
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

long FindCurrentDbIndex(void)
{
    long i;
    for(i=0; i<gd.dictsCount; i++)
    {
        if (gd.dicts[i] == GetCurrentFile() )
            return i;
    }
    return 0;
}
