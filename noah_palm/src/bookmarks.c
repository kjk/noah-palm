/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Owner: Lukasz Szweda (qkiss@wp.pl)
*/

#include "common.h"
#include "bookmarks.h"

#ifdef NOAH_LITE
#error "This code doesn't work for Noah Lite"
#endif

/* name of the database that holds bookmarks sorted by name */
#define BOOKMARKS_DB_BY_NAME "Noah_bookmarks_n"

/* name of the database that holds bookmarks sorted by the time 
   they were bookmarked */
#define BOOKMARKS_DB_BY_TIME "Noah_bookmarks_t"

#define BOOKMARKS_DB_TYPE 'bkmk'

/* Open one of the bookmark databases. If sortType is bkmSortByName
   use bookmarks sorted by name, if bkmSortByTime use bookmarks
   sorted by the bookmarking time */
static Err OpenBookmarksDB(AppContext* appContext, BookmarkSortType sortType)
{
    Err      err;
    char *   dbName;
 
    if ( sortType == appContext->currBookmarkDbType )
    {
        // it's already open
        Assert( appContext->bookmarksDb );
        return errNone;
    }

    switch( sortType )
    {
        case bkmSortByName:
            dbName = BOOKMARKS_DB_BY_NAME;
            break;
        case bkmSortByTime:
            dbName = BOOKMARKS_DB_BY_TIME;
            break;
        default:
            Assert(false);
            break;
    }

    CloseBookmarksDB(appContext);

    appContext->bookmarksDb = OpenDbByNameCreatorType(dbName, APP_CREATOR, BOOKMARKS_DB_TYPE);

    if (!appContext->bookmarksDb)
    {
        err = DmCreateDatabase(0, dbName, APP_CREATOR,  BOOKMARKS_DB_TYPE, false);
        if ( errNone != err)
            return err;

        appContext->bookmarksDb = OpenDbByNameCreatorType(dbName, APP_CREATOR, BOOKMARKS_DB_TYPE);
        if (!appContext->bookmarksDb)
            return dmErrCantOpen;
    }
    Assert(appContext->bookmarksDb);

    appContext->currBookmarkDbType = sortType;
    return errNone;
}

Err CloseBookmarksDB(AppContext* appContext)
{
    if (appContext->bookmarksDb != NULL)
    {
        Assert( bkmInvalid != appContext->currBookmarkDbType );
        DmCloseDatabase(appContext->bookmarksDb);
        appContext->bookmarksDb = NULL;
        appContext->currBookmarkDbType = bkmInvalid;
    }
    return errNone;
}

static void BookmarksListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    Int16       stringWidthP = 160; // max width of the string in the list selection window
    Int16       stringLenP;
    Boolean     truncatedP = false;
    MemHandle   recHandle;
    char *      str;
    AppContext* appContext=GetAppContext();

    Assert(itemNum >= 0);
    
    recHandle = DmQueryRecord(appContext->bookmarksDb, itemNum);
    Assert(recHandle); // no reason it shouldn't work
    if (recHandle)
    {
        str = MemHandleLock(recHandle);
        stringLenP = StrLen(str);
        FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
        WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
        MemHandleUnlock(recHandle);
    }
}

/* Return the number of bookmarked words */
static UInt16 BookmarksWordCount(AppContext* appContext)
{
    Assert(appContext->bookmarksDb);
    return DmNumRecords(appContext->bookmarksDb);
}

/* Get number of bookmarked words. Doesn't depend on the database being open */
UInt16 GetBookmarksCount(AppContext *appContext)
{
    Boolean     fOpenedDatabase = false;
    UInt16      bookmarksCount = 0;

    if (NULL == appContext->bookmarksDb)
    {
        OpenBookmarksDB(appContext, bkmSortByName);
        fOpenedDatabase = true;
    }

    if (appContext->bookmarksDb)
        bookmarksCount = BookmarksWordCount(appContext);

    if (fOpenedDatabase)
        CloseBookmarksDB(appContext);

    return bookmarksCount;
}

