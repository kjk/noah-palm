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

struct MemData
{
    AbstractFile *      file;       /* link back to the file description */
    DmOpenRef           openDb;      /* number of records in the pdb */
    UInt16              recsCount;
    OneMemRecordInfo *  recsInfo;   /* for each record infor about it's size and the
                                       offset of the record in the file */
};

Boolean memOpenDb(struct MemData *memData);
struct MemData *memNew(AbstractFile *file);
void    memInit(struct MemData *memData,AbstractFile *file);
void    memDeinit(struct MemData *memData);
UInt16  memGetRecordsCount(struct MemData *memData);
void *  memLockRecord(struct MemData *memData,UInt16 recNo);
void    memUnlockRecord(struct MemData *memData, UInt16 recNo);
long    memGetRecordSize(struct MemData *memData, UInt16 recNo);
void *  memLockRegion(struct MemData *memData, UInt16 recNo, UInt16 offset, UInt16 size);
void    memUnlockRegion(struct MemData *memData, char *regionPtr);
#endif
