/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "common.h"
#include "extensible_buffer.h"
#include "fs_ex.h"

typedef struct
{
    UInt32  recOffset;
    char    attrib;
    char    uniqueId[3];
} PdbRecordHeader;

// Initialize vfs, return false if couldn't be initialized.
// Should only be called once
Boolean FsVfsInit(FS_Settings* fsSettings)
{
    Boolean   fPresent = true;
    Err       err;
    UInt32    vfsMgrVersion;
    UInt16    volRef;
    UInt32    volIterator;
    Assert(fsSettings);      
    Assert(!fsSettings->vfsPresent);

    err = FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &vfsMgrVersion);
    if (err)
    {
        fPresent = false;
        LogG( "FsVfsInit(): FtrGet() failed, no VFS" );
        goto Exit;
    }

    volIterator = expIteratorStart;
    while ( (volIterator != vfsIteratorStop) && (fsSettings->vfsVolumeCount < MAX_VFS_VOLUMES))
    {
        err = VFSVolumeEnumerate(&volRef, &volIterator);
        if (errNone == err)
        {
            fsSettings->vfsVolumeRef[fsSettings->vfsVolumeCount ++] = volRef;
        }
        else
        {
            fPresent = false;
            goto Exit;
        }
    }
Exit:
    fsSettings->vfsPresent = fPresent;
    
#ifdef DEBUG
    if ( fPresent )
    {
        LogV1("FsVfsInit(): VFS present, %d volumes", fsSettings->vfsVolumeCount );    
    }
    else
    {
        LogG( "FsVfsInit(): VFS not present");
    }
#endif
    return fPresent;
}

/* De-initialize vfs, should only be called once */
void FsVfsDeinit(FS_Settings* fsSettings)
{
#ifdef DEBUG
    if ( fsSettings->vfsPresent )
    {
        LogV1("FsVfsDeinit(): VFS present, %d volumes", fsSettings->vfsVolumeCount );    
    }
    else
    {
        LogG( "FsVfsDeinit(): VFS not present");
    }
#endif
    fsSettings->vfsPresent = false;
}

/* return true if given file attributes represent a file
   we're interested in */
static Boolean fIsFile(UInt32 fileAttr)
{
    if ((fileAttr &
         (vfsFileAttrHidden | vfsFileAttrSystem | vfsFileAttrVolumeLabel |
          vfsFileAttrDirectory)) != 0)
    {
        return false;
    }
    return true;
}

static Boolean fIsDir(UInt32 fileAttr)
{
    if ( (fileAttr & (vfsFileAttrHidden | vfsFileAttrSystem | vfsFileAttrVolumeLabel )) != 0)
        return false;

    if ( 0 == (fileAttr & vfsFileAttrDirectory) )
        return false;

    return  true;
}

static char *BuildFullFileName(char *dir, char *file)
{
    int     strLen;
    char *  fullFileName;

    /* full name is a sum of lenghts of dir and file plus one for a separator
    and one for trailing zero */
    strLen = StrLen(dir) + StrLen(file) + 2;    
    fullFileName = (char*)new_malloc_zero(strLen);
    if (NULL==fullFileName) return NULL;

    StrCopy( fullFileName, dir );
    strLen = StrLen(fullFileName);
    if ( (strLen>0) && ('/' != fullFileName[strLen-1]) )
        StrCat( fullFileName, "/" );
    StrCat( fullFileName, file );
    return fullFileName;
}

Boolean ReadPdbHeader(UInt16 volRef, char *fileName, PdbHeader *hdr)
{
    Err                err = errNone;
    FileRef            fileRef = 0;
    UInt32             bytesRead;

    err = VFSFileOpen(volRef, fileName, vfsModeRead, &fileRef);
    if (errNone != err)
    {
        fileRef = 0;
        LogV1( "ReadPdbHeader(%s) VFSFileOpen() fail", fileName );
        goto Error;
    }

    err = VFSFileRead(fileRef, sizeof(PdbHeader), (void *) hdr, &bytesRead);
    if ((err != errNone) && (bytesRead != sizeof(PdbHeader)))
    {
        LogV1( "ReadPdbHeader(%s) VFSFileRead() fail", fileName );
        goto Error;
    }

    if ((0 == hdr->recordsCount) || (hdr->recordsCount > 300))
    {
        /* I assume that this is a bogus file (non-PDB) */
        LogV1( "ReadPdbHeader(%s) not a pdb", fileName );
        goto Error;
    }

    if (errNone != VFSFileClose(fileRef))
    {
        LogV1( "ReadPdbHeader(%s) VFSFileClose() fail", fileName );
        goto Error;
    }
    return true;
Error:
    if (0 != fileRef)
    {
        VFSFileClose(fileRef);
    }
    return false;
}


// max size of VFS path. 256+40 is a bit magical, hopefully this is
// more than enough (256 according to docs)
#define VFS_MAX_PATH_SIZE 256+40

