/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/
#ifndef _DISPLAY_INFO_H_
#define _DISPLAY_INFO_H_

#include <PalmOS.h>

typedef struct
{
    int         size;              /* total size of raw data */
    char *      data;              /* raw data */
    int         linesCount;        /* number of lines */
    int         linesTotalCount;   /* total number of lines allocated */
    char **     lines;             /* pointer to n-th line */
} DisplayInfo;

DisplayInfo * diNew(void);
void          diFree(DisplayInfo * di);
void          diFreeData(DisplayInfo * di);
int           diGetLinesCount(DisplayInfo * di);
char *        diGetLine(DisplayInfo * di, int lineNo);
int           diGetLineSize(DisplayInfo * di, int lineNo);
void          diSetRawTxt(DisplayInfo * di, char *rawTxt);
void          convertRawTextToDisplayInfo(DisplayInfo * di, char *rawTxt);
void          diCopyToClipboard( DisplayInfo *di);
#endif
