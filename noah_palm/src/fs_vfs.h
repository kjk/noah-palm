/*
  Copyright (C) 2000,2001, 2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)

  Implementation of VFS for standard VFS API in PalmOS 4.0 
  (Sony CLIE, Palm 505m, HandEra etc.)
 */
#ifndef _FS_VFS_H_
#define _FS_VFS_H_

#include <VFSMgr.h>
#include <ExpansionMgr.h>
#include <PalmOS.h>
#include "common.h"
#include "fs.h"
#include "fs_ex.h"

struct VfsData
{
    UInt16          volRef;
    FileRef         dirRef;
    FileInfoType    fileInfo;
    UInt32          dirIter;
    /* if false, we need to init iteration of findfirst()/findnext() */
    Boolean         iterationInitedP;
    Boolean         headerInitedP;    /* has the header already been loaded */

    char            name[dmDBNameLength];
    UInt32          dbCreator;
    UInt32          dbType;
    Boolean         recursiveP; /* recursive dir traversal or just one dir */
    Boolean         worksP;     /* has been initilized successfully? */

    char            *fullFileName;
    char            *currDir;

    StringStack     dirsToVisit;
};

Boolean FsVfsInit(void);
void    FsVfsDeinit(void);
Boolean FFsVfsPresent(void);

struct VfsData *vfsNew(void);
Err             vfsInit(struct VfsData *vfs);
void            VfsDeinit(struct VfsData *vfs);

UInt32          vfsGetDbCreator(struct VfsData *vfs);
UInt32          vfsGetDbType(struct VfsData *vfs);
char *          vfsGetDbName(struct VfsData *vfs);
Boolean         vfsReadHeader(struct VfsData *vfs);
Err             vfsCopyExternalToMem(struct VfsData *vfs, UInt32 offset, UInt32 size, void *dstBuf);

#endif
