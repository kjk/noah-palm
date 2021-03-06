/**
 * @file common.h
 * Declarations of commonly-used and general functions.
 */
 
/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#pragma warn_a5_access on 

#include <PalmOS.h>
#include "cw_defs.h"

#include "dynamic_input_area.h"
#include "mem_leak.h"
#include "display_info.h"
#include "extensible_buffer.h"
#include "better_formatting.h"
#include "copy_block.h"
#include "bookmarks.h"


// define for a special build of i_noah which uses internal server for access
//#define INTERNAL_BUILD 1

//#define _RECORD_SPEED_ 1

#ifdef DEBUG
#define     Assert(c)         ErrFatalDisplayIf(!(c),#c)
#else
#define    Assert(c)          {}
#endif

#define strlen StrLen

#define WORD_MAX_LEN 40
typedef char WordStorageType[WORD_MAX_LEN];

#define MAGIC_RESIDENT_LOOKUP_PREFIX "arslexis.res.lookup:"
#define MAGIC_RESIDENT_LOOKUP_PREFIX_LEN sizeof(MAGIC_RESIDENT_LOOKUP_PREFIX)-1

#ifndef I_NOAH

#define  ENGPOL_TYPE            'enpl'
#define  SIMPLE_TYPE            'simp'
#define  WORDNET_LITE_TYPE      'wnet'

#ifdef DEMO
#define  WORDNET_PRO_TYPE       'wnde'
#else
#define  WORDNET_PRO_TYPE       'wn20'
#endif

#define MAX_DICTS 10

#endif //I_NOAH

#define evtFieldChanged          (firstUserEvent+1)
#define evtNewWordSelected       (firstUserEvent+2)
#define evtNewDatabaseSelected   (firstUserEvent+3)
#define evtShowMalformedAlert    (firstUserEvent+4)
#define evtRegistrationFailed    (firstUserEvent+5)
#define evtRegistrationOk        (firstUserEvent+6)
#define evtConnectionFinished    (firstUserEvent+7)
#define evtRefresh               (firstUserEvent+8)
#ifdef I_NOAH
#define evtReformatLastResponse  (firstUserEvent+9)
#endif

// information about the area for displaying definitions:
// start x, start y, number of lines (dy).
// assume that width is full screen (160)
#define DRAW_DI_X 0
#define DRAW_DI_Y 0
#define FONT_DY  11

#define FRM_RSV_H 16
#define FRM_RSV_W 10
#define FRM_MIN_H 160
#define FRM_PREF_H pinMaxConstraintSize
#define FRM_MAX_H pinMaxConstraintSize
#define FRM_MIN_W 160
#define FRM_PREF_W pinMaxConstraintSize
#define FRM_MAX_W pinMaxConstraintSize

typedef enum 
{
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

#ifndef I_NOAH

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

#endif // I_NOAH


#ifdef NOAH_PRO
#include "noah_pro.h"
#include "noah_pro_rcp.h"
#endif

#ifdef NOAH_PRO
#include "pronunciation.h"
#endif 

#ifdef NOAH_LITE
#include "noah_lite.h"
#include "noah_lite_rcp.h"
#endif

#ifdef THESAURUS
#include "thes.h"
#include "thes_rcp.h"
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

#ifdef I_NOAH
#include "i_noah.h"
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
    appErrWordNotFound=appErrorClass, // 0x8000
    appErrMalformedResponse, // 0x8001
    appErrBadAuthorization  // 0x8002
} AppErr;    

#define appNotifyResidentLookupEvent APP_CREATOR

typedef struct
{
    Boolean fCreated;
    char    *fileName;
} LogInfo;

