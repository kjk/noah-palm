/*
  Copyright (C) 2000,2001, 2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)

  Implements Virtual File System procedures for the standard, in-memory
  databases.
 */
#ifdef NOAH_LITE
#include "noah_lite.h"
#endif

#ifdef NOAH_PRO
#include "noah_pro.h"
#endif

#ifdef THESAURUS
#include "thes.h"
#endif

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
        Assert( errNone == err );
        if ( errNone != err )
            return;

        DmDatabaseInfo(cardNo, dbId, dbName, 0, 0, 0, 0, 0, 0, 0, 0, &typeFound, &creatorFound);

        Assert( typeFound = type );
        Assert( creatorFound = creator );

        file = AbstractFileNewFull( eFS_MEM, creatorFound, typeFound, dbName );
        if (NULL==file) return;

        /* call the callback function to notify that matching db has been found */
        (*pCB)(file);
    }
}

static void     memDeinit(void *data);

static Err memInit(void *data)
{
    MemData     *memData = (MemData*) data;
 
    memData->iterationInitedP = false;
    memData->recsInfo = NULL;
    memData->openDb = 0;
    return ERR_NONE;
}

static void memCloseDb(void *data)
{
    MemData     *memData = (MemData*) data;

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


static void memDeinit(void *data)
{
    MemData     *memData = (MemData*) data;

    memCloseDb(data);
    if (NULL != memData->recsInfo)
    {
        new_free(memData->recsInfo);
        memData->recsInfo = NULL;
    }
}

/* Open the database. May fail, in which case return non-zero value */
static int memOpenDb(void *data)
{
    UInt16      i;
    Err         err;
    MemData     *memData = (MemData*) data;

    if (memData->openDb != 0)
    {
        return 0;
    }
    //memData->openDb = DmOpenDatabase(memData->cardNo, memData->dbId, dmModeReadWrite);
    memData->openDb = DmOpenDatabase(memData->cardNo, memData->dbId, dmModeReadOnly);
    if ( 0 == memData->openDb )
    {
        err = DmGetLastErr();
        return 1;
    }
    memData->recsCount = DmNumRecords(memData->openDb);
    memData->recsInfo = (OneMemRecordInfo *) new_malloc(memData->recsCount *sizeof(OneMemRecordInfo));

    if (NULL == memData->recsInfo)
    {
        return 1;
    }

    for (i = 0; i < memData->recsCount; i++)
    {
        memData->recsInfo[i].size = -1;
        memData->recsInfo[i].data = NULL;
        memData->recsInfo[i].lockCount = 0;
    }
    return 0;
}

static UInt32 memGetDbCreator(void *data)
{
    MemData     *memData = (MemData*) data;
    return memData->dbCreator;
}

static UInt32 memGetDbType(void *data)
{
    MemData     *memData = (MemData*) data;
    return memData->dbType;
}

static char *memGetDbName(void *data)
{
    MemData     *memData = (MemData*) data;
    return memData->name;
}

/* Init iteration over files in TRGPro CF.
   Return: false if failed somewhere. */
static void memIterateRestart(void *data)
{
    MemData     *memData = (MemData*) data;
    memData->iterationInitedP = false;
}

/* Iterate over files on TRGPro CF, but only in top-most directory.
   Return false if no more files.
 */
static Boolean memInterateOverDbs(void *data)
{
    Err err;
    MemData     *memData = (MemData*) data;

    MemSet(&memData->name, 32, 0);
    if (false == memData->iterationInitedP)
    {
        err = DmGetNextDatabaseByTypeCreator(1, &memData->stateInfo, 0, 0,
                                             0, &(memData->cardNo),
                                             &(memData->dbId));
        memData->iterationInitedP = true;
    }
    else
    {
        err = DmGetNextDatabaseByTypeCreator(0, &memData->stateInfo, 0, 0,
                                             0, &(memData->cardNo),
                                             &(memData->dbId));
    }

    if (0 != err)
    {
        return false;
    }
    DmDatabaseInfo(memData->cardNo, memData->dbId, memData->name,
                   0, 0, 0, 0, 0, 0, 0, 0, &(memData->dbType),
                   &(memData->dbCreator));
    return true;
}

/* find the database with a given creator, type and name. Return
true if found, false otherwise. Also sets the database as a
"current" one. */
static UInt16 memGetRecordsCount(void *data)
{
    MemData     *memData = (MemData*) data;

    if ( 0 == memData->openDb )
        memOpenDb(data);
    return memData->recsCount;
}

static void *memLockRecord(void *data,UInt16 recNo)
{
    MemHandle recHandle;
    MemData     *memData = (MemData*) data;

    if ( 0 == memData->openDb )
    {
        if ( 0 != memOpenDb(data) )
            return NULL;
    }
    if (0 == memData->recsInfo[recNo].lockCount)
    {
        recHandle = DmQueryRecord(memData->openDb, recNo);
        memData->recsInfo[recNo].data = MemHandleLock(recHandle);
        memData->recsInfo[recNo].size = MemHandleSize(recHandle);
    }
    ++memData->recsInfo[recNo].lockCount;
    return memData->recsInfo[recNo].data;
}

static void memUnlockRecord(void *data, UInt16 record_no)
{
    MemHandle recHandle;
    MemData     *memData = (MemData*) data;

    Assert(memData->recsInfo[record_no].lockCount >= 0);
    --memData->recsInfo[record_no].lockCount;
    if (0 == memData->recsInfo[record_no].lockCount)
    {
        recHandle = DmQueryRecord(memData->openDb, record_no);
        MemHandleUnlock(recHandle);
    }
}

static long memGetRecordSize(void *data, UInt16 recNo)
{
    MemData     *memData = (MemData*) data;

    if ( 0 == memData->openDb )
        memOpenDb(data);
    if (-1 == memData->recsInfo[recNo].size)
    {
        memLockRecord(data, recNo);
        memUnlockRecord(data, recNo);
    }
    return memData->recsInfo[recNo].size;
}

static void *memLockRegion(void *data, UInt16 recNo, UInt16 offset, UInt16 size)
{
    char        *recordPtr;
    /* MemData     *memData = (MemData*) data; */

    recordPtr = (char *) memLockRecord(data, recNo);
    return (void *) (recordPtr + size);
}

static void memUnlockRegion(void *data, void *regionPtr)
{
    UInt16      i;
    char        *recDataStart;
    char        *recDataEnd;
    MemData     *memData = (MemData*) data;

    for (i = 0; i < memData->recsCount; i++)
    {
        recDataStart = (char *) memData->recsInfo[i].data;
        if (NULL != recDataStart)
        {
            recDataEnd = recDataStart + memData->recsInfo[i].size;
            if (((char *) data >= recDataStart)
                && ((char *) regionPtr <= recDataEnd))
            {
                memUnlockRecord(data, i);
                return;
            }
        }
    }
    Assert(false);
    return;
}

#if 0
static Boolean memCacheRecords(void *data, int recsCount, UInt16 * recs)
{
    return true;
}
#endif

static Boolean  memSetCurrentDict(void *data, DBInfo *dbInfo)
{
    /* MemData     *memData = (MemData*) data; */

    memIterateRestart(data);
    return vfsFindDb( GetCreatorFromVfsType( dbInfo->vfsDbType ),
               GetTypeFromVfsType( dbInfo->vfsDbType ), dbInfo->dbName );
}

void setVfsToMem(Vfs *vfs)
{

    vfs->init = &memInit;
    vfs->deinit = &memDeinit;
    vfs->iterateRestart = &memIterateRestart;
    vfs->iterateOverDbs = &memInterateOverDbs;
    vfs->findDb = &vfsFindDb;

    vfs->getDbCreator = &memGetDbCreator;
    vfs->getDbType = &memGetDbType;
    vfs->getDbName = &memGetDbName;
    vfs->getDbPath = NULL;

    vfs->getRecordsCount = &memGetRecordsCount;
    vfs->getRecordSize = &memGetRecordSize;
    vfs->lockRecord = &memLockRecord;
    vfs->unlockRecord = &memUnlockRecord;
    vfs->lockRegion = &memLockRegion;
    vfs->unlockRegion = &memUnlockRegion;
    //vfs->cacheRecords = &memCacheRecords;

    /* those exists only for external memory */
    vfs->readHeader = NULL;
    vfs->copyExternalToMem = NULL;
    vfs->setCurrentDict = &memSetCurrentDict;
}

