/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "common.h"
#include "fs.h"
#include "fs_ex.h"

/* Remove the cache database */
void dcDelCacheDb(void)
{
    LocalID     dbId = 0;
    dbId = DmFindDatabase(0, VFS_CACHE_DB_NAME);
    if (0 == dbId)
        return;

    DmDeleteDatabase(0, dbId );
}

struct DbCacheData *dcNew(AbstractFile *file, UInt32 cacheCreator)
{
    struct DbCacheData *data = (struct DbCacheData*) new_malloc_zero( sizeof(struct DbCacheData) );
    if (NULL==data) return NULL;
    Assert( 0 != cacheCreator );
    data->cacheCreator = cacheCreator;
    dcInit(data);
    if (!vfsInitCacheData(file, data) )
    {
        dcDeinit(data);
        new_free(data);
        LogV1( "dcNew(%s), vfsInitCacheData() failed", file->fileName );
        return NULL;
    }
    LogV1( "dcNew(%s) ok", file->fileName );
    return data;
}

void dcInit(struct DbCacheData *cache)
{
    cache->lockRegionCacheRec = 1;
}

void dcDeinit(struct DbCacheData *cache)
{
#ifdef DEBUG
    int i;
    /* make sure that every opened record has been closed */
    for (i = 0; i < cache->recsCount; i++)
    {
        if ( NULL != cache->recsInfo )
        {
            Assert((0 == cache->recsInfo[i].lockCount)
               || (-1 == cache->recsInfo[i].lockCount));
        }
    }
#endif
    if (NULL != cache->recsInfo)
    {
        new_free(cache->recsInfo);
        cache->recsInfo = NULL;
    }

    if (NULL != cache->cacheDbInfoRec)
    {
        new_free(cache->cacheDbInfoRec);
        cache->cacheDbInfoRec = NULL;
    }

    dcCloseCacheDb(cache);
    LogG( "dcDeinit() ok" );
}

#define LOCK_REGION_REC_SIZE 4096

UInt16 dcGetRecordsCount(struct DbCacheData *cache)
{
    return cache->recsCount;
}

long dcGetRecordSize(struct DbCacheData *cache, UInt16 recNo)
{
    Assert(recNo < cache->recsCount);
    return cache->recsInfo[recNo].size;
}


/* Create a cache db. Assumes that a header info for the
   real database has been read, so we have enough information
   to do the job (number of records and database name)
   Also create the first record.*/
LocalID dcCreateCacheDb(AbstractFile* file, struct DbCacheData *cache)
{
    Err             err = errNone;
    CacheDBInfoRec  *dbFirstRec = NULL;
    MemHandle       recHandle = 0;
    UInt16          recPos = 0;
    UInt32          firstRecSize = 0;
    LocalID         dbId = 0;

    err = DmCreateDatabase(0, VFS_CACHE_DB_NAME, cache->cacheCreator, VFS_CACHE_TYPE, false);
    if (err)
    {
        LogV1( "dcCreateCacheDb(), DmCreateDatabase() failed with err=%d", err );
        goto Error;
    }

    dbId = DmFindDatabase(0, VFS_CACHE_DB_NAME);
    if (0 == dbId)
    {
        LogG( "dcCreateCacheDb(), DmFindDatabase() failed");
        goto Error;
    }

    cache->cacheDbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (0 == cache->cacheDbRef)
    {
        LogG( "dcCreateCacheDb(), DmOpenDatabase() failed");
        goto Error;
    }

    firstRecSize = sizeof(CacheDBInfoRec) + (sizeof(int) * cache->recsCount);
    dbFirstRec = (CacheDBInfoRec *) new_malloc(firstRecSize);

    if (NULL == dbFirstRec)
    {
        LogG( "dcCreateCacheDb(), new_malloc(firstRecSize) failed" );
        goto Error;
    }

    /* ugly shortcut: sets the mapping real record no->cached record no
       the initial value of 0 means: record has not been cached yet */
    MemSet(dbFirstRec, firstRecSize, 0);

    recHandle = DmNewRecord(cache->cacheDbRef, &recPos, firstRecSize);
    if (0 == recHandle)
    {
        LogG( "dcCreateCacheDb(), DmNewRecord() failed");
        goto Error;
    }

    Assert(0 == recPos);

    err = DmReleaseRecord(cache->cacheDbRef, recPos, false);
    if (err)
    {
        LogV1( "dcCreateCacheDb(), DmReleaseRecord() failed with err=%d", err );
        goto Error;
    }

    dbFirstRec->signature = VFS_CACHE_DB_SIG_V1;
    dbFirstRec->realDbRecsCount = fsGetRecordsCount(file);

    err = dcUpdateFirstCacheRec(file, cache, dbFirstRec);
    if (err)
    {
        LogV1( "dcCreateCacheDb(), dcUpdateFirstCacheRec() failed with err=%d", err );
        goto Error;
    }

    cache->cacheDbInfoRec = dbFirstRec;
    cache->recRealCachedNoMap = (UInt16 *) (((char *) dbFirstRec) + sizeof(CacheDBInfoRec));

    recPos = dmMaxRecordIndex;
    recHandle = DmNewRecord(cache->cacheDbRef, &recPos, LOCK_REGION_REC_SIZE);
    if (0 == recHandle)
    {
        goto Error;
    }

    Assert(1 == recPos);

    err = DmReleaseRecord(cache->cacheDbRef, recPos, false);
    if (err)
    {
        goto Error;
    }

    cache->lockRegionCacheRec = recPos;
    LogG( "dcCreateCacheDb(): ok");
    return errNone;

Error:
    LogG( "dcCreateCacheDb(): failed");
    if (0 != cache->cacheDbRef)
    {
        DmCloseDatabase(cache->cacheDbRef);
        cache->cacheDbRef = 0;
    }

    if (NULL != dbFirstRec)
    {
        new_free(dbFirstRec);
    }
    /* return some error */
    err = DmGetLastErr();
    if (0 == err)
    {
        err = 1;
    }
    return err;
}


