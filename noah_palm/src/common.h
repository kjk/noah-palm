/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <PalmOS.h>
#include <PalmNavigator.h>  // for 5-way
#include <68K/Hs.h>
#include <Chars.h>

#include "dynamic_input_area.h"
#include "word_matching_pattern.h"

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

#define  ENGPOL_TYPE            'enpl'
#define  SIMPLE_TYPE            'simp'

#define  WORDNET_LITE_TYPE      'wnet'

#ifdef DEMO
#define  WORDNET_PRO_TYPE       'wnde'
#else
#define  WORDNET_PRO_TYPE       'wn20'
#endif

#define evtFieldChanged          (firstUserEvent+1)
#define evtNewWordSelected       (firstUserEvent+2)
#define evtNewDatabaseSelected   (firstUserEvent+3)

// information about the area for displaying definitions:
// start x, start y, number of lines (dy).
// assume that width is full screen (160)
#define DRAW_DI_X 0
#define DRAW_DI_Y 0
#define FONT_DY  11

// 2003-11-25 andrzejc  - replaced by gd.dispLinesCount to accomodate DynamicInputArea
//#define DRAW_DI_LINES 13
#define FRM_RSV_H 16
#define FRM_RSV_W 10
#define FRM_MIN_H 160
#define FRM_PREF_H FRM_MIN_H
#define FRM_MAX_H 225
#define FRM_MIN_W 160
#define FRM_PREF_W FRM_MIN_W
#define FRM_MAX_W FRM_PREF_W

typedef enum 
{
    appHasNotifyMgr
} AppFlags;   

#define AppTestFlag(flag) ((gd.flags & (1<<(flag)))!=0)
#define AppSetFlag(flag) (gd.flags|=(1<<(flag)))
#define AppResetFlag(flag) (gd.flags&= ~(1<<(flag)))
#define AppHasNotifyMgr() AppTestFlag(appHasNotifyMgr)

#define SEARCH_TXT   "Searching..."

