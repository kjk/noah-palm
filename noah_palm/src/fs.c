/*
  Copyright (C) 2000,2001, 2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#ifdef NOAH_PRO
#include "noah_pro.h"
#endif

#ifdef NOAH_LITE
#include "noah_lite.h"
#endif

#ifdef THESAURUS
#include "thes.h"
#endif

#include "fs.h"
#include "fs_mem.h"
#ifdef FS_VFS
#include "fs_vfs.h"
#endif

extern GlobalData gd;

Boolean FvalidFsType( eFS_TYPE fsType)
{
    if (eFS_MEM == fsType) return true;
#ifdef FS_VFS
    if (eFS_VFS == fsType) return true;
#endif
    return false;
}

/* Initialize all file systems i.e. VFS at present. */
void FsInit()
{
#ifdef FS_VFS
    FsVfsInit();
#endif
}

/* De-initialize all file systems i.e. VFS at present. */
void FsDeinit()
{
#ifdef FS_VFS
    FsVfsDeinit();
#endif
}

AbstractFile *AbstractFileNew(void)
{
    return (AbstractFile *) new_malloc_zero(sizeof(AbstractFile));
}

AbstractFile *AbstractFileNewFull( eFS_TYPE fsType, UInt32 creator, UInt32 type, char *fileName )
{
    AbstractFile *file = AbstractFileNew();

    Assert(fileName);

    if (file)
    {
        file->fsType = fsType;
        file->creator = creator;
        file->type = type;
        file->fileName = strdup(fileName);
        if (NULL==file->fileName)
        {
            AbstractFileFree(file);
            return NULL;
        }
    }
    return file;
}

void AbstractFileFree(AbstractFile *file)
{
    Assert( NULL != file );
    if (file->fileName) new_free(file->fileName);
    new_free(file);
}

/* serialize hte information about file so that it can be saved in preferences.
Given a file struct returns a blob and size of the blob in pSize. Memory
allocated here, caller has to free. Can be deserialized using AbstractFileDeserialize */
char *AbstractFileSerialize( AbstractFile *file, int *pSizeOut )
{
    int size = sizeof(int) + sizeof(eFS_TYPE) + 2*sizeof(UInt32)+ strlen(file->fileName);
    char *blob = new_malloc( size );
    int *pSize = (int*)blob;
    eFS_TYPE *pFsType;
    UInt32  *pUInt32;

    /* total size of the blob */
    *pSize = size;
    blob += sizeof(int);

    /* vfs type */
    pFsType = (eFS_TYPE*)blob;
    *pFsType = file->fsType;
    blob += sizeof(eFS_TYPE);

    /* file creator */
    pUInt32 = (UInt32*)blob;
    *pUInt32 = file->creator;
    blob += sizeof( UInt32 );

    /* file type */
    pUInt32 = (UInt32*)blob;
    *pUInt32 = file->type;
    blob += sizeof( UInt32 );

    /* file name */
    MemMove(blob,file->fileName,strlen(file->fileName));
    *pSizeOut = size;
    return blob;
}

/* Deserialize the blob describing AbstractFile. Memory allocated here, both
AbstractFile and AbstractFile.fileName must be released by caller.*/
AbstractFile *AbstractFileDeserialize( char *blob )
{
    AbstractFile    *file;
    int             size;
    int             *pSize;
    eFS_TYPE        *pFsType;
    int             fileNameLen;
    UInt32          *pUInt32;

    file = AbstractFileNew();
    if (NULL == file) return NULL;

    /* size */
    pSize=(int*)blob;
    size = *pSize;
    blob += sizeof(int);

    /* vfs type */
    pFsType = (eFS_TYPE*) blob;
    file->fsType = *pFsType;
    blob += sizeof(eFS_TYPE);

    /* file creator */
    pUInt32 = (UInt32*)blob;
    file->creator = *pUInt32;
    blob += sizeof(UInt32);

    /* file type */
    pUInt32 = (UInt32*)blob;
    file->type = *pUInt32;
    blob += sizeof(UInt32);

    /* file name */
    fileNameLen = size-sizeof(int)-sizeof(eFS_TYPE)-2*sizeof(UInt32);
    file->fileName = new_malloc(fileNameLen+1);
    if ( NULL == file->fileName )
    {
        new_free( file );
        return NULL;
    }
    MemMove( file->fileName, blob, fileNameLen );
    file->fileName[fileNameLen] = 0;
    return file;
}


