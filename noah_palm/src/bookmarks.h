/* support for Bookmarks
   12/12/2003 Lukasz Szweda */

#ifndef _BOOKMARKS_H_
#define _BOOKMARKS_H_

#include <PalmOS.h>

#define Noah10Bookmarks   0x03211000
#define AppBookmarksId    Noah10Bookmarks

#define IsValidBookmarksRecord(recData) (AppBookmarksId==*((long*)(recData)))

void AddBookmark(AppContext* appContext, char * word);
void DeleteBookmark(AppContext* appContext, char * word);
Boolean BookmarksFormHandleEvent(EventType * event);

#endif
