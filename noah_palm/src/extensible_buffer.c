/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/

#include <PalmOS.h>
#include "extensible_buffer.h"
#include "common.h"
#include "better_formatting.h"

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
    if (!copy)
        return NULL;

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
        ebufAddChar(buf, str[i]);
}

void ebufAddStr(ExtensibleBuffer * buf, char *str)
{
    ebufAddStrN(buf, str, StrLen(str));
}

/*
  if a give line doesn't fit in one line on a display, wrap it
  into as many lines as needed
 */
/* 
void ebufWrapLine(ExtensibleBuffer *buf, int line_start, int line_len, int spaces_at_start)
{
    char *txt;
    Int16 string_dx;
    Int16 string_len;
    Boolean fits;
    Boolean found;
    int pos;
    int i;

    txt = ebufGetDataPointer(buf);
    if (NULL == txt)
        return;

    string_dx = 152;
    string_len = line_len;
    txt += line_start;

    fits = false;
    FntCharsInWidth(txt, &string_dx, &string_len, &fits);
    if (fits)
        return;
 
    pos = string_len - 1;
    found = false;
    while ((pos > 0) && (found == false))
    {
        switch (txt[pos])
        {
        case ' ':
            found = true;
            txt[pos] = '\n';
            for (i = 0; i < spaces_at_start; i++)
            {
                ebufInsertChar(buf, ' ', line_start + pos + i + 1);
            }
            pos += spaces_at_start;
            break;
        case ';':
        case ',':
            found = true;
            ebufInsertChar(buf, '\n', line_start + pos + 1);
            for (i = 0; i < spaces_at_start; i++)
            {
                ebufInsertChar(buf, ' ', line_start + pos + i + 2);
            }
            pos += spaces_at_start + 1;
            break;
        }
        --pos;
    }
    if (found == false)
    {
        ebufInsertChar(buf, '\n', line_start + string_len - 1);
        pos = line_start + string_len;
    }
}
*/
/* wrap all lines that are too long to fit on the screen */
void ebufWrapBigLines(ExtensibleBuffer *buf)
{
    int len;
    char *txt;
    int curr_pos;
    int spaces;
    int line_len;
    Boolean line_start;
    AppContext* appContext=GetAppContext();
    FontID prevfont;

    prevfont = FntGetFont();

    curr_pos = 0;
    txt = ebufGetDataPointer(buf);
    len = ebufGetDataSize(buf);

    if(GetDisplayListStyle(appContext)!=0)
    {
        ShakeSortExtBuf(buf);
        Format1OnSortedBuf(GetDisplayListStyle(appContext), buf);
        Format2OnSortedBuf(GetDisplayListStyle(appContext), buf);
        txt = ebufGetDataPointer(buf);
        len = ebufGetDataSize(buf);
    }    

    /* find every line, calculate the number of spaces at its
       beginning & fetch this to a proc that will wrap this line
       so it'll fit into a screen */
    /* this is the first line */
    line_start = true;
    do
    {
        if (line_start)
        {
            line_len = 0;
            while ((curr_pos + line_len < len)
                   && (txt[curr_pos + line_len] == ' '))
                ++line_len;
            spaces = line_len;
            while ((curr_pos + line_len < len)
                   && (txt[curr_pos + line_len] != '\n'))
                ++line_len;
            ebufWrapLine(buf, curr_pos, line_len, spaces, appContext);
            /* this might have changed our buffer, so we have to re-get it */
            txt = ebufGetDataPointer(buf);
            len = ebufGetDataSize(buf);
            line_start = false;
        }
        /* find another line */
        if ((len > 0) && (txt[curr_pos] == '\n'))
            line_start = true;
        ++curr_pos;
    }
    while (curr_pos < len);
    FntSetFont(prevfont);
}

