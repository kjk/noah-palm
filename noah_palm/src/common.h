/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <PalmOS.h>

#include "dynamic_input_area.h"

#define WORD_MAX_LEN 40
#define HISTORY_ITEMS 5

#include "mem_leak.h"
#include "display_info.h"
#include "extensible_buffer.h"
#include "better_formatting.h"

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

#define MAX_DICTS 10

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
    appHasNotifyMgr,
    appHasFiveWayNav,
    appHasHsNav
} AppFlags;   

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
} AppError;

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

#include "fs.h"
#include "fs_ex.h"


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

typedef enum _AppFeature
{
    appFtrContext,
    appFtrLeaksFile,
    appFtrMenuSupportDbRefNum
} AppFeature;  

/**
 * Application-specific runtime errors compatible with PalmOS Error Manager.
 */
typedef enum _AppErr
{
    appErrWordNotFound=appErrorClass
} AppErr;    

#define appNotifyResidentLookupEvent APP_CREATOR

typedef struct
{
    Boolean fCreated;
    char    *fileName;
} LogInfo;

typedef struct _AppContext
{
    AbstractFile*     dicts[MAX_DICTS];
    int                dictsCount;
    AppError         err;
    DisplayInfo *      currDispInfo;
    ExtensibleBuffer * helpDispBuf;
    long               currentWord;
    long               wordsCount;
    int                firstDispLine;
    int                lastDispLine;
    long               listItemOffset;
    long               prevTopItem;
    int                penUpsToConsume;
    long               prevSelectedWord;
    long               selectedWord;
#ifdef NOAH_PRO  
    Boolean            prefsPresent;
#endif  
    char               lastWord[WORD_MAX_LEN];
    long               ticksEventTimeout;
#ifndef NOAH_LITE  
    int                historyCount;
    char*             wordHistory[HISTORY_ITEMS];
    Boolean            fFirstRun; // is this first run or not 
#endif    
    AppPrefs          prefs;
#ifdef DEBUG
    long               currentStressWord;
#endif

    /**
     * Number of displayed lines. Replaces DRAW_DI_LINES from common.h to accomodate DynamicInputArea.
     * Should be updated every time display is resized.
     */
    int                 dispLinesCount;
    
    /**
     * Current screen width. Cached so that we don't have to query it every time we need to convert
     * hardcoded coordinates into relative ones.
     */
    int                 screenWidth;

    /**
     * Current screen height.
     * @see screenWidth
     */
    int                 screenHeight;
    
    /**
     * Flags related with system notifications and present features.
     * @see AppFlags in file common.h
     */
    long               flags;

    /**
     * Settings used by DynamicInputArea implementation.
     */
    DIA_Settings diaSettings;
      
    FS_Settings fsSettings;
    AbstractFile* currentFile;
    
    int osVersion;
    int maxScreenDepth;
    
#ifdef DEBUG
    LogInfo log;
    char logBuffer[512];
#endif

    DmOpenRef wmpCacheDb;
    long wmpLastWordPos;
                  
} AppContext;

#define AppTestFlag(appContext, flag) (((appContext)->flags & (1<<(flag)))!=0)
#define AppSetFlag(appContext, flag) ((appContext)->flags |= (1<<(flag)))
#define AppResetFlag(appContext, flag) ((appContext)->flags &= ~(1<<(flag)))
#define AppHasNotifyMgr(appContext) AppTestFlag(appContext, appHasNotifyMgr)

extern AppContext* GetAppContext();


long GetMaxListItems();

extern Boolean get_defs_record(AbstractFile* file, long entry_no, int first_record_with_defs_len, int defs_len_rec_count, int first_record_with_defs, int *record_out, long *offset_out, long *len_out);
extern Boolean get_defs_records(AbstractFile* file, long entry_count, int first_record_with_defs_len,  int defs_len_rec_count, int first_record_with_defs, SynsetDef * synsets);

/*
void    dh_save_font(void);
void    dh_restore_font(void);
void    dh_set_current_y(int y);
void    dh_display_string(const char *str, int font, int dy);
*/

void    ClearRectangle(Int16 sx, Int16 sy, Int16 ex, Int16 ey);
void ClearDisplayRectangle(AppContext* appContext);
void DrawCenteredString(AppContext* appContext, const char *str, int dy);
void    DrawWord(char *word, int pos_y);
char *  GetNthTxt(int n, char *txt);
char *  GetWnPosTxt(int partOfSpeech);

void DrawCentered(AppContext* appContext, char *txt);

#ifdef DEBUG
void stress(long step);
#endif

