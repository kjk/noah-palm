/* support for Bookmarks
   12/12/2003 Lukasz Szweda */

#ifndef _BOOKMARKS_H_
#define _BOOKMARKS_H_

#include <PalmOS.h>

#define BOOKMARKS_REC_SIZE WORDS_CACHE_SIZE + 1
#define BOOKMARKS_SORT_BY_NAME 0
#define BOOKMARKS_SORT_BY_TIME 1
#define BOOKMARKS_SORT_BY_FIRST BOOKMARKS_SORT_BY_NAME
#define BOOKMARKS_SORT_BY_LAST BOOKMARKS_SORT_BY_TIME

Err AddBookmark(AppContext* appContext, char * word);
void DeleteBookmark(AppContext* appContext, char * word);
Boolean BookmarksFormHandleEvent(EventType * event);
void BookmarksListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data);
Err OpenBookmarksDB(AppContext* appContext, Int16 bookmarksSortBySelection);
Err CloseBookmarksDB(AppContext* appContext);
UInt16 BookmarksNumRecords(AppContext* appContext);
Int16 BookmarksByNameCompare(char * r1, char * r2, Int16 sortOrder, SortRecordInfoPtr /* info1 */, 
	SortRecordInfoPtr /* info2 */, MemHandle /* appInfoH */);

#endif