typedef struct _AppContext
{
#ifndef I_NOAH
    AbstractFile *     dicts[MAX_DICTS];
    int                dictsCount;
    AppError           err;
#endif // I_NOAH    


    DisplayInfo *      currDispInfo;

#ifdef NOAH_LITE
    ExtensibleBuffer * helpDispBuf;
#endif

#ifndef I_NOAH    
    long               currentWord;
    long               wordsCount;
#endif // I_NOAH
    
    int                firstDispLine;
    int                lastDispLine;
    /* Pointer to sth. we want to store.
    I use it in DisplayPrefsForm to store old DisplayPrefs and restore them if Cancel
    You are free to use it, but add comment when.:
    --- in displayPrefsForm
    
    */
    void             * ptrOldDisplayPrefs;

    /*Stores all info about actual selection*/
    CopyBlock          copyBlock;
    
#ifdef NOAH_PRO
    /*Stores info about pronunciation records*/
    PronunciationData  pronData;
#endif

#ifdef NOAH_PRO
    /*True if arm is enable on device*/
    Boolean            armIsPresent;
#endif

#ifndef I_NOAH    
    long               listItemOffset;
    long               prevTopItem;
    int                penUpsToConsume;
    long               prevSelectedWord;
    long               selectedWord;
#ifdef NOAH_PRO  
    Boolean            prefsPresent;
#endif  

#endif // I_NOAH

    WordStorageType    lastWord;
    UInt16             fldInsPt;

    long               ticksEventTimeout;
#ifndef NOAH_LITE  
    int                historyCount;
    const char *       wordHistory[HISTORY_ITEMS];
    int                posInHistory;

#ifndef I_NOAH

    Boolean            fFirstRun; // is this first run or not 
    
#endif // I_NOAH
    
#endif    
    AppPrefs           prefs;
    AppPrefs           tmpPrefs;
    
#ifdef DEBUG
#ifdef I_NOAH
    Boolean            fInStress;
#endif
#ifndef I_NOAH    
    long               currentStressWord;
#endif
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
     * @see AppFlags
     */
    long               flags;

    /**
     * Settings used by DynamicInputArea implementation.
     */
    DIA_Settings diaSettings;
    
#ifndef I_NOAH      
    FS_Settings fsSettings;
    AbstractFile* currentFile;
#endif // I_NOAH
    
    int osVersion;
    int maxScreenDepth;
    
#ifdef DEBUG
    LogInfo log;
    char logBuffer[512];
#endif

#ifndef I_NOAH
    DmOpenRef wmpCacheDb;
    long wmpLastWordPos;
#endif // I_NOAH    
                  
    DmOpenRef bookmarksDb;

    // which bookmark database is currently open
    BookmarkSortType currBookmarkDbType;
    
#ifdef I_NOAH
    NetIPAddr serverIpAddress;
    ExtensibleBuffer lastResponse;
    ExtensibleBuffer currentDefinition;
    ExtensibleBuffer currentWordBuf;
    void* currentConnectionData;
    Boolean lookupStatusBarVisible;
    ExtensibleBuffer lastMessage;
    
    char** wordsList;
    UInt16 wordsInListCount;
    MainFormContent mainFormContent;
    
#endif      

#ifdef _RECORD_SPEED_
    char * recordSpeedDescription;
    UInt32 recordSpeedTicksCount;
#endif
    // if this resident mode lookup, this points to a word to lookup
    char *  residentWordLookup;
    Boolean fInResidentMode;
} AppContext;

#define AppTestFlag(appContext, flag) (((appContext)->flags & (1<<(flag)))!=0)
#define AppSetFlag(appContext, flag) ((appContext)->flags |= (1<<(flag)))
#define AppResetFlag(appContext, flag) ((appContext)->flags &= ~(1<<(flag)))

extern AppContext* GetAppContext();

long GetMaxListItems();

#ifndef I_NOAH

extern Boolean get_defs_record(AbstractFile* file, long entry_no, int first_record_with_defs_len, int defs_len_rec_count, int first_record_with_defs, int *record_out, long *offset_out, long *len_out, struct _WcInfo *wcInfo);
extern Boolean get_defs_records(AbstractFile* file, long entry_count, int first_record_with_defs_len,  int defs_len_rec_count, int first_record_with_defs, SynsetDef * synsets);

#endif // I_NOAH

void    ClearRectangle(Int16 sx, Int16 sy, Int16 ex, Int16 ey);
void    ClearDisplayRectangle(AppContext* appContext);
void    DrawCenteredString(AppContext* appContext, const char *str, int dy);
void    DrawStringCenteredInRectangle(AppContext* appContext, const char *str, int dy);
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
void strtolower(char *txt);
void AddWordToHistory(AppContext* appContext, const char *word);
void AddToHistory(AppContext* appContext, UInt32 wordNo);
void FreeHistory(AppContext* appContext);
Boolean FHistoryCanGoForward(AppContext* appContext);
Boolean FHistoryCanGoBack(AppContext* appContext);
const char *HistoryGoBack(AppContext *appContext);
const char *HistoryGoForward(AppContext *appContext);
Boolean FTryClipboard(AppContext* appContext, Boolean fRequireExact);
#endif
void    SetPopupLabel(FormType * frm, UInt16 listID, UInt16 popupID, Int16 txtIdx);

