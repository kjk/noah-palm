/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Andrzej Ciarkowski
*/

#ifndef __INET_WORD_LOOKUP_H__
#define __INET_WORD_LOOKUP_H__

#include "common.h"

extern void StartWordLookup(AppContext* appContext, const Char* word);
extern void StartRandomWordLookup(AppContext* appContext);

#ifdef DEBUG
void testParseResponse(char *txt);
#endif

#endif