/* Open/create the cache db.
   First see if cache db has already been created
   earlier. If not create it.
   If yes but for different database delete and and create new.
   If exist read info for cacheDbInfoRec
 */
Err dcCacheDbRef(AbstractFile* file, struct DbCacheData *cache)
{
    Err             err = errNone;
    LocalID         dbId = 0;
    MemHandle       recHandle;
    CacheDBInfoRec  *dbFirstRec;
    CacheDBInfoRec  *firstRecMem;
    UInt32          firstRecSize = 0;
    UInt32          reslFirstRecSize = 0;

    /* check if not already opened */
    if (0 != cache->cacheDbRef)
    {
        return errNone;
    }

    dbId = DmFindDatabase(0, VFS_CACHE_DB_NAME);
    if (0 == dbId)
    {
        /* no such database, create */
        err = dcCreateCacheDb(file, cache);
        if (errNone != err)
        {
            LogV1("dcCacheDbRef(), dcCreateCacheDb() failed with err=%d", err );
            goto Error;
        }
        return errNone;
    }

    cache->cacheDbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (0 == cache->cacheDbRef)
    {
        LogG("dcCacheDbRef(), DmOpenDatabase() failed" );
        goto Error;
    }
    recHandle = DmQueryRecord(cache->cacheDbRef, 0);
    dbFirstRec = (CacheDBInfoRec*) MemHandleLock(recHandle);
    if (NULL == dbFirstRec)
    {
        LogG("dcCacheDbRef(), MemHandleLock(recHandle) failed" );
        goto Error;
    }
    reslFirstRecSize = MemHandleSize(recHandle);
    firstRecSize = sizeof(CacheDBInfoRec) + (sizeof(int) * fsGetRecordsCount(file));

    /* is this a correct record? */
    if ((VFS_CACHE_DB_SIG_V1 == dbFirstRec->signature) &&
        (dbFirstRec->realDbRecsCount == fsGetRecordsCount(file)) &&
        (reslFirstRecSize == firstRecSize))
    {
        /* yes, it's a good cache database */
        firstRecMem = (CacheDBInfoRec *) new_malloc(firstRecSize);
        if (NULL == firstRecMem)
        {
            LogV1("dcCacheDbRef(), new_malloc(firstRecSize=%ld) failed", (long) firstRecSize );
            goto Error;
        }
        MemMove(firstRecMem, dbFirstRec, firstRecSize);
        cache->cacheDbInfoRec = firstRecMem;
        cache->recRealCachedNoMap = (UInt16 *)(((char *) firstRecMem) + sizeof(CacheDBInfoRec));
        MemHandleUnlock(recHandle);
    }
    else
    {
        /* no, it isn't so delete and re-create */
        MemHandleUnlock(recHandle);
        DmCloseDatabase(cache->cacheDbRef);
        cache->cacheDbRef = 0;
        err = DmDeleteDatabase(0, dbId);
        if (err)
        {
            LogV1("dcCacheDbRef(), DmDeleteDatabase() failed with err=%d", err );
            goto Error;
        }
        err = dcCreateCacheDb(file, cache);
        if (errNone != err)
        {
            LogV1("dcCacheDbRef(), dcCreateCacheDb() failed with err=%d", err );
            goto Error;
        }
    }
    return errNone;
Error:
    LogG( "dcCacheDbRef(): failed");
    if (errNone == err)
    {
        err = DmGetLastErr();
        if (errNone == 0)
        {
            err = 1;
        }
    }
    return err;
}

