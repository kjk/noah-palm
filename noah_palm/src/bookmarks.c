#include "cw_defs.h"

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

#include "common.h"
#include "word_compress.h"

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