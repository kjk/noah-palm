#include "common.h"
#include "bookmarks.h"

void BookmarksListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
/*
    char        *str;
    Int16       stringWidthP = 160; // max width of the string in the list selection window
    Int16       stringLenP;
    Boolean     truncatedP = false;
    long        realItemNo;
    AppContext* appContext=GetAppContext();

    Assert(itemNum >= 0);
    str = dictGetWord(GetCurrentFile(appContext), realItemNo);
    stringLenP = StrLen(str);

    FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
    WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
*/
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
            LstSetDrawFunction(list, BookmarksListDrawFunc);
            FrmDrawForm(frm);
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
