/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: szymon knitter (szknitter@wp.pl)
  
  Set of functions that improve the word display format:
  
  +  window event handle function for DisplayPrefsForm
  +  functions that format word definition in extBuffer
  +  ebufWrapLine (moved from extensible_buffer.c)
  +  functions used to draw word
  
*/
//#pragma warn_a5_access on

#include "common.h"

#ifdef NOAH_PRO
#include "noah_pro_2nd_segment.h"
#endif

#ifdef I_NOAH
#include "inet_definition_format.h"
#endif

/* Free ptrOldDisplayPrefs if its not NULL*/
void bfFreePTR(AppContext *appContext)
{
    if(appContext->ptrOldDisplayPrefs != NULL)
        new_free(appContext->ptrOldDisplayPrefs);
    appContext->ptrOldDisplayPrefs = NULL;
}

/* When we change global background we need to change all backgrounds to global background color */
static void SetAllBackGroundLikeGlobal(DisplayPrefs *displayPrefs)
{
    displayPrefs->pos.bgcolR = displayPrefs->bgcolR;
    displayPrefs->pos.bgcolG = displayPrefs->bgcolG;
    displayPrefs->pos.bgcolB = displayPrefs->bgcolB;
    displayPrefs->word.bgcolR = displayPrefs->bgcolR;
    displayPrefs->word.bgcolG = displayPrefs->bgcolG;
    displayPrefs->word.bgcolB = displayPrefs->bgcolB;
    displayPrefs->definition.bgcolR = displayPrefs->bgcolR;
    displayPrefs->definition.bgcolG = displayPrefs->bgcolG;
    displayPrefs->definition.bgcolB = displayPrefs->bgcolB;
    displayPrefs->example.bgcolR = displayPrefs->bgcolR;
    displayPrefs->example.bgcolG = displayPrefs->bgcolG;
    displayPrefs->example.bgcolB = displayPrefs->bgcolB;
    displayPrefs->synonym.bgcolR = displayPrefs->bgcolR;
    displayPrefs->synonym.bgcolG = displayPrefs->bgcolG;
    displayPrefs->synonym.bgcolB = displayPrefs->bgcolB;
    displayPrefs->defList.bgcolR = displayPrefs->bgcolR;
    displayPrefs->defList.bgcolG = displayPrefs->bgcolG;
    displayPrefs->defList.bgcolB = displayPrefs->bgcolB;
    displayPrefs->posList.bgcolR = displayPrefs->bgcolR;
    displayPrefs->posList.bgcolG = displayPrefs->bgcolG;
    displayPrefs->posList.bgcolB = displayPrefs->bgcolB;
    displayPrefs->pronunciation.bgcolR = displayPrefs->bgcolR;
    displayPrefs->pronunciation.bgcolG = displayPrefs->bgcolG;
    displayPrefs->pronunciation.bgcolB = displayPrefs->bgcolB;
}

/* Sets &prefs (only used by SetDefaultDisplayParam) */
static void SetPrefsAs(DisplayElementPrefs *prefs,FontID font, int colR, int colG, int colB, int bgR, int bgG, int bgB)
{
    prefs->font = font;
    prefs->colorR = colR;
    prefs->colorG = colG;
    prefs->colorB = colB;
    prefs->bgcolR = bgR;
    prefs->bgcolG = bgG;
    prefs->bgcolB = bgB;
}

/* Sets rest of display params to actual listStyle */
void SetDefaultDisplayParam(DisplayPrefs *displayPrefs, Boolean onlyFont, Boolean onlyColor)
{
    DisplayElementPrefs prefs[16];
    DisplayElementPrefs *prefsToSet;

    // format 3(0): (old)
    SetPrefsAs(&prefs[ 0], stdFont,        0,  0,  0, 255,255,255 );    // color is black, font is small

    // format 1:
    SetPrefsAs(&prefs[ 1], largeBoldFont,  0,  0,255, 255,255,255 );    // word
    SetPrefsAs(&prefs[ 2], boldFont,       0,220,  0, 255,255,255 );    // pos
    SetPrefsAs(&prefs[ 3], stdFont,        0,  0,  0, 255,255,255 );    // definition

    SetPrefsAs(&prefs[ 4], stdFont,  221, 34, 17, 255,255,255 );    // example
    SetPrefsAs(&prefs[ 5], boldFont,   0,  0, 68, 255,255,255 );    // synonyms
    SetPrefsAs(&prefs[ 6], stdFont,    0,  0,  0, 255,255,255 );    // definition list
    SetPrefsAs(&prefs[ 7], boldFont,   0,220,  0, 255,255,255 );    // pos list

    // format 2: (default)
    SetPrefsAs(&prefs[ 8], largeBoldFont,  0,  0,255, 255,255,255 );    // word
    SetPrefsAs(&prefs[ 9], boldFont,       0,220,  0, 255,255,255 );    // pos
    SetPrefsAs(&prefs[10], stdFont,        0,  0,  0, 255,255,255 );    // definition

    SetPrefsAs(&prefs[11], stdFont,  221, 34, 17, 255,255,255 );    // example
    SetPrefsAs(&prefs[12], boldFont,   0,  0, 68, 255,255,255 );    // synonyms
    SetPrefsAs(&prefs[13], stdFont,    0,  0,  0, 255,255,255 );    // definition list
    SetPrefsAs(&prefs[14], boldFont,   0,220,  0, 255,255,255 );    // pos list

    //pronunciation for both
    SetPrefsAs(&prefs[15], largeBoldFont,  80, 160,  0, 255,255,255 );    //pronunciation
    
    prefsToSet = &prefs[0];

    if(!onlyFont && !onlyColor)
    {
            switch(displayPrefs->listStyle)
            {
                case 1:
                    displayPrefs->word       = prefsToSet[1];
                    displayPrefs->pos        = prefsToSet[2];
                    displayPrefs->definition = prefsToSet[3];
                    displayPrefs->example    = prefsToSet[4];
                    displayPrefs->synonym    = prefsToSet[5];
                    displayPrefs->defList    = prefsToSet[6];
                    displayPrefs->posList    = prefsToSet[7];
                    displayPrefs->pronunciation = prefsToSet[15];
                    displayPrefs->bgcolR     = 255;
                    displayPrefs->bgcolG     = 255;
                    displayPrefs->bgcolB     = 255;
                    displayPrefs->enablePronunciation = false;
                    displayPrefs->enablePronunciationSpecialFonts = false;
                    break;
                case 2:
                    displayPrefs->word       = prefsToSet[8];
                    displayPrefs->pos        = prefsToSet[9];
                    displayPrefs->definition = prefsToSet[10];
                    displayPrefs->example    = prefsToSet[11];
                    displayPrefs->synonym    = prefsToSet[12];
                    displayPrefs->defList    = prefsToSet[13];
                    displayPrefs->posList    = prefsToSet[14];
                    displayPrefs->pronunciation = prefsToSet[15];
                    displayPrefs->bgcolR     = 255;
                    displayPrefs->bgcolG     = 255;
                    displayPrefs->bgcolB     = 255;
                    displayPrefs->enablePronunciation = false;
                    displayPrefs->enablePronunciationSpecialFonts = false;
                    break;
                case 0:  //without break  
                    default: //if not supported then dafault
                    displayPrefs->pos        = prefsToSet[0];
                    displayPrefs->word       = prefsToSet[0];
                    displayPrefs->definition = prefsToSet[0];
                    displayPrefs->example    = prefsToSet[0];
                    displayPrefs->synonym    = prefsToSet[0];
                    displayPrefs->defList       = prefsToSet[0];
                    displayPrefs->posList    = prefsToSet[0];
                    displayPrefs->pronunciation = prefsToSet[15];
                    displayPrefs->bgcolR     = 255;
                    displayPrefs->bgcolG     = 255;
                    displayPrefs->bgcolB     = 255;
                    displayPrefs->enablePronunciation = false;
                    displayPrefs->enablePronunciationSpecialFonts = false;
                break;
            }

            if(!IsColorSupported(GetAppContext())) //we need to set all colors to black&white
            {   
                    displayPrefs->bgcolR     = 255;
                    displayPrefs->bgcolG     = 255;
                    displayPrefs->bgcolB     = 255;
                    SetAllBackGroundLikeGlobal(displayPrefs);                                            
                    displayPrefs->pos.colorR        = 0;
                    displayPrefs->pos.colorG        = 0;
                    displayPrefs->pos.colorB        = 0;
                    displayPrefs->word.colorR       = 0;
                    displayPrefs->word.colorG       = 0;
                    displayPrefs->word.colorB       = 0;
                    displayPrefs->definition.colorR = 0;
                    displayPrefs->definition.colorG = 0;
                    displayPrefs->definition.colorB = 0;
                    displayPrefs->example.colorR    = 0;
                    displayPrefs->example.colorG    = 0;
                    displayPrefs->example.colorB    = 0;
                    displayPrefs->synonym.colorR    = 0;
                    displayPrefs->synonym.colorG    = 0;
                    displayPrefs->synonym.colorB    = 0;
                    displayPrefs->defList.colorR    = 0;
                    displayPrefs->defList.colorG    = 0;
                    displayPrefs->defList.colorB    = 0;
                    displayPrefs->posList.colorR    = 0;
                    displayPrefs->posList.colorG    = 0;
                    displayPrefs->posList.colorB    = 0;
                    displayPrefs->pronunciation.colorR = 0;
                    displayPrefs->pronunciation.colorG = 0;
                    displayPrefs->pronunciation.colorB = 0;
            }
    }   
}

