#include "common.h"
#include "bookmarks.h"

Err OpenBookmarksDB(AppContext* appContext, Int16 bookmarksSortBySelection)
{
    Err err;
    LocalID dbID;
    char * bookmarksDBByName = "Noah_bookmarks_n";
    char * bookmarksDBByTime = "Noah_bookmarks_t";
    char * bookmarksDB;

    if (bookmarksSortBySelection == BOOKMARKS_SORT_BY_NAME)   // we open the bookmarks-sorted-by-name database
    {
        bookmarksDB = bookmarksDBByName;
        Assert(StrLen(bookmarksDB) < dmDBNameLength);
    }
    else if (bookmarksSortBySelection == BOOKMARKS_SORT_BY_TIME)
    {
        bookmarksDB = bookmarksDBByTime;
        Assert(StrLen(bookmarksDB) < dmDBNameLength);
    }
    
    CloseBookmarksDB(appContext);
    if (appContext->bookmarksDb == NULL)
    {
        dbID = DmFindDatabase(0, bookmarksDB);
        if (!dbID)
        {
            err = DmCreateDatabase(0, bookmarksDB, 'NoAH', 'data', false);
            if (err) return err;
            dbID = DmFindDatabase(0, bookmarksDB);
            if (!dbID) return dmErrCantOpen;
        }
        appContext->bookmarksDb = DmOpenDatabase(0, dbID, dmModeReadWrite);
        if (NULL == appContext->bookmarksDb)
            return dmErrCantOpen;
    }
    return errNone;
}

Err CloseBookmarksDB(AppContext* appContext)
{
    if (appContext->bookmarksDb != NULL)
    {
        DmCloseDatabase(appContext->bookmarksDb);
        appContext->bookmarksDb = NULL;
    }
    return errNone;
}

void BookmarksListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    Int16       stringWidthP = 160; // max width of the string in the list selection window
    Int16       stringLenP;
    Boolean     truncatedP = false;
    MemHandle   recHandle;
    char        *str;
    AppContext* appContext=GetAppContext();

    Assert(itemNum >= 0);
    
    recHandle = DmQueryRecord(appContext->bookmarksDb, itemNum);
    if (recHandle)
    {
        str = MemHandleLock(recHandle);
        stringLenP = StrLen(str);
        FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
        WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
        MemHandleUnlock(recHandle);
    }
    else
    {
        WinDrawChars("-- NOT FOUND --", 15, bounds->topLeft.x, bounds->topLeft.y);
    }
}

UInt16 BookmarksNumRecords(AppContext* appContext)
{
    return DmNumRecords(appContext->bookmarksDb);
}

Boolean BookmarksFormHandleEvent(EventType * event)
{
    Boolean handled = false;
    FormType * frm = NULL;
    ListType *  list = NULL;
    char *      listTxt = NULL;
    AppContext* appContext = GetAppContext();

    frm = FrmGetActiveForm();
    switch (event->eType)
    {
        case frmOpenEvent:
            list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
            OpenBookmarksDB(appContext, appContext->bookmarksSortBySelection);
            LstSetDrawFunction(list, BookmarksListDrawFunc);
            LstSetListChoicesEx(list, NULL, BookmarksNumRecords(appContext));
            FrmDrawForm(frm);
            list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listSortBy));
            listTxt = LstGetSelectionText(list, appContext->bookmarksSortBySelection);
            LstSetSelection(list, appContext->bookmarksSortBySelection);
            CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupSortBy)), listTxt);
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
                    break;
            }
            break;

        case popSelectEvent:
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, event->data.popSelect.listID));
            listTxt = LstGetSelectionText(list, event->data.popSelect.selection);
            switch (event->data.popSelect.listID)
            {
                case listSortBy:
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupSortBy)), listTxt);
                    appContext->bookmarksSortBySelection = LstGetSelection(list);
                    OpenBookmarksDB(appContext, appContext->bookmarksSortBySelection);
                    list = (ListType *) FrmGetObjectPtr(frm,  FrmGetObjectIndex(frm, listMatching));
                    LstSetListChoicesEx(list, NULL, BookmarksNumRecords(appContext));
                    FrmDrawForm(frm);
                    break;

                default:
                    break;
            }
            break;

        case lstSelectEvent:
        {
            MemHandle recHandle;
            char * str;
            recHandle = DmQueryRecord(appContext->bookmarksDb, appContext->listItemOffset + (UInt32) event->data.lstSelect.selection);
            if (recHandle)
            {
                str = MemHandleLock(recHandle);
                appContext->currentWord = dictGetFirstMatching(GetCurrentFile(appContext), str);
                MemHandleUnlock(recHandle);
            }
            SendNewWordSelected();
            FrmReturnToForm(0);
            return true;
        }

        default:
            break;
    }
    return handled;
}