/* Iterate over all files on the vfs file system and call the callback
for each file */
void FsVfsFindDb( FS_Settings* fsSettings, FIND_DB_CB *cbCheckFile, void* context )
{
    Err             err;
    FileRef         dirRef;
    char *          currDir = NULL;
    char *          newDir;
    char *          fileName;
    UInt16          currVolRef;
    UInt32          dirIter;
    FileInfoType    fileInfo;
    StringStack     dirsToVisit;
    Boolean         fRecursive = true;
    PdbHeader       hdr;
    AbstractFile *  file;
    int             currVolume;

    if (!FVfsPresent(fsSettings))
    {
        LogG( "FsVfsFindDb(): VFS not present" );
        return;
    }        

    MemSet( &fileInfo, 0, sizeof(fileInfo) );
    fileInfo.nameBufLen = VFS_MAX_PATH_SIZE;
    fileInfo.nameP = (char*)new_malloc_zero(fileInfo.nameBufLen);
    if ( NULL == fileInfo.nameP ) return;

    currVolume = 0;
ScanNextVolume:
    if (currVolume >= fsSettings->vfsVolumeCount) goto NoMoreFiles;
    currVolRef = fsSettings->vfsVolumeRef[currVolume++];

    /* restart iterating over directories */
    currDir = strdup( "/" );
    if ( NULL == currDir ) goto NoMoreFiles;

    ssInit(&dirsToVisit);
    ssPush(&dirsToVisit, currDir );

ScanNextDir:
    currDir = ssPop(&dirsToVisit);

    if ( NULL == currDir) goto NoMoreFiles;
    LogV1("FsVfsFindDb(), currDir=%s", currDir );
    err = VFSFileOpen(currVolRef, currDir, vfsModeRead, &dirRef);
    if (err != errNone)
        goto NoMoreFiles;

    dirIter = expIteratorStart;
    while (dirIter != expIteratorStop)
    {
        err = VFSDirEntryEnumerate(dirRef, &dirIter, &fileInfo);
        if (err != errNone)
        {
            // LogV1("VFSDirEnum() failed with err=%d", err );
            goto FailedDirEnum;
        }

        // if a dir - add to the list of dirs to visit next
        if (fIsDir(fileInfo.attributes) && fRecursive )
        {
            newDir = BuildFullFileName( currDir, fileInfo.nameP );
            if ( NULL == newDir ) goto NoMoreFiles;
            if ( !ssPush( &dirsToVisit, newDir ) )
            {
                new_free(newDir);
                goto NoMoreFiles;
            }
        } 
        else if (fIsFile(fileInfo.attributes))
        {
            fileName = BuildFullFileName( currDir, fileInfo.nameP );
            if ( NULL == fileName ) goto NoMoreFiles;
            LogV1("FsVfsFindDb(): file %s", fileName );
            file = NULL;
            if ( ReadPdbHeader(currVolRef, fileName, &hdr ) )
            {
                file = AbstractFileNewFull( eFS_VFS, hdr.creator, hdr.type, fileName );
            }
            new_free(fileName);
            if (file)
            {
                file->volRef = currVolRef;
                (*cbCheckFile)(context, file);
                AbstractFileFree(file);
            }
        }
    }

    Assert(expIteratorStop == dirIter);
    Assert( 0 != dirRef );
FailedDirEnum:
    new_free(currDir);
    currDir = NULL;
    VFSFileClose(dirRef);
    dirRef = 0;
    if (fRecursive)
        goto ScanNextDir;

    goto ScanNextVolume;

NoMoreFiles:
    if (0 != dirRef)
        VFSFileClose(dirRef);

    if ( NULL != currDir )
        new_free(currDir);

    while(true)
    {
        currDir = ssPop(&dirsToVisit);
        if (NULL==currDir) break;
        new_free(currDir);
    }

    if ( fileInfo.nameP )
        new_free(fileInfo.nameP);
    ssDeinit(&dirsToVisit);
}

/* get the size of the file, 0 means a failure */
static UInt32 vfsGetFileSize(FileRef fileRef)
{
    UInt32  fileSize = 0;
    Err     err = errNone;

    err = VFSFileSize(fileRef, &fileSize);
    if (err)
    {
        LogV1( "vfsGetFileSize(), VFSFileSize() failed with err=%d", err );
        return 0;
    }
    return fileSize;
}

#define COPY_AT_ONCE 1024

