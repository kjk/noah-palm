#include "common.h"
#include "bookmarks.h"

Boolean BookmarksFormHandleEvent(EventType * event)
{
    Boolean handled = true;
    FormType * frm = NULL;

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

        default:
            handled = false;
            break;
    }
    return handled;
}