/* sets fonts (used in ebufWrapLine */
void SetOnlyFont(char type,DisplayPrefs *displayPrefs)
{
     // I use "case (char) FORMAT_POS:" instead of "case FORMAT_POS:" because
     // FORMAT_POS is greater than 127 and we use signed char!!!
     // (because if "char z = 150;" and "#define FORMAT_POS 150" then "z != FORMAT_POS" !!!)
     switch(type)
     {
         case (char) FORMAT_POS : 
             FntSetFont(displayPrefs->pos.font);
             break;
         case (char) FORMAT_WORD : 
             FntSetFont(displayPrefs->word.font);
             break;
         case (char) FORMAT_DEFINITION : 
             FntSetFont(displayPrefs->definition.font);
             break;
         case (char) FORMAT_EXAMPLE : 
             FntSetFont(displayPrefs->example.font);
             break;
         case (char) FORMAT_SYNONYM : 
             FntSetFont(displayPrefs->synonym.font);
             break;
         case (char) FORMAT_LIST : 
             FntSetFont(displayPrefs->defList.font);
             break;
         case (char) FORMAT_BIG_LIST : 
             FntSetFont(displayPrefs->posList.font);
             break;
         case (char) FORMAT_PRONUNCIATION : 
             FntSetFont(displayPrefs->pronunciation.font);
             break;
         default:
             FntSetFont((FontID) 0x00);
             break;
     }
}

/* used in display */
void SetDrawParam(char type, DisplayPrefs *displayPrefs, AppContext * appContext)
{
     // I use "case (char) FORMAT_POS:" instead of "case FORMAT_POS:" because
     // FORMAT_POS is greater than 127 and we use signed char!!!
     // (because if "char z = 150;" and "#define FORMAT_POS 150" then "z != FORMAT_POS" !!!)
     switch(type)
     {
         case (char) FORMAT_POS : 
             FntSetFont(displayPrefs->pos.font);
             SetTextColorRGB(displayPrefs->pos.colorR,
                             displayPrefs->pos.colorG,
                             displayPrefs->pos.colorB,
                             appContext);
             SetBackColorRGB(displayPrefs->pos.bgcolR,
                             displayPrefs->pos.bgcolG,
                             displayPrefs->pos.bgcolB,
                             appContext);
             break;
         case (char) FORMAT_WORD : 
             FntSetFont(displayPrefs->word.font);
             SetTextColorRGB(displayPrefs->word.colorR,
                             displayPrefs->word.colorG,
                             displayPrefs->word.colorB,
                             appContext);
             SetBackColorRGB(displayPrefs->word.bgcolR,
                             displayPrefs->word.bgcolG,
                             displayPrefs->word.bgcolB,
                             appContext);
             break;
         case (char) FORMAT_DEFINITION : 
             FntSetFont(displayPrefs->definition.font);
             SetTextColorRGB(displayPrefs->definition.colorR,
                             displayPrefs->definition.colorG,
                             displayPrefs->definition.colorB,
                             appContext);
             SetBackColorRGB(displayPrefs->definition.bgcolR,
                             displayPrefs->definition.bgcolG,
                             displayPrefs->definition.bgcolB,
                             appContext);
             break;
         case (char) FORMAT_EXAMPLE : 
             FntSetFont(displayPrefs->example.font);
             SetTextColorRGB(displayPrefs->example.colorR,
                             displayPrefs->example.colorG,
                             displayPrefs->example.colorB,
                             appContext);
             SetBackColorRGB(displayPrefs->example.bgcolR,
                             displayPrefs->example.bgcolG,
                             displayPrefs->example.bgcolB,
                             appContext);
             break;
         case (char) FORMAT_SYNONYM : 
             FntSetFont(displayPrefs->synonym.font);
             SetTextColorRGB(displayPrefs->synonym.colorR,
                             displayPrefs->synonym.colorG,
                             displayPrefs->synonym.colorB,
                             appContext);
             SetBackColorRGB(displayPrefs->synonym.bgcolR,
                             displayPrefs->synonym.bgcolG,
                             displayPrefs->synonym.bgcolB,
                             appContext);
             break;
         case (char) FORMAT_LIST : 
             FntSetFont(displayPrefs->defList.font);
             SetTextColorRGB(displayPrefs->defList.colorR,
                             displayPrefs->defList.colorG,
                             displayPrefs->defList.colorB,
                             appContext);
             SetBackColorRGB(displayPrefs->defList.bgcolR,
                             displayPrefs->defList.bgcolG,
                             displayPrefs->defList.bgcolB,
                             appContext);
             break;
         case (char) FORMAT_BIG_LIST : 
             FntSetFont(displayPrefs->posList.font);
             SetTextColorRGB(displayPrefs->posList.colorR,
                             displayPrefs->posList.colorG,
                             displayPrefs->posList.colorB,
                             appContext);
             SetBackColorRGB(displayPrefs->posList.bgcolR,
                             displayPrefs->posList.bgcolG,
                             displayPrefs->posList.bgcolB,
                             appContext);
             break;
         case (char) FORMAT_PRONUNCIATION : 
             FntSetFont(displayPrefs->pronunciation.font);
             SetTextColorRGB(displayPrefs->pronunciation.colorR,
                             displayPrefs->pronunciation.colorG,
                             displayPrefs->pronunciation.colorB,
                             appContext);
             SetBackColorRGB(displayPrefs->pronunciation.bgcolR,
                             displayPrefs->pronunciation.bgcolG,
                             displayPrefs->pronunciation.bgcolB,
                             appContext);
             break;
         default:
             FntSetFont((FontID) 0x00);
             SetBackColorWhite(appContext);
             SetTextColorBlack(appContext);
             break;
     }
}

