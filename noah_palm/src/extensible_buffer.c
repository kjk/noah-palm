/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/

#include <PalmOS.h>
#include "extensible_buffer.h"
#include "common.h"
#include "better_formatting.h"

#ifdef NOAH_PRO

#include "armlet_runner.h"

#endif
/* Kind of like a constructor in C++ */
ExtensibleBuffer *ebufNew(void)
{
    ExtensibleBuffer *buf = NULL;
    buf = (ExtensibleBuffer *) new_malloc_zero(sizeof(ExtensibleBuffer));
    if (NULL == buf)
        return NULL;
    ebufInit(buf,0);
    return buf;
}

/* Kind of like a destructor in C++ */
void ebufDelete(ExtensibleBuffer *buf)
{
    ebufFreeData(buf);
    new_free(buf);
}

/* Initialize the object */
void ebufInit(ExtensibleBuffer *buf, int minSize)
{
    buf->minSize = minSize;
    buf->allocated = minSize;
    buf->used = 0;
    if ( 0 != minSize )
        buf->data = (char *) new_malloc(minSize);
    else
        buf->data = NULL;
}

/* Initilize buffer with a string */
void ebufInitWithStr(ExtensibleBuffer *buf, char *s)
{
    int size;
    size = StrLen(s);
    ebufInit(buf,size+1);
    ebufAddStr(buf,s);
}

/* Initilize buffer with a string of given length */
void ebufInitWithStrN(ExtensibleBuffer *buf, char *s, int len)
{
    ebufInit(buf, len+1);
    ebufAddStrN(buf,s,len);
}


/* Reset the buffer i.e., mark as clear but don't free the data.
Essential for re-use to avoid re-allocating this over and
over again */
void ebufReset(ExtensibleBuffer *buf)
{
    buf->used = 0;
    if (buf->allocated > 0)
    {
        Assert(buf->data);
        MemSet(buf->data, buf->allocated, 0);
    }
}

void ebufResetWithStr(ExtensibleBuffer *buf, char *s)
{
    ebufReset(buf);
    ebufAddStr(buf, s);
}

void ebufFreeData(ExtensibleBuffer *buf)
{
    if (buf->data)
        new_free(buf->data);
    ebufInit(buf, 0);
}

void ebufReplaceChar(ExtensibleBuffer *buf, char c, int pos)
{
    if (pos > buf->used)
        return;
    buf->data[pos] = c;
}

void ebufInsertChar(ExtensibleBuffer *buf, char c, int pos)
{
    int newAllocated;
    char *newData;

    /* we can only insert up to the last pos in the buffer */
    if (pos > buf->used)
        return;

    if ((buf->used + 1) >= buf->allocated)
    {
        if (0 == buf->allocated)
        {
            newAllocated = 256;
        }
        else
        {
            newAllocated = buf->allocated + 256;
        }
        newData = (char *) new_malloc(newAllocated);
        if (!newData)
            return;
        if (buf->data)
        {
            MemMove(newData, buf->data, buf->used);
        }
        buf->allocated = newAllocated;
        if (buf->data)
            new_free(buf->data);
        buf->data = newData;
    }
    /* make room if inserting */
    if (pos < buf->used)
    {
        MemMove(&(buf->data[pos + 1]), &(buf->data[pos]), buf->used - pos);
    }
    buf->data[pos] = c;
    ++buf->used;
}

int ebufGetDataSize(ExtensibleBuffer *buf)
{
    return buf->used;
}

int ebufGetBufSize(ExtensibleBuffer *buf)
{
    return buf->allocated;
}

/*
  Return a pointer to a shared copy of text in the buffer
*/
char *ebufGetDataPointer(ExtensibleBuffer *buf)
{
    return buf->data;
}

/*
  Take over the ownership of the text accumulated in the buffer.
 */
char *ebufGetTxtOwnership(ExtensibleBuffer *buf)
{
    char *tmp;

    tmp = buf->data;
    ebufInit(buf, buf->minSize);
    return tmp;
}

/*
  Return the copy of the text in the buffer. Caller needs to free it.
*/
char *ebufGetTxtCopy(ExtensibleBuffer *buf)
{
    char *copy = NULL;

    if (0 == buf->used)
        return NULL;

    /* FIXME: a bit hackish; I allocate one byte more to make sure
       that there is always a 0 at the end because at this time I dont
       guarantee that buf->used also includes 0 at the end of buf->data
       (somehow I should, but... */
    copy = (char *) new_malloc(buf->used + 1);
    if (copy)
        MemMove(copy, buf->data, buf->used);
    return copy;
}

