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

static AbstractFile *currFile=NULL;

void SetCurrentFile(AbstractFile *file)
{
    currFile = file;
}

AbstractFile *GetCurrentFile(void)
{
    return currFile;
}

Boolean FvalidFsType( eFsType fsType)
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

AbstractFile *AbstractFileNewFull( eFsType fsType, UInt32 creator, UInt32 type, char *fileName )
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
    Assert( NULL == file->data.memData );
    Assert( NULL == file->data.vfsData );
    if (file->fileName) new_free(file->fileName);
    new_free(file);
}

/* serialize hte information about file so that it can be saved in preferences.
Given a file struct returns a blob and size of the blob in pSize. Memory
allocated here, caller has to free. Can be deserialized using AbstractFileDeserialize */
char *AbstractFileSerialize( AbstractFile *file, int *pSizeOut )
{
    int size = sizeof(int) + sizeof(eFsType) + 2*sizeof(UInt32)+ strlen(file->fileName);
    char *blob = new_malloc( size );
    int *pSize = (int*)blob;
    eFsType *pFsType;
    UInt32  *pUInt32;

    /* total size of the blob */
    *pSize = size;
    blob += sizeof(int);

    /* vfs type */
    pFsType = (eFsType*)blob;
    *pFsType = file->fsType;
    blob += sizeof(eFsType);

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
    eFsType        *pFsType;
    int             fileNameLen;
    UInt32          *pUInt32;

    file = AbstractFileNew();
    if (NULL == file) return NULL;

    /* size */
    pSize=(int*)blob;
    size = *pSize;
    blob += sizeof(int);

    /* vfs type */
    pFsType = (eFsType*) blob;
    file->fsType = *pFsType;
    blob += sizeof(eFsType);

    /* file creator */
    pUInt32 = (UInt32*)blob;
    file->creator = *pUInt32;
    blob += sizeof(UInt32);

    /* file type */
    pUInt32 = (UInt32*)blob;
    file->type = *pUInt32;
    blob += sizeof(UInt32);

    /* file name */
    fileNameLen = size-sizeof(int)-sizeof(eFsType)-2*sizeof(UInt32);
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

Boolean FsFileOpen(AbstractFile *file)
{
    switch(file->fsType)
    {
        case eFS_MEM:
            file->data.memData = memNew(file);
            if ( !memOpenDb( file->data.memData ) )
            {
                new_free( file->data.memData );
                file->data.memData = NULL;
                return false;
            }
            break;
#ifdef FS_VFS
        case eFS_VFS:
            file->data.vfsData = stdNew();
            // TODO;
            //vfs
            break;
#endif
        default:
            Assert(0);
    }
    SetCurrentFile(file);
    return true;
}

void FsFileClose(AbstractFile *file)
{
    switch(file->fsType)
    {
        case eFS_MEM:
            if ( file->data.memData )
            {
                memDeinit( file->data.memData );
                new_free( file->data.memData);
                file->data.memData = NULL;
            }
            break;
#ifdef FS_VFS
        case eFS_VFS:
            if ( file->data.vfsData )
            {
                VfsDeinit( file->data.vfsData );
                new_free( file->data.vfsData );
                file->data.vfsData = NULL;
            }
            break;
#endif
        default:
            Assert(0);
    }
}

UInt16 fsGetRecordsCount(AbstractFile *file)
{
    switch(file->fsType)
    {
        case eFS_MEM:
            return memGetRecordsCount(file->data.memData );
#ifdef FS_VFS
        case eFS_VFS:
            return VfsUnlockRegion(file->data.vfsData);
#endif
        default:
            Assert(0);
            return 0;
    }
}

UInt16 fsGetRecordSize(AbstractFile *file, UInt16 recNo)
{
    switch(file->fsType)
    {
        case eFS_MEM:
            return memGetRecordSize(file->data.memData, recNo);
#ifdef FS_VFS
        case eFS_VFS:
            return VfsGetRecordSize(file->data.vfsData, recNo);
#endif
        default:
            Assert(0);
            return 0;
    }
}

void *fsLockRecord(AbstractFile *file, UInt16 recNo)
{
    switch (file->fsType)
    {
        case eFS_MEM:
            return memLockRecord(file->data.memData, recNo);
#ifdef FS_VFS
        case eFS_VFS:
            return VfsLockRecord(file->data.vfsData, recNo);
#endif
        default:
            Assert(0);
            return NULL;
    }
}

void fsUnlockRecord(AbstractFile *file, UInt16 recNo)
{
    switch(file->fsType)
    {
        case eFS_MEM:
            memUnlockRecord(file->data.memData, recNo );
            break;
#ifdef FS_VFS
        case eFS_VFS:
            VfsUnlockRecord(file->data.vfsData, recNo );
            break;
#endif
        default:
            Assert(0);
    }
}

void *fsLockRegion(AbstractFile *file, UInt16 recNo, UInt16 offset, UInt16 size)
{
    switch(file->fsType)
    {
        case eFS_MEM:
            return memLockRegion(file->data.memData, recNo, offset, size );
#ifdef FS_VFS
        case eFS_VFS:
            return VfsLockRegion(file->data.vfsData, recNo, offset, size );
#endif
        default:
            Assert(0);
            return NULL;
    }
}

void fsUnlockRegion(AbstractFile *file, char *data)
{
    switch(file->fsType)
    {
        case eFS_MEM:
            memUnlockRegion(file->data.memData, data );
            break;
#ifdef FS_VFS
        case eFS_VFS:
            VfsUnlockRegion(file->data.vfsData, data );
            break;
#endif
        default:
            Assert(0);
    }
}

/* Number of records in "current" database */
UInt16 CurrFileGetRecordsCount(void)
{
    return fsGetRecordsCount(currFile);
}

/* get the size of a given record */
long CurrFileGetRecordSize(UInt16 recNo)
{
    return fsGetRecordSize(currFile,recNo);
}

/* lock a given record and return a pointer to its data. It should
   only be used for records that are fully cacheable */
void * CurrFileLockRecord(UInt16 recNo)
{
    return fsLockRecord(currFile,recNo);
}

/* unlock a given record. Should only be used for records that are
   fully cacheable */
void CurrFileUnlockRecord(UInt16 recNo)
{
    return fsUnlockRecord(currFile, recNo);
}

/* lock a region of a record and return a pointer to its data*/
void *CurrFileLockRegion(UInt16 recNo, UInt16 offset, UInt16 size)
{
    return fsLockRegion(currFile,recNo,offset,size);
}

/* unlock a region of a record */
void CurrFileUnlockRegion(void *data)
{
    return fsUnlockRegion(currFile,data);
}