#ifndef I_NOAH

Boolean dictNew(AbstractFile* file);
void dictDelete(AbstractFile* file);
long dictGetWordsCount(AbstractFile* file);
long dictGetFirstMatching(AbstractFile* file, char *word);
char* dictGetWord(AbstractFile* file, long wordNo);
Err dictGetDisplayInfo(AbstractFile* file, long wordNo, int dx, DisplayInfo * di);
void FreeDicts(AppContext* appContext);

#endif // I_NOAH

void RedrawMainScreen(AppContext* appContext);
void DrawDescription(AppContext* appContext, long wordNo);
void DisplayHelp(AppContext* appContext);
void DisplayAbout(AppContext* appContext);
void HideScrollbar(void);
void SetScrollbarState(DisplayInfo * di, int maxLines, int firstLine);

int GetOsVersion();
int GetMaxScreenDepth(AppContext* appContext);
Boolean IsColorSupported(AppContext* appContext);
int GetCurrentScreenDepth(AppContext* appContext);
void SetTextColor(AppContext* appContext, RGBColorType *color);
void SetTextColorRGB(AppContext* appContext, PackedRGB rgb);
void SetBackColor(AppContext* appContext,RGBColorType *color);
void SetBackColorRGB(AppContext* appContext, PackedRGB rgb);
void SetGlobalBackColor(AppContext* appContext);

Boolean CreateHelpData(AppContext* appContext);
void FreeInfoData(AppContext* appContext);
long CalcListOffset(long itemsCount, long itemNo);
void StringStrip(char *src);
void StringSanitize(char *src);
void StringExtractWord(char *src);
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
void SendEvtWithType(int eType);

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
int     ssCount(StringStack *ss);

#ifdef DEBUG

void LogInit(AppContext* appContext, char* fileName);
void Log(AppContext* appContext, const char* txt);
void LogStrN(AppContext* appContext, const char *txt, long size);

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

void            RememberLastWord(AppContext* appContext, FormType * frm, int objId);

void            DoFieldChanged(AppContext* appContext);
void            SendFieldChanged(void);
void            SendNewDatabaseSelected(int db);
void            SendStopEvent(void);
char *          strdup(const char *s);
long            FindCurrentDbIndex(AppContext* appContext);

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

extern void FrmSetObjectPosByID(FormType* frm, UInt16 objId, Int16 x, Int16 y);

extern void UpdateFrmBounds(FormType *frm);

extern void SetListHeight(FormType *frm, UInt16 objId, int linesCount);

/**
 * Synchronizes screen resolution cached in @c appContext with actual screen resolution.
 */
extern void SyncScreenSize(AppContext* appContext);

Boolean FResidentModeEnabled();

Err DefaultFormInit(AppContext* appContext, FormType* frm);

extern Err AppHandleSysNotify(SysNotifyParamType* param);
extern Err AppNotifyInit(AppContext* appContext);
#ifndef I_NOAH
extern Err AppNotifyFree(Boolean beResident);
#endif // I_NOAH

extern AppContext* GetAppContext();

#define IsValidPrefRecord(recData) (AppPrefId==*((long*)(recData)))

extern void FldSelectAllText(FieldType* field);

#ifndef I_NOAH
extern UInt16 FldGetSelectedText(FieldType* field, char* buffer, UInt16 bufferSize);
extern AbstractFile* FindOpenDatabase(AppContext* appContext, const char* name);
#endif // I_NOAH

Err ErrFindDatabaseByNameTypeCreator(const char *dbName, UInt32 type, UInt32 creator, LocalID *dbId);
DmOpenRef OpenDbByNameTypeCreator(char *dbName, UInt32 type, UInt32 creator);

/**
 * Calculates the @c current to @c total ratio and 
 * renders result as percents, optionally filling @c buffer
 * with textual representation.
 * @param buffer @c char array with space for at least 5 elements, or NULL if not needed.
 * @return ratio in percents.
 */ 
extern UInt16 PercentProgress(char* buffer, UInt32 current, UInt32 total);

