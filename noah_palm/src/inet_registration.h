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
 * for processing using StartConnection() function. 
 * @param serialNumber serial number that user entered.
 */
extern void StartRegistration(AppContext* appContext, const Char* serialNumber);

#endif