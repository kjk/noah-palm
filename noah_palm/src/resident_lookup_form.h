/**
 * @file resident_lookup_form.h
 * Interface of <code>formResidentLookup</code>.
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#ifndef __RESIDENT_LOOKUP_FORM_H__
#define __RESIDENT_LOOKUP_FORM_H__

#include "common.h"

/**
 * Displays <code>formResidentLookup</code> as modal dialog.
 * Initializes and sets the form's event handler. Then calls FrmDoDialog()
 * to show it and frees resources associated with form afterwards.
 * @param appContext context variable used to keep current state of application.
 * @return error code, <code>errNone</code> if ok.
 */
extern Err PopupResidentLookupForm(AppContext* appContext);

#endif