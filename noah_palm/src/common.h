/*
  Copyright (C) 2000,2001, 2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <PalmOS.h>
#include "mem_leak.h"
#include "display_info.h"
#include "extensible_buffer.h"
#include "fs.h"

#ifdef DEBUG
#define     Assert(c)         ErrFatalDisplayIf(!(c),#c)
#else
#define    Assert(c)          {}
#endif

#define strlen StrLen

#define  SIMPLE_TYPE      'simp'

#define  WORDNET_LITE_TYPE      'wnet'

#ifndef DEMO
#define  WORDNET_PRO_TYPE      'wn20'
#else
#define  WORDNET_PRO_TYPE      'wnde'
#endif


#define  DRAW_DI_X 0

#define SEARCH_TXT   "Searching..."

char *strdup( char *str );

typedef enum
{
    ERR_NONE = 0,
    ERR_NO_MEM,
    ERR_NO_PREF_DB_CREATE,
    ERR_NO_PREF_DB_OPEN,
    ERR_TOO_MANY_DBS,
    ERR_DICT_INIT,
    ERR_SILENT,
    ERR_GENERIC,
} NoahErrors;

/* What type of scrolling should tapping/up and down hw buttons do:
   - none
   - scroll by line
   - scroll by page
*/
typedef enum
{
    scrollNone = 0,
    scrollLine,
    scrollHalfPage,
    scrollPage
} ScrollType;

/* What should we do at startup:
   - nothing (display About and info screens)
   - display the definition of the word in a clipboard (if any)
   - display the definition of the last word we were seeing (if any)
*/
typedef enum
{
    startupActionNone = 0,
    startupActionClipboard,
    startupActionLast
} StartupAction;

/* What we should do at startup when there are more than one database:
   - ask users to select a database
   - go to the last used database
*/
typedef enum
{
    dbStartupActionAsk = 0,
    dbStartupActionLast,
} DatabaseStartupAction;

typedef struct
{
    long    synsetNo;
    int     record;
    long    offset;
    long    dataSize;
} SynsetDef;


#define MAX_DICTS 10
long GetMaxListItems();

Boolean get_defs_record(long entry_no, int first_record_with_defs_len,
                     int defs_len_rec_count,
                     int first_record_with_defs, int *record_out,
                     long *offset_out, long *len_out);
Boolean get_defs_records(long entry_count,
                      int first_record_with_defs_len,
                      int defs_len_rec_count,
                      int first_record_with_defs, SynsetDef * synsets);

void    dh_save_font(void);
void    dh_restore_font(void);
void    dh_set_current_y(int y);
void    dh_display_string(const char *str, int font, int dy);

void    ClearRectangle(Int16 sx, Int16 sy, Int16 ex, Int16 ey);
void    DrawCenteredString(const char *str, int dy);
void    DrawDisplayInfo(DisplayInfo * di, int firstLine, Int16 x,
                     Int16 y, int maxLines);
void    DrawWord(char *word, int pos_y);
char    *GetNthTxt(int n, char *txt);
char    *GetWnPosTxt(int partOfSpeech);

void    DrawCentered(char *txt);
void    DrawCacheRec(int recNum);

#ifdef DEBUG
void stress(long step);
#endif

/* those functions are not available for Noah Lite */
#ifndef NOAH_LITE
void HistoryListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
void strtolower(char *txt);
void AddToHistory(UInt32 wordNo);
void SetPopupLabel(FormType * frm, UInt16 listID, UInt16 popupID, Int16 txtIdx, char *txtBuf);
#endif

Boolean dictNew(void);
void    dictDelete(void);
long    dictGetWordsCount(void);
long    dictGetFirstMatching(char *word);
char    *dictGetWord(long wordNo);
Err     dictGetDisplayInfo(long wordNo, int dx, DisplayInfo * di);
void    FreeDicts(void);

void    DrawDescription(UInt32 wordNo);
void    DisplayHelp(void);
void    HideScrollbar(void);
void    SetScrollbarState(DisplayInfo * di, int maxLines, int firstLine);

int     GetOsVersion(void);
int     GetMaxScreenDepth();
Boolean IsColorSupported();
int     GetCurrentScreenDepth();
void    SetTextColorBlack(void);
void    SetTextColorRed(void);

Boolean CreateInfoData(void);
void    FreeInfoData(void);
long    CalcListOffset(long itemsCount, long itemNo);
void    RemoveWhiteSpaces( char *src );
void    DefScrollUp(ScrollType scroll_type);
void    DefScrollDown(ScrollType scroll_type);
char    *GetDatabaseName(int dictNo);
void    LstSetListChoicesEx(ListType * list, char **itemText, long itemsCount);
void    LstSetSelectionEx(ListType * list, long itemNo);
void    LstMakeItemVisibleEx(ListType * list, long itemNo);
void    LstSetSelectionMakeVisibleEx(ListType * list, long itemNo);
void    CtlHideControlEx( FormType *frm, UInt16 objID);
void    CtlShowControlEx( FormType *frm, UInt16 objID);
void    ListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
void    ListDbDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);

/* stores all the information about the stack of strings */
typedef struct
{
    int     stackedCount;
    char    **strings;
    int     freeSlots;
} StringStack;


void    ssInit  ( StringStack *ss );
void    ssDeinit( StringStack *ss );
Boolean ssPush  ( StringStack *ss, char *str );
char    *ssPop  ( StringStack *ss );

#ifdef DEBUG

typedef struct
{
    Boolean fCreated;
    char    *fileName;
} LogInfo;

extern LogInfo g_Log;
extern char    g_logBuf[512];

void LogInit(LogInfo *logInfo, char *fileName);
void Log(LogInfo *logInfo, char *txt);

#define LogG(f) Log(&g_Log,f)

#define LogV1(f,d) StrPrintF( g_logBuf, f, d );\
        LogG( g_logBuf );

#define LogV2(f,d1,d2) StrPrintF( g_logBuf, f, d1, d2 );\
        Log( &g_Log, g_logBuf );
#else
#define LogG(f)
#define LogInit(i,f)
#define LogV1(f,d)
#define LogV2(f,d1,d2)
#endif

void EvtSetInt( EventType *event, int i);
int EvtGetInt( EventType *event );

#endif
