/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/
#include "common.h"

#ifdef MEM_LEAKS

/* #define LEAK_OUT_NAME "/home/kjk/src/dict_public/noah/palm/leaks.txt" */

#define LEAK_OUT_NAME "c:\\leaks.txt"

static void DeleteFileOnce()
{
    HostFILE        *hf = NULL;
    UInt32 value=0;
    Err error=FtrGet(APP_CREATOR, appFtrLeaksFile, &value);
    Assert(!error);
    /* delete the file if this is the first write */
    if ( !value )
    {
        hf = HostFOpen(LEAK_OUT_NAME, "w");
        if (hf)
        {
            value=1;
            HostFClose(hf);
            error=FtrSet(APP_CREATOR, appFtrLeaksFile, value);
        }
    }
}

MemPtr new_malloc_fn(long size, char *file, long line)
{
    MemPtr      addr = NULL;
    HostFILE    *hf = NULL;

    Assert(file);

    DeleteFileOnce();
    addr = MemPtrNew(size);
    if (addr) MemSet( addr, size, 0);
    hf = HostFOpen(LEAK_OUT_NAME, "a");
    if (hf)
    {
        HostFPrintF(hf, "@ 0x%lx 0x%lx %s %ld\n", (long) addr, (long) size,
                    file, line);
        HostFClose(hf);
    }
    return addr;
}

void new_free_fn(MemPtr addr, char *file, long line)
{
    HostFILE *hf = NULL;

    Assert(addr);
    Assert(file);

    DeleteFileOnce();
    MemPtrFree(addr);
    hf = HostFOpen(LEAK_OUT_NAME, "a");
    if (hf)
    {
        HostFPrintF(hf, "! 0x%lx %s %ld\n", (long) addr, file, line);
        HostFClose(hf);
    }
}

#else
MemPtr new_malloc_zero(long size)
{
    MemPtr addr = new_malloc(size);
    if (addr) MemSet( addr, size, 0);
    return addr;
}
#endif

