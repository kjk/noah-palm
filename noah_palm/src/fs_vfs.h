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

Boolean FsVfsInit(void);
void    FsVfsDeinit(void);
Boolean FFsVfsPresent(void);

Boolean vfsInitCacheData(AbstractFile *file, struct DbCacheData *cacheData);
Err     vfsCopyExternalToMem(AbstractFile *file, UInt32 offset, UInt32 size, void *dstBuf);
#endif
