/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "fs.h"
#include "fs_ex.h"
#include "fs_vfs.h"
#include "extensible_buffer.h"

#define MAX_VFS_VOLUMES 3
static Boolean g_fVfsPresent = false;
static Boolean g_VfsVolumeCount = 0;
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
    Boolean   fPresent = false;
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
        goto Exit;
    }

    volIterator = expIteratorStart;
    /* for now only enumerate one volume */
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
    return fPresent;
}

/* De-initialize vfs, should only be called once */
void FsVfsDeinit(void)
{
    g_fVfsPresent = false;
}

/* Return true if VFS is present. Should call VfsInit before calling it. */
Boolean FFsVfsPresent(void)
{
    return g_fVfsPresent;
}

void FsVfsFindDb( IS_DB_OK *cbDbOk FIND_DB_CB *cbDbFound )
{
    if (!FFsVfsPresent()) return;

    /* TODO: make it work */
}


void BuildFullPath(char *dir, char *file, char *fullPath)
{
    int     strLen;
    StrCopy( fullPath, dir );
    strLen = StrLen(fullPath);
    if ( (strLen>0) && ('/' != fullPath[strLen-1]) )
        StrCat( fullPath, "/" );
    StrCat( fullPath, file );
}

UInt16   VfsUnlockRegion(VfsData *data)
{
    return dcGetRecordsCount(&data->cacheData);
}

long VfsGetRecordSize(VfsData *data, UInt16 recNo)
{
    return dcGetRecordSize(&data->cacheData, recNo);
}

void *VfsLockRecord(VfsData *data, UInt16 recNo)
{
    return dcLockRecord(&data->cacheData, recNo);
}

void  VfsUnlockRecord(VfsData *data, UInt16 recNo)
{
    return dcUnlockRecord(&data->cacheData, recNo);
}

void  *VfsLockRegion(VfsData *data, UInt16 recNo, UInt16 offset, UInt16 size)
{
    return dcLockRegion( &data->cacheData, recNo, offset, size);
}

void  VfsUnlockRegion(VfsData *data, char *dataPtr)
{
    return dcUnlockRegion(&data->cacheData, dataPtr);
}

#if 0
// TODO: remove ?
void vfsStdSetCurrentDir(void *data, char *dir)
{
    VfsData *std = (VfsData*)data;
    // undone: really necessary?
    ssDeinit( &std->dirsToVisit );
    ssPush( &std->dirsToVisit, dir );
}
#endif

// max size of VFS path. 256+40 is a bit magical, hopefully this is
// more than enough (256 according to docs)
#define VFS_MAX_PATH_SIZE 256+40

static void VfsDeinit(void *data)
{
    VfsData *std = (VfsData*)data;

    ssDeinit( &std->dirsToVisit );
    dcDeinit(&std->cacheData);

    std->worksP = false;

    if ( std->fileInfo.nameP)
    {
        new_free( std->fileInfo.nameP );
        std->fileInfo.nameP = NULL;
    }

    if ( std->fullFileName )
    {
        new_free( std->fullFileName );
        std->fullFileName = NULL;
    }

    if ( std->currDir )
    {
        new_free( std->currDir );
        std->currDir = NULL;
    }

    if (0 != std->dirRef)
    {
        VFSFileClose(std->dirRef);
        std->dirRef = 0;
    }
}

VfsData *stdNew(void)
{
    return NULL;
}

/* Init VFS. Return true if exist and inited properly */
static Err stdInit(void *data)
{
    Err         err = errNone;
    UInt32      vfsMgrVersion;
    UInt16      volRef;
    UInt32      volIterator;
    VfsData     *std = (VfsData*)data;

    MemSet( std, sizeof(VfsData), 0 );
    dcInit(&std->cacheData);

    std->recursiveP = true;
    std->worksP = false;

    std->fileInfo.nameP = new_malloc(VFS_MAX_PATH_SIZE);
    if ( NULL == std->fileInfo.nameP )
    {
        VfsDeinit(data);
        return ERR_NO_MEM;
    }
    std->fileInfo.nameBufLen = VFS_MAX_PATH_SIZE;

    std->fullFileName = new_malloc(VFS_MAX_PATH_SIZE);
    if ( NULL == std->fullFileName )
    {
        VfsDeinit(data);
        return ERR_NO_MEM;
    }
    std->currDir = NULL;
    ssInit(&std->dirsToVisit);

    err = FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &vfsMgrVersion);
    if (err)
    {
        DrawDebug("FtrGet(sysFileCVSMgr) failed");
        VfsDeinit(data);
        return err;
    }


    volIterator = expIteratorStart;
    /* for now only enumerate one volume */
    while (volIterator != vfsIteratorStop)
    {
        err = VFSVolumeEnumerate(&volRef, &volIterator);
        if (errNone == err)
        {
            std->volRef = volRef;
            volIterator = vfsIteratorStop;
        }
        else
        {
            DrawDebug("FtrGet(sysFileCVSMgr) failed");
            VfsDeinit(data);
            return false;
        }
    }
    std->worksP = true;
    return errNone;
}

