#include "common.h"
#include "bookmarks.h"

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
