/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _FS_H_
#define _FS_H_

#include "common.h"

typedef enum
{
    DB_PRO_DM,
    DB_EP_DM,
    DB_SIMPLE_DM,
    DB_LEX_DM,
    DB_LEX_TRG,
    DB_LEX_MP,
    DB_LEX_STD,
    DB_ROGET_DM
} VFSDBType;

#define HISTORY_ITEMS 5
#define WORD_MAX_LEN 40
#define DB_NAME_SIZE 280

/* For every database we keep per-database settings */
typedef struct
{
    VFSDBType   vfsDbType;
    char        dbName[32];                // name for all databases
    char        dbFullPath[DB_NAME_SIZE];  // full path to db for external card dbs
    int         historyCount;
    UInt32      wordHistory[HISTORY_ITEMS];
    char        lastWord[WORD_MAX_LEN];
} DBInfo;

/* An "object" for virtual file system. Those are the function
   that all "file system" (PalmOS's db, TRGPro's FFS etc.)
   should implement. 
   For each file system there must be a struct in GlobalData
   to keep its working data. Lame but true.
*/
typedef struct
{
    Err         (*init) (void *data);
    void        (*deinit) (void *data);
    void        (*iterateRestart) (void *data);
    Boolean     (*iterateOverDbs) (void *data);
    Boolean     (*findDb) (UInt32 creator, UInt32 type, char *name);

    UInt32      (*getDbCreator) (void *data);
    UInt32      (*getDbType) (void *data);
    char        *(*getDbName) (void *data);
    char        *(*getDbPath) (void *data);

    UInt16      (*getRecordsCount) (void *data);
    long        (*getRecordSize) (void *data, UInt16 recNo);
    void        *(*lockRecord) (void *data, UInt16 recNo);
    void        (*unlockRecord) (void *data, UInt16 recNo);
    void        *(*lockRegion) (void *data, UInt16 recNo, UInt16 offset, UInt16 size);
    void        (*unlockRegion) (void *data, void *dataPtr);
    Boolean     (*cacheRecords) (void *data, int recsCount, UInt16 * recs);
    Err         (*copyExternalToMem) (void *data, UInt32 offset, UInt32 size, void *dstBuf);
    Boolean     (*setCurrentDict) (void *data, DBInfo *dbInfo);
    Boolean     (*readHeader) (void *data);

    void        *vfsData;
}Vfs;

#define dmDBNameLength    32

typedef enum  {
    eFS_MEM = 4,
#ifdef FS_VFS
    eFS_VFS
#endif
} eFS_TYPE;

/* this struct describes the abstract file access api. All file operations come
through this struct */
typedef struct
{
    eFS_TYPE  fsType;
    UInt32    creator;
    UInt32    type;
    char     *fileName;  /* dbName in case of internal mem, full path in case of vfs */
} AbstractFile;

typedef void (FIND_DB_CB)(AbstractFile *);
typedef Boolean (IS_DB_OK)(UInt32 creator, UInt32 type, char *name);

Boolean FFileExists(AbstractFile *file);
Boolean FvalidFsType( eFS_TYPE fsType );

AbstractFile *AbstractFileNew(void);
AbstractFile *AbstractFileNewFull( eFS_TYPE fsType, UInt32 creator, UInt32 type, char *fileName );
void AbstractFileFree(AbstractFile *file);
char *AbstractFileSerialize( AbstractFile *file, int *pSizeOut );
AbstractFile *AbstractFileDeserialize( char *blob );

/* this is defined in fs_mem.c but exported globally */
void FsMemFindDb(UInt32 creator, UInt32 type, char *name, FIND_DB_CB *pCB);

/* this is defined in fs_vfs.c but exported globally */
void FsVfsFindDb( IS_DB_OK *cbDbOk, FIND_DB_CB *cbDbFound );


typedef struct
{
    char    name[dmDBNameLength];
    Int16   flags;
    Int16   version;
    UInt32  createTime;
    UInt32  modifyTime;
    UInt32  backupTime;
    UInt32  modificationNumber;
    UInt32  appInfoID;
    UInt32  sortInfoID;
    UInt32  type;
    UInt32  creator;
    UInt32  idSeed;
    UInt32  nextRecordList;
    Int16   recordsCount;
} PdbHeader;

typedef struct
{
    UInt32  recOffset;
    char    attrib;
    char    uniqueId[3];
} PdbRecordHeader;

void FsInit();
void FsDeinit();
void FsFindDb(UInt32 creator, UInt32 type, char *name);

Err         vfsInit(void);
void        vfsDeinit(void);
void        vfsIterateRestart(void);
Boolean     vfsIterateOverDbs(void);
Boolean     vfsFindDb(UInt32 creator, UInt32 type, char *name);
UInt16      vfsGetRecordsCount(void);
UInt32      vfsGetDbCreator(void);
UInt32      vfsGetDbType(void);
char        *vfsGetDbName(void);
char        *vfsGetFullDbPath(void);
Boolean     vfsSetCurrentDict(DBInfo *dbInfo);
long        vfsGetRecordSize(UInt16 recNo);
void        *vfsLockRecord(UInt16 recNo);
void        vfsUnlockRecord(UInt16 recNo);
void        *vfsLockRegion(UInt16 recNo, UInt16 offset, UInt16 size);
void        vfsUnlockRegion(void *data);
Err         vfsCopyExternalToMem(UInt32 offset, UInt32 size, void *dstBuf);
Boolean     vfsReadHeader(void);

#if 0
Boolean     vfsCacheRecords(int recsCount, UInt16 * recs);
#endif

#endif
