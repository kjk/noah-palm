/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Owner: Krzysztof Kowalczyk

  Support for word bookmarks in Noah Pro and Thesaurus
*/

#ifndef _BOOKMARKS_H_
#define _BOOKMARKS_H_

#include <PalmOS.h>

/* those are IDs of list items in listSortBy list in formBookmarks */
#define BKM_LST_SEL_NAME 0
#define BKM_LST_SEL_TIME 1

/* Words can be sorted by name or time when they were bookmarked.
   We make enums equal to list item ids to simplify the code. */
typedef enum 
{
    bkmSortByName = BKM_LST_SEL_NAME,
    bkmSortByTime = BKM_LST_SEL_TIME,
    bkmInvalid
} BookmarkSortType;

/* Unfortunately I don't know how to solve the problem of AppContext refering
   to BookmarkSortType which is defined here and depends on knowing AppContext
   other than by forward reference. */
struct _AppContext;

Err     AddBookmark(struct _AppContext* appContext, char * word);
void    DeleteBookmark(struct _AppContext* appContext, char * word);
Boolean BookmarksFormHandleEvent(EventType * event);
Err     CloseBookmarksDB(struct _AppContext* appContext);
UInt16  GetBookmarksCount(struct _AppContext *appContext);

Err BookmarksFormLoad(struct _AppContext* appContext);

#endif
