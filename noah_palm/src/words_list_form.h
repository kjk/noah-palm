/**
 * @file words_list_form.h
 * Interface to form that shows list of words obtained from server and allows to choose one from them.
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#ifndef __WORDS_LIST_FORM_H__ 
#define __WORDS_LIST_FORM_H__

#include "common.h"

/**
 * Loads form from resources into memory, sets it's event handler and performs additional initialization.
 * @param appContext @c AppContext of current application.
 * @return error code, @c errNone if form is successfully loaded and initialized.
 */
extern Err WordsListFormLoad(AppContext* appContext);

#endif
