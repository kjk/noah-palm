/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _FS_H_
#define _FS_H_

#include <PalmOS.h>

#define dmDBNameLength    32

// TODO: remove if not needed
#define HISTORY_ITEMS 5
#define WORD_MAX_LEN 40
#define DB_NAME_SIZE 280

typedef enum  {
    eFS_MEM = 4,
    eFS_VFS
} eFsType;

struct MemData;
struct DbCacheData;

struct RogetInfo;
struct WnInfo;
struct WnLiteInfo;
struct SimpleInfo;
struct EngPolInfo;

/* this struct describes the abstract file access api. All file operations come
through this struct */
typedef struct
{
    eFsType   fsType;
    UInt32    type;
    UInt32    creator;

    /* those two only used for memory database */
    UInt16    cardNo;
    LocalID   dbId;

    char     *fileName;  /* dbName in case of internal mem, full path in case of vfs */

    /* those are runtime-only */

    UInt16  volRef;  /* vfs only - on which volume the file is */
    union {
        struct MemData      *memData;
        struct DbCacheData  *cacheData;
    } data;

    union {
        struct RogetInfo  *roget;
        struct WnInfo     *wn;
        struct WnLiteInfo *wnLite;
        struct SimpleInfo *simple;
        struct EngPolInfo *engpol;
    } dictData;

} AbstractFile;

typedef void (FIND_DB_CB)(AbstractFile *);
//typedef Boolean (IS_DB_OK)(UInt32 creator, UInt32 type, char *name);

Boolean FFileExists(AbstractFile *file);
Boolean FvalidFsType( eFsType fsType );

AbstractFile    *AbstractFileNew(void);
AbstractFile    *AbstractFileNewFull( eFsType fsType, UInt32 creator, UInt32 type, char *fileName );
void            AbstractFileFree(AbstractFile *file);
char            *AbstractFileSerialize( AbstractFile *file, int *pSizeOut );
AbstractFile    *AbstractFileDeserialize( char *blob );

/* this is defined in fs_mem.c but exported globally */
void            FsMemFindDb(UInt32 creator, UInt32 type, char *name, FIND_DB_CB *pCB);
/* this is defined in fs_vfs.c but exported globally */
//void            FsVfsFindDb( IS_DB_OK *cbDbOk, FIND_DB_CB *cbDbFound );
void FsVfsFindDb( FIND_DB_CB *cbCheckFile );

void    FsInit();
void    FsDeinit();

Boolean FsFileOpen(AbstractFile *file);
void    FsFileClose(AbstractFile *file);

void         SetCurrentFile(AbstractFile *file);
AbstractFile *GetCurrentFile(void);

UInt16  CurrFileGetRecordsCount(void);
long    CurrFileGetRecordSize(UInt16 recNo);
void    *CurrFileLockRecord(UInt16 recNo);
void    CurrFileUnlockRecord(UInt16 recNo);
void    *CurrFileLockRegion(UInt16 recNo, UInt16 offset, UInt16 size);
void    CurrFileUnlockRegion(void *data);

#endif