Int16 BookmarksByNameCompare(char * r1, char * r2, Int16 sortOrder, SortRecordInfoPtr /* info1 */, 
	SortRecordInfoPtr /* info2 */, MemHandle /* appInfoH */)
{
    return StrCompare(r1, r2);
}

Err AddBookmark(AppContext* appContext, char * word)
{
    Err err;
    MemHandle recHandle;
    char * str;
    UInt16 pos;
    Boolean recFound;
    Int16 lastSortBySelection;

    lastSortBySelection = appContext->bookmarksSortBySelection;

    // 1. See if we already have the record in bookmarks.
    //    We assume that if the record wasn't found in sorted-by-name database, it doesn't exist in the other.
    //    The sorted-by-name database is quicker to scan, because we can use binary search.
    recFound = false;
    if (OpenBookmarksDB(appContext, BOOKMARKS_SORT_BY_NAME) == errNone)
    {
        pos = DmFindSortPosition(appContext->bookmarksDb, word, NULL, (DmComparF *) BookmarksByNameCompare, 0);
        
        // DmFindSortPosition returns the position there the new record should be placed
        // so if the record exist it's position is (pos - 1)
        if (pos > 0)
            pos--;
        
        recHandle = DmQueryRecord(appContext->bookmarksDb, pos);
        if (recHandle)
        {
            str = MemHandleLock(recHandle);
            if (!StrCompare(word, str))
                recFound = true;
            MemHandleUnlock(recHandle);
        }
    }
    if (recFound)
        return 1;
    // 2. If we haven't fount the record, we add it to databases
    else {
        if (OpenBookmarksDB(appContext, BOOKMARKS_SORT_BY_NAME) == errNone)
        {
            // the record must be put alphabetically, so we must obtain it's position
            pos = DmFindSortPosition(appContext->bookmarksDb, word, NULL, (DmComparF *) BookmarksByNameCompare, 0);
            recHandle = DmNewRecord(appContext->bookmarksDb, &pos, BOOKMARKS_REC_SIZE);
            if (recHandle)
            {
                str = MemHandleLock(recHandle);
                err = DmWrite(str, 0, word, BOOKMARKS_REC_SIZE);
                MemHandleUnlock(recHandle);
                DmReleaseRecord(appContext->bookmarksDb, pos, true);
            }
        }

        if (OpenBookmarksDB(appContext, BOOKMARKS_SORT_BY_TIME) == errNone)
        {
            // the record must be put chronogically, so we simply put it at the end of this database
            pos = dmMaxRecordIndex;
            recHandle = DmNewRecord(appContext->bookmarksDb, &pos, BOOKMARKS_REC_SIZE);
            if (recHandle)
            {
                str = MemHandleLock(recHandle);
                err = DmWrite(str, 0, word, BOOKMARKS_REC_SIZE);
                MemHandleUnlock(recHandle);
                DmReleaseRecord(appContext->bookmarksDb, pos, true);
            }
        }
    }
    appContext->bookmarksSortBySelection = lastSortBySelection;
    OpenBookmarksDB(appContext, appContext->bookmarksSortBySelection);
    return errNone;
}

void DeleteBookmark(AppContext* appContext, char * word)
{
    UInt16 num, i;
    Int16 lastSortBySelection, sortBy;
    MemHandle recHandle;
    char * str;
    
    lastSortBySelection = appContext->bookmarksSortBySelection;

    // for all the sorting databases we iterate, find the record and remove it
    for (sortBy = BOOKMARKS_SORT_BY_FIRST; sortBy <= BOOKMARKS_SORT_BY_LAST; sortBy++)
    {
        if (OpenBookmarksDB(appContext, sortBy) == errNone)
        {
            num = BookmarksNumRecords(appContext);
            for (i = 0; i < num; i++)
            {
                recHandle = DmQueryRecord(appContext->bookmarksDb, i);
                if (recHandle)
                {
                    str = MemHandleLock(recHandle);
                    if (!StrCompare(str, word))
                    {
                        MemHandleUnlock(recHandle);
                        DmRemoveRecord(appContext->bookmarksDb, i);
                    }
                    else
                        MemHandleUnlock(recHandle);
                }
            }
        }
    }
    appContext->bookmarksSortBySelection = lastSortBySelection;
    OpenBookmarksDB(appContext, appContext->bookmarksSortBySelection);
}