void dcCloseCacheDb(struct DbCacheData *cache)
{
    if (cache->cacheDbRef != 0)
    {
        DmCloseDatabase(cache->cacheDbRef);
        cache->cacheDbRef = 0;
    }
}

/*
Find the record that should be deleted.

I should implement some fancy eviction strategy, but for now let's just delete
the first available one (counting from the end).

Returns a record number to delete or -1 if there is no record available to
delete.
*/
static int dcFindRecordToDelete(struct DbCacheData *cache)
{

    int    i;

    for(i=cache->recsCount-1; i>=0; i--)
    {
        if ( (0 != cache->recRealCachedNoMap[i]) && (0==cache->recsInfo[i].lockCount) )
        {
            return i;
        }
    }
    return -1;
}

/*
Cache a record from a database in external memory (CF etc.) in internal memory.
Try to create a record in the cache database of the same size. If can't
create a record assume it's lack of memory so delete unsued record from the
cache database and re-try creating a cached record. Repeat until create succeeds
or there are no more unused records in cache database to delete in which case
return an error.

How to choose a record to delete: Last Recently Used? Least Frequently Used?

*/
Err dcCacheRecord(AbstractFile* file, struct DbCacheData *cache, UInt16 recNo)
{
    Err             err = errNone;
    UInt16          recPos = dmMaxRecordIndex;
    UInt32          recSize;
    MemHandle       recHandle;
    char            *recData;
    UInt32          offsetInPdbFile;
    int             recToDelete, realRecToDelete;

    recSize = cache->recsInfo[recNo].size;
    offsetInPdbFile = cache->recsInfo[recNo].offset;

    while(1)
    {
    
        recHandle = DmNewRecord(cache->cacheDbRef, &recPos, recSize);
        if (0 == recHandle)
        {
            LogG( "dcCacheRecord(): failed to create new record" );
            /* failed to create new record-assume it's because there is no 
            free memory left. try to free some memory by deleting records */
            recToDelete = dcFindRecordToDelete(cache);
            if (-1 == recToDelete )
            {
                err = dmErrMemError;
                goto Error;
            }
            Assert(0==cache->recsInfo[recToDelete].lockCount);
            Assert(0!=cache->recRealCachedNoMap[recToDelete]);
            /* re-try creating a record */
            realRecToDelete = cache->recRealCachedNoMap[recToDelete];
            err = DmDeleteRecord(cache->cacheDbRef, realRecToDelete);
            if ( errNone != err )
            {
                goto Error;
            }
            cache->recRealCachedNoMap[recToDelete] = 0;
            continue;
        }
        err = DmReleaseRecord(cache->cacheDbRef, recPos, false);
        break;
    }

    if (err)
        goto Error;

    recHandle = DmGetRecord(cache->cacheDbRef, recPos);
    if (0 == recHandle)
    {
        LogG( "dcCacheRecord(): DmGetRecord() failed" );
        goto Error;
    }

    recData = (char*) MemHandleLock(recHandle);
    err = vfsCopyExternalToMem( file, offsetInPdbFile, recSize, recData);
    MemHandleUnlock(recHandle);
    DmReleaseRecord(cache->cacheDbRef, recPos, false );
    if (err)
    {
        goto Error;
    }

    cache->recRealCachedNoMap[recNo] = recPos;
    err = dcUpdateFirstCacheRec(file, cache, cache->cacheDbInfoRec);
    if (err)
    {
        goto Error;
    }
    return errNone;

Error:
    LogG( "dcCacheRecord() failed" );
    if (errNone == err)
    {
        err = DmGetLastErr();
    }
    if (errNone == err)
    {
        err = 1;
    }
    return err;
}

/* Update the first record in cache with info in dbFirstRec.
   Assumes that the database is created and open and first record
   already exists. */
Err dcUpdateFirstCacheRec(AbstractFile* file, struct DbCacheData *cache, CacheDBInfoRec * dbFirstRec)
{
    MemHandle   recHandle = 0;
    Err         err = errNone;
    void        *recData = NULL;
    UInt32      firstRecSize = 0;

    recHandle = DmGetRecord(cache->cacheDbRef, 0);
    if (0 == recHandle)
    {
        goto Error;
    }

    recData = MemHandleLock(recHandle);

    firstRecSize = sizeof(CacheDBInfoRec) + (sizeof(int) * fsGetRecordsCount(file));
    DmWrite(recData, 0, (void *) dbFirstRec, firstRecSize);
    MemHandleUnlock(recHandle);
    DmReleaseRecord(cache->cacheDbRef, 0, false);

    return errNone;

  Error:
    err = DmGetLastErr();
    if (0 == err)
    {
        err = 1;
    }
    return err;
}

