/* support for Find Word Matching Pattern
   29/11/2003 Lukasz Szweda */

#ifndef _WMP_H_
#define _WMP_H_

#include "common.h"

#define WMP_REC_PACK_SIZE 15000
#define WMP_REC_SIZE sizeof(long)

Boolean TappedInRect(RectangleType * rect);
Err OpenMatchingPatternDB(AppContext* appContext);
Err CloseMatchingPatternDB(AppContext* appContext);
Err ClearMatchingPatternDB(AppContext* appContext);
Err ReadPattern(AppContext* appContext, char * pattern);
Err WritePattern(AppContext* appContext, char * pattern);
Err ReadMatchingPatternRecord(AppContext* appContext, long pos, long * elem);
Err WriteMatchingPatternRecord(AppContext* appContext, long elem);
long NumMatchingPatternRecords(AppContext* appContext);
int WordMatchesPattern(char * word, char * pattern);
void FillMatchingPatternDB(AppContext* appContext, char * pattern);
void PatternListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);

#endif
