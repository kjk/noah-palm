/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)

*/

#ifndef _BETTER_FORMATTING_H_
#define _BETTER_FORMATTING_H_
//formatting tag code (an unused in database asci code)
//and DisplayPrefs structure
#define FORMAT_TAG        152
#define FORMAT_WORD       153
#define FORMAT_DEFINITION 154
#define FORMAT_EXAMPLE    155
#define FORMAT_POS        156
#define FORMAT_SYNONYM    157  
#define FORMAT_LIST       158  
#define FORMAT_BIG_LIST   159  

typedef struct {
    FontID font;
    int    colorR, colorG, colorB;
    int    bgcolR, bgcolG, bgcolB;
} DisplayElementPrefs;

typedef struct {
    int listStyle;
    int bgcolR, bgcolG, bgcolB;
    DisplayElementPrefs pos;
    DisplayElementPrefs word;
    DisplayElementPrefs definition;
    DisplayElementPrefs example;
    DisplayElementPrefs synonym;
    DisplayElementPrefs defList;
    DisplayElementPrefs posList;
} DisplayPrefs;

typedef enum {  //when displayed - reverse ordered !!!!
    layoutDefault = 0,
    layoutFormat1,
    layoutFormat2
} LayoutType;

typedef enum {
    actTagWord = 0,
    actTagPos,
    actTagSynonym,
    actTagList,    
    actTagBigList,    
    actTagDefinition,
    actTagExample
} ActualTag;

void SetDrawParam(char type, DisplayPrefs *displayPrefs,struct _AppContext * appContext);
void SetDefaultDisplayParam(DisplayPrefs *displayPrefs, Boolean onlyFont, Boolean onlyColor);

Boolean FormatWantsWord(struct _AppContext *appContext);
Boolean DisplayPrefFormHandleEvent(EventType * event);

Boolean ShakeSortExtBuf(ExtensibleBuffer *buf);
void Format1OnSortedBuf(int format_id, ExtensibleBuffer *buf);
void Format2OnSortedBuf(int format_id, ExtensibleBuffer *buf);
void ebufWrapLine(ExtensibleBuffer *buf, int lineStart, int lineLen, int spacesAtStart, struct _AppContext *appContext);
void DrawDisplayInfo(DisplayInfo * di, int firstLine, Int16 x, Int16 y, int maxLines);

#endif