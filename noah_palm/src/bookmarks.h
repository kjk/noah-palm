/* support for Bookmarks
   12/12/2003 Lukasz Szweda */

#ifndef _BOOKMARKS_H_
#define _BOOKMARKS_H_

#include <PalmOS.h>

#define BOOKMARKS_REC_SIZE WORDS_CACHE_SIZE

void AddBookmark(AppContext* appContext, char * word);
void DeleteBookmark(AppContext* appContext, char * word);
Boolean BookmarksFormHandleEvent(EventType * event);
void BookmarksListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
Err OpenBookmarksDB(AppContext* appContext, Int16 bookmarksSortBySelection);
Err CloseBookmarksDB(AppContext* appContext);
UInt16 BookmarksNumRecords(AppContext* appContext);

#endif