static void RunColorSetForm(int *Rinout, int *Ginout, int *Binout)
{
    RGBColorType rgb_color;

    rgb_color.index = 0;
    rgb_color.r = *Rinout;
    rgb_color.g = *Ginout;
    rgb_color.b = *Binout;

    if(IsColorSupported(GetAppContext()))
        UIPickColor (&rgb_color.index, &rgb_color, UIPickColorStartRGB,"Choose color", NULL); 

    *Rinout = rgb_color.r;
    *Ginout = rgb_color.g;
    *Binout = rgb_color.b;
}

/* Set color to draw buttons */
static void SetColorButton(ActualTag actTag, Boolean back, DisplayPrefs *displayPrefs,AppContext * appContext)
{
    if(!back)
    switch(actTag)
    {
         case actTagPos:
             SetTextColorRGB(displayPrefs->pos.colorR,
                          displayPrefs->pos.colorG,
                          displayPrefs->pos.colorB,
                          appContext);
             break;                    
         case actTagWord:
             SetTextColorRGB(displayPrefs->word.colorR,
                          displayPrefs->word.colorG,
                          displayPrefs->word.colorB,
                          appContext);
             break;                    
         case actTagDefinition:
             SetTextColorRGB(displayPrefs->definition.colorR,
                          displayPrefs->definition.colorG,
                          displayPrefs->definition.colorB,
                          appContext);
             break;                    
         case actTagExample:
             SetTextColorRGB(displayPrefs->example.colorR,
                          displayPrefs->example.colorG,
                          displayPrefs->example.colorB,
                          appContext);
             break;                    
         case actTagSynonym:
             SetTextColorRGB(displayPrefs->synonym.colorR,
                          displayPrefs->synonym.colorG,
                          displayPrefs->synonym.colorB,
                          appContext);
             break;                    
         case actTagDefList:
             SetTextColorRGB(displayPrefs->defList.colorR,
                          displayPrefs->defList.colorG,
                          displayPrefs->defList.colorB,
                          appContext);
             break;                    
         case actTagPosList:
             SetTextColorRGB(displayPrefs->posList.colorR,
                          displayPrefs->posList.colorG,
                          displayPrefs->posList.colorB,
                          appContext);
             break;                    
         case actTagPronunciation:
             SetTextColorRGB(displayPrefs->pronunciation.colorR,
                          displayPrefs->pronunciation.colorG,
                          displayPrefs->pronunciation.colorB,
                          appContext);
             break;                    
    }
    else
    switch(actTag)
    {
         case actTagPos:
             SetTextColorRGB(displayPrefs->pos.bgcolR,
                          displayPrefs->pos.bgcolG,
                          displayPrefs->pos.bgcolB,
                          appContext);
             break;                    
         case actTagWord:
             SetTextColorRGB(displayPrefs->word.bgcolR,
                          displayPrefs->word.bgcolG,
                          displayPrefs->word.bgcolB,
                          appContext);
             break;                    
         case actTagDefinition:
             SetTextColorRGB(displayPrefs->definition.bgcolR,
                          displayPrefs->definition.bgcolG,
                          displayPrefs->definition.bgcolB,
                          appContext);
             break;                    
         case actTagExample:
             SetTextColorRGB(displayPrefs->example.bgcolR,
                          displayPrefs->example.bgcolG,
                          displayPrefs->example.bgcolB,
                          appContext);
             break;                    
         case actTagSynonym:
             SetTextColorRGB(displayPrefs->synonym.bgcolR,
                          displayPrefs->synonym.bgcolG,
                          displayPrefs->synonym.bgcolB,
                          appContext);
             break;                    
         case actTagDefList:
             SetTextColorRGB(displayPrefs->defList.bgcolR,
                          displayPrefs->defList.bgcolG,
                          displayPrefs->defList.bgcolB,
                          appContext);
             break;                    
         case actTagPosList:
             SetTextColorRGB(displayPrefs->posList.bgcolR,
                          displayPrefs->posList.bgcolG,
                          displayPrefs->posList.bgcolB,
                          appContext);
             break;                    
         case actTagPronunciation:
             SetTextColorRGB(displayPrefs->pronunciation.bgcolR,
                          displayPrefs->pronunciation.bgcolG,
                          displayPrefs->pronunciation.bgcolB,
                          appContext);
             break;                    
    }
    
}

/* Draw color rectangles over the buttons */
static void RedrawFormElements(ActualTag actTag, DisplayPrefs *displayPrefs, AppContext *appContext)
{
/* Now we dont use Font button draw in current font... but we can do it here!*/
//    FontID prev_font;
    RectangleType r;

//    prev_font = FntGetFont();
//    WinDrawChars("Aa",2, 70, 30);
    
    r.topLeft.x = 160 - 22;
    r.topLeft.y = 16;
    r.extent.x  = 20;
    r.extent.y  = 10;
    SetTextColorRGB(appContext->prefs.displayPrefs.bgcolR,
                    appContext->prefs.displayPrefs.bgcolG,
                    appContext->prefs.displayPrefs.bgcolB,appContext);
    WinDrawRectangle(&r, 4);

    r.topLeft.x = 95;
    r.topLeft.y = 29;
    r.extent.x  = 20;
    r.extent.y  = 12;
    SetColorButton(actTag,false,displayPrefs,appContext);
    WinDrawRectangle(&r, 4);

    r.topLeft.x = 160 - 22;
    r.topLeft.y = 29;
    r.extent.x  = 20;
    r.extent.y  = 12;
    SetColorButton(actTag,true,displayPrefs,appContext);
    WinDrawRectangle(&r, 4);

    SetTextColorBlack(appContext);
//    FntSetFont(prev_font);
}

/* Makes definition and draw it! */
static void RedrawExampleDefinition(AppContext* appContext)
{
    ExtensibleBuffer *Buf;
    char *rawTxt;
    
    char pos[3], word[3], def[3], example[3], synonym[3], point[3];  
    
    //initialize data (resident mode)
    pos[0]=FORMAT_TAG;  pos[1]=FORMAT_POS;  pos[2]=0;
    word[0]=FORMAT_TAG; word[1]=FORMAT_WORD; word[2]=0;
    def[0]=FORMAT_TAG; def[1]=FORMAT_DEFINITION; def[2]=0;
    example[0]=FORMAT_TAG; example[1]=FORMAT_EXAMPLE; example[2]=0;
    synonym[0]=FORMAT_TAG; synonym[1]=FORMAT_SYNONYM; synonym[2]=0;
    point[0]=149; point[1]=' '; point[2]=0;


    Buf = ebufNew();
    Assert(Buf);

    //it will do format later (it looks like normal selected word)
    if(appContext->prefs.displayPrefs.listStyle != 0)
    {
        ebufAddStr(Buf, synonym); 
        ebufAddStr(Buf, "word\n"); 
    }
    ebufAddStr(Buf, pos); 
    ebufAddStr(Buf, point); 
    ebufAddStr(Buf, "(Noun) "); 
    ebufAddStr(Buf, word); 
    if(appContext->prefs.displayPrefs.listStyle == 0)
        ebufAddStr(Buf, "word, ");
    ebufAddStr(Buf, "word2, word3\n"); 
#ifndef THESAURUS   //why? think :)
    ebufAddStr(Buf, def); 
    ebufAddStr(Buf, "definition1\n"); 
    ebufAddStr(Buf, pos); 
    ebufAddStr(Buf, point); 
    ebufAddStr(Buf, "(Noun) "); 
    if(appContext->prefs.displayPrefs.listStyle == 0)
    {
        ebufAddStr(Buf, word); 
        ebufAddStr(Buf, "word\n"); 
    }
    ebufAddStr(Buf, def); 
    ebufAddStr(Buf, "definition2\n"); 
    ebufAddStr(Buf, example); 
    ebufAddStr(Buf, "example\n"); 
    ebufAddStr(Buf, pos); 
    ebufAddStr(Buf, point); 
    ebufAddStr(Buf, "(Verb) "); 
    if(appContext->prefs.displayPrefs.listStyle == 0)
    {
        ebufAddStr(Buf, word); 
        ebufAddStr(Buf, "word\n"); 
    }
    ebufAddStr(Buf, def); 
    ebufAddStr(Buf, "definition\n\0"); 
#else
    ebufAddStr(Buf, pos); 
    ebufAddStr(Buf, point); 
    ebufAddStr(Buf, "(Noun) "); 
    if(appContext->prefs.displayPrefs.listStyle == 0)
    {
        ebufAddStr(Buf, word); 
        ebufAddStr(Buf, "word, "); 
    }
    ebufAddStr(Buf, word); 
    ebufAddStr(Buf, "word3, word4\n"); 
    ebufAddStr(Buf, pos); 
    ebufAddStr(Buf, point); 
    ebufAddStr(Buf, "(Verb) "); 
    if(appContext->prefs.displayPrefs.listStyle == 0)
    {
        ebufAddStr(Buf, word); 
        ebufAddStr(Buf, "word, "); 
    }
    ebufAddStr(Buf, word); 
    ebufAddStr(Buf, "word4, word5\n\0"); 
#endif

    ebufAddChar(Buf, '\0');
    ebufWrapBigLines(Buf);

    if (NULL == appContext->currDispInfo)
    {
        appContext->currDispInfo = diNew();
        if (NULL == appContext->currDispInfo)
        {
            ebufDelete(Buf);
            return;
        }
    }
    rawTxt = ebufGetDataPointer(Buf);
    diSetRawTxt(appContext->currDispInfo, rawTxt);
    ebufDelete(Buf);

    appContext->firstDispLine = 0;
    SetGlobalBackColor(appContext);
    ClearRectangle(DRAW_DI_X, 42, appContext->screenWidth, appContext->screenHeight-42-15);
    SetBackColorWhite(appContext);
    DrawDisplayInfo(appContext->currDispInfo, 0, DRAW_DI_X, 30+12, appContext->dispLinesCount);
}