Boolean BookmarksFormHandleEvent(EventType * event)
{
    Boolean     handled = false;
    FormType *  frm;
    ListType *  bkmList, * sortTypeList;
    char *      listTxt;
    UInt16      bookmarksCount;
    AppContext* appContext = GetAppContext();

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case frmOpenEvent:
            OpenBookmarksDB(appContext, appContext->prefs.bookmarksSortType);
            bkmList = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listBookmarks));
            bookmarksCount = BookmarksWordCount(appContext);
            Assert( 0 != bookmarksCount );

            LstSetDrawFunction(bkmList, BookmarksListDrawFunc);
            LstSetListChoices(bkmList, NULL, bookmarksCount);

            sortTypeList = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listSortBy));
            // list position matches enums for simplicity
            LstSetSelection(sortTypeList, (Int16)appContext->prefs.bookmarksSortType);

            listTxt = LstGetSelectionText(sortTypeList, appContext->prefs.bookmarksSortType);
            CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupSortBy)), listTxt);

            FrmDrawForm(frm);

            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonCancel:
                    CloseBookmarksDB(appContext);
                    FrmReturnToForm(0);
                    handled = true;
                    break;

                case popupSortBy:
                    // do nothing
                    break;

                default:
                    Assert(false);
                    break;
            }
            break;

        case popSelectEvent:
            switch (event->data.popSelect.listID)
            {
                case listSortBy:
                    Assert( appContext->currBookmarkDbType == appContext->prefs.bookmarksSortType );
                    if ((BookmarkSortType) event->data.popSelect.selection != appContext->prefs.bookmarksSortType)
                    {
                        // we changed sorting type
                        sortTypeList = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listSortBy));
                        listTxt = LstGetSelectionText(sortTypeList, event->data.popSelect.selection);
                        CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupSortBy)), listTxt);

                        // list position matches enums for simplicity
                        OpenBookmarksDB(appContext, (BookmarkSortType) event->data.popSelect.selection);
                        appContext->prefs.bookmarksSortType = (BookmarkSortType) event->data.popSelect.selection;
#ifdef DEBUG
                        // word count shouldn't change
                        bookmarksCount = BookmarksWordCount(appContext);
                        bkmList = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listBookmarks));
                        Assert( LstGetNumberOfItems(bkmList) == bookmarksCount );
#endif
                        FrmDrawForm(frm);
                    }
                    handled = true;
                    break;

                default:
                    Assert(false);
                    break;
            }
            break;

        case lstSelectEvent:
        {
            MemHandle recHandle;
            char *    word;
            recHandle = DmQueryRecord(appContext->bookmarksDb, event->data.lstSelect.selection);
            Assert( recHandle ); // no reason it shouldn't work
            if (recHandle)
            {
                word = (char*)MemHandleLock(recHandle);
                Assert(word);
                appContext->currentWord = dictGetFirstMatching(GetCurrentFile(appContext), word);
                MemHandleUnlock(recHandle);
                SendNewWordSelected();
            }
            CloseBookmarksDB(appContext);
            FrmReturnToForm(0);
            handled = true;
        }

        default:
            break;
    }
    return handled;
}

static Int16 BookmarksByNameCompare(char * r1, char * r2, Int16 sortOrder, SortRecordInfoPtr /* info1 */, 
    SortRecordInfoPtr /* info2 */, MemHandle /* appInfoH */)
{
    // need to use that to have the same sorting as in list of words
    return p_istrcmp(r1,r2);
}

/* Create a new record recNum in a databse db that contains given word */
static Err WriteWordInRecord(DmOpenRef db, UInt16 recNum, char *word)
{
    int         wordLen;
    MemHandle   recHandle;
    MemPtr      recData;
    Err         err;

    Assert(db);
    Assert(word);

    wordLen = StrLen(word)+1;

    recHandle = DmNewRecord(db, &recNum, wordLen);
    if (!recHandle)
        return DmGetLastErr();

    recData = MemHandleLock(recHandle);
    Assert( recData );

    err = DmWrite(recData, 0, word, wordLen);
    MemHandleUnlock(recHandle);
    DmReleaseRecord(db, recNum, true);
    return err;
}