/* Init virtual file system and return true if exist and inited properly */
Err vfsInit(void)
{
    return (*gd.currVfs->init) (gd.currVfs->vfsData);
}

/* Cleanup and free resources allocated in vfsInit().
   Assume that can be called multiple times in a row without harm */
void vfsDeinit(void)
{
    return (*gd.currVfs->deinit) (gd.currVfs->vfsData);
}

void vfsIterateRestart(void)
{
    return (*gd.currVfs->iterateRestart) (gd.currVfs->vfsData);
}

/* Iterate over files in vfs. Return false if no more files.
   Sets the file as "current"
 */
Boolean vfsIterateOverDbs(void)
{
    return (*gd.currVfs->iterateOverDbs) (gd.currVfs->vfsData);
}

/* Number of records in "current" database */
UInt16 vfsGetRecordsCount(void)
{
    return (*gd.currVfs->getRecordsCount) (gd.currVfs->vfsData);
}

UInt32 vfsGetDbCreator(void)
{
    return (*gd.currVfs->getDbCreator) (gd.currVfs->vfsData);
}

UInt32 vfsGetDbType(void)
{
    return (*gd.currVfs->getDbType) (gd.currVfs->vfsData);
}

char *vfsGetDbName(void)
{
    return (*gd.currVfs->getDbName) (gd.currVfs->vfsData);
}

/* Return a full path to a database name. Only relevant for dbs on
an external memory card, so if a vfs driver doesn't support it - 
just return empty string */
char *vfsGetFullDbPath(void)
{
    if (*gd.currVfs->getDbPath)
        return (*gd.currVfs->getDbPath) (gd.currVfs->vfsData);
    else
        return "";
}

/* get the size of a given record */
long vfsGetRecordSize(UInt16 recNo)
{
    return (*gd.currVfs->getRecordSize) (gd.currVfs->vfsData, recNo);
}

/* lock a given record and return a pointer to its data. It should
   only be used for records that are fully cacheable */
void * vfsLockRecord(UInt16 recNo)
{
    return (*gd.currVfs->lockRecord) (gd.currVfs->vfsData, recNo);
}

/* unlock a given record. Should only be used for records that are
   fully cacheable */
void vfsUnlockRecord(UInt16 recNo)
{
    return (*gd.currVfs->unlockRecord) (gd.currVfs->vfsData, recNo);
}

/* lock a region of a record and return a pointer to its data*/
void *vfsLockRegion(UInt16 recNo, UInt16 offset, UInt16 size)
{
    return (*gd.currVfs->lockRegion) (gd.currVfs->vfsData, recNo, offset, size);
}

/* unlock a region of a record */
void vfsUnlockRegion(void *data)
{
    return (*gd.currVfs->unlockRegion) (gd.currVfs->vfsData, data);
}

#if 0
/* hint: tell which records should be cached in in-memory database.
   As it stands we copy records from the file to in-memory db in
   this function (as opposed to lazy copying). Return false if
   something terrible has happened */
Boolean vfsCacheRecords(int recsCount, UInt16 * recs)
{
    return (*gd.currVfs->cacheRecords) (recsCount, recs);
}
#endif

Boolean vfsSetCurrentDict(DBInfo *dbInfo)
{
    return (*gd.currVfs->setCurrentDict) (gd.currVfs->vfsData, dbInfo);
}

/* find the database with a given creator, type and name. Return
   true if found, false otherwise. Also sets the database as a
   "Current" one. */
Boolean vfsFindDb(UInt32 creator, UInt32 type, char *name)
{
    while (true == vfsIterateOverDbs())
    {
        if ((creator != 0) && (creator != vfsGetDbCreator()))
        {
            continue;
        }

        if ((type != 0) && (type != vfsGetDbType()))
        {
            continue;
        }

        if ((NULL != name) && (StrCompare(name, vfsGetDbName()) != 0))
        {
            continue;
        }
        return true;
    }

    return false;
}

Err vfsCopyExternalToMem(UInt32 offset, UInt32 size, void *dstBuf)
{
    return (*gd.currVfs->copyExternalToMem) (gd.currVfs->vfsData, offset, size, dstBuf);
}

Boolean vfsReadHeader(void)
{
    return (*gd.currVfs->readHeader) (gd.currVfs->vfsData);
}
