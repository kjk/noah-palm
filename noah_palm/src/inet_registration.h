/**
 * @file inet_registration.h
 * Declarations of public registration functions.
 *
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 *
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#ifndef __INET_REGISTRATION_H__
#define __INET_REGISTRATION_H__

#include "common.h"

/**
 * Starts a new internet registration connection with iNoah server.
 * If any connection is currently in progress it is aborted. This
 * function prepares request, context variable and submits the connection
 * for processing using StartConnection() function. When completed registration
 * it queues another connection that looks up the specified word.
 * @param userId user id which user wants to assign to this device.
 * @param word word to lookup after registration completes successfully.
 */
extern void StartRegistrationWithLookup(AppContext* appContext, const Char* userId, const Char* word);

#endif