/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */

#include "common.h"
#include "display_info.h"

/* local functions */
static void addLine(DisplayInfo * di, int lineNo, char *p);
static int  rawTxtLinesCount(char *txt);
static void diCreateLinesInfo(DisplayInfo * di);

DisplayInfo *
diNew(void)
{
    return (DisplayInfo *) new_malloc_zero(sizeof(DisplayInfo));
}

void
diFree(DisplayInfo * di)
{
    if (!di)
    {
        return;
    }
    diFreeData(di);
    new_free((void *) di);
    return;
}

void
diFreeData(DisplayInfo * di)
{
    Assert(di);

    if (di->data)
    {
        new_free((void *) di->data);
    }

    if (di->lines)
    {
        new_free((void *) di->lines);
    }
    MemSet(di, sizeof(DisplayInfo), 0);
}

int
diGetLinesCount(DisplayInfo * di)
{
    return di->linesCount;
}

char *
diGetLine(DisplayInfo * di, int lineNo)
{
    return di->lines[lineNo];
}

int
diGetLineSize(DisplayInfo * di, int lineNo)
{
    char *line;
    line = diGetLine(di, lineNo);
    return StrLen(line);
}

/* calc the number of lines in raw text */
static int
rawTxtLinesCount(char *txt)
{
    int linesCount = 1;
    while (*txt)
    {
        if (('\n' == txt[0]))
        {
            ++linesCount;
        }
        ++txt;
    }
    if(txt[-1] =='\n')
        --linesCount;
    return linesCount;
}

static void
addLine(DisplayInfo * di, int lineNo, char *p)
{
    Assert(di);
    Assert(p);
    Assert((lineNo >= 0) && (lineNo < di->linesCount));

    //It was used to mark line as bold (I don't know if it is realy used)
    if (1 == p[0])
    {
        ++p;
    }
    di->lines[lineNo] = p;
}
/* create lines info in DisplayInfo based on it's raw txt */
static void
diCreateLinesInfo(DisplayInfo * di)
{
    char *p;
    int lineNo;
    int linesCount;

    Assert(di);

    linesCount = rawTxtLinesCount(di->data);
    if (di->linesTotalCount < linesCount)
    {
        /* need to re-allocate info about lines */
        di->linesTotalCount = linesCount;
        if (di->lines)
        {
            new_free(di->lines);
            di->lines = NULL;
        }

        di->lines = (char **) new_malloc(linesCount * sizeof(char *));
        if (NULL == di->lines)
        {
            return;
        }
    }

    di->linesCount = linesCount;
    p = di->data;
    lineNo = 0;
    addLine(di, lineNo++, p);
    while (*p)
    {
        if (*p == '\n')
        {
            if (p[1])
            {
                addLine(di, lineNo++, p + 1);
            }
            *p = 0;
        }
        ++p;
    }
}

/* replace the raw text in DisplayInfo with a given raw text
and create info about lines. Re-use existing memory allocated
for raw txt */
void
diSetRawTxt(DisplayInfo * di, char *rawTxt)
{
    Assert(di);
    Assert(rawTxt);

    if (di->size < StrLen(rawTxt) + 1)
    {
        if (di->data)
        {
            new_free(di->data);
        }

        /* need to reallocate memory for raw text */
        di->data = (char *) new_malloc(StrLen(rawTxt) + 1);
        if (NULL == di->data)
        {
            return;
        }
        di->size = StrLen(rawTxt);
    }

    MemMove(di->data, rawTxt, StrLen(rawTxt) + 1);

    diCreateLinesInfo(di);
    return;
}

void
convertRawTextToDisplayInfo(DisplayInfo * di, char *rawTxt)
{
    Assert(di);
    Assert(rawTxt);

    di->data = rawTxt;
    di->size = StrLen(rawTxt);

    di->linesTotalCount = 0;
    di->lines = NULL;

    diCreateLinesInfo(di);
}

void diCopyToClipboard(DisplayInfo *di)
{
    ExtensibleBuffer clipboardBuf;
    int              linesCount;
    int              i;
    char *           defTxt;
    int              defTxtLen;

    //if sth. is selected we will copy it instead of whole definition
    if(cbCopyToClippboard(GetAppContext()))
        return;

    Assert( di );
    ebufInit(&clipboardBuf,256);
    linesCount = diGetLinesCount(di);
    for (i = 0; i < linesCount; i++)
    {
        defTxt = diGetLine(di, i);
        ebufAddStr(&clipboardBuf, defTxt);
        ebufAddChar(&clipboardBuf, '\n');
    }
    //remove all display formating tags
    bfStripBufferFromTags(&clipboardBuf);
    
    defTxt = ebufGetDataPointer(&clipboardBuf);
    defTxtLen = StrLen(defTxt);
    ClipboardAddItem(clipboardText, defTxt, defTxtLen);
    ebufFreeData(&clipboardBuf);
}

