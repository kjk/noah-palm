/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)

  Implements Virtual File System procedures for the standard, in-memory
  databases.
 */

#include "common.h"
#include "mem_leak.h"
#include "fs.h"
#include "fs_mem.h"

/* iterate over all dbs in internal memory and call pCB for each db that
matches creator/type/name. creator/type/name can be null.
It passes AbstractFile allocated here, caller needs to free it.*/
void FsMemFindDb(UInt32 creator, UInt32 type, char *name, FIND_DB_CB *pCB)
{
    AbstractFile        *file;
    char                dbName[32];
    DmSearchStateType   stateInfo;
    UInt16              cardNo = 0;
    LocalID             dbId;
    UInt32              creatorFound;
    UInt32              typeFound;
    Boolean             fNewSearch = true;
    Err                 err = errNone;

    while (true)
    {
        MemSet(dbName, 32, 0);
        err = DmGetNextDatabaseByTypeCreator(fNewSearch, &stateInfo, type, creator, 0, &cardNo, &dbId);
        fNewSearch = false;

        if ( errNone != err )
            return;

        DmDatabaseInfo(cardNo, dbId, dbName, 0, 0, 0, 0, 0, 0, 0, 0, &typeFound, &creatorFound);

        Assert( typeFound = type );
        Assert( creatorFound = creator );

        file = AbstractFileNewFull( eFS_MEM, creatorFound, typeFound, dbName );
        if (NULL==file) return;
        file->cardNo = cardNo;
        file->dbId = dbId;

        /* call the callback function to notify that matching db has been found */
        (*pCB)(file);
    }
}

struct MemData *memNew(AbstractFile *file)
{
    struct MemData *memData = (struct MemData *)new_malloc_zero(sizeof(struct MemData));
    if (memData)
    {
        memInit(memData,file);
    }
    return memData;
}

void memInit(struct MemData *memData,AbstractFile *file)
{ 
    memData->file = file;
    memData->recsInfo = NULL;
    memData->openDb = 0;
}

void memCloseDb(struct MemData *memData)
{
#ifdef DEBUG
    int i;
    /* make sure that every opened record has been closed */
    for (i = 0; i < memData->recsCount; i++)
    {
        Assert((0 == memData->recsInfo[i].lockCount)
               || (-1 == memData->recsInfo[i].lockCount));
    }
#endif

    if (memData->openDb != 0)
    {
        DmCloseDatabase(memData->openDb);
        memData->openDb = 0;
    }
}


void memDeinit(struct MemData *memData)
{
    memCloseDb(memData);
    if (NULL != memData->recsInfo)
    {
        new_free(memData->recsInfo);
        memData->recsInfo = NULL;
    }
}

/* Open the database. May fail, in which case return false */
Boolean memOpenDb(struct MemData *memData)
{
    UInt16      i;
    Err         err;

    Assert ( 0 == memData->openDb != 0);

    memData->openDb = DmOpenDatabase(memData->file->cardNo, memData->file->dbId, dmModeReadOnly);
    if ( 0 == memData->openDb )
    {
        err = DmGetLastErr();
        return false;
    }
    memData->recsCount = DmNumRecords(memData->openDb);
    memData->recsInfo = (OneMemRecordInfo *) new_malloc(memData->recsCount *sizeof(OneMemRecordInfo));

    if (NULL == memData->recsInfo)
        return false;

    for (i = 0; i < memData->recsCount; i++)
    {
        memData->recsInfo[i].size = -1;
        memData->recsInfo[i].data = NULL;
        memData->recsInfo[i].lockCount = 0;
    }
    return true;
}

/* find the database with a given creator, type and name. Return
true if found, false otherwise. Also sets the database as a
"current" one. */
UInt16 memGetRecordsCount(struct MemData *memData)
{
    return memData->recsCount;
}

void *memLockRecord(struct MemData *memData,UInt16 recNo)
{
    MemHandle recHandle;

    /* LogV1("memLockRecord(%d)", recNo ); */

    if (0 == memData->recsInfo[recNo].lockCount)
    {
        recHandle = DmQueryRecord(memData->openDb, recNo);
        memData->recsInfo[recNo].data = MemHandleLock(recHandle);
        memData->recsInfo[recNo].size = MemHandleSize(recHandle);
    }
    ++memData->recsInfo[recNo].lockCount;
    return memData->recsInfo[recNo].data;
}

void memUnlockRecord(struct MemData *memData, UInt16 recNo)
{
    MemHandle recHandle;
    Assert(memData->recsInfo[recNo].lockCount >= 0);

    /* LogV1("memUnlockRecord(%d)", recNo ); */

    --memData->recsInfo[recNo].lockCount;
    if (0 == memData->recsInfo[recNo].lockCount)
    {
        recHandle = DmQueryRecord(memData->openDb, recNo);
        MemHandleUnlock(recHandle);
    }
}

long memGetRecordSize(struct MemData *memData, UInt16 recNo)
{
    if (-1 == memData->recsInfo[recNo].size)
    {
        memLockRecord(memData, recNo);
        memUnlockRecord(memData, recNo);
    }
    return memData->recsInfo[recNo].size;
}

void *memLockRegion(struct MemData *memData, UInt16 recNo, UInt16 offset, UInt16 size)
{
    char        *recordPtr;

    recordPtr = (char *) memLockRecord(memData, recNo);
    return (void *) (recordPtr + offset);
}

void memUnlockRegion(struct MemData *memData, char *regionPtr)
{
    UInt16      i;
    char        *recDataStart;
    char        *recDataEnd;

    for (i = 0; i < memData->recsCount; i++)
    {
        recDataStart = (char *) memData->recsInfo[i].data;
        if (NULL != recDataStart)
        {
            recDataEnd = recDataStart + memData->recsInfo[i].size;
            if ((regionPtr >= recDataStart) && (regionPtr <= recDataEnd))
            {
                memUnlockRecord(memData, i);
                return;
            }
        }
    }
    Assert(false);
    return;
}