static Boolean DisplPrefFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    Boolean handled=false;
    if (DIA_Supported(&appContext->diaSettings))
    {
        UInt16 index=0;
        RectangleType newBounds;
        WinGetBounds(WinGetDisplayWindow(), &newBounds);
        WinSetBounds(FrmGetWindowHandle(frm), &newBounds);
        
        index=FrmGetObjectIndex(frm, buttonOk);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-1-newBounds.extent.y;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonCancel);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-1-newBounds.extent.y;
        FrmSetObjectBounds(frm, index, &newBounds);

        index=FrmGetObjectIndex(frm, buttonDefault);
        Assert(index!=frmInvalidObjectId);
        FrmGetObjectBounds(frm, index, &newBounds);
        newBounds.topLeft.y=appContext->screenHeight-1-newBounds.extent.y;
        FrmSetObjectBounds(frm, index, &newBounds);
        
        FrmDrawForm(frm);
        
        handled=true;
    }
    return handled;
}

static void InitOldDisplayPrefs(AppContext *appContext)
{
    appContext->ptrOldDisplayPrefs = new_malloc(sizeof(DisplayPrefs));
}

static void CopyParamsFromTo(DisplayPrefs *src, DisplayPrefs *dst)
{
    if(src == NULL || dst == NULL)
        return;

    dst->listStyle = src->listStyle;
    dst->bgcolR = src->bgcolR;
    dst->bgcolG = src->bgcolG;
    dst->bgcolB = src->bgcolB;
    dst->pos = src->pos;
    dst->word = src->word;
    dst->definition = src->definition;
    dst->example = src->example;
    dst->synonym = src->synonym;
    dst->defList = src->defList;
    dst->posList = src->posList;
    dst->pronunciation = src->pronunciation;
}

#ifdef I_NOAH
static void ReformatLastResponse(AppContext* appContext)
{
    if (mainFormShowsDefinition==appContext->mainFormContent)
    {
        const char* responseBegin=ebufGetDataPointer(&appContext->lastResponse);
        Assert(responseBegin);
        const char* responseEnd=responseBegin+ebufGetDataSize(&appContext->lastResponse);
        Err error=ProcessDefinitionResponse(appContext, responseBegin, responseEnd);
        Assert(!error);
    }        
}
#endif