extern void SafeStrNCopy(char *dst, int dstLen, const char *srcStr, int srcStrLen);

#define MillisecondsToTicks(millis) ((((float)SysTicksPerSecond())*((float)(millis)))/1000.0)

/**
 * Searches sequence for occurence of subsequence.
 * @param begin pointer to first character of sequence to search.
 * @param end pointer to one-past-last character of sequence to search.
 * @param subStr subseqeunce to find (null-terminated).
 * @return pointer to first occurence of @c subStr, or @c end if it's not found.
 */
extern const char* StrFind(const char* begin, const char* end, const char* subStr);

/**
 * Tests if sequence starts with subsequence.
 * @param begin pointer to first character of sequence to test.
 * @param end pointer to one-past-last character of sequence to test.
 * @param subStr string to look for (null-terminated).
 * @return @c true, if sequence [<code>begin</code>, <code>end</code>) starts with @c subStr.
 */
extern Boolean StrStartsWith(const char* begin, const char* end, const char* subStr);

/**
 * Converts character sequence to it's numeric representation.
 * @param begin pointer to first character of sequence to convert.
 * @param end pointer to one-past-last character of sequence to convert.
 * @param resut on successful completion will contain numeric representation of sequence [<code>begin</code>, <code>end</code>).
 * @param base base of numeric system used to perform converion, effective range [2, 36).
 * @return error code, @c errNone if conversion is successful.
 */
extern Err StrAToIEx(const char* begin, const char* end, Int32* result, UInt16 base);

/**
 * Scans sequence for occurence of any of given characters.
 * @param begin pointer to first character of sequence to search.
 * @param end pointer to one-past-last character of sequence to search.
 * @param chars characters to scan for (null-terminated).
 * @return pointer to first occurence of given characters, or @c end if not found.
 */
extern const char* StrFindOneOf(const char* begin, const char* end, const char* chars);

/**
 * Repositions sequence bounduary so that it doesn't contain any trailing whitespace (space, tab, line-feed, carriage-return).
 * @param begin pointer to first character of sequence to trim.
 * @param end on entry contains pointer to one-past-last character of sequence, on return will contain pointer one-past-last not-whitespace character.
 */
 extern void StrTrimTail(const char* begin, const char** end);

/**
 * Performs URL-encoding according to RFC2396 (URI: Generic Syntax).
 * Encoded sequence is returned in @c encoded parameter. It should be freed by client using 
 * @c new_free().
 * @param begin pointer to first character of sequence to encode.
 * @param end pointer to one-past-last character of sequence to encode.
 * @param encoded on successful return contains pointer to encoded sequence (not null-terminated).
 * @param encodedLength on successful return contains length of encoded sequence.
 * @return error code, @c errNone on success.
 * @see RFC2396 Chapter 2
 */
extern Err StrUrlEncode(const char* begin, const char* end, char** encoded, UInt16* encodedLength);

extern Int16 LstGetSelectionByListID(const FormType* form, UInt16 listID);

extern void CreateNewMemo(const char *memoBody, int memoBodySize);

#ifdef DEBUG 

extern void LogErrorToMemo_(const char* message, Err error);
#define LogErrorToMemo(message, error) LogErrorToMemo_(message, error)

#else

#define LogErrorToMemo(message, error) (message), (error)

#endif

#ifdef _RECORD_SPEED_
void StartTiming(AppContext* appContext, char * description);
void StopTiming(AppContext* appContext);
#endif

void FldClearInsert(FormType *frm, int fldId, const char *txt);
void RememberFieldWordMain(AppContext *appContext, FormType *frm);
void GoToFindWordForm(AppContext *appContext, FormType *frm);
void RedisplayWord(AppContext *appContext);
long GenRandomLong(long range);
void ParseResidentWord(AppContext *appContext, char *cmdPBP);
void DoWord(AppContext* appContext, char *word);
Err  ErrWebBrowserCommand(Boolean subLaunch, UInt16 launchFlags, UInt16 command, char *parameterP, UInt32 *resultP);

#ifdef THESAURUS
void SavePreferencesThes(AppContext* appContext);
#endif

#ifdef I_NOAH
void SavePreferencesInoah(AppContext* appContext);
#endif

void DumpStrToMemo(const char* begin, const char* end);
#endif
