/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _FS_H_
#define _FS_H_

#include <PalmOS.h>

#define DB_NAME_SIZE 280

typedef enum  {
    eFS_NONE,
    eFS_MEM = 5,
    eFS_VFS
} eFsType;

struct MemData;
struct DbCacheData;

struct _RogetInfo;
struct _WnInfo;
struct _WnLiteInfo;
struct _SimpleInfo;
struct _EngPolInfo;

// format of the PRC/PDB file
typedef struct _PdbHeader
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

/* this struct describes the abstract file access api. All file operations come
through this struct */
typedef struct _AbstractFile
{
    eFsType   fsType;
    UInt32    type;
    UInt32    creator;

    /* those two only used for memory database */
    UInt16    cardNo;
    LocalID   dbId;

    char *    fileName;  /* dbName in case of internal mem, full path in case of vfs */

    /* those are runtime-only */

    UInt16  volRef;  /* vfs only - on which volume the file is */
    union {
        struct MemData *      memData;
        struct DbCacheData *  cacheData;
    } data;

    union {
        struct _RogetInfo  *    roget;
        struct _WnInfo     *    wn;
        struct _WnLiteInfo *    wnLite;
        struct _SimpleInfo *    simple;
        struct _EngPolInfo *    engpol;
    } dictData;

} AbstractFile;

#define MAX_VFS_VOLUMES 3

typedef struct _FS_Settings 
{
    Boolean vfsPresent;
    int vfsVolumeCount;
    UInt16 vfsVolumeRef[MAX_VFS_VOLUMES];
} FS_Settings;

#define FVfsPresent(fsSettings) ((fsSettings)->vfsPresent)

typedef void (FIND_DB_CB)(void* context, AbstractFile *);

Boolean FFileExists(AbstractFile* file);
Boolean FValidFsType(eFsType type);

AbstractFile *  AbstractFileNew(void);
AbstractFile *  AbstractFileNewFull( eFsType fsType, UInt32 creator, UInt32 type, char *fileName );
void            AbstractFileFree(AbstractFile* file);
char *          AbstractFileSerialize( AbstractFile* file, int *pSizeOut );
AbstractFile *  AbstractFileDeserialize( char *blob );

/* this is defined in fs_mem.c but exported globally */
void  FsMemFindDb( UInt32 creator, UInt32 type, char *name, FIND_DB_CB *pCB, void* context );
/* this is defined in fs_vfs.c but exported globally */
void FsVfsFindDb( FS_Settings* fsSettings, FIND_DB_CB *cbCheckFile, void* context );

void    FsInit(FS_Settings* fsSettings);
void    FsDeinit(FS_Settings* fsSettings);

struct _AppContext;

Boolean FsFileOpen(struct _AppContext* appContext, AbstractFile* file);
void    FsFileClose(AbstractFile* file);

void    SetCurrentFile(struct _AppContext* appContext, AbstractFile* file);
#define GetCurrentFile(appContext) ((appContext)->currentFile)

extern UInt16 fsGetRecordsCount(AbstractFile* file);
extern UInt16 fsGetRecordSize(AbstractFile* file, UInt16 recNo);
extern void* fsLockRecord(AbstractFile* file, UInt16 recNo);
extern void fsUnlockRecord(AbstractFile* file, UInt16 recNo);
extern void* fsLockRegion(AbstractFile* file, UInt16 recNo, UInt16 offset, UInt16 size);
extern void fsUnlockRegion(AbstractFile* file, char *data);

extern Boolean ReadPdbHeader(UInt16 volRef, char *fileName, PdbHeader *hdr);


#endif