Boolean DisplayPrefFormHandleEvent(EventType * event)
{
    int  setColor = 0;
    ActualTag  actTag = actTagWord;
    FormType *  frm = NULL;
    ListType *  list = NULL;
    char *      listTxt = NULL;
    char        txt[20];
    AppContext* appContext;

    frm = FrmGetActiveForm();
    appContext=GetAppContext();
    
    switch (event->eType)
    {
        case winDisplayChangedEvent:
                DisplPrefFormDisplayChanged(appContext, frm);
                actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                SetPopupLabel(frm, listListStyle, popupListStyle, 2 - appContext->prefs.displayPrefs.listStyle);
                SetPopupLabel(frm, listActTag, popupActTag, actTag);
                RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                RedrawExampleDefinition(appContext);
                return true;
            break;
        case frmOpenEvent:
            cbNoSelection(appContext);
            InitOldDisplayPrefs(appContext);
            CopyParamsFromTo(&appContext->prefs.displayPrefs, (DisplayPrefs*)appContext->ptrOldDisplayPrefs);    
            FrmDrawForm(FrmGetActiveForm());
            actTag = actTagWord;
            SetPopupLabel(frm, listListStyle, popupListStyle, 2 - appContext->prefs.displayPrefs.listStyle);
            SetPopupLabel(frm, listActTag, popupActTag, actTag);
            RedrawExampleDefinition(appContext);
            RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
            return true;

        case popSelectEvent:
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, event->data.popSelect.listID));
            listTxt = LstGetSelectionText(list, event->data.popSelect.selection);
            switch (event->data.popSelect.listID)
            {
                case listListStyle:
                    appContext->prefs.displayPrefs.listStyle = 2 - (LayoutType) event->data.popSelect.selection;
                    MemMove(txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupListStyle)),txt);
                    SetDefaultDisplayParam(&appContext->prefs.displayPrefs,false,false);
                    RedrawExampleDefinition(appContext);
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                    break;
                case listActTag:
                    actTag = (ActualTag) event->data.popSelect.selection;
                    MemMove(txt, listTxt, StrLen(listTxt) + 1);
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupActTag)),txt);
                    RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                    break;
                default:
                    Assert(0);
                    break;
            }
            return true;
            break;
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case popupListStyle:
                case popupActTag:
                    // need to propagate the event down to popus
                    return false;
                    break;
                case buttonFpos:
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    switch(actTag)
                    {
                        case actTagPos:
                            appContext->prefs.displayPrefs.pos.font = FontSelect (appContext->prefs.displayPrefs.pos.font);
                            break;                    
                        case actTagWord:
                            appContext->prefs.displayPrefs.word.font = FontSelect (appContext->prefs.displayPrefs.word.font);
                            break;                    
                        case actTagDefinition:
                            appContext->prefs.displayPrefs.definition.font = FontSelect (appContext->prefs.displayPrefs.definition.font);
                            break;                    
                        case actTagExample:
                            appContext->prefs.displayPrefs.example.font = FontSelect (appContext->prefs.displayPrefs.example.font);
                            break;                    
                        case actTagSynonym:
                            appContext->prefs.displayPrefs.synonym.font = FontSelect (appContext->prefs.displayPrefs.synonym.font);
                            break;                    
                        case actTagDefList:
                            appContext->prefs.displayPrefs.defList.font = FontSelect (appContext->prefs.displayPrefs.defList.font);
                            break;                    
                        case actTagPosList:
                            appContext->prefs.displayPrefs.posList.font = FontSelect (appContext->prefs.displayPrefs.posList.font);
                            break;                    
                        case actTagPronunciation:
                            if(!appContext->prefs.displayPrefs.enablePronunciationSpecialFonts)
                                appContext->prefs.displayPrefs.pronunciation.font = FontSelect (appContext->prefs.displayPrefs.pronunciation.font);
                            break;                    
                    }
                    RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                    RedrawExampleDefinition(appContext);
                    break;
                case buttonCOLpos:
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    switch(actTag)
                    {
                        case actTagPos:
                            RunColorSetForm(&appContext->prefs.displayPrefs.pos.colorR,
                                            &appContext->prefs.displayPrefs.pos.colorG,
                                            &appContext->prefs.displayPrefs.pos.colorB);
                            break;                    
                        case actTagWord:
                            RunColorSetForm(&appContext->prefs.displayPrefs.word.colorR,
                                            &appContext->prefs.displayPrefs.word.colorG,
                                            &appContext->prefs.displayPrefs.word.colorB);
                            break;                    
                        case actTagDefinition:
                            RunColorSetForm(&appContext->prefs.displayPrefs.definition.colorR,
                                            &appContext->prefs.displayPrefs.definition.colorG,
                                            &appContext->prefs.displayPrefs.definition.colorB);
                            break;                    
                        case actTagExample:
                            RunColorSetForm(&appContext->prefs.displayPrefs.example.colorR,
                                            &appContext->prefs.displayPrefs.example.colorG,
                                            &appContext->prefs.displayPrefs.example.colorB);
                            break;                    
                        case actTagSynonym:
                            RunColorSetForm(&appContext->prefs.displayPrefs.synonym.colorR,
                                            &appContext->prefs.displayPrefs.synonym.colorG,
                                            &appContext->prefs.displayPrefs.synonym.colorB);
                            break;                    
                        case actTagDefList:
                            RunColorSetForm(&appContext->prefs.displayPrefs.defList.colorR,
                                            &appContext->prefs.displayPrefs.defList.colorG,
                                            &appContext->prefs.displayPrefs.defList.colorB);
                            break;                    
                        case actTagPosList:
                            RunColorSetForm(&appContext->prefs.displayPrefs.posList.colorR,
                                            &appContext->prefs.displayPrefs.posList.colorG,
                                            &appContext->prefs.displayPrefs.posList.colorB);
                            break;                    
                        case actTagPronunciation:
                            RunColorSetForm(&appContext->prefs.displayPrefs.pronunciation.colorR,
                                            &appContext->prefs.displayPrefs.pronunciation.colorG,
                                            &appContext->prefs.displayPrefs.pronunciation.colorB);
                            break;                    
                    }
                    RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                    RedrawExampleDefinition(appContext);
                    setColor = 2;
                    break;
                case buttonBGpos:
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    switch(actTag)
                    {
                        case actTagPos:
                            RunColorSetForm(&appContext->prefs.displayPrefs.pos.bgcolR,
                                            &appContext->prefs.displayPrefs.pos.bgcolG,
                                            &appContext->prefs.displayPrefs.pos.bgcolB);
                            break;                    
                        case actTagWord:
                            RunColorSetForm(&appContext->prefs.displayPrefs.word.bgcolR,
                                            &appContext->prefs.displayPrefs.word.bgcolG,
                                            &appContext->prefs.displayPrefs.word.bgcolB);
                            break;                    
                        case actTagDefinition:
                            RunColorSetForm(&appContext->prefs.displayPrefs.definition.bgcolR,
                                            &appContext->prefs.displayPrefs.definition.bgcolG,
                                            &appContext->prefs.displayPrefs.definition.bgcolB);
                            break;                    
                        case actTagExample:
                            RunColorSetForm(&appContext->prefs.displayPrefs.example.bgcolR,
                                            &appContext->prefs.displayPrefs.example.bgcolG,
                                            &appContext->prefs.displayPrefs.example.bgcolB);
                            break;                    
                        case actTagSynonym:
                            RunColorSetForm(&appContext->prefs.displayPrefs.synonym.bgcolR,
                                            &appContext->prefs.displayPrefs.synonym.bgcolG,
                                            &appContext->prefs.displayPrefs.synonym.bgcolB);
                            break;                    
                        case actTagDefList:
                            RunColorSetForm(&appContext->prefs.displayPrefs.defList.bgcolR,
                                            &appContext->prefs.displayPrefs.defList.bgcolG,
                                            &appContext->prefs.displayPrefs.defList.bgcolB);
                            break;                    
                        case actTagPosList:
                            RunColorSetForm(&appContext->prefs.displayPrefs.posList.bgcolR,
                                            &appContext->prefs.displayPrefs.posList.bgcolG,
                                            &appContext->prefs.displayPrefs.posList.bgcolB);
                            break;                    
                        case actTagPronunciation:
                            RunColorSetForm(&appContext->prefs.displayPrefs.pronunciation.bgcolR,
                                            &appContext->prefs.displayPrefs.pronunciation.bgcolG,
                                            &appContext->prefs.displayPrefs.pronunciation.bgcolB);
                            break;                    
                    }
                    RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                    RedrawExampleDefinition(appContext);
                    setColor = 2;
                    break;
                case buttonGlobalBGcol:
                    RunColorSetForm(&appContext->prefs.displayPrefs.bgcolR,
                                    &appContext->prefs.displayPrefs.bgcolG,
                                    &appContext->prefs.displayPrefs.bgcolB);
                    SetAllBackGroundLikeGlobal(&appContext->prefs.displayPrefs);
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                    RedrawExampleDefinition(appContext);
                    setColor = 2;
                    break;
                case buttonDefault:
                    SetDefaultDisplayParam(&appContext->prefs.displayPrefs,false,false);
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                    RedrawExampleDefinition(appContext);
                    break;
                case buttonOk:
#ifdef NOAH_PRO 
                    SavePreferencesNoahPro(appContext);
#endif                    
#ifdef THESAURUS
                    /*TODO: Andrzej please make SavePreferencesThes() runable 
                        for other modules
                        make it not "static"
                    */
                    //SavePreferencesThes(appContext);
#endif                  
                    bfFreePTR(appContext);

#ifndef I_NOAH                    
                    SendNewWordSelected();
#else
                    ReformatLastResponse(appContext);
#endif

                    FrmReturnToForm(0);
                    break;
                case buttonCancel:
                    CopyParamsFromTo((DisplayPrefs*)appContext->ptrOldDisplayPrefs, &appContext->prefs.displayPrefs);    
                    bfFreePTR(appContext);
#ifndef I_NOAH                    
                    SendNewWordSelected();
#else
                    ReformatLastResponse(appContext);
#endif
                    FrmReturnToForm(0);
                    break;
                default:
                    Assert(0);
                    break;
            }
            //we need to redraw form when change colors! (realy needed only if colors > 65000)
            if(setColor != 0)
            {
                actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                FrmDrawForm(FrmGetActiveForm());
                SetPopupLabel(frm, listListStyle, popupListStyle, 2 - appContext->prefs.displayPrefs.listStyle);
                SetPopupLabel(frm, listActTag, popupActTag, actTag);
                RedrawFormElements(actTag,&appContext->prefs.displayPrefs,appContext);
                RedrawExampleDefinition(appContext);
                setColor--;
                if(setColor > 0)
                    setColor--;
            }
            return true;
        default:
            break;
    }
    return false;
}

