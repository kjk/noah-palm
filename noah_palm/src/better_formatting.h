/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: szymon knitter (szknitter@wp.pl)
*/

#ifndef _BETTER_FORMATTING_H_
#define _BETTER_FORMATTING_H_

// storing rgb color as UInt32 is easier to code for
typedef UInt32 PackedRGB;


//extern PackedRGB PackRGB_f(int r, int g, int b);

#define PackRGB(r,g,b)  ((((unsigned long)r)<<16)&0xFF0000L) | \
                        (((g)<<8)&0xFF00L) | \
                        ((b)&0xFFL)

#define RGBGetR(packed) ((packed & 0xff0000L)>>16)
#define RGBGetG(packed) ((packed & 0xff00L)>>8)
#define RGBGetB(packed) ((packed & 0xff))

// this saves memory. Don't forget to update TestRGBStandardColors() in i_noah.c
// when you add new colors to this list
//#define WHITE_Packed PackRGB(0xff,0xff,0xff)
#define WHITE_Packed 0xffffff
//#define BLACK_Packed PackRGB(0x00,0x00,0x00)
#define BLACK_Packed 0x0
//#define RED_Packed   PackRGB(0xff,0x00,0x00)
#define RED_Packed   0xff0000
//#define BLUE_Packed  PackRGB(0x00,0x00,0xff)
#define BLUE_Packed  0x0000ff

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
    PackedRGB color;
    PackedRGB bgCol;
} DisplayElementPrefs;

typedef struct {
    int         listStyle;
    PackedRGB   bgCol;
    Boolean     fEnablePronunciation;
    Boolean     fEnablePronunciationSpecialFonts;
    DisplayElementPrefs pos;
    DisplayElementPrefs word;
    DisplayElementPrefs definition;
    DisplayElementPrefs example;
    DisplayElementPrefs synonym;
    DisplayElementPrefs defList;
    DisplayElementPrefs posList;
    DisplayElementPrefs pronunciation;
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
int  ebufWrapLine(ExtensibleBuffer *buf, int lineStart, int lineLen, int spacesAtStart, struct _AppContext *appContext);
void DrawDisplayInfo(DisplayInfo * di, int firstLine, Int16 x, Int16 y, int maxLines);

void bfFreePTR(struct _AppContext *appContext);
void bfStripBufferFromTags(ExtensibleBuffer *buf);

Boolean IsTag(char a, char b);
void SetOnlyFont(char type,DisplayPrefs *displayPrefs);
int  XchgInBuffer(char *txt, int offset1, int offset2, int end2);
int  CmpPos(char *pos1, char *pos2);

extern Err DisplayPrefsFormLoad(struct _AppContext* appContext);

#endif
