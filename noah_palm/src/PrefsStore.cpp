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
* 2-byte unique id
* 2-byte type of an item (bool, int, string)
* bool is 1 byte (0/1)
* UInt16 is 2-byte unsigned int
* UInt32 is 4-byte unsigned int
* string is: 4-byte length of the string (including terminating 0) followed
  by string characters (also including terminating 0)
*/

PrefsStoreReader::PrefsStoreReader(char *dbName, UInt32 dbCreator, UInt32 dbType)
    : _dbName(dbName), _dbCreator(dbCreator), _dbType(dbType), _db(0),
      _recHandle(NULL), _recData(NULL)
{
    Assert(dbName);
    Assert(StrLen(dbName) < dmDBNameLength);
}

PrefsStoreReader::~PrefsStoreReader()
{
    if (_recHandle)
        MemHandleLock(_recHandle);
    if (_db)
        DmCloseDatabase(_db);
}

#define PREFS_STORE_RECORD_ID "aRSp"  // comes from "ArsLexis preferences"
#define FValidPrefsStoreRecord(recData) (0==MemCmp(recData,PREFS_STORE_RECORD_ID,StrLen(PREFS_STORE_RECORD_ID)))

// Open preferences database and find a record that contains preferences.
// Return errNone if opened succesfully, otherwise an error:
//   psErrNoPrefDatabase - pref database couldn't be found
// devnote: it scans through all records even though we only store preferences
// in one record because I want to be able to use preferences database used
// in earlier versions of Noah Pro/Thes.
Err PrefsStoreReader::ErrOpenPrefsDatabase()
{
    if (_db)
    {
        Assert(_recHandle);
        Assert(_recData);
        return errNone;
    }

    LocalID dbId;
    Err err = ErrFindDatabaseByNameTypeCreator(_dbName, _dbType, _dbCreator, &dbId);
    if (dmErrCantFind==err)
        return psErrNoPrefDatabase;
    if (err)
        return err;
    Assert(0!=dbId);

    _db = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (!_db)
        return DmGetLastErr();

    UInt16 recsCount = DmNumRecords(_db);
    for (UInt16 recNo = 0; recNo < recsCount; recNo++)
    {
        _recHandle = DmQueryRecord(_db, recNo);
        _recData = (unsigned char*)MemHandleLock(_recHandle);
        if ( (MemHandleSize(_recHandle)>=4) && FValidPrefsStoreRecord(_recData) )
        {
            // we found the record with prefernces so remember _recData and _recHandle
            // those must be freed in destructor
            return errNone;
        }
        MemHandleUnlock(_recHandle);
        _recHandle = NULL;
    }
    return psErrNoPrefDatabase;  // should it be more specific psErrNoPrefRecord?
}

static char *deserStringInPlace(unsigned char **data, long *pCurrBlobSize)
{
    Assert(*pCurrBlobSize>=2);
    if (*pCurrBlobSize<2)
        return NULL;
    int strLen = deserInt( data, pCurrBlobSize );
    Assert(0 == (*data)[strLen-1]);
    if (0!=(*data)[strLen-1])
        return NULL;
    char * str = (char*)*data;
    *data += strLen;
    *pCurrBlobSize -= strLen;
    return str;
}

Err PrefsStoreReader::ErrGetPrefItemWithId(int uniqueId, PrefItem *prefItem)
{
    Assert(prefItem);

    Err err = ErrOpenPrefsDatabase();
    if (err)
        return err;

    Assert(_db);
    Assert(_recHandle);
    Assert(_recData);

    // usually when we Assert() we don't error out on the same condition
    // but in this case, while highly improbably, it's conceivable that some
    // other app created a database with the same name, creator, type and a
    // record that has the same magic header and we don't want to crash
    // in this case
    long recSizeLeft = (long)MemHandleSize(_recHandle);
    Assert(recSizeLeft>=4);
    if (recSizeLeft<4)
        return psErrDatabaseCorrupted;
    unsigned char *currData = _recData;
    Assert(FValidPrefsStoreRecord(currData));
    // skip the header
    currData += 4;
    recSizeLeft-=4;
    while(recSizeLeft!=0)
    {
        // get unique id and type
        Assert(recSizeLeft>=2);
        if (recSizeLeft<2)
            return psErrDatabaseCorrupted;
        int id = deserInt(&currData,&recSizeLeft);
        Assert(recSizeLeft>=2);
        if (recSizeLeft<2)
            return psErrDatabaseCorrupted;
        PrefItemType type = (PrefItemType)deserInt(&currData,&recSizeLeft);
        switch (type)
        {
            case pitBool:
                Assert(recSizeLeft>=1);
                if (recSizeLeft<1)
                    return psErrDatabaseCorrupted;
                unsigned char boolVal = deserByte(&currData,&recSizeLeft);
                if (0==boolVal)
                    prefItem->value.boolVal=false;
                if (1==boolVal)
                    prefItem->value.boolVal=true;
                return psErrDatabaseCorrupted;
                break;
            case pitUInt16:
                Assert(recSizeLeft>=2);
                if (recSizeLeft<2)
                    return psErrDatabaseCorrupted;
                prefItem->value.uint16Val = (UInt16)deserInt(&currData, &recSizeLeft);
                break;
            case pitUInt32:
                Assert(recSizeLeft>=4);
                if (recSizeLeft<4)
                    return psErrDatabaseCorrupted;
                prefItem->value.uint32Val = (UInt32)deserLong(&currData, &recSizeLeft);
                break;
            case pitStr:
                prefItem->value.strVal = deserStringInPlace(&currData, &recSizeLeft);
                if(NULL==prefItem->value.strVal)
                    return psErrDatabaseCorrupted;\
                break;
            default:
                Assert(0);
                return psErrDatabaseCorrupted;
        }
        if (id==uniqueId)
        {
            prefItem->uniqueId=id;
            prefItem->type=type;
            return errNone;
        }
    }
    return psErrItemNotFound;
}