Err DisplayPrefsFormLoad(AppContext* appContext)
{
    Err error=errNone;
    FormType* form=FrmInitForm(formDisplayPrefs);
    if (!form)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    error=DefaultFormInit(appContext, form);
    if (!error)
    {
        FrmSetActiveForm(form);
        FrmSetEventHandler(form, DisplayPrefFormHandleEvent);
    }
    else 
        FrmDeleteForm(form);
OnError:
    return error;    
}

// return true if a,b represents a tag
Boolean IsTag(char a, char b)
{
    if(a != (char)FORMAT_TAG)
        return false;
    switch(b)
    {
        case (char)FORMAT_POS:
        case (char)FORMAT_WORD:
        case (char)FORMAT_DEFINITION: 
        case (char)FORMAT_LIST:
        case (char)FORMAT_BIG_LIST:
        case (char)FORMAT_SYNONYM:
        case (char)FORMAT_EXAMPLE: 
        case (char)FORMAT_PRONUNCIATION: 
            return true;
        default: 
            return false;
    }
}

/* Remove all tags from buffer*/
void bfStripBufferFromTags(ExtensibleBuffer *buf)
{
    int i = 0;

    while(i+1 < buf->used)
    {
        if(IsTag(buf->data[i],buf->data[i+1]))
        {
            ebufDeleteChar(buf, i);
            ebufDeleteChar(buf, i);
        }  
        else
            i++;    
    }
    
    if(buf->data[buf->used] != '\0')
    {
        ebufAddChar(buf,'\0');
    }    
}

/*return:   r<0 if pos1 < pos2
            r=0 if pos1 = pos2
            r>0 if pos1 > pos2   */
static int  CmpPos(char *pos1, char *pos2)
{
    int i,len1,len2;
    
    i = 1;
    while( !IsTag(pos1[i],pos1[i+1]) && pos1[i+1]!='\0')
        i++;
    len1 = i;
    
    i = 1;
    while( !IsTag(pos2[i],pos2[i+1]) && pos2[i+1]!='\0')
        i++;
    len2 = i;

    i = 0;    
    while(pos1[i] == pos2[i] && i < len1 && i < len2)
        i++;
    //end of pos1 or end of pos2?
    if(i < len1 && i < len2)
    {
        if(pos1[i] < pos2[i])
           return (-1);   
        if(pos1[i] > pos2[i])
           return (1);   
    }
    return(len1 - len2);
}

/*  Xchg AbcdEef -> EefAbcd   return updated offset2 */
/*            |off1   |off2       |end2              */
/*  before: abCdefghijKlmnopqrstuwXyz                */
/*  after:  abKlmnopqrstuwCdefghijXyz                */
/*                        |off2 - return             */
static int XchgInBuffer(char *txt, int offset1, int offset2, int end2)
{
    int  i;
    char z;
    
    //reverse all
    for(i = 0; i < (end2-offset1)/2 ; i++)
    {
        z                 = txt[offset1 + i];
        txt[offset1 + i]  = txt[end2 - i - 1];
        txt[end2 - i - 1] = z;
    }
    //mirror offset2
    i = offset2 - offset1;
    offset2 = end2 - i;
    //reverse 1st
    for(i = 0; i < (offset2-offset1)/2 ; i++)
    {
        z                    = txt[offset1 + i];
        txt[offset1 + i]     = txt[offset2 - i - 1];
        txt[offset2 - i - 1] = z;
    }
    //reverse 2nd
    for(i = 0; i < (end2-offset2)/2 ; i++)
    {
        z                 = txt[offset2 + i];
        txt[offset2 + i]  = txt[end2 - i - 1];
        txt[end2 - i - 1] = z;
    }
    return offset2;
}

/*  Sort buffer by text after 'FORMAT_TAG''FORMAT_POS'
    sort method - shake sort                            */
Boolean ShakeSortExtBuf(ExtensibleBuffer *buf)
{
    int  i, n;
    int  L, r, k, j; //to shake sort
    int  *array;
    char *txt;
    int  len;
    Boolean ret = false;

    txt = ebufGetDataPointer(buf);
    len = ebufGetDataSize(buf);
    
    //if buffer without '\0' at the end
    if(txt[len-1] != '\0')
        len++;
    
    //get number of elements (n)
    n = 0;
    for(i = 0; i < len-1; i++)  
        if(txt[i] == (char)FORMAT_TAG && txt[i+1] == (char)FORMAT_POS)
            n++;
    if(n < 2)
        return ret;
    //make array of offsets
    array = (int *) new_malloc(n*sizeof(int));
    if (!array)
        return ret;
    n = 0;
    for(i = 0; i < len-1; i++)  
        if(txt[i] == (char)FORMAT_TAG && txt[i+1] == (char)FORMAT_POS)
        {
            array[n] = i;    
            n++;
        }

    //sort array!
    L = 1;
    r = n-1;
    k = n-1;
    do
    {
        for(j = r; j >= L; j--)
            if(CmpPos(&txt[array[j-1]], &txt[array[j]]) > 0)
            {
                if(j < n-1)
                    i = array[j+1];
                else
                    i = len-1;
                //reverse in buffer
                array[j] =  XchgInBuffer(txt, array[j-1], array[j], i);
                ret = true;
                k=j;
            }
            
        L = k + 1;
        for(j = L; j <= r; j++)
            if(CmpPos(&txt[array[j-1]], &txt[array[j]]) > 0)
            {
                if(j < n-1)
                    i = array[j+1];
                else
                    i = len-1;
                //reverse in buffer
                array[j] =  XchgInBuffer(txt, array[j-1], array[j], i);
                ret = true;
                k = j;            
            }
        r = k - 1;
    }while(L <= r);
    //buffer is sorted!!!
#ifdef WN_PRO_DICT  //we need to detect when we add "1) " or "I " in PRO!
    if(!ret)
    {
        if(CmpPos(&txt[array[n-1]], &txt[array[n-2]]) == 0)
        {
            if(n <= 2)  
                ret = true; //we added 2nd word and it have the same POS ("1) " and "2) " added)
            else
                if(CmpPos(&txt[array[n-3]], &txt[array[n-2]]) != 0)
                    ret = true; //we added 2nd with the same POS ("1) " and "2) " added)
        }
        else
        {
            if(CmpPos(&txt[array[0]], &txt[array[n-2]]) == 0)
                ret = true; //we added 2nd diffrent POS ("I " and "II " added)
        }
    }       
#endif
    //free array!!!!!!!!!
    new_free((int *)array);
    return ret;
}

//delete all text until tag or EOB is reached
static void ebufDeletePos(ExtensibleBuffer *buf, int pos)
{
    while( !IsTag(buf->data[pos],buf->data[pos+1]) && pos < buf->used)
        ebufDeleteChar(buf, pos);
}

//word is marked as synonym! and synonyms are marked as words!
//we need to change it... and sort synonyms after definition and examples
//also add "synonyms:" text (but not in thes.).
static void XchgWordsWithSynonyms(ExtensibleBuffer *buf)
{
    int  i, j, k;

    i = 0;
    if(buf->data[1] == (char)FORMAT_SYNONYM || buf->data[1] == (char)FORMAT_WORD)
    {
        buf->data[1] = FORMAT_WORD;
        i = 2;
    }
    else
        return; //we dont have "word" at the begining. This is not Format1 or Format2...
    
    while(i < buf->used)
    {
        //set i on word tag
        while(i < buf->used && !(buf->data[i]==(char)FORMAT_TAG && buf->data[i+1]==(char)FORMAT_WORD))
            i++;
        if(i < buf->used)    
        {
            ebufReplaceChar(buf, FORMAT_SYNONYM, i + 1);
#ifndef THESAURUS   //why? think :)
            ebufInsertStringOnPos(buf, "Synonyms: ", i + 2);
#endif
        }
        //set j on next tag
        j = i+2;
        while(j < buf->used && !IsTag(buf->data[j],buf->data[j+1]))
            j++;
    
        k = j;
        //set k on pos //but not if its reached
        if(buf->data[j+1] != (char)FORMAT_POS)
            k = j+2;
        while(k < buf->used && buf->data[k]!='\0' && !(buf->data[k]==(char)FORMAT_TAG && buf->data[k+1]==(char)FORMAT_POS))
            k++;
            
        //(i,j)(j,k) ---> (j,k)(i,j)
        if(j < buf->used && j!=k)
            XchgInBuffer(buf->data, i, j, k);
        i = k+2;        
    }
}

