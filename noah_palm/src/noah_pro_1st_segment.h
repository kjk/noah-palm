/**
 * @file noah_pro_1st_segment.h
 * Declarations of functions called from noah_pro_2nd_segment.c that are contained in noah_pro.c
 */
 
 #include "common.h"
 
extern Err AppCommonInit(AppContext* appContext);
extern Err AppCommonFree(AppContext* appContext);
extern void RemoveNonexistingDatabases(AppContext* appContext);
extern void ScanForDictsNoahPro(AppContext* appContext, Boolean fAlwaysScanExternal);
extern Boolean DictInit(AppContext* appContext, AbstractFile *file);
extern void DictCurrentFree(AppContext* appContext);
