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

/* Iterate over all files on the vfs file system and call the callback
for each file */
void FsVfsFindDb( FIND_DB_CB *cbCheckFile )
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

#if 0
// TODO: remove ?
void vfsStdSetCurrentDir(struct VfsData *vfs, char *dir)
{
    // undone: really necessary?
    ssDeinit( &vfs->dirsToVisit );
    ssPush( &vfs->dirsToVisit, dir );
}
#endif

// max size of VFS path. 256+40 is a bit magical, hopefully this is
// more than enough (256 according to docs)
#define VFS_MAX_PATH_SIZE 256+40

void VfsDeinit(struct VfsData *vfs)
{
    ssDeinit( &vfs->dirsToVisit );
    //dcDeinit(&vfs->cacheData);

    vfs->worksP = false;

    if ( vfs->fileInfo.nameP)
    {
        new_free( vfs->fileInfo.nameP );
        vfs->fileInfo.nameP = NULL;
    }

    if ( vfs->fullFileName )
    {
        new_free( vfs->fullFileName );
        vfs->fullFileName = NULL;
    }

    if ( vfs->currDir )
    {
        new_free( vfs->currDir );
        vfs->currDir = NULL;
    }

    if (0 != vfs->dirRef)
    {
        VFSFileClose(vfs->dirRef);
        vfs->dirRef = 0;
    }
}

struct VfsData *vfsNew(void)
{
    struct VfsData *data = (struct VfsData*) new_malloc( sizeof(struct VfsData) );
    if ( errNone != vfsInit(data) )
    {
        new_free( data );
        return NULL;
    }
    return data;
}

/* Init VFS. Return true if exist and inited properly */
Err vfsInit(struct VfsData *vfs)
{
    Err         err = errNone;
    UInt32      vfsMgrVersion;
    UInt16      volRef;
    UInt32      volIterator;

    MemSet( vfs, sizeof(struct VfsData), 0 );
    //dcInit(&vfs->cacheData);

    vfs->recursiveP = true;
    vfs->worksP = false;

    vfs->fileInfo.nameP = new_malloc(VFS_MAX_PATH_SIZE);
    if ( NULL == vfs->fileInfo.nameP )
    {
        VfsDeinit(vfs);
        return ERR_NO_MEM;
    }
    vfs->fileInfo.nameBufLen = VFS_MAX_PATH_SIZE;

    vfs->fullFileName = new_malloc(VFS_MAX_PATH_SIZE);
    if ( NULL == vfs->fullFileName )
    {
        VfsDeinit(vfs);
        return ERR_NO_MEM;
    }
    vfs->currDir = NULL;
    ssInit(&vfs->dirsToVisit);

    err = FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &vfsMgrVersion);
    if (err)
    {
        DrawDebug("FtrGet(sysFileCVSMgr) failed");
        VfsDeinit(vfs);
        return err;
    }


    volIterator = expIteratorStart;
    /* for now only enumerate one volume */
    while (volIterator != vfsIteratorStop)
    {
        err = VFSVolumeEnumerate(&volRef, &volIterator);
        if (errNone == err)
        {
            vfs->volRef = volRef;
            volIterator = vfsIteratorStop;
        }
        else
        {
            DrawDebug("FtrGet(sysFileCVSMgr) failed");
            VfsDeinit(vfs);
            return false;
        }
    }
    vfs->worksP = true;
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

