/*
  Copyright (C) 2000-2002 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _VFS_DM_H_
#define _VFS_DM_H_

#include <PalmOS.h>

typedef struct
{
    void    *data;
    long    size;
    int     lockCount;
} OneMemRecordInfo;

#define dmDBNameLength    32

typedef struct
{
    /* if false, we need to init iteration of findfirst()/findnext() */
    Boolean             iterationInitedP;
    DmOpenRef           openDb;
    DmSearchStateType   stateInfo;

    char                name[32];
    UInt16              cardNo;
    LocalID             dbId;

    UInt16              recsCount;  /* number of records in the pdb */

    UInt32              dbCreator;
    UInt32              dbType;
    /* for each record infor about it's size and the
       offset of the record in the file */
    OneMemRecordInfo     *recsInfo;
} MemData;

void setVfsToMem(Vfs *vfs);
#endif
