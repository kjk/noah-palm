/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _FS_EX_
#define _FS_EX_

/* We need to cache databases from external memory cards in internal memory.
   This file has an interface for dealing with cached databases. */

/* VFS uses a database in memory to cache
   frequently referenced records from CF. Those
   are params uniquely identifying this database.*/

#include "fs_vfs.h"

/* vfsc ~= VFS cache */
#define VFS_CACHE_TYPE 'vfsc'
#define VFS_CACHE_DB_NAME "fs cache"
/* signature of the first record in cache db */
#define VFS_CACHE_DB_SIG_V1 0x44454647

typedef struct
{
    UInt32      signature;           /* for verification purpose */
    UInt32      realDbCreator;
    UInt32      realDbType;
    char        realDbName[32];
    UInt16      realDbRecsCount;     /* number of records in the real database */

    /* ... and the rest is of 2*realDbRecsCount size
      that for each record in real database says what
      is the corresponding record in cache db */
} CacheDBInfoRec;

/* for each record in an original database (stored on CF)
   remember its size and offset (initialized by reading
   db header from CF db), lockCount and pointer to data
   in cached record (if locked), and a corresponding
   record number in a cache database */
typedef struct
{
    int     lockCount; /* number of times this has been locked */
    void    *data;     /* if locked points to a memory holding a cached record */
    UInt16  size;      /* size of the record */
    UInt32  offset;    /* offset of the record within *pdb file */
} OneVfsRecordInfo;

struct DbCacheData
{
    UInt32              cacheCreator;
    DmOpenRef           cacheDbRef;   /* pointer to opened cached database */
    int                 recsCount;    /* number of records in the pdb */
    /* mapping of record no in real db <=>
    record no in cached db */
    UInt16              *recRealCachedNoMap;
    OneVfsRecordInfo    *recsInfo;
    CacheDBInfoRec      *cacheDbInfoRec;

    /* record number for holding a cache for lock region
    This record should not be bigger than 4 kB */
    int                 lockRegionCacheRec;
    char                *lastLockedRegion;
};

/* dc stands for Database Cache */
struct DbCacheData *dcNew(AbstractFile *file, UInt32 cacheCreator);
void         dcDelCacheDb(void);
void         dcInit(struct DbCacheData *cache);
void         dcDeinit(struct DbCacheData *cache);
UInt16       dcGetRecordsCount(struct DbCacheData *cache);
long         dcGetRecordSize(struct DbCacheData *cache, UInt16 recNo);
LocalID      dcCreateCacheDb(struct DbCacheData *cache);
Err          dcCacheDbRef(struct DbCacheData *cache);
void         dcCloseCacheDb(struct DbCacheData *cache);
Err          dcCacheRecord(struct DbCacheData *cache, UInt16 recNo);
Err          dcUpdateFirstCacheRec(struct DbCacheData *cache, CacheDBInfoRec * dbFirstRec);
void        *dcLockRecord(struct DbCacheData *cache, UInt16 recNo);
void        *dcLockRegion(struct DbCacheData *cache, UInt16 recNo, UInt16 offset, UInt16 size);
void         dcUnlockRecord(struct DbCacheData *cache, UInt16 recNo);
void         dcUnlockRegion(struct DbCacheData *cache,char *regionPtr);
//Boolean dcCacheRecords(struct DbCacheData *cache, int recsCount, UInt16 * recs);

#endif