Err vfsCopyExternalToMem(AbstractFile *file, UInt32 offset, UInt32 size, void *dstBuf)
{
    Err     err = errNone;
    char    buf[COPY_AT_ONCE];
    FileRef fileRef = 0;
    UInt32  leftToCopy;
    UInt32  currentOffset;
    UInt32  copyThisTime;
    UInt32  bytesRead;

    leftToCopy = size;
    currentOffset = 0;

    // LogV1( "vfsCopyExternalToMem() offset: %ld", offset );
    err = VFSFileOpen(file->volRef, file->fileName, vfsModeRead, &fileRef);
    if (errNone != err)
    {
        LogG( "vfsCopyExternalToMem(): FSFileOpen() failed" );
        LogG( file->fileName );
        goto Error;
    }

    err = VFSFileSeek(fileRef, vfsOriginBeginning, offset);
    if ( err != errNone )
    {
        LogG( "vfsCopyExternalToMem(): VFSFileSeek() failed" );
        LogG( file->fileName );
        goto Error;
    }
    while (leftToCopy > 0)
    {
        if (leftToCopy >= COPY_AT_ONCE)
        {
            copyThisTime = COPY_AT_ONCE;
        }
        else
        {
            copyThisTime = leftToCopy;
        }
        err = VFSFileRead(fileRef, copyThisTime, (void *) buf, &bytesRead);
        if ((err != errNone) || (copyThisTime != bytesRead))
        {
#ifdef DEBUG
            if ( err != errNone )
            {
                /* LogV2( "vfsCopyExternalToMem(%s): VFSFileRead() failed, err=%d", file->fileName, err ); */
                LogV1( "vfsCopyExternalToMem(%s): VFSFileRead() failed", file->fileName );
                LogV1( "err=%d", err );
            }
            else
            {
                LogV1( "vfsCopyExternalToMem(%s): VFSFileRead() failed because copyThisTime != bytesRead", file->fileName );
            }
#endif
            goto Error;
        }

        err = DmWrite(dstBuf, currentOffset, buf, copyThisTime);
        if (err)
        {
            LogG( "vfsCopyExternalToMem(): DmWrite() failed" );
            LogG( file->fileName );
            goto Error;
        }
        currentOffset += copyThisTime;
        leftToCopy -= copyThisTime;
    }

    Assert(0 == leftToCopy);
    err = VFSFileClose(fileRef);
    if (errNone != err)
    {
        LogG( "vfsCopyExternalToMem(): VFSFileClose() failed" );
        LogG( file->fileName );
        goto Error;
    }

    return errNone;
  Error:
    LogG( "vfsCopyExternalToMem(): failed" );
    LogG( file->fileName );
    if (0 != fileRef)
    {
        VFSFileClose(fileRef);
    }

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

Boolean vfsInitCacheData(AbstractFile *file, struct DbCacheData *cacheData)
{
    PdbHeader       hdr;
    int             i;
    Err             err;
    FileRef         fileRef = 0;
    UInt32          fileSize;
    UInt32          recSize;
    UInt32          bytesRead;
    PdbRecordHeader pdbRecHeader;

    err = VFSFileOpen(file->volRef, file->fileName, vfsModeRead, &fileRef);
    if (errNone != err)
    {
        fileRef = 0;
        goto Error;
    }

    err = VFSFileRead(fileRef, sizeof(PdbHeader), (void *) &hdr, &bytesRead);
    if ((err != errNone) && (bytesRead != sizeof(PdbHeader)))
    {
        LogV1( "vfsInitCacheData(%s): VFSFileRead() for PdbHeader failed", file->fileName );
        goto Error;
    }

    LogV1( "vfsInitCacheData(): hdr.recordsCount=%d", hdr.recordsCount );
    cacheData->recsCount = hdr.recordsCount;
    cacheData->recsInfo = (OneVfsRecordInfo*) new_malloc_zero(cacheData->recsCount * sizeof(OneVfsRecordInfo));

    if (NULL == cacheData->recsInfo)
    {
        LogV1( "vfsInitCacheData(): cacheData->recsInfo null, cacheData->recsCount", cacheData->recsCount );
        goto Error;
    }

    for (i = 0; i < cacheData->recsCount; i++)
    {
        err = VFSFileRead(fileRef, sizeof(PdbRecordHeader), (void *) &pdbRecHeader, &bytesRead);
        if ((err != errNone) || (bytesRead != sizeof(PdbRecordHeader)))
        {
            // that's ok, the file on external memory card doesn't have to be *pdb file
            LogV1("vfsInitCacheData(), VFSFileRead() failed with err=%d", err );
            goto Error;
        }
        cacheData->recsInfo[i].offset = pdbRecHeader.recOffset;
    }

    /* calc sizes of all records, size of rec n is offset of rec n+1
       - offset of rec n */
    for (i = 1; i < cacheData->recsCount; i++)
    {
        recSize = (cacheData->recsInfo[i].offset - cacheData->recsInfo[i -1].offset);
        cacheData->recsInfo[i - 1].size = (UInt16) recSize;
    }
    fileSize = vfsGetFileSize(fileRef);
    if (0 == fileSize)
    {
        LogG("vfsInitCacheData(), vfsGetFileSize() returned 0" );
        goto Error;
    }
    i = cacheData->recsCount - 1;
    cacheData->recsInfo[i].size = (UInt16) (fileSize - cacheData->recsInfo[i].offset);

    /* initialize data for caching purposes */
    for (i = 0; i < cacheData->recsCount; i++)
    {
        cacheData->recsInfo[i].data = NULL;
        cacheData->recsInfo[i].lockCount = 0;
    }

    err = VFSFileClose(fileRef);
    if (errNone != err)
    {
        LogV1( "vfsInitCacheData(), VFSFileClose() failed with err=%", err );
        goto Error;
    }

    return true;

Error:
    if (0 != fileRef)
        VFSFileClose(fileRef);
    return false;
}

