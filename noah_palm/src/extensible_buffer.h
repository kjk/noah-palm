/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/
#ifndef _EXTENSIBLE_BUFFER_H_
#define _EXTENSIBLE_BUFFER_H_

/* Extensible buffer for easier construction of string data
   of unknown length */
typedef struct
{
    int     allocated; // how many bytes allocated 
    char *  data;     // pointer to data
    int     used;      // how many bytes used (out of all allocated)
    int     minSize;   // how much memory to pre-allocated upon initialization
} ExtensibleBuffer;

ExtensibleBuffer *ebufNew(void);
void    ebufInit(ExtensibleBuffer *buf, int minSize);
void    ebufInitWitStr(ExtensibleBuffer *buf, char *s);
void    ebufReset(ExtensibleBuffer *buf);
void    ebufResetWithStr(ExtensibleBuffer *buf, char *s);
void    ebufDelete(ExtensibleBuffer *buf);
void    ebufFreeData(ExtensibleBuffer *buf);
void    ebufInsertChar(ExtensibleBuffer *buf, char c, int pos);
void    ebufReplaceChar(ExtensibleBuffer *buf, char c, int pos);
int     ebufGetDataSize(ExtensibleBuffer *buf);
int     ebufGetBufSize(ExtensibleBuffer *buf);
char *  ebufGetTxtCopy(ExtensibleBuffer * buf);
char *  ebufGetDataPointer(ExtensibleBuffer * buf);
char *  ebufGetTxtOwnership(ExtensibleBuffer * buf);
void    ebufAddChar(ExtensibleBuffer *buf, char c);
void    ebufAddStrN(ExtensibleBuffer *buf, char *str, int strLen);
void    ebufAddStr(ExtensibleBuffer *buf, char *str);
void    ebufWrapLine(ExtensibleBuffer *buf, int lineStart, int lineLen, int spacesAtStart);
void    ebufWrapBigLines(ExtensibleBuffer *buf);

#endif