void *dcLockRecord(AbstractFile* file, struct DbCacheData *cache, UInt16 recNo)
{
    UInt16      cachedRecNo;
    MemHandle   recHandle;
    UInt32      cachedRecSize = 0;
    Err         err = errNone;

    LogV1("dcLockRecord(%d)", recNo );

    err = dcCacheDbRef(file, cache);
    if (errNone != err)
    {
        LogV1( "dcLockRecord(), dcCacheDbRef() failed with err=%d", err );
        return NULL;
    }

    /* already locked, so bump the lock counter
       and return data */
    if (0 != cache->recsInfo[recNo].lockCount)
    {
        ++cache->recsInfo[recNo].lockCount;
        return cache->recsInfo[recNo].data;
    }

    /* not yet locked, check if cached */
    if (0 == cache->recRealCachedNoMap[recNo])
    {
        /* not cached yet, so cache it */
        err = dcCacheRecord(file, cache,recNo);
        if (errNone != err)
        {
            /* bad, bad, bad */
            LogV1( "dcLockRecord(), dcCacheRecord() failed with err=%d", err );
            return NULL;
        }
    }

    Assert(0 != cache->recRealCachedNoMap[recNo]);
    cachedRecNo = cache->recRealCachedNoMap[recNo];
    recHandle = DmQueryRecord(cache->cacheDbRef, cachedRecNo);
    cache->recsInfo[recNo].data = MemHandleLock(recHandle);
    cache->recsInfo[recNo].lockCount = 1;

    cachedRecSize = MemHandleSize(recHandle);
    Assert(cache->recsInfo[recNo].size == MemHandleSize(recHandle));
    return cache->recsInfo[recNo].data;
}

void dcUnlockRecord(struct DbCacheData *cache, UInt16 recNo)
{
    MemHandle   recHandle;
    UInt16      cachedRecNo;

    Assert(cache->recsInfo[recNo].lockCount >= 0);

    LogV1("dcUnlockRecord(%d)", recNo );

    --cache->recsInfo[recNo].lockCount;
    if (0 == cache->recsInfo[recNo].lockCount)
    {
        cachedRecNo = cache->recRealCachedNoMap[recNo];
        recHandle = DmQueryRecord(cache->cacheDbRef, cachedRecNo);
        MemHandleUnlock(recHandle);
    }
}

/*
  The following two have very big oversimplifaction, but work for
  wn_lite_ex_support.c.

  Assume that lockRegion is followed by unlockRegion (for the same region)
  making unlockRegion basically a no-op. Lock region will use a fixed record
  (number 1) to copy the data from recNo at given offset to it.
*/

void *dcLockRegion(AbstractFile* file, struct DbCacheData *cache, UInt16 recNo, UInt16 offset, UInt16 size)
{
    char        *recData;
    void        *dstData;
    MemHandle   recHandle;
    UInt32      offsetInPdbFile;
    Err         err = errNone;

    recHandle = DmGetRecord(cache->cacheDbRef, cache->lockRegionCacheRec);
    if (0 == recHandle)
    {
        goto Error;
    }
    dstData = MemHandleLock(recHandle);
    /* copy data from CF to memory record */
    offsetInPdbFile = cache->recsInfo[recNo].offset + offset;
    err = vfsCopyExternalToMem(file, offsetInPdbFile, size, dstData);
    MemHandleUnlock(recHandle);
    DmReleaseRecord(cache->cacheDbRef, cache->lockRegionCacheRec, false);
    if (err)
    {
        goto Error;
    }

    recHandle = DmQueryRecord(cache->cacheDbRef, cache->lockRegionCacheRec);
    recData = (char*)MemHandleLock(recHandle);
    cache->lastLockedRegion = recData;

    return (void*)recData;
Error:
    LogG("dcLockRegion failed");
    return NULL;
}


/* TODO: optimize by opening the record one with DmGetRecord() at the
   beginning and keeping it open, closing on exit, so this would become
   a no-op */
void dcUnlockRegion(struct DbCacheData *cache, char *regionPtr)
{
    MemHandle recHandle;

    Assert(regionPtr == cache->lastLockedRegion);

    if (regionPtr != cache->lastLockedRegion)
    {
        LogG("dcUnlockRegion() failure!");
    }

    recHandle = DmQueryRecord(cache->cacheDbRef, cache->lockRegionCacheRec);
    MemHandleUnlock(recHandle);
}

