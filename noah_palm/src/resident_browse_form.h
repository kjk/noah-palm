/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/
/**
 * @file resident_browse_form.h
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
 
#ifndef __RESIDENT_BROWSE_FORM_H__ 
#define __RESIDENT_BROWSE_FORM_H__

#include "common.h"

/**
 * Displays <code>formResidentBrowse</code> as modal dialog.
 * Initializes and sets the form's event handler. Then calls FrmDoDialog()
 * to show it and frees resources associated with form afterwards.
 * @param appContext context variable used to keep current state of application.
 * @return error code, <code>errNone</code> if ok.
 */
extern Err PopupResidentBrowseForm(AppContext* appContext);

#endif