//return true if format wants leading "word" before first "pos"
Boolean FormatWantsWord(AppContext* appContext)
{
    switch(appContext->prefs.displayPrefs.listStyle)
    {
        case 1: //no break!
        case 2:
            return true;
        case 0: //0 is Format 3 mode (the old one)   
        default:
            return false;
    }
}

//format 1
void Format1OnSortedBuf(int format_id, ExtensibleBuffer *buf)
{
    int  i, j;
    int  first_pos;
    int  number;    
    char str_number[10];

    if(format_id != 1)  //as it was without formatting
        return;

    XchgWordsWithSynonyms(buf);

    i = 0;
    while((buf->data[i] != (char)FORMAT_TAG || buf->data[i+1] != (char)FORMAT_POS) && i+1 < buf->used)
        i++;
    
    first_pos = i + 2;
    if(first_pos > buf->used)
        return;   
    i = first_pos;
    number = 1;
    while( i+1 < buf->used )
    {
        if(buf->data[i] == (char)FORMAT_TAG && buf->data[i+1] == (char)FORMAT_POS)
        {
            i += 2;
            
            if(CmpPos(&buf->data[i], &buf->data[first_pos])==0)
            {
                //add numbers to buff (2. 3. 4. etc)
                ebufDeletePos(buf, i);
                number++;                
                StrPrintF(str_number,"%d: \0",number);                     

                ebufReplaceChar(buf, FORMAT_LIST, i - 1);

                j = 0;
                while(str_number[j] != '\0')
                {
                    ebufInsertChar(buf, str_number[j], i + j);
                    j++;
                }
                i += j-1;
            }
            else
            {
                //put 1. in first_pos (if its not a single pos type)
                if(buf->data[first_pos] == (char)149)
                {
                    ebufDeleteChar(buf, first_pos);
                    ebufDeleteChar(buf, first_pos);
                    i-=2;
                }

                j = 0;
                while( !IsTag(buf->data[first_pos+j],buf->data[first_pos+j+1]) )
                {
                    if(buf->data[first_pos+j] == ')' || buf->data[first_pos+j]== '(')
                    {
                        ebufDeleteChar(buf, first_pos+j);
                        j--;
                        i--;
                    }
                    j++;                
                }
                j = first_pos + j;

                ebufInsertChar(buf, FORMAT_TAG, j);
                ebufInsertChar(buf, FORMAT_BIG_LIST, j + 1);
                if(number > 1)
                {
                    ebufInsertChar(buf, '1', j + 2);
                    j++;
                    i++;
                }
                ebufInsertChar(buf, ':', j + 2);
                ebufInsertChar(buf, ' ', j + 3);
                i += 4;
                number = 1;
                first_pos = i;
                i++;
            }

        }
        else
            i++;
    } 
    
    //put 1. in first_pos
    if(buf->data[first_pos] == (char)149)
    {
        ebufDeleteChar(buf, first_pos);
        ebufDeleteChar(buf, first_pos);
    }
    j = 0;
    while( !IsTag(buf->data[first_pos+j],buf->data[first_pos+j+1]) )
    {
        if(buf->data[first_pos+j] == ')' || buf->data[first_pos+j]== '(')
        {
            ebufDeleteChar(buf, first_pos+j);
            j--;
        }
        j++;                
    }
    j = first_pos + j;
    ebufInsertChar(buf, FORMAT_TAG, j);
    ebufInsertChar(buf, FORMAT_BIG_LIST, j + 1);
    if(number > 1)
        ebufInsertChar(buf, '1', (j++) + 2);
    ebufInsertChar(buf, ':', j + 2);
    ebufInsertChar(buf, ' ', j + 3);
}

//format 2
void Format2OnSortedBuf(int format_id, ExtensibleBuffer *buf)
{
    int  i, j;
    int  first_pos;
    int  number;    
    int  bignumber;    
    char str_number[10];
    char *roman[10];// = {""," I"," II"," III"," IV"," V"," VI"," VII"," VIII"," IX"};

    //initialize data (resident mode)
    roman[0] = " ";
    roman[1] = " I";
    roman[2] = " II";
    roman[3] = " III";
    roman[4] = " IV";
    roman[5] = " V";
    roman[6] = " VI";
    roman[7] = " VII";
    roman[8] = " VIII";
    roman[9] = " IX";

    if(format_id != 2)  //as it was without formatting
        return;

    XchgWordsWithSynonyms(buf);

    //ebufInsertStringOnPos(buf, "word", 0);
    //ebufInsertChar(buf, FORMAT_WORD, 0);
    //ebufInsertChar(buf, FORMAT_TAG, 0);

    i = 0;
    while((buf->data[i] != (char)FORMAT_TAG || buf->data[i+1] != (char)FORMAT_POS) && i+1 < buf->used)
        i++;

    first_pos = i + 2;
    if(first_pos > buf->used)
        return;   
    i = first_pos;
    number = 1;
    bignumber = 1;
    while( i+1 < buf->used )
    {
        if(buf->data[i] == (char)FORMAT_TAG && buf->data[i+1] == (char)FORMAT_POS)
        {
            i += 2;
            
            if(CmpPos(&buf->data[i], &buf->data[first_pos])==0)
            {
                //add numbers to buff (2. 3. 4. etc)
                ebufDeletePos(buf, i);
                number++;                
                StrPrintF(str_number,"%d) \0",number);                     
                
                j = 0;
                while(str_number[j] != '\0')
                {
                    ebufInsertChar(buf, str_number[j], i + j);
                    j++;
                }
                ebufReplaceChar(buf, FORMAT_LIST, i - 1);
                i += j-1;
            }
            else
            {
                //put 1) in first_pos (if its not a single pos type)
                if(buf->data[first_pos] == (char)149)
                {
                    ebufDeleteChar(buf, first_pos);
                    ebufDeleteChar(buf, first_pos);
                    i-=2;
                }

                j = 0;
                while( !IsTag(buf->data[first_pos+j],buf->data[first_pos+j+1]) )
                {
                    if(buf->data[first_pos+j] == ')' || buf->data[first_pos+j]== '(')
                    {
                        ebufDeleteChar(buf, first_pos+j);
                        j--;
                        i--;
                    }
                    j++;                
                }
                j = first_pos + j;

                ebufInsertChar(buf, '\n', j);
                j++;
                i++;
                                
                if(number > 1)
                {
                    ebufInsertChar(buf, FORMAT_TAG, j);
                    ebufInsertChar(buf, FORMAT_LIST , j + 1);
                    ebufInsertChar(buf, '1', j + 2);
                    ebufInsertChar(buf, ')', j + 3);
                    ebufInsertChar(buf, ' ', j + 4);
                    i += 5;
                }
                
                StrPrintF(str_number,"%c%c%s \0", FORMAT_TAG, FORMAT_BIG_LIST, roman[bignumber%10]);                     
                j = 0;
                while(str_number[j] != '\0')
                {
                    ebufInsertChar(buf, str_number[j], first_pos - 2 + j);
                    j++;
                }
                i += j-1;
                
                i++;
                number = 1;
                first_pos = i;
                i++;
                bignumber++;
            }

        }
        else
            i++;
    } 

    //put 1) in first_pos (if its not a single pos type)
    if(buf->data[first_pos] == (char)149)
    {
        ebufDeleteChar(buf, first_pos);
        ebufDeleteChar(buf, first_pos);
    }

    j = 0;
    while( !IsTag(buf->data[first_pos+j],buf->data[first_pos+j+1]) )
    {
        if(buf->data[first_pos+j] == ')' || buf->data[first_pos+j]== '(')
        {
            ebufDeleteChar(buf, first_pos+j);
            j--;
        }
        j++;                
    }

    //goto end of pos
    j = first_pos + 1;
    while( !IsTag(buf->data[j], buf->data[j+1]) )
        j++;

    ebufInsertChar(buf, '\n', j);
    j++;
                                
    if(number > 1)
    {
        ebufInsertChar(buf, FORMAT_TAG, j);
        ebufInsertChar(buf, FORMAT_LIST , j + 1);
        ebufInsertChar(buf, '1', j + 2);
        ebufInsertChar(buf, ')', j + 3);
        ebufInsertChar(buf, ' ', j + 4);
    }
    
    if(bignumber > 1)
    {    
        StrPrintF(str_number,"%c%c%s \0", FORMAT_TAG, FORMAT_BIG_LIST, roman[bignumber%10]);                     
        j = 0;
        while(str_number[j] != '\0')
        {
            ebufInsertChar(buf, str_number[j], first_pos - 2 + j);
            j++;
        }
    }
}