Err PrefsStoreReader::ErrGetBool(int uniqueId, Boolean *value)
{
    PrefItem    prefItem;
    Err err = ErrGetPrefItemWithId(uniqueId, &prefItem);
    if (err)
        return err;
    Assert(prefItem.uniqueId == uniqueId);
    if (prefItem.type != pitBool)
        return psErrItemTypeMismatch;
    *value = prefItem.value.boolVal;
    return errNone;
}

Err PrefsStoreReader::ErrGetUInt16(int uniqueId, UInt16 *value)
{
    PrefItem    prefItem;
    Err err = ErrGetPrefItemWithId(uniqueId, &prefItem);
    if (err)
        return err;
    Assert(prefItem.uniqueId == uniqueId);
    if (prefItem.type != pitUInt16)
        return psErrItemTypeMismatch;
    *value = prefItem.value.uint16Val;
    return errNone;
}

Err PrefsStoreReader::ErrGetUInt32(int uniqueId, UInt32 *value)
{
    PrefItem    prefItem;
    Err err = ErrGetPrefItemWithId(uniqueId, &prefItem);
    if (err)
        return err;
    Assert(prefItem.uniqueId == uniqueId);
    if (prefItem.type != pitUInt32)
        return psErrItemTypeMismatch;
    *value = prefItem.value.uint32Val;
    return errNone;
}

// the string returned points to data inside the object that is only valid
// while the object is alive. If client wants to use it after that, he must
// make a copy
Err PrefsStoreReader::ErrGetStr(int uniqueId, char **value)
{
    PrefItem    prefItem;
    Err err = ErrGetPrefItemWithId(uniqueId, &prefItem);
    if (err)
        return err;
    Assert(prefItem.uniqueId == uniqueId);
    if (prefItem.type != pitStr)
        return psErrItemTypeMismatch;
    *value = prefItem.value.strVal;
    return errNone;
}

PrefsStoreWriter::PrefsStoreWriter(char *dbName, UInt32 dbCreator, UInt32 dbType)
    : _dbName(dbName), _dbCreator(dbCreator), _dbType(dbType), _itemsCount(0)
{
    Assert(dbName);
    Assert(StrLen(dbName) < dmDBNameLength);
}

PrefsStoreWriter::~PrefsStoreWriter()
{
}

static PrefItem * FindPrefItemById(PrefItem *items, int itemsCount, int uniqueId)
{
    for(int i=0; i<itemsCount; i++)
    {
        if (items[i].uniqueId==uniqueId)
            return &(items[i]);
    }
    return NULL;
}

Err PrefsStoreWriter::ErrSetItem(PrefItem *item)
{
    if ( NULL != FindPrefItemById((PrefItem*)_items, _itemsCount, item->uniqueId) )
    {
        Assert(0); // we assert because we never want this to happen
        return psErrDuplicateId;
    }

    // TODO: make it grow dynamically in the future
    if (_itemsCount>=MAX_PREFS_ITEMS)
        return memErrNotEnoughSpace;

    _items[_itemsCount++] = *item;
    return errNone;
}

Err PrefsStoreWriter::ErrSetBool(int uniqueId, Boolean value)
{
    PrefItem    prefItem;

    prefItem.type = pitBool;
    prefItem.uniqueId = uniqueId;
    prefItem.value.boolVal = value;

    return ErrSetItem( &prefItem );
}

Err PrefsStoreWriter::ErrSetUInt16(int uniqueId, UInt16 value)
{
    PrefItem    prefItem;

    prefItem.type = pitUInt16;
    prefItem.uniqueId = uniqueId;
    prefItem.value.uint16Val = value;

    return ErrSetItem( &prefItem );
}

