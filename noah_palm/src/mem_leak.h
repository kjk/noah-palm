/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/
#ifndef _MEM_LEAK_H_
#define _MEM_LEAK_H_

#include <PalmOS.h>

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

#ifdef __cplusplus 

#ifdef MEM_LEAKS

/**
 * Operator placement new for leak tracking purposes.
 * It's automatically used through macro @c new when @c MEM_LEAKS is defined.
 */
inline void* operator new(unsigned long size, const char* file, long line)
{
    return new_malloc_fn(size, const_cast<char*>(file), line);
}

/**
 * Complementary placement delete operator to <code>operator new(unsigned long, const char*, long)</code>.
 * Not really needed, as we don't use exceptions, so probably it won't be ever called. But if it was ever needed,
 * it's already here.
 */
inline void operator delete(void* p, const char* file, long line) 
{
    if (p) // yep, standard states it clearly: delete 0 is allowed (the same as free(0) - our new_free_fn() isn't compatible here), and is no-op.
        new_free_fn(p, const_cast<char*>(file), line);
}

/**
 * Plain operator delete called when @c MEM_LEAKS is defined.
 * It simply calls @c new_free_fn with empty string and line number 0,
 * as we can't pass these parameters the way it's done with new - 
 * unfortunatelly it's not possible to call custom operator delete.
 * Nevertheless, information about file/line of freeing memory isn't really important when tracking leaks,
 * as it plainly says that there's nothing to track :-)
 */
void operator delete(void* p);

#define new new(__FILE__, __LINE__)

#else

inline void* operator new(unsigned long size)
{
    return MemPtrNew(size);
}

inline void operator delete(void* p)
{
    if (p)
        MemPtrFree(p);
}

#endif // MEM_LEAKS

#endif // __cplusplus

#endif
