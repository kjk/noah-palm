/**
 * @file cookie_request.h
 * Interface to iNoah cookie request.
 *
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 *
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#ifndef __COOKIE_REQUEST_H__
#define __COOKIE_REQUEST_H__

#include "common.h"

/**
 * Starts a new internet cookie request connection with iNoah server.
 * If any connection is currently in progress it is aborted. This
 * function prepares request, context variable and submits the connection
 * for processing using StartConnection() function. After cookie is received 
 * new word lookup connection is queued with specified word.
 * @param word word to find after cookie is successfully obtained.
 */
extern void StartCookieRequestWithWordLookup(AppContext* appContext, const Char* word);

#endif