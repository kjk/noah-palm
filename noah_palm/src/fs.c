/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "cw_defs.h"

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
#include "fs_vfs.h"

AbstractFile *currFile=NULL;

void SetCurrentFile(AbstractFile *file)
{
#ifdef DEBUG
    if ( NULL == file )
    {
        LogG( "SetCurrentFile(NULL)" );
    }
    else
    {
        LogV1( "SetCurrentFile(%s)", file->fileName);
    }
#endif       
    currFile = file;
}

AbstractFile *GetCurrentFile(void)
{
    return currFile;
}

/* Initialize all file systems i.e. VFS at present. */
void FsInit()
{
    FsVfsInit();
}

/* De-initialize all file systems i.e. VFS at present. */
void FsDeinit()
{
    FsVfsDeinit();
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
    FsFileClose( file );
    Assert( NULL == file->data.memData );
    Assert( NULL == file->data.cacheData );
    if (file->fileName) new_free(file->fileName);
    new_free(file);
}

/* serialize hte information about file so that it can be saved in preferences.
Given a file struct returns a blob and size of the blob in pSize. Memory
allocated here, caller has to free. Can be deserialized using AbstractFileDeserialize */
char *AbstractFileSerialize( AbstractFile *file, int *pSizeOut )
{
    int size = sizeof(int) + sizeof(eFsType) + 2*sizeof(UInt32)+ strlen(file->fileName);
    char *blob = (char*)new_malloc( size );
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
    file->fileName = (char *)new_malloc(fileNameLen+1);
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
    LogV1( "FsFileOpen(%s)", file->fileName );
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
        case eFS_VFS:
#ifdef THESAURUS
            file->data.cacheData = dcNew(file, THES_CREATOR);
#endif
#ifdef NOAH_PRO
            file->data.cacheData = dcNew(file, NOAH_PRO_CREATOR);
#endif
#ifdef NOAH_LITE
            Assert(0);
#endif
            if ( NULL == file->data.cacheData )
            {
                LogG( "FsFileOpen(%s) dcNew failed, file->data.cacheData is NULL" );
                return false;
            }
            break;
        default:
            Assert(0);
            break;
    }
    SetCurrentFile(file);
    return true;
}

void FsFileClose(AbstractFile *file)
{
    LogV1( "FsFileClose(%s)", file->fileName );
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
        case eFS_VFS:
            if ( file->data.cacheData )
            {
                dcDeinit( file->data.cacheData );
                new_free( file->data.cacheData );
                file->data.cacheData = NULL;
            }
            break;
        default:
            Assert(0);
    }
    SetCurrentFile(NULL);
}

UInt16 fsGetRecordsCount(AbstractFile *file)
{
    switch(file->fsType)
    {
        case eFS_MEM:
            return memGetRecordsCount(file->data.memData );
        case eFS_VFS:
            return dcGetRecordsCount(file->data.cacheData);
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
        case eFS_VFS:
            return dcGetRecordSize(file->data.cacheData, recNo);
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
        case eFS_VFS:
            return dcLockRecord(file->data.cacheData, recNo);
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
        case eFS_VFS:
            dcUnlockRecord(file->data.cacheData, recNo );
            break;
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
        case eFS_VFS:
            return dcLockRegion(file->data.cacheData, recNo, offset, size );
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
        case eFS_VFS:
            dcUnlockRegion(file->data.cacheData, data );
            break;
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
    fsUnlockRecord(currFile, recNo);
}

/* lock a region of a record and return a pointer to its data*/
void *CurrFileLockRegion(UInt16 recNo, UInt16 offset, UInt16 size)
{
    return fsLockRegion(currFile,recNo,offset,size);
}

/* unlock a region of a record */
void CurrFileUnlockRegion(char *data)
{
    fsUnlockRegion(currFile,data);
}


inline Boolean FValidFsType(eFsType type)
{
    if ((eFS_NONE==type) || (eFS_MEM==type) || (eFS_VFS==type))
        return true;
    else
        return false;
}