void ebufAddChar(ExtensibleBuffer *buf, char c)
{
    ebufInsertChar(buf, c, buf->used);
}

void ebufAddStrN(ExtensibleBuffer *buf, char *str, int strLen)
{
    int i;
    for (i = 0; i < strLen; i++)
        ebufInsertChar(buf, str[i], buf->used);
}

void ebufAddStr(ExtensibleBuffer * buf, char *str)
{
    ebufAddStrN(buf, str, StrLen(str));
}

//delete one char from buffer
//we will not free memory!!!
void ebufDeleteChar(ExtensibleBuffer *buf, int pos)
{
    if (pos > buf->used || pos < 0)
        return;
    if (pos < buf->used - 1)
        MemMove(&(buf->data[pos]), &(buf->data[pos+1]), buf->used - pos - 1);
    buf->used--;
}

//insert string into buf[pos]
void ebufInsertStringOnPos(ExtensibleBuffer *buf, char *string, int pos)
{
    int i;
    i = StrLen(string) - 1;
    for(; i >= 0; i--)
        ebufInsertChar(buf, string[i], pos);
}

/*make a copy of buffer - faster then inserting char by char*/
void ebufCopyBuffer(ExtensibleBuffer *dst, ExtensibleBuffer *src)
{
    char *newData;
    if(dst->allocated < src->allocated)
    {
        newData = (char *) new_malloc(src->allocated);
        if (!newData)
            return;
        if(dst->data)
            new_free(dst->data);
        dst->data = newData;
        dst->allocated = src->allocated;
    }
    MemMove(dst->data, src->data, src->allocated);
    dst->used = src->used;
}

/* wrap all lines that are too long to fit on the screen */
void ebufWrapBigLines(ExtensibleBuffer *buf, Boolean sort)
{
    int         len;
    char *      txt;
    char *      txt2;
    char *      txt3;
    int         curr_pos;
    int         spaces;
    int         line_len;
    AppContext* appContext=GetAppContext();
    FontID      prevfont;

    prevfont = FntGetFont();

    curr_pos = 0;
    if(appContext->prefs.displayPrefs.listStyle!=0)
    {
        if(sort)
            ShakeSortExtBuf(buf);
        Format1OnSortedBuf(appContext->prefs.displayPrefs.listStyle, buf);
#ifndef NOAH_PRO
        Format2OnSortedBuf(appContext->prefs.displayPrefs.listStyle, buf);
#else
        if(appContext->prefs.displayPrefs.listStyle==2)
            if(appContext->armIsPresent)
                armFormat2onSortedBuffer(buf);
            else
                Format2OnSortedBuf(2, buf);
#endif   
    }
    txt = buf->data;
    len = buf->used;

    /* find every line, calculate the number of spaces at its
       beginning & fetch this to a proc that will wrap this line
       so it'll fit into a screen */
    /* this is the first line */
    line_len = curr_pos;
    while ((line_len < len)
        && (txt[line_len] == ' '))
        ++line_len;
    spaces = line_len - curr_pos;
    while ((line_len < len)
        && (txt[line_len] != '\n'))
        ++line_len;
    line_len -= curr_pos;    
    curr_pos += ebufWrapLine(buf, curr_pos, line_len, spaces, appContext);
    txt = buf->data;
    len = buf->used;
    /* this is the rest */
    ++curr_pos;
    txt2 = txt + curr_pos;    
    txt3 = txt + len;
    do
    {
        if (txt2[0] == '\n')
        {
            curr_pos = txt2 - txt;
            ++curr_pos;
            line_len = curr_pos;
            while ((line_len < len)
                   && (txt[line_len] == ' '))
                ++line_len;
            spaces = line_len - curr_pos;
            while ((line_len < len)
                   && (txt[line_len] != '\n'))
                ++line_len;
            line_len -= curr_pos;    
            curr_pos += ebufWrapLine(buf, curr_pos, line_len, spaces, appContext);
            /* this might have changed our buffer, so we have to re-get it */
            txt = buf->data;
            len = buf->used;
            txt2 = txt + curr_pos;    
            txt3 = txt + len;
        }
        txt2++;
    }
    while (txt2 < txt3);
    FntSetFont(prevfont);
}

void ebufSwap(ExtensibleBuffer* buf1, ExtensibleBuffer* buf2)
{
    ExtensibleBuffer temp=*buf1;
    *buf1=*buf2;
    *buf2=temp;
}