Err PrefsStoreWriter::ErrSetUInt32(int uniqueId, UInt32 value)
{
    PrefItem    prefItem;

    prefItem.type = pitUInt32;
    prefItem.uniqueId = uniqueId;
    prefItem.value.uint32Val = value;

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

// Create a blob containing serialized preferences.
// Devnote: caller needs to free memory returned.
// TODO: move ser* (serData etc.) functions from common.c to here
// after changing prefs in all apps to use PrefsStore
static void* SerializeItems(PrefItem *items, int itemsCount, long *pBlobSize)
{
    Assert(items);
    Assert(itemsCount>=0);
    Assert(pBlobSize);

    char *  prefsBlob = NULL;
    long    blobSize, blobSizePhaseOne;
    /* phase one: calculate the size of the blob */
    /* phase two: create the blob */
    for(int phase=1; phase<=2; phase++)
    {
        blobSize = 0;
        Assert( 4 == StrLen(PREFS_STORE_RECORD_ID) );

        serData( (char*)PREFS_STORE_RECORD_ID, StrLen(PREFS_STORE_RECORD_ID), prefsBlob, &blobSize );
        for(int item=0; item<itemsCount; item++)
        {
            serInt( items[item].uniqueId, prefsBlob, &blobSize);
            serInt( (int)items[item].type, prefsBlob, &blobSize);
            switch( items[item].type )
            {
                case pitBool:
                    if (true==items[item].value.boolVal)
                        serByte(1, prefsBlob, &blobSize);
                    else
                        serByte(0, prefsBlob, &blobSize);
                    break;
                case pitUInt16:
                    serInt( (int)items[item].value.uint16Val, prefsBlob, &blobSize);
                    break;
                case pitUInt32:
                    serLong( (long)items[item].value.uint32Val, prefsBlob, &blobSize);
                    break;
                case pitStr:
                    serString( items[item].value.strVal, prefsBlob, &blobSize);
                    break;
                default:
                    Assert(0);
                    break;
            }            
        }

        if ( 1 == phase )
        {
            Assert( blobSize > 0 );
            blobSizePhaseOne = blobSize;
            prefsBlob = (char*)new_malloc( blobSize );
            if (NULL == prefsBlob)
                return NULL;
        }
    }
    Assert( blobSize == blobSizePhaseOne );
    Assert( blobSize >= 4 );

    *pBlobSize = blobSize;
    Assert( prefsBlob );
    return prefsBlob;
}

// Save preferences previously set via ErrSet*() calls to a database.
// If something goes wrong, returns an error
// Possible errors:
//   memErrNotEnoughSpace - not enough memory to allocate needed structures
//   errors from Dm*() calls
Err PrefsStoreWriter::ErrSavePreferences()
{
    Err     err = errNone;
    long    blobSize;
    void *  prefsBlob = SerializeItems(_items, _itemsCount, &blobSize);
    if ( NULL == prefsBlob ) 
        return memErrNotEnoughSpace;

    DmOpenRef db = DmOpenDatabaseByTypeCreator(_dbType, _dbCreator, dmModeReadWrite);
    if (!db)
    {
        err = DmCreateDatabase(0, _dbName, _dbCreator, _dbType, false);
        if ( err)
            return err;

        db = DmOpenDatabaseByTypeCreator(_dbType, _dbCreator, dmModeReadWrite);
        if (!db)
            return DmGetLastErr();
    }

    UInt16    recNo = 0;
    UInt16    recsCount = DmNumRecords(db);
    MemHandle recHandle;
    Boolean   fRecordBusy = false;
    Boolean   fRecFound = false;
    void *    recData;
    long      recSize;
    while (recNo < recsCount)
    {
        recHandle = DmGetRecord(db, recNo);
        fRecordBusy = true;
        recData = MemHandleLock(recHandle);
        recSize = MemHandleSize(recHandle);
        if (IsValidPrefRecord(recData))
        {
            fRecFound = true;
            break;
        }
        MemPtrUnlock(recData);
        DmReleaseRecord(db, recNo, true);
        fRecordBusy = false;
        ++recNo;
    }

    if (fRecFound && blobSize>recSize)
    {
        /* need to resize the record */
        MemPtrUnlock(recData);
        DmReleaseRecord(db,recNo,true);
        fRecordBusy = false;
        recHandle = DmResizeRecord(db, recNo, blobSize);
        if ( NULL == recHandle )
            return DmGetLastErr();
        recData = MemHandleLock(recHandle);
        Assert( MemHandleSize(recHandle) == blobSize );        
    }

    if (!fRecFound)
    {
        recNo = 0;
        recHandle = DmNewRecord(db, &recNo, blobSize);
        if (!recHandle)
        {
            err = DmGetLastErr();
            goto CloseDbExit;
        }
        recData = MemHandleLock(recHandle);
        fRecordBusy = true;
    }

    err = DmWrite(recData, 0, prefsBlob, blobSize);
    MemPtrUnlock(recData);
    if (fRecordBusy)
        DmReleaseRecord(db, recNo, true);
CloseDbExit:
    // if had error before - preserve that error
    // otherwise return error code from DmCloseDatabase()
    if (err)
        DmCloseDatabase(db);
    else
        err = DmCloseDatabase(db);
    new_free( prefsBlob );
    return err;
}