/* Save a given word in bookmarks database. */
Err AddBookmark(AppContext* appContext, char * word)
{
    Err                 err;
    UInt16              pos;
    Boolean             fWordAlreadyBookmarked;
    MemHandle           recHandle;
    char *              wordInRec;
    BookmarkSortType    currDbOpen;

    currDbOpen = appContext->currBookmarkDbType;

    // 1. See if we already have the record in bookmarks.
    //    We assume that if the record wasn't found in sorted-by-name database, it doesn't exist in the other.
    //    The sorted-by-name database is quicker to scan, because we can use binary search.

    err = OpenBookmarksDB(appContext, bkmSortByName);
    if (errNone != err)
        goto Exit;

    pos = DmFindSortPosition(appContext->bookmarksDb, word, NULL, (DmComparF *) BookmarksByNameCompare, 0);
    
    fWordAlreadyBookmarked = false;
    // DmFindSortPosition returns the position there the new record should be placed
    // so if the record exist its position is (pos - 1)
    recHandle = DmQueryRecord(appContext->bookmarksDb, pos>0 ? pos-1 : pos);
    // it's ok if we don't get handle - this might be an empty database
    if (recHandle)
    {
        wordInRec = (char*)MemHandleLock(recHandle);
        if (0 == StrCompare(word, wordInRec))
            fWordAlreadyBookmarked = true;
        MemHandleUnlock(recHandle);
    }
    
    if (fWordAlreadyBookmarked)
    {
        err = errNone;
        goto Exit;
    }

    // 2. If we haven't found the record, we add it to databases

    WriteWordInRecord(appContext->bookmarksDb, pos, word);
    CloseBookmarksDB(appContext);

    err = OpenBookmarksDB(appContext, bkmSortByTime);
    if (errNone != err)
        goto Exit;
    // the record must be put chronogically, so we add it at the beginning of the database
    WriteWordInRecord(appContext->bookmarksDb, 0, word);
    CloseBookmarksDB(appContext);

Exit:
    if (bkmInvalid != currDbOpen)
        OpenBookmarksDB(appContext, currDbOpen);
    return err;
}

/* Delete a bookmark for a given word in a bookmark database indicated by sortType */
static Err DeleteBookmarkInDB(AppContext* appContext, BookmarkSortType sortType, char *word)
{
    Err          err;
    UInt16       recsCount, i;
    MemHandle    recHandle;
    char *       wordInRecord;

    err = OpenBookmarksDB(appContext, sortType);
    if ( errNone != err )
        return err;

    recsCount = DmNumRecords(appContext->bookmarksDb);
    for (i = 0; i < recsCount; i++)
    {
        recHandle = DmQueryRecord(appContext->bookmarksDb, i);
        if (!recHandle)
        {
            err = DmGetLastErr();
            goto OnError;
        }

        wordInRecord = (char*)MemHandleLock(recHandle);
        Assert(wordInRecord);
        if (0 == StrCompare(wordInRecord, word))
        {
            MemHandleUnlock(recHandle);
            DmRemoveRecord(appContext->bookmarksDb, i);
            break;
        }
        MemHandleUnlock(recHandle);
    }
OnError:
    CloseBookmarksDB(appContext);
    return err;
}

/* Remove a given word from bookmarks database. It's ok for the word
   to not be bookmarked. */
void DeleteBookmark(AppContext* appContext, char *word)
{
    BookmarkSortType    currDbOpen;

    currDbOpen = appContext->currBookmarkDbType;
    // ignore error in DeleteBookmarkInDB - not much that we can do
    DeleteBookmarkInDB(appContext, bkmSortByName, word);    
    DeleteBookmarkInDB(appContext, bkmSortByTime, word);    
    if (bkmInvalid != currDbOpen)
        OpenBookmarksDB(appContext, currDbOpen);
}
