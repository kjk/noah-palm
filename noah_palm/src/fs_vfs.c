/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "extensible_buffer.h"

#include "fs.h"
#include "fs_ex.h"
#include "fs_vfs.h"

#define MAX_VFS_VOLUMES 3
static Boolean g_fVfsPresent = false;
static int     g_VfsVolumeCount = 0;
static UInt16  g_VfsVolumeRef[MAX_VFS_VOLUMES];

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

/* Initialize vfs, return false if couldn't be initialized.
Should only be called once */
Boolean FsVfsInit(void)
{
    Boolean   fPresent = true;
    Err       err;
    UInt32    vfsMgrVersion;
    UInt16    volRef;
    UInt32    volIterator;

    /* should not be called twice */
    Assert( !g_fVfsPresent );
    err = FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &vfsMgrVersion);
    if (err)
    {
        fPresent = false;
        LogG( "FsVfsInit(): FtrGet() failed, no VFS" );
        goto Exit;
    }

    volIterator = expIteratorStart;
    while ( (volIterator != vfsIteratorStop) && (g_VfsVolumeCount < MAX_VFS_VOLUMES))
    {
        err = VFSVolumeEnumerate(&volRef, &volIterator);
        if (errNone == err)
        {
            g_VfsVolumeRef[g_VfsVolumeCount++] = volRef;
        }
        else
        {
            fPresent = false;
            goto Exit;
        }
    }
Exit:
    g_fVfsPresent = fPresent;
#ifdef DEBUG
    if ( fPresent )
    {
        StrPrintF( g_logBuf, "FsVfsInit(): VFS present, %d volumes", g_VfsVolumeCount );    
        LogG( g_logBuf );
    }
    else
    {
        LogG( "FsVfsInit(): VFS not present");
    }
#endif
    return fPresent;
}

/* De-initialize vfs, should only be called once */
void FsVfsDeinit(void)
{
#ifdef DEBUG
    if ( g_fVfsPresent )
    {
        StrPrintF( g_logBuf, "FsVfsDeinit(): VFS present, %d volumes", g_VfsVolumeCount );    
        LogG( g_logBuf );
    }
    else
    {
        LogG( "FsVfsDeinit(): VFS not present");
    }
#endif
    g_fVfsPresent = false;
}

