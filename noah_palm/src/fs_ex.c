/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#ifdef NOAH_PRO
/* only supported for Noah Pro, will bomb if used with Noah Lite */
#include "noah_pro.h"
#endif

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

void dcInit(DbCacheData *cache)
{
    cache->lockRegionCacheRec = 1;
}

void dcDeinit(DbCacheData *cache)
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
}

#define LOCK_REGION_REC_SIZE 4096

UInt16 dcGetRecordsCount(DbCacheData *cache)
{
    return cache->recsCount;
}

long dcGetRecordSize(DbCacheData *cache, UInt16 recNo)
{
    Assert(recNo < cache->recsCount);
    return cache->recsInfo[recNo].size;
}


/* Create a cache db. Assumes that a header info for the
   real database has been read, so we have enough information
   to do the job (number of records and database name)
   Also create the first record.*/
LocalID dcCreateCacheDb(DbCacheData *cache)
{
    Err             err = errNone;
    CacheDBInfoRec  *dbFirstRec = NULL;
    MemHandle       recHandle = 0;
    UInt16          recPos = 0;
    UInt32          firstRecSize = 0;
    LocalID         dbId = 0;

    err = DmCreateDatabase(0, VFS_CACHE_DB_NAME, NOAH_PRO_CREATOR, VFS_CACHE_TYPE, false);
    if (err)
    {
        goto Error;
    }

    dbId = DmFindDatabase(0, VFS_CACHE_DB_NAME);
    if (0 == dbId)
    {
        goto Error;
    }

    cache->cacheDbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (0 == cache->cacheDbRef)
    {
        goto Error;
    }

    firstRecSize = sizeof(CacheDBInfoRec) + (sizeof(int) * cache->recsCount);
    dbFirstRec = (CacheDBInfoRec *) new_malloc(firstRecSize);

    if (NULL == dbFirstRec)
    {
        goto Error;
    }

    /* ugly shortcut: sets the mapping real record no->cached record no
       the initial value of 0 means: record has not been cached yet */
    MemSet(dbFirstRec, firstRecSize, 0);

    recHandle = DmNewRecord(cache->cacheDbRef, &recPos, firstRecSize);
    if (0 == recHandle)
    {
        goto Error;
    }

    Assert(0 == recPos);

    err = DmReleaseRecord(cache->cacheDbRef, recPos, false);
    if (err)
    {
        goto Error;
    }

    dbFirstRec->signature = VFS_CACHE_DB_SIG_V1;
    dbFirstRec->realDbCreator = vfsGetDbCreator();
    dbFirstRec->realDbType = vfsGetDbType();
    MemMove(dbFirstRec->realDbName, vfsGetDbName(), 32);
    dbFirstRec->realDbRecsCount = vfsGetRecordsCount();

    err = dcUpdateFirstCacheRec(cache, dbFirstRec);
    if (err)
    {
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
    return errNone;

  Error:
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
Err dcCacheDbRef(DbCacheData *cache)
{
    Err             err = errNone;
    LocalID         dbId = 0;
    MemHandle       recHandle;
    CacheDBInfoRec  *dbFirstRec;
    CacheDBInfoRec  *firstRecMem;
    UInt32          firstRecSize = 0;
    UInt32          reslFirstRecSize = 0;

    /* need to read database header first */
    if (false == vfsReadHeader())
    {
        goto Error;
    }

    /* check if not already opened */
    if (0 != cache->cacheDbRef)
    {
        return errNone;
    }

    dbId = DmFindDatabase(0, VFS_CACHE_DB_NAME);
    if (0 == dbId)
    {
        /* no such database, create */
        err = dcCreateCacheDb(cache);
        if (errNone != err)
        {
            goto Error;
        }
        return errNone;
    }

    cache->cacheDbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (0 == cache->cacheDbRef)
    {
        goto Error;
    }
    recHandle = DmQueryRecord(cache->cacheDbRef, 0);
    dbFirstRec = (CacheDBInfoRec*) MemHandleLock(recHandle);
    if (NULL == dbFirstRec)
    {
        goto Error;
    }
    reslFirstRecSize = MemHandleSize(recHandle);
    firstRecSize = sizeof(CacheDBInfoRec) + (sizeof(int) * vfsGetRecordsCount());

    /* is this a correct record? */
    if ((VFS_CACHE_DB_SIG_V1 == dbFirstRec->signature) &&
        (dbFirstRec->realDbRecsCount == vfsGetRecordsCount()) &&
        (reslFirstRecSize == firstRecSize))
    {
        /* yes, it's a good cache database */
        firstRecMem = (CacheDBInfoRec *) new_malloc(firstRecSize);
        if (NULL == firstRecMem)
        {
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
            goto Error;
        }
        err = dcCreateCacheDb(cache);
        if (errNone != err)
        {
            goto Error;
        }
    }
    return errNone;
  Error:
    DrawDebug("cacheDbRef error");
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

void dcCloseCacheDb(DbCacheData *cache)
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
static int dcFindRecordToDelete(DbCacheData *cache)
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
Err dcCacheRecord(DbCacheData *cache, UInt16 recNo)
{
    Err             err = errNone;
    UInt16          recPos = dmMaxRecordIndex;
    UInt32          recSize;
    MemHandle       recHandle;
    char            *recData;
    UInt32          offsetInPdbFile;
    int             recToDelete;

    recSize = cache->recsInfo[recNo].size;

    offsetInPdbFile = cache->recsInfo[recNo].offset;
    /*DrawCacheRec(recNo);*/

    while(1)
    {
    
        recHandle = DmNewRecord(cache->cacheDbRef, &recPos, recSize);
        if (0 == recHandle)
        {
            /* failed to create new record-assume it's because there is no 
            free memory left. try to free some memory by deleting records */
            recToDelete = dcFindRecordToDelete(cache);
            if (-1 == recToDelete )
            {
                err = dmErrMemError;
                goto Error;
            }
            /* re-try creating a record */
            err = DmDeleteRecord(cache->cacheDbRef, recToDelete );
            if ( errNone != err )
            {
                goto Error;
            }
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
        goto Error;
    }

    recData = (char*) MemHandleLock(recHandle);
    err = vfsCopyExternalToMem(offsetInPdbFile, recSize, recData);
    MemHandleUnlock(recHandle);
    DmReleaseRecord(cache->cacheDbRef, recPos, false );
    if (err)
    {
        goto Error;
    }

    cache->recRealCachedNoMap[recNo] = recPos;
    err = dcUpdateFirstCacheRec(cache, cache->cacheDbInfoRec);
    if (err)
    {
        goto Error;
    }
    return errNone;

  Error:
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
Err dcUpdateFirstCacheRec(DbCacheData *cache, CacheDBInfoRec * dbFirstRec)
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

    firstRecSize = sizeof(CacheDBInfoRec) + (sizeof(int) * vfsGetRecordsCount());
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

void *dcLockRecord(DbCacheData *cache, UInt16 recNo)
{
    UInt16      cachedRecNo;
    MemHandle   recHandle;
    UInt32      cachedRecSize = 0;

    if (errNone != dcCacheDbRef(cache))
    {
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
        if (errNone != dcCacheRecord(cache,recNo))
        {
            /* bad, bad, bad */
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

void dcUnlockRecord(DbCacheData *cache, UInt16 recNo)
{
    MemHandle   recHandle;
    UInt16      cachedRecNo;


    Assert(cache->recsInfo[recNo].lockCount >= 0);
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

void *dcLockRegion(DbCacheData *cache, UInt16 recNo, UInt16 offset, UInt16 size)
{
    void        *recData;
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
    err = vfsCopyExternalToMem(offsetInPdbFile, size, dstData);
    MemHandleUnlock(recHandle);
    DmReleaseRecord(cache->cacheDbRef, cache->lockRegionCacheRec, false);
    if (err)
    {
        goto Error;
    }

    recHandle = DmQueryRecord(cache->cacheDbRef, cache->lockRegionCacheRec);
    recData = MemHandleLock(recHandle);
    cache->lastLockedRegion = recData;

    return recData;
  Error:
    DrawDebug("dcLockRegion failed");
    return NULL;
}


/* TODO: optimize by opening the record one with DmGetRecord() at the
   beginning and keeping it open, closing on exit, so this would become
   a no-op */
void dcUnlockRegion(DbCacheData *cache, void *regionPtr)
{
    MemHandle recHandle;

    Assert(regionPtr == cache->lastLockedRegion);

    if (regionPtr != cache->lastLockedRegion)
    {
        DrawDebug("dcUnlockRegion() failure!");
    }

    recHandle = DmQueryRecord(cache->cacheDbRef, cache->lockRegionCacheRec);
    MemHandleUnlock(recHandle);
}

#if 0
/* try to cache recsCount records, record numbers to cache
are in recs.
Return true if ok, false if there was a problem (most likely not
enough memory to create cache database */
Boolean dcCacheRecords(DbCacheData *cache, int recsCount, UInt16 * recs)
{
    int i;
    for (i = 0; i < recsCount; i++)
    {
        DrawCacheRec(i);
        if (NULL == vfsLockRecord(recs[i]))
        {
            return false;
        }
        vfsUnlockRecord(recs[i]);
    }
    return true;
}
#endif
