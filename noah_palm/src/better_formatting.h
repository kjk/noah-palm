/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: szymon knitter (szknitter@wp.pl)
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
#define FORMAT_PRONUNCIATION 160 

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
    DisplayElementPrefs pronunciation;
    Boolean enablePronunciation;
    Boolean enablePronunciationSpecialFonts;
} DisplayPrefs;

typedef enum {  //when displayed - reverse ordered !!!!
    layoutDefault = 0,
    layoutFormat1,
    layoutFormat2
} LayoutType;

typedef enum {
    actTagWord = 0,
    actTagPosList,    
    actTagPos,
    actTagDefList,    
// in thes we don't have definition and example so we need to move down the definition 
#ifndef THESAURUS
    actTagDefinition,
#endif
    actTagSynonym,
    actTagExample,
#ifdef THESAURUS
    actTagDefinition,
#endif
/*TODO: move: actTagPronunciation (when it will be finished) up (after word)*/
    actTagPronunciation
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

void bfFreePTR(struct _AppContext *appContext);
void bfStripBufferFromTags(ExtensibleBuffer *buf);

Boolean IsTag(char a, char b);
void SetOnlyFont(char type,DisplayPrefs *displayPrefs);

extern Err DisplayPrefsFormLoad(struct _AppContext* appContext);

#endif