typedef enum
{
    ERR_NONE = 0,
    ERR_NO_MEM,
    ERR_NO_PREF_DB_CREATE,
    ERR_NO_PREF_DB_OPEN,
    ERR_TOO_MANY_DBS,
    ERR_DICT_INIT,
    ERR_SILENT,
    ERR_GENERIC
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
    dbStartupActionLast
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
void    ClearDisplayRectangle();
void    DrawCenteredString(const char *str, int dy);
void    DrawDisplayInfo(DisplayInfo * di, int firstLine, Int16 x,
                     Int16 y, int maxLines);
void    DrawWord(char *word, int pos_y);
char *  GetNthTxt(int n, char *txt);
char *  GetWnPosTxt(int partOfSpeech);

void    DrawCentered(char *txt);
void    DrawCacheRec(int recNum);

#ifdef DEBUG
void stress(long step);
#endif

// those functions are not available for Noah Lite
#ifndef NOAH_LITE
void    HistoryListInit(FormType *frm);
void    HistoryListSetState(FormType *frm);
void    HistoryListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
void    strtolower(char *txt);
void    AddToHistory(UInt32 wordNo);
void    FreeHistory(void);
void    SetPopupLabel(FormType * frm, UInt16 listID, UInt16 popupID, Int16 txtIdx, char *txtBuf);
Boolean FTryClipboard(void);
#endif

Boolean dictNew(void);
void    dictDelete(void);
long    dictGetWordsCount(void);
long    dictGetFirstMatching(char *word);
char *  dictGetWord(long wordNo);
Err     dictGetDisplayInfo(long wordNo, int dx, DisplayInfo * di);
void    FreeDicts(void);

void    RedrawMainScreen();
void    DrawDescription(long wordNo);
void    DisplayHelp(void);
void    HideScrollbar(void);
void    SetScrollbarState(DisplayInfo * di, int maxLines, int firstLine);

int     GetOsVersion(void);
int     GetMaxScreenDepth();
Boolean IsColorSupported();
int     GetCurrentScreenDepth();
void    SetTextColorBlack(void);
void    SetTextColorRed(void);

Boolean CreateHelpData(void);
void    FreeInfoData(void);
long    CalcListOffset(long itemsCount, long itemNo);
void    RemoveWhiteSpaces( char *src );
void    DefScrollUp(ScrollType scroll_type);
void    DefScrollDown(ScrollType scroll_type);
void    ScrollWordListByDx(FormType *frm, int dx);
char *  GetDatabaseName(int dictNo);
void    LstSetListChoicesEx(ListType * list, char **itemText, long itemsCount);
void    LstSetSelectionEx(ListType * list, long itemNo);
void    LstMakeItemVisibleEx(ListType * list, long itemNo);
void    LstSetSelectionMakeVisibleEx(ListType * list, long itemNo);
void    CtlHideControlEx( FormType *frm, UInt16 objID);
void    CtlShowControlEx( FormType *frm, UInt16 objID);
void    ListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
void    ListDbDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
void    SendNewWordSelected(void);

/* stores all the information about the stack of strings */
typedef struct
{
    int     stackedCount;
    char ** strings;
    int     freeSlots;
} StringStack;


void    ssInit  ( StringStack *ss );
void    ssDeinit( StringStack *ss );
Boolean ssPush  ( StringStack *ss, char *str );
char *  ssPop   ( StringStack *ss );

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

void            EvtSetInt(EventType *event, int i);
int             EvtGetInt(EventType *event);

void            serByte    (unsigned char val, char *prefsBlob, long *pCurrBlobSize);
void            serInt     (int val, char *prefsBlob, long *pCurrBlobSize);
void            serLong    (long val, char *prefsBlob, long *pCurrBlobSize);
unsigned char   deserByte  (unsigned char **data, long *pBlobSizeLeft);
int             deserInt   (unsigned char **data, long *pBlobSizeLeft);
long            deserLong  (unsigned char **data, long *pBlobSizeLeft);
void            serData    (char *data, long dataSize, char *prefsBlob, long *pCurrBlobSize);
void            deserData  (unsigned char *valOut, int len, unsigned char **data, long *pBlobSizeLeft);
void            serString  (char *str, char *prefsBlob, long *pCurrBlobSize);
char *          deserString(unsigned char **data, long *pCurrBlobSize);
void            deserStringToBuf(char *buf, int bufSize, unsigned char **data, long *pCurrBlobSize);

void            RememberLastWord(FormType * frm);
void            DoFieldChanged(void);
void            SendFieldChanged(void);
void            SendNewDatabaseSelected(int db);
void            SendStopEvent(void);
char *          strdup(char *s);
long            FindCurrentDbIndex(void);

/* 5-Way support */
void            InitFiveWay(void);
Boolean         HaveFiveWay( void );
Boolean         HaveHsNav( void );

#define HsNavCenterPressed(eventP) \
( \
  IsHsFiveWayNavEvent(eventP) && \
  ((eventP)->data.keyDown.chr == vchrRockerCenter) && \
  (((eventP)->data.keyDown.modifiers & commandKeyMask) != 0) \
)

#define HsNavDirectionPressed(eventP, nav) \
( \
  IsHsFiveWayNavEvent(eventP) && \
  ( vchrRocker ## nav == vchrRockerUp) ? \
   (((eventP)->data.keyDown.chr == vchrPageUp) || \
    ((eventP)->data.keyDown.chr == vchrRocker ## nav)) : \
   (vchrRocker ## nav == vchrRockerDown) ? \
   (((eventP)->data.keyDown.chr == vchrPageDown) || \
    ((eventP)->data.keyDown.chr == vchrRocker ## nav)) : \
    ((eventP)->data.keyDown.chr == vchrRocker ## nav) \
)

#define HsNavKeyPressed(eventP, nav) \
( \
  ( vchrRocker ## nav == vchrRockerCenter ) ? \
  HsNavCenterPressed(eventP) : \
  HsNavDirectionPressed(eventP, nav) \
)

#define IsHsFiveWayNavEvent(eventP) \
( \
    HaveHsNav() && ((eventP)->eType == keyDownEvent) && \
    ( \
        ((((eventP)->data.keyDown.chr == vchrPageUp) || \
          ((eventP)->data.keyDown.chr == vchrPageDown)) && \
         (((eventP)->data.keyDown.modifiers & commandKeyMask) != 0)) \
        || \
        (TxtCharIsRockerKey((eventP)->data.keyDown.modifiers, \
                            (eventP)->data.keyDown.chr)) \
    ) \
)

#define FiveWayCenterPressed(eventP) \
( \
  NavSelectPressed(eventP) || \
  HsNavCenterPressed(eventP) \
)

#define FiveWayDirectionPressed(eventP, nav) \
( \
  NavDirectionPressed(eventP, nav) || \
  HsNavDirectionPressed(eventP, nav) \
)

/* only to be used with Left, Right, Up, and Down; use
   FiveWayCenterPressed for Select/Center */
#define FiveWayKeyPressed(eventP, nav) \
( \
  NavKeyPressed(eventP, nav) || \
  HsNavKeyPressed(eventP, nav) \
)

#define IsFiveWayEvent(eventP) \
( \
  HaveHsNav() ? IsHsFiveWayNavEvent(eventP) : IsFiveWayNavEvent(eventP) \
)

/**
 * Resizes object on form given object's id.
 * @param frm form containing object.
 * @param objId object's id.
 * @param x new topLext.x coordinate.
 * @param y new topLeft.y coordinate.
 * @param ex new extent.x (width).
 * @param ey new extent.y (height).
 */
extern void FrmSetObjectBoundsByID(FormType* frm, UInt16 objId, Int16 x, Int16 y, Int16 ex, Int16 ey);

/**
 * Synchronizes screen resolution cached in <code>gd</code> with actual screen resolution.
 */
extern void SyncScreenSize();


extern Err DefaultFormInit(FormType* frm);
extern void AppHandleSysNotify(SysNotifyParamType* param);

#endif
