/*
  Copyright (C) Krzysztof Kowalczyk
  Owner: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/

#include "common.h"
#include "PrefsStore.hpp"

/*
The idea is to provide an API for storing/reading preferences
in a Palm database in a format that is easily upgradeable.
Each item in a preferences database has a type (int, bool, string)
and unique id. That way, when we add new items we want to store in
preferences database we just create a new unique id. The saving part
just needs to be updated to save a new item, reading part must be updated
to ignore the case when a preference item is missing.

We provide separate classes for reading and writing preferences
because they're used in that way.

To save preferences:
* construct PrefsStoreWriter object
* call ErrSet*() to set all preferences items
* call ErrSavePreferences()

To read preferences:
* construct PrefsStoreReader object
* call ErrGet*() to get desired preferences items

Serialization format:
* data is stored as blob in one record in a database whose name, creator and
  type are provided by the caller
* first 4-bytes of a blob is a header (to provide some protection against
  reading stuff we didn't create)
* then we have each item serialized

Serialization of an item:
* 4-byte unique id
* 2-byte type of an item (bool, int, string)
* bool is 1 byte (0/1)
* int is 4-byte unsigned int
* string is: 4-byte length of the string (including terminating 0) followed
  by string characters (also including terminating 0)
*/

static PrefItem * FindPrefItemById(PrefItem *items, int itemsCount, int uniqueId)
{
    for(int i=0; i<itemsCount; i++)
    {
        if (items[i].uniqueId==uniqueId)
            return &(items[i]);
    }
    return NULL;
}

PrefsStoreReader::PrefsStoreReader(char *dbName, UInt32 dbCreator, UInt32 dbType)
    : _dbName(dbName), _dbCreator(dbCreator), _dbType(dbType)
{
    Assert( dbName );
    Assert( StrLen(dbName) < 32 );
}

Err PrefsStoreReader::ErrGetInt(int uniqueId, int *value)
{
    *value = 0;
    return errNone;
}

// the string returned points to data inside the object that is only valid
// while the object is alive. If client wants to use it after that, he must
// make a copy
Err PrefsStoreReader::ErrGetStr(int uniqueId, char **value)
{
    *value = NULL;
    return errNone;
}

PrefsStoreWriter::PrefsStoreWriter(char *dbName, UInt32 dbCreator, UInt32 dbType)
    : _dbName(dbName), _dbCreator(dbCreator), _dbType(dbType), _itemsCount(0)
{
    Assert( dbName );
    Assert( StrLen(dbName) < 32 );
}

PrefsStoreWriter::~PrefsStoreWriter()
{
}

Err PrefsStoreWriter::ErrSetItem(PrefItem *item)
{
    if ( NULL != FindPrefItemById((PrefItem*)_items, _itemsCount, item->uniqueId) )
        return psErrDuplicateId;

    // TODO: make it grow dynamically in the future
    if (_itemsCount>=MAX_PREFS_ITEMS)
        return psErrNoMem; // TODO: change it to standard out-of-mem error code

    _items[_itemsCount++] = *item;
    return errNone;
}

Err PrefsStoreWriter::ErrSetInt(int uniqueId, int value)
{
    PrefItem    prefItem;

    prefItem.type = pitInt;
    prefItem.uniqueId = uniqueId;
    prefItem.value.intVal = value;

    return ErrSetItem( &prefItem );
}

// value must point to a valid location during ErrSavePrefernces() since
// for perf reasons we don't make a copy of the string
Err PrefsStoreWriter::ErrSetStr(int uniqueId, char *value)
{
    PrefItem    prefItem;

    prefItem.type = pitStr;
    prefItem.uniqueId = uniqueId;
    prefItem.value.strVal = value;

    return ErrSetItem( &prefItem );
}

Err PrefsStoreWriter::ErrSetBool(int uniqueId, Boolean value)
{
    PrefItem    prefItem;

    prefItem.type = pitBool;
    prefItem.uniqueId = uniqueId;
    prefItem.value.boolVal = value;

    return ErrSetItem( &prefItem );
}

Err PrefsStoreWriter::ErrSavePreferences()
{

    return errNone;
}