// those functions are not available for Noah Lite
#ifndef NOAH_LITE
void HistoryListInit(AppContext* appContext, FormType *frm);
void HistoryListSetState(AppContext* appContext, FormType *frm);
void HistoryListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
void    strtolower(char *txt);
void AddToHistory(AppContext* appContext, UInt32 wordNo);
void FreeHistory(AppContext* appContext);
Boolean FTryClipboard(AppContext* appContext);
#endif
void    SetPopupLabel(FormType * frm, UInt16 listID, UInt16 popupID, Int16 txtIdx);

Boolean dictNew(AbstractFile* file);
void dictDelete(AbstractFile* file);
long dictGetWordsCount(AbstractFile* file);
long dictGetFirstMatching(AbstractFile* file, char *word);
char* dictGetWord(AbstractFile* file, long wordNo);
Err dictGetDisplayInfo(AbstractFile* file, long wordNo, int dx, DisplayInfo * di);
void FreeDicts(AppContext* appContext);

void RedrawMainScreen(AppContext* appContext);
void DrawDescription(AppContext* appContext, long wordNo);
void DisplayHelp(AppContext* appContext);
void DisplayAbout(AppContext* appContext);
void HideScrollbar(void);
void SetScrollbarState(DisplayInfo * di, int maxLines, int firstLine);

int GetOsVersion(AppContext* appContext);
int GetMaxScreenDepth(AppContext* appContext);
Boolean IsColorSupported(AppContext* appContext);
int GetCurrentScreenDepth(AppContext* appContext);
void SetTextColor(AppContext* appContext, RGBColorType *color);
void SetTextColorBlack(AppContext* appContext);
void SetTextColorRed(AppContext* appContext);

Boolean CreateHelpData(AppContext* appContext);
void FreeInfoData(AppContext* appContext);
long CalcListOffset(long itemsCount, long itemNo);
void RemoveWhiteSpaces( char *src );
void DefScrollUp(AppContext* appContext, ScrollType scroll_type);
void DefScrollDown(AppContext* appContext, ScrollType scroll_type);
void ScrollWordListByDx(AppContext* appContext, FormType* frm, int dx);
char* GetDatabaseName(AppContext* appContext, int dictNo);
void LstSetListChoicesEx(ListType * list, char **itemText, long itemsCount);
void LstSetSelectionEx(AppContext* appContext, ListType * list, long itemNo);
void LstMakeItemVisibleEx(AppContext* appContext, ListType * list, long itemNo);
void LstSetSelectionMakeVisibleEx(AppContext* appContext, ListType * list, long itemNo);
void CtlHideControlEx( FormType *frm, UInt16 objID);
void CtlShowControlEx( FormType *frm, UInt16 objID);
void ListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
void ListDbDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
void SendNewWordSelected(void);

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

void LogInit(AppContext* appContext, char* fileName);
void Log(AppContext* appContext, char* txt);

#define LogG(f) Log(GetAppContext(), f)

#define LogV1(f,d) StrPrintF(GetAppContext()->logBuffer, f, d );\
        LogG(GetAppContext()->logBuffer )

#define LogV2(f,d1,d2) StrPrintF( GetAppContext()->logBuffer, f, d1, d2 );\
        LogG(GetAppContext()->logBuffer )
        
#else

#define LogG(f)
#define LogInit(appContext, f)
#define LogV1(f, d)
#define LogV2(f, d1, d2)

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

void RememberLastWord(AppContext* appContext, FormType * frm);
void DoFieldChanged(AppContext* appContext);
void            SendFieldChanged(void);
void            SendNewDatabaseSelected(int db);
void            SendStopEvent(void);
char *          strdup(char *s);
long FindCurrentDbIndex(AppContext* appContext);

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
extern void SyncScreenSize(AppContext* appContext);

Err DefaultFormInit(AppContext* appContext, FormType* frm);

extern Err AppHandleSysNotify(SysNotifyParamType* param);
extern Err AppNotifyInit(AppContext* appContext);
extern Err AppNotifyFree(AppContext* appContext, Boolean beResident);

extern AppContext* GetAppContext();

#define IsValidPrefRecord(recData) (AppPrefId==*((long*)(recData)))

extern UInt16 FldGetSelectedText(FieldType* field, Char* buffer, UInt16 bufferSize);

extern AbstractFile* FindOpenDatabase(AppContext* appContext, const Char* name);

extern Err PrepareWordDefinition(AppContext* appContext, Char* term);
extern Err PrepareWordDefinitionByWordNumber(AppContext* appContext, long wordNo);

#endif
