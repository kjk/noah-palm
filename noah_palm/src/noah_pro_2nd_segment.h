/**
 * @file noah_pro_2nd_segment.h
 * Declaration of functions that are called only during normal probram runs.
 */
 
#include "common.h"

extern void* SerializePreferencesNoahPro(AppContext* appContext, long *pBlobSize);
extern void SavePreferencesNoahPro(AppContext* appContext);
extern Err InitNoahPro(AppContext* appContext);
extern void StopNoahPro(AppContext* appContext);
extern void DoWord(AppContext* appContext, char *word);
extern Err AppLaunch(char *cmdPBP);
