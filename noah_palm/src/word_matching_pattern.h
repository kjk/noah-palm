/* support for Find Word Matching Pattern
   29/11/2003 Lukasz Szweda */

#ifndef _WMP_H_
#define _WMP_H_

#include <PalmOS.h>

#define WMP_REC_PACK_SIZE 15000
#define WMP_REC_SIZE sizeof(long)

Boolean TappedInRect(RectangleType * rect);
Err OpenMatchingPatternDB();
Err CloseMatchingPatternDB();
Err ReadPattern(char * pattern);
Err WritePattern(char * pattern);
Err ReadMatchingPatternRecord(long pos, long * elem);
Err WriteMatchingPatternRecord(long elem);
long NumMatchingPatternRecords();
int WordMatchesPattern(char * word, char * pattern);
void FillMatchingPatternDB(char * pattern);
void PatternListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);

#endif