//from "extensible_buffer.c" cut lines on smaller ones
void ebufWrapLine(ExtensibleBuffer *buf, int line_start, int line_len, int spaces_at_start, AppContext* appContext)
{
    char *txt;
    Int16 string_dx;
    Int16 pos_dx;
    Int16 pos_dx2;
    Int16 string_len;
    Int16 pos_len;
    Int16 tagOffset;
    Boolean fits;
    Boolean found;
    int pos, breakLine;
    int i,j;

    txt = ebufGetDataPointer(buf);
    if (NULL == txt)
        return;

    string_dx = 152;
    pos_dx = 152;
    string_len = line_len;
    txt += line_start;
    tagOffset = 0;

    if( IsTag(txt[0],txt[1]) )
    {
        SetOnlyFont(txt[1],&appContext->prefs.displayPrefs);    //we dont reset it to default before return!
        string_len -= 2;
        tagOffset += 2;
    }
    
    //we need to put as many as we can in one line    
    fits = false;
    
    //is there more than one tag in his line???
    j = 1;    
    while(j < string_len && !IsTag(txt[j],txt[j+1]))
        j++;
     
    //more than one tag in line!!!    
    if(j < string_len)
    {
        pos_dx = 0; 
        found = false;
        while(!found)
        {
            pos_len = j - tagOffset + 1;
            pos_dx2 = pos_dx;
            pos_dx += FntCharsWidth(&txt[tagOffset], pos_len); 	
            
            if(pos_dx >= string_dx)
            {
                pos_dx2 = string_dx - pos_dx2;
                FntCharsInWidth(&txt[tagOffset], &pos_dx2, &pos_len, &fits);
                pos = pos_len + tagOffset - 1;
                breakLine = pos_len + tagOffset - 1;
                found = true;
                fits = false;
            }        
            else
            {
                if( IsTag(txt[j],txt[j+1]) )
                {
                    SetOnlyFont(txt[j+1],&appContext->prefs.displayPrefs);
                    tagOffset = j+2;
                    j += 2;
                    while(j < string_len && !IsTag(txt[j],txt[j+1]))
                        j++;
                }
                else
                {
                    //j = string_len
                    //and it fits perfect
                    return;
                }
            }
        };//end while
    }
    else
    {
        //only one(at the begining) or zero tags in line
        FntCharsInWidth(&txt[tagOffset], &string_dx, &string_len, &fits);
        if (fits)
            return;
        pos = string_len - 1 + tagOffset;
        breakLine = string_len - 1 + tagOffset;
    }        
        
    if (fits)
        return;

    /* doesn't fit, so we have to cut it */
    /* find last space or some other charactar that is save
       to wrap around */
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
        /* didn't find so just brutally insert a newline */
        ebufInsertChar(buf, '\n', line_start + breakLine);
    }
}

//from "common.c"
/* display maxLines from DisplayInfo, starting with first line,
starting at x,y  coordinates */
void DrawDisplayInfo(DisplayInfo * di, int firstLine, Int16 x, Int16 y,
                int maxLines)
{
    int totalLines;
    int i,j;
    char *line;
    Int16 curY;
    Int16 curX;
    Int16 fontDY;
    FontID prev_font;
    int tagOffset;
    AppContext *appContext = GetAppContext();
    
    Assert(firstLine >= 0);
    Assert(maxLines > 0);

    curY = y;
    prev_font = FntGetFont();
    totalLines = diGetLinesCount(di) - firstLine;
    if (totalLines > maxLines)
    {
        totalLines = maxLines;
    }
    //find and set
    //previous TAG
    if(firstLine > 0)
    {    
        line = diGetLine(di, firstLine);
        if( !IsTag(line[0],line[1]) )   //no tag at the begining
        {
            i = firstLine - 1;
            while(i >= 0)
            {
                line = diGetLine(di, i);
                for(j = strlen(line) - 2; j >= 0; j--)
                    if( IsTag(line[j],line[j+1]) )
                    { 
                        SetDrawParam(line[j+1], &appContext->prefs.displayPrefs,appContext);
                        appContext->copyBlock.firstTag = line[j+1];
                        j = -1;
                        i = -1;
                    }
                i--;
            }
            if(i != -2)
                appContext->copyBlock.firstTag = FORMAT_TAG;
        }
    }    

    appContext->copyBlock.nextDy[0] = curY; 

    for (i = 0; i < totalLines; i++)
    {
        line = diGetLine(di, i + firstLine);
        fontDY = FntCharHeight();
        tagOffset = 0;
        curX = x;
  
        if( IsTag(line[0],line[1]) )
        {
            SetDrawParam(line[1],&appContext->prefs.displayPrefs,appContext);
            tagOffset += 2; 
            fontDY = FntCharHeight();
        }

        //if there are more tags in one line!
        j = tagOffset;
        while(line[j] != '\0')
        {
            while( !IsTag(line[j],line[j+1]) && line[j] != '\0')
                j++;

            if(curY + fontDY < (appContext->screenHeight - DRAW_DI_Y - 16))	
            {
                appContext->lastDispLine = i + firstLine;
                WinDrawChars(&line[tagOffset], j - tagOffset, curX, curY);
            }

            if(line[j] != '\0')
            {
                //we have another tag in this line
                curX += FntCharsWidth (&line[tagOffset], j - tagOffset);
                tagOffset = j + 2;
                if(line[j+1] != '\0')
                    SetDrawParam(line[j+1],&appContext->prefs.displayPrefs,appContext);
                j = j + 1;
                if(fontDY < FntCharHeight())
                    fontDY = FntCharHeight();
            }
            else
            {
                curX += FntCharsWidth (&line[tagOffset], j - tagOffset);
            }
        }

        appContext->copyBlock.maxDx[i] = curX;
        curY += fontDY;
        appContext->copyBlock.nextDy[i+1] = curY;
        if(curY >= (appContext->screenHeight - DRAW_DI_Y - 16))
            i = totalLines; 	
    }
    FntSetFont(prev_font);
    SetBackColorWhite(appContext);
    SetTextColorBlack(appContext);

//#ifndef I_NOAH
    cbInvertSelection(appContext);
//#endif    
}

