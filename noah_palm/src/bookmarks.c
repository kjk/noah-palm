#include "common.h"
#include "bookmarks.h"

Err OpenBookmarksDB(AppContext* appContext, Int16 bookmarksSortBySelection)
{
    Err err;
    LocalID dbID;
    char * bookmarksDBByName = "Noah_bookmarks_n";
    char * bookmarksDBByTime = "Noah_bookmarks_t";
    char * bookmarksDB;

    if (bookmarksSortBySelection == 0)   // we open the bookmarks-sorted-by-name database
    {
        bookmarksDB = bookmarksDBByName;
        Assert(StrLen(bookmarksDB) < dmDBNameLength);
    }
    else if (bookmarksSortBySelection == 1)
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

        default:
            break;
    }
    return handled;
}

void AddBookmark(AppContext* appContext, char * word)
{
}

void DeleteBookmark(AppContext* appContext, char * word)
{
}