/* get the size of the file, 0 means a failure */
static UInt32 stdGetFileSize(FileRef fileRef)
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

static Err stdCopyExternalToMem(void *data, UInt32 offset, UInt32 size, void *dstBuf)
{
    Err     err = errNone;
    char    buf[COPY_AT_ONCE];
    FileRef fileRef = 0;
    UInt32  leftToCopy;
    UInt32  currentOffset;
    UInt32  copyThisTime;
    UInt32  bytesRead;
    VfsData *std = (VfsData*)data;

    leftToCopy = size;
    currentOffset = 0;

    err = VFSFileOpen(std->volRef, std->fullFileName, vfsModeRead, &fileRef);
    if (errNone != err)
    {
        DrawDebug2("VFSFileOpen() failed:", std->fullFileName);
        goto Error;
    }

    err = VFSFileSeek(fileRef, fsOriginBeginning, offset);
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

/* read all the necessary info from the database header
   (creator, type, name, number of records, offset in the
   file for each record).
   Return false if sth. goes really wrong
*/
static Boolean stdReadHeader(void *data)
{
    Err                err = errNone;
    FileRef            fileRef = 0;
    UInt32             bytesRead;
    PdbHeader          hdr;
    Int16              i;
    UInt32             fileSize;
    UInt32             recSize;
    PdbRecordHeader    pdbRecHeader;
    char               recSizeBuf[30];
    char               recNumBuf[10];
    VfsData            *std = (VfsData*)data;

    if (true == std->headerInitedP)
    {
        return true;
    }

    err = VFSFileOpen(std->volRef, std->fullFileName, vfsModeRead, &fileRef);

    if (errNone != err)
    {
        fileRef = 0;
        DrawDebug2("fo() fail:", std->fullFileName);
        goto Error;
    }

    err = VFSFileRead(fileRef, sizeof(PdbHeader), (void *) &hdr, &bytesRead);
    if ((err != errNone) && (bytesRead != sizeof(PdbHeader)))
    {
        DrawDebug2("fr() fail:", std->fullFileName);
        goto Error;
    }

    std->dbCreator = hdr.creator;
    std->dbType = hdr.type;

    std->cacheData.recsCount = hdr.recordsCount;

/*      DrawDebug2Num ( "recsCount:", (UInt32) hdr.recordsCount ); */
    MemMove(std->name, hdr.name, dmDBNameLength);

    if ((0 == std->cacheData.recsCount) || (std->cacheData.recsCount > 200))
    {
        /* I assume that this is a bogus file (non-PDB) */
        goto Error;
    }

    if (std->cacheData.recsInfo)
    {
        new_free(std->cacheData.recsInfo);
        std->cacheData.recsInfo = NULL;
    }
    std->cacheData.recsInfo = (OneVfsRecordInfo*) new_malloc(std->cacheData.recsCount * sizeof(OneVfsRecordInfo));

    if (NULL == std->cacheData.recsInfo)
    {
        DrawDebug2Num("no recsInfo", std->cacheData.recsCount);
        goto Error;
    }

    for (i = 0; i < std->cacheData.recsCount; i++)
    {

        err = VFSFileRead(fileRef, sizeof(PdbRecordHeader), (void *) &pdbRecHeader, &bytesRead);
        if ((err != errNone) || (bytesRead != sizeof(PdbRecordHeader)))
        {
            // that's ok, the file on external memory card doesn't have to be *pdb file
            goto Error;
        }
        std->cacheData.recsInfo[i].offset = pdbRecHeader.recOffset;
    }


    /* calc sizes of all records, size of rec n is offset of rec n+1
       - offset of rec n */
    for (i = 1; i < std->cacheData.recsCount; i++)
    {
        recSize = (std->cacheData.recsInfo[i].offset - std->cacheData.recsInfo[i -1].offset);

#ifdef NOAH_PRO
#include "noah_pro_rcp.h"
        if ( recSize > 0xffff )
        {
            MemSet( recSizeBuf, sizeof(recSizeBuf), 0 );
            MemSet( recNumBuf, sizeof(recNumBuf), 0 );
            StrIToA(recSizeBuf, recSize);
            StrIToA(recNumBuf, (UInt32) i);
            FrmCustomAlert( alertBadRecSize, recNumBuf, recSizeBuf, " " );
        }
#endif

        std->cacheData.recsInfo[i - 1].size = (UInt16) recSize;
    }
    fileSize = stdGetFileSize(fileRef);
    if (0 == fileSize)
    {
        goto Error;
    }
    i = std->cacheData.recsCount - 1;
    std->cacheData.recsInfo[i].size = (UInt16) (fileSize - std->cacheData.recsInfo[i].offset);

    /* initialize data for caching purposes */
    for (i = 0; i < std->cacheData.recsCount; i++)
    {
        std->cacheData.recsInfo[i].data = NULL;
        std->cacheData.recsInfo[i].lockCount = 0;
    }

    if (errNone != VFSFileClose(fileRef))
    {
        DrawDebug("stdReadHeader() VFSFileClose() fail");
        goto Error;
    }

    std->headerInitedP = true;
    return true;
  Error:
    DrawDebug2("rh() f", std->fullFileName);
    if (0 != fileRef)
    {
        VFSFileClose(fileRef);
    }
    return false;
}

static UInt32 stdGetDbCreator(void *data)
{
    VfsData *std = (VfsData*)data;

    stdReadHeader(std);
    return std->dbCreator;
}

static UInt32 stdGetDbType(void *data)
{
    VfsData *std = (VfsData*)data;
    stdReadHeader(std);
    return std->dbType;
}

static char *stdGetDbName(void *data)
{
    VfsData *std = (VfsData*)data;
    stdReadHeader(std);
    return std->name;
}

static char *stdGetDbPath(void *data)
{
    VfsData *std = (VfsData*)data;

    stdReadHeader(std);
    return std->fullFileName;
}

/* Init iteration over files in VFS. */
static void stdIterateRestart(void *data)
{
    VfsData *std = (VfsData*)data;
    std->iterationInitedP = false;
}

/* return true if given file attributes represent a file
   we're interested in */
static Boolean isFileP(UInt32 fileAttr)
{
    if ((fileAttr &
         (vfsFileAttrHidden | vfsFileAttrSystem | vfsFileAttrVolumeLabel |
          vfsFileAttrDirectory)) != 0)
    {
        return false;
    }
    return true;
}

static Boolean isDirP(UInt32 fileAttr)
{
    if ( (fileAttr & (vfsFileAttrHidden | vfsFileAttrSystem | vfsFileAttrVolumeLabel )) != 0)
        return false;

    if ( 0 == (fileAttr & vfsFileAttrDirectory) )
        return false;

    return  true;
}

/* Iterate over files on external memory. If std.recursiveP then
   do a recursive scan.
   Return false if no more files.
 */
Boolean stdIterateOverDbs(void *data)
{
    Err     err;
    VfsData *std = (VfsData*)data;

    std->headerInitedP = false;

    if (false == std->iterationInitedP)
    {
        if (0 != std->dirRef)
        {
            VFSFileClose(std->dirRef);
            std->dirRef = 0;
        }

        if (std->currDir)
            new_free(std->currDir);

        std->currDir = ssPop( &std->dirsToVisit );
        // exit if no more dirs to visit
        if ( NULL == std->currDir )
            goto NoMoreFiles;

        err = VFSFileOpen(std->volRef, std->currDir, vfsModeRead, &std->dirRef);
        if (err != errNone)
        {
            DrawDebug("iter_over_db() VFSFileOpen() fail");
            goto NoMoreFiles;
        }
        std->iterationInitedP = true;
        std->dirIter = expIteratorStart;
    }

    while (std->dirIter != expIteratorStop)
    {
        err = VFSDirEntryEnumerate(std->dirRef, &std->dirIter, &std->fileInfo);
        if (err != errNone)
        {
            DrawDebug2Num("VFSDirEnum() failed", err);
            goto NoMoreFiles;
        }

        // if a dir - add to the list of dirs to visit next
        if (isDirP(std->fileInfo.attributes) && std->recursiveP )
        {
            BuildFullPath( std->currDir, std->fileInfo.nameP, std->fullFileName );
            ssPush( &std->dirsToVisit, std->fullFileName );
        } 
        else if (isFileP(std->fileInfo.attributes))
        {
            BuildFullPath( std->currDir, std->fileInfo.nameP, std->fullFileName );
            return true;
        }
    }

    Assert(expIteratorStop == std->dirIter);
    // this could be a tail recursion
    if (std->recursiveP )
    {
        std->iterationInitedP = false;
        return stdIterateOverDbs(data);
    }

NoMoreFiles:
    std->iterationInitedP = false;
    if (0 != std->dirRef)
    {
        VFSFileClose(std->dirRef);
        std->dirRef = 0;
    }
    return false;
}