/* Return true if VFS is present. Should call VfsInit before calling it. */
Boolean FFsVfsPresent(void)
{
    return g_fVfsPresent;
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
    char    *fullFileName;

    /* full name is a sum of lenghts of dir and file plus one for a separator
    and one for trailing zero */
    strLen = StrLen(dir) + StrLen(file) + 2;    
    fullFileName = new_malloc_zero(strLen);
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
        DrawDebug2("rh/fo() fail:", fileName);
        goto Error;
    }

    err = VFSFileRead(fileRef, sizeof(PdbHeader), (void *) hdr, &bytesRead);
    if ((err != errNone) && (bytesRead != sizeof(PdbHeader)))
    {
        DrawDebug2("rh/fr() fail:", fileName);
        goto Error;
    }

    if ((0 == hdr->recordsCount) || (hdr->recordsCount > 300))
    {
        /* I assume that this is a bogus file (non-PDB) */
        goto Error;
    }

    if (errNone != VFSFileClose(fileRef))
    {
        DrawDebug2("rh/fc fail:", fileName);
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
void FsVfsFindDb( FIND_DB_CB *cbCheckFile )
{
    Err             err;
    FileRef         dirRef;
    char            *currDir;
    char            *newDir;
    char            *fileName;
    UInt16          currVolRef;
    UInt32          dirIter;
    FileInfoType    fileInfo;
    StringStack     dirsToVisit;
    Boolean         fRecursive = true;
    PdbHeader       hdr;
    AbstractFile    *file;
    int             currVolume;

    if (!FFsVfsPresent())
    {
        LogG( "FsVfsFindDb(): VFS not present" );
        return;
    }        

    MemSet( &fileInfo, 0, sizeof(fileInfo) );
    fileInfo.nameBufLen = VFS_MAX_PATH_SIZE;
    fileInfo.nameP = new_malloc_zero(fileInfo.nameBufLen);
    if ( NULL == fileInfo.nameP ) return;

    currVolume = 0;
ScanNextVolume:
    if (currVolume >= g_VfsVolumeCount) goto NoMoreFiles;
    currVolRef = g_VfsVolumeRef[currVolume++];

    /* restart iterating over directories */
    currDir = strdup( "/" );
    if ( NULL == currDir ) goto NoMoreFiles;

    ssInit(&dirsToVisit);
    ssPush(&dirsToVisit, currDir );

ScanNextDir:
    currDir = ssPop(&dirsToVisit);
    if ( NULL == currDir) goto NoMoreFiles;
    err = VFSFileOpen(currVolRef, currDir, vfsModeRead, &dirRef);
    if (err != errNone)
        goto NoMoreFiles;

    dirIter = expIteratorStart;
    while (dirIter != expIteratorStop)
    {
        err = VFSDirEntryEnumerate(dirRef, &dirIter, &fileInfo);
        if (err != errNone)
        {
            DrawDebug2Num("VFSDirEnum() failed", err);
            goto NoMoreFiles;
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
            file = NULL;
            if ( ReadPdbHeader(currVolRef, fileName, &hdr ) )
            {
                file = AbstractFileNewFull( eFS_VFS, hdr.creator, hdr.type, fileName );
            }
            new_free(fileName);
            if (file)
            {
                file->volRef = currVolRef;
                (*cbCheckFile)(file);
                AbstractFileFree(file);
            }
        }
    }

    Assert(expIteratorStop == dirIter);
    Assert( 0 != dirRef );
    new_free(currDir);
    VFSFileClose(dirRef);
    dirRef = 0;
    if (fRecursive)
        goto ScanNextDir;

    goto ScanNextVolume;

NoMoreFiles:
    if (0 != dirRef)
        VFSFileClose(dirRef);

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

    err = VFSFileOpen(file->volRef, file->fileName, vfsModeRead, &fileRef);
    if (errNone != err)
    {
        DrawDebug2("VFSFileOpen() failed:", file->fileName);
        goto Error;
    }

    err = VFSFileSeek(fileRef, vfsOriginBeginning, offset);
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
            goto Error;
        }

        err = DmWrite(dstBuf, currentOffset, buf, copyThisTime);
        if (err)
        {
            goto Error;
        }
        currentOffset += copyThisTime;
        leftToCopy -= copyThisTime;
    }

    Assert(0 == leftToCopy);
    err = VFSFileClose(fileRef);
    if (errNone != err)
    {
        DrawDebug("FfsClose() failed");
        goto Error;
    }

    return errNone;
  Error:
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

    MemSet( &hdr, 0, sizeof(hdr) );
    if ( !ReadPdbHeader( file->volRef, file->fileName, &hdr ) )
        return false;

    cacheData->recsCount = hdr.recordsCount;
    cacheData->recsInfo = (OneVfsRecordInfo*) new_malloc(cacheData->recsCount * sizeof(OneVfsRecordInfo));

    if (NULL == cacheData->recsInfo)
    {
        DrawDebug2Num("no recsInfo", cacheData->recsCount);
        goto Error;
    }

    err = VFSFileOpen(file->volRef, file->fileName, vfsModeRead, &fileRef);
    if (errNone != err)
    {
        fileRef = 0;
        goto Error;
    }

    for (i = 0; i < cacheData->recsCount; i++)
    {

        err = VFSFileRead(fileRef, sizeof(PdbRecordHeader), (void *) &pdbRecHeader, &bytesRead);
        if ((err != errNone) || (bytesRead != sizeof(PdbRecordHeader)))
        {
            // that's ok, the file on external memory card doesn't have to be *pdb file
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

    if (errNone != VFSFileClose(fileRef))
    {
        goto Error;
    }

    return true;

Error:
    if (0 != fileRef)
        VFSFileClose(fileRef);
    return false;
}

