/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/
#ifndef _MEM_LEAK_H_
#define _MEM_LEAK_H_

#include <PalmOS.h>
#include "cw_defs.h"

#ifdef MEM_LEAKS

MemPtr  new_malloc_fn(long size, char *file, long line);
void    new_free_fn(MemPtr addr, char *file, long line);

#define new_malloc(size) new_malloc_fn(size,__FILE__,__LINE__)
#define new_malloc_zero(size) new_malloc_fn(size,__FILE__,__LINE__)
#define new_free(addr) new_free_fn(addr,__FILE__,__LINE__)

#else 

#define new_malloc(size) MemPtrNew(size)
#define new_free(addr)   MemPtrFree(addr)
MemPtr  new_malloc_zero(long size);

#endif

#endif
