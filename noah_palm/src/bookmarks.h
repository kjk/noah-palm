/* support for Bookmarks
   12/12/2003 Lukasz Szweda */

#ifndef _BOOKMARKS_H_
#define _BOOKMARKS_H_

#include <PalmOS.h>

#define Noah10BookmarksByName   0x03210100
#define AppBookmarksByNameId    Noah10BookmarksByName
#define Noah10BookmarksByTime   0x03211100
#define AppBookmarksByTimeId    Noah10BookmarksByTime

#define IsValidBookmarksByNameRecord(recData) (AppBookmarksByNameId == *((long*)(recData)))
#define IsValidBookmarksByTimeRecord(recData) (AppBookmarksByTimeId == *((long*)(recData)))

#define BOOKMARKS_REC_SIZE WORDS_CACHE_SIZE
#define BOOKMARKS_BY_NAME_RECORD_ID_SIZE sizeof(AppBookmarksByNameId)
#define BOOKMARKS_BY_TIME_RECORD_ID_SIZE sizeof(AppBookmarksByNameId)
#define BOOKMARKS_BY_NAME_REC_PACK_COUNT ((0xFF00 - BOOKMARKS_BY_NAME_RECORD_ID_SIZE) / WORDS_CACHE_SIZE)
#define BOOKMARKS_BY_TIME_REC_PACK_COUNT ((0xFF00 - BOOKMARKS_BY_TIME_RECORD_ID_SIZE) / WORDS_CACHE_SIZE)

void AddBookmark(AppContext* appContext, char * word);
void DeleteBookmark(AppContext* appContext, char * word);
Boolean BookmarksFormHandleEvent(EventType * event);
void BookmarksListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);

#endif
