/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Owner: Lukasz Szweda (qkiss@wp.pl)

  support for Find Word Matching Pattern
*/

#ifndef _WMP_H_
#define _WMP_H_

#include "common.h"

#define WMP_REC_PACK_COUNT 15000     // number of records in one pattern record pack
#define WMP_REC_SIZE sizeof(long)   // size of a single record in record pack

Err     OpenMatchingPatternDB(AppContext* appContext);
Err     CloseMatchingPatternDB(AppContext* appContext);
Err     ClearMatchingPatternDB(AppContext* appContext);
Err     ReadPattern(AppContext* appContext, char * pattern);
Err     WritePattern(AppContext* appContext, char * pattern);
Err     ReadMatchingPatternRecord(AppContext* appContext, long pos, long * elem);
Err     WriteMatchingPatternRecord(AppContext* appContext, long elem);
long    NumMatchingPatternRecords(AppContext* appContext);
int     WordMatchesPattern(char * word, char * pattern);
void    FillMatchingPatternDB(AppContext* appContext, char * pattern);
void    PatternListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
Boolean FindPatternFormHandleEvent(EventType* event);

#endif