Err vfsCopyExternalToMem(struct VfsData *vfs, UInt32 offset, UInt32 size, void *dstBuf)
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

    err = VFSFileOpen(vfs->volRef, vfs->fullFileName, vfsModeRead, &fileRef);
    if (errNone != err)
    {
        DrawDebug2("VFSFileOpen() failed:", vfs->fullFileName);
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

/* read all the necessary info from the database header
   (creator, type, name, number of records, offset in the
   file for each record).
   Return false if sth. goes really wrong
*/
Boolean vfsReadHeader(struct VfsData *vfs)
{
    Err                err = errNone;
    FileRef            fileRef = 0;
    UInt32             bytesRead;
    PdbHeader          hdr;
    Int16              i;
    UInt32             fileSize;
    UInt32             recSize;
    PdbRecordHeader    pdbRecHeader;
#ifdef NOAH_PRO
    char               recSizeBuf[30];
    char               recNumBuf[10];
#endif

    if (true == vfs->headerInitedP)
    {
        return true;
    }

    err = VFSFileOpen(vfs->volRef, vfs->fullFileName, vfsModeRead, &fileRef);

    if (errNone != err)
    {
        fileRef = 0;
        DrawDebug2("fo() fail:", vfs->fullFileName);
        goto Error;
    }

    err = VFSFileRead(fileRef, sizeof(PdbHeader), (void *) &hdr, &bytesRead);
    if ((err != errNone) && (bytesRead != sizeof(PdbHeader)))
    {
        DrawDebug2("fr() fail:", vfs->fullFileName);
        goto Error;
    }

    vfs->dbCreator = hdr.creator;
    vfs->dbType = hdr.type;

    vfs->cacheData.recsCount = hdr.recordsCount;

/*      DrawDebug2Num ( "recsCount:", (UInt32) hdr.recordsCount ); */
    MemMove(vfs->name, hdr.name, dmDBNameLength);

    if ((0 == vfs->cacheData.recsCount) || (vfs->cacheData.recsCount > 200))
    {
        /* I assume that this is a bogus file (non-PDB) */
        goto Error;
    }

    if (vfs->cacheData.recsInfo)
    {
        new_free(vfs->cacheData.recsInfo);
        vfs->cacheData.recsInfo = NULL;
    }
    vfs->cacheData.recsInfo = (OneVfsRecordInfo*) new_malloc(vfs->cacheData.recsCount * sizeof(OneVfsRecordInfo));

    if (NULL == vfs->cacheData.recsInfo)
    {
        DrawDebug2Num("no recsInfo", vfs->cacheData.recsCount);
        goto Error;
    }

    for (i = 0; i < vfs->cacheData.recsCount; i++)
    {

        err = VFSFileRead(fileRef, sizeof(PdbRecordHeader), (void *) &pdbRecHeader, &bytesRead);
        if ((err != errNone) || (bytesRead != sizeof(PdbRecordHeader)))
        {
            // that's ok, the file on external memory card doesn't have to be *pdb file
            goto Error;
        }
        vfs->cacheData.recsInfo[i].offset = pdbRecHeader.recOffset;
    }


    /* calc sizes of all records, size of rec n is offset of rec n+1
       - offset of rec n */
    for (i = 1; i < vfs->cacheData.recsCount; i++)
    {
        recSize = (vfs->cacheData.recsInfo[i].offset - vfs->cacheData.recsInfo[i -1].offset);

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

        vfs->cacheData.recsInfo[i - 1].size = (UInt16) recSize;
    }
    fileSize = stdGetFileSize(fileRef);
    if (0 == fileSize)
    {
        goto Error;
    }
    i = vfs->cacheData.recsCount - 1;
    vfs->cacheData.recsInfo[i].size = (UInt16) (fileSize - vfs->cacheData.recsInfo[i].offset);

    /* initialize data for caching purposes */
    for (i = 0; i < vfs->cacheData.recsCount; i++)
    {
        vfs->cacheData.recsInfo[i].data = NULL;
        vfs->cacheData.recsInfo[i].lockCount = 0;
    }

    if (errNone != VFSFileClose(fileRef))
    {
        DrawDebug("vfsReadHeader() VFSFileClose() fail");
        goto Error;
    }

    vfs->headerInitedP = true;
    return true;
  Error:
    DrawDebug2("rh() f", vfs->fullFileName);
    if (0 != fileRef)
    {
        VFSFileClose(fileRef);
    }
    return false;
}

UInt32 vfsGetDbCreator(struct VfsData *vfs)
{
    vfsReadHeader(vfs);
    return vfs->dbCreator;
}

UInt32 vfsGetDbType(struct VfsData *vfs)
{
    vfsReadHeader(vfs);
    return vfs->dbType;
}

char *vfsGetDbName(struct VfsData *vfs)
{
    vfsReadHeader(vfs);
    return vfs->name;
}

char *stdGetDbPath(struct VfsData *vfs)
{
    vfsReadHeader(vfs);
    return vfs->fullFileName;
}

/* Init iteration over files in VFS. */
static void stdIterateRestart(struct VfsData *vfs)
{
    vfs->iterationInitedP = false;
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
Boolean stdIterateOverDbs(struct VfsData *vfs)
{
    Err     err;

    vfs->headerInitedP = false;

    if (false == vfs->iterationInitedP)
    {
        if (0 != vfs->dirRef)
        {
            VFSFileClose(vfs->dirRef);
            vfs->dirRef = 0;
        }

        if (vfs->currDir)
            new_free(vfs->currDir);

        vfs->currDir = ssPop( &vfs->dirsToVisit );
        // exit if no more dirs to visit
        if ( NULL == vfs->currDir )
            goto NoMoreFiles;

        err = VFSFileOpen(vfs->volRef, vfs->currDir, vfsModeRead, &vfs->dirRef);
        if (err != errNone)
        {
            DrawDebug("iter_over_db() VFSFileOpen() fail");
            goto NoMoreFiles;
        }
        vfs->iterationInitedP = true;
        vfs->dirIter = expIteratorStart;
    }

    while (vfs->dirIter != expIteratorStop)
    {
        err = VFSDirEntryEnumerate(vfs->dirRef, &vfs->dirIter, &vfs->fileInfo);
        if (err != errNone)
        {
            DrawDebug2Num("VFSDirEnum() failed", err);
            goto NoMoreFiles;
        }

        // if a dir - add to the list of dirs to visit next
        if (isDirP(vfs->fileInfo.attributes) && vfs->recursiveP )
        {
            BuildFullPath( vfs->currDir, vfs->fileInfo.nameP, vfs->fullFileName );
            ssPush( &vfs->dirsToVisit, vfs->fullFileName );
        } 
        else if (isFileP(vfs->fileInfo.attributes))
        {
            BuildFullPath( vfs->currDir, vfs->fileInfo.nameP, vfs->fullFileName );
            return true;
        }
    }

    Assert(expIteratorStop == vfs->dirIter);
    // this could be a tail recursion
    if (vfs->recursiveP )
    {
        vfs->iterationInitedP = false;
        return stdIterateOverDbs(vfs);
    }

NoMoreFiles:
    vfs->iterationInitedP = false;
    if (0 != vfs->dirRef)
    {
        VFSFileClose(vfs->dirRef);
        vfs->dirRef = 0;
    }
    return false;
}

