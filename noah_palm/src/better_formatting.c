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

#if 0
// just in case PackRGB macro turns out to be buggy
PackedRGB PackRGB_f(int r,int g, int b)
{
    PackedRGB res;
    res = (long)r<<16;
    res &= 0xff0000L;
    res |= (g<<8)&0xff00L;
    res |= b&0xffL;
    return res;
}
#endif

/* Free ptrOldDisplayPrefs if its not NULL*/
void bfFreePTR(AppContext *appContext)
{
    if(appContext->ptrOldDisplayPrefs != NULL)
        new_free(appContext->ptrOldDisplayPrefs);
    appContext->ptrOldDisplayPrefs = NULL;
}

/* When we change global background we need to change all backgrounds to global background color */
static void SetAllBackGroundLikeGlobal(DisplayPrefs *displayPrefs, PackedRGB rgb)
{

    displayPrefs->pos.bgCol = rgb;
    displayPrefs->word.bgCol = rgb;
    displayPrefs->definition.bgCol = rgb;
    displayPrefs->example.bgCol = rgb;
    displayPrefs->synonym.bgCol = rgb;
    displayPrefs->defList.bgCol = rgb;
    displayPrefs->posList.bgCol = rgb;
    displayPrefs->pronunciation.bgCol = rgb;
}

/* Sets &prefs (only used by SetDefaultDisplayParam) */
static void SetPrefsAs(DisplayElementPrefs *prefs, FontID font, PackedRGB color, PackedRGB bgCol)
{
    prefs->font = font;
    prefs->color = color;
    prefs->bgCol = bgCol;
}

/* Sets rest of display params to actual listStyle */
void SetDefaultDisplayParam(DisplayPrefs *displayPrefs, Boolean onlyFont, Boolean onlyColor)
{
    DisplayElementPrefs prefs[16];
    DisplayElementPrefs *prefsToSet;

    // format 3(0): (old)
    SetPrefsAs(&prefs[ 0], stdFont,        BLACK_Packed, WHITE_Packed);    // color is black, font is small

    // format 1:
    SetPrefsAs(&prefs[ 1], largeBoldFont,  PackRGB(0,0,255), WHITE_Packed );    // word
    SetPrefsAs(&prefs[ 2], boldFont,       PackRGB(0,220,0), WHITE_Packed );    // pos
    SetPrefsAs(&prefs[ 3], stdFont,        BLACK_Packed, WHITE_Packed );    // definition

    SetPrefsAs(&prefs[ 4], stdFont,  PackRGB(221,34,17), WHITE_Packed );    // example
    SetPrefsAs(&prefs[ 5], boldFont, PackRGB(0,0,68),    WHITE_Packed );    // synonyms
    SetPrefsAs(&prefs[ 6], stdFont,  BLACK_Packed,       WHITE_Packed );    // definition list
    SetPrefsAs(&prefs[ 7], boldFont, PackRGB(0,220,0),   WHITE_Packed );    // pos list

    // format 2: (default)
    SetPrefsAs(&prefs[ 8], largeBoldFont,  PackRGB(0,0,255), WHITE_Packed );    // word
    SetPrefsAs(&prefs[ 9], boldFont,       PackRGB(0,220,0), WHITE_Packed );    // pos
    SetPrefsAs(&prefs[10], stdFont,        BLACK_Packed,     WHITE_Packed );    // definition

    SetPrefsAs(&prefs[11], stdFont,   PackRGB(221,34,17), WHITE_Packed );    // example
    SetPrefsAs(&prefs[12], boldFont,  PackRGB(0,0,68),    WHITE_Packed );    // synonyms
    SetPrefsAs(&prefs[13], stdFont,   BLACK_Packed,       WHITE_Packed );    // definition list
    SetPrefsAs(&prefs[14], boldFont,  PackRGB(0,220,0),   WHITE_Packed );    // pos list

    //pronunciation for both
    SetPrefsAs(&prefs[15], largeBoldFont, PackRGB(80,160,0), WHITE_Packed );    //pronunciation
    
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
                    break;
                case 2:
                    displayPrefs->word       = prefsToSet[8];
                    displayPrefs->pos        = prefsToSet[9];
                    displayPrefs->definition = prefsToSet[10];
                    displayPrefs->example    = prefsToSet[11];
                    displayPrefs->synonym    = prefsToSet[12];
                    displayPrefs->defList    = prefsToSet[13];
                    displayPrefs->posList    = prefsToSet[14];
                    break;
                case 0:  //without break  
                    default: //if not supported then dafault
                    displayPrefs->pos        = prefsToSet[0];
                    displayPrefs->word       = prefsToSet[0];
                    displayPrefs->definition = prefsToSet[0];
                    displayPrefs->example    = prefsToSet[0];
                    displayPrefs->synonym    = prefsToSet[0];
                    displayPrefs->defList    = prefsToSet[0];
                    displayPrefs->posList    = prefsToSet[0];
                break;
            }
            // settings common to all styles
            displayPrefs->pronunciation = prefsToSet[15];
            displayPrefs->bgCol = WHITE_Packed;
            displayPrefs->enablePronunciation = true;
            displayPrefs->enablePronunciationSpecialFonts = false;

            if(!IsColorSupported(GetAppContext())) //we need to set all colors to black&white
            {   
                displayPrefs->bgCol = WHITE_Packed;
                SetAllBackGroundLikeGlobal(displayPrefs, displayPrefs->bgCol);
                displayPrefs->pos.color = BLACK_Packed;
                displayPrefs->word.color = BLACK_Packed;
                displayPrefs->definition.color = BLACK_Packed;
                displayPrefs->example.color = BLACK_Packed;
                displayPrefs->synonym.color = BLACK_Packed;
                displayPrefs->defList.color = BLACK_Packed;
                displayPrefs->posList.color = BLACK_Packed;
                displayPrefs->pronunciation.color = BLACK_Packed;
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

static void SetFontAndCols(AppContext *appContext, DisplayElementPrefs *displayPrefs)
{
    FntSetFont(displayPrefs->font);
    SetTextColorRGB(appContext, displayPrefs->color);
    SetBackColorRGB(appContext, displayPrefs->bgCol);
}

/* used in display */
void SetDrawParam(char type, DisplayPrefs *displayPrefs, AppContext * appContext)
{
    // I use "case (char) FORMAT_POS:" instead of "case FORMAT_POS:" because
    // FORMAT_POS is greater than 127 and we use signed char!!!
    // (because if "char z = 150;" and "#define FORMAT_POS 150" then "z != FORMAT_POS" !!!)
    switch(type)
    {
        case (char) FORMAT_POS:
            SetFontAndCols(appContext, &(displayPrefs->pos));
            break;
        case (char) FORMAT_WORD:
            SetFontAndCols(appContext, &(displayPrefs->word));
            break;
        case (char) FORMAT_DEFINITION:
            SetFontAndCols(appContext, &(displayPrefs->definition));
            break;
        case (char) FORMAT_EXAMPLE:
            SetFontAndCols(appContext, &(displayPrefs->example));
            break;
        case (char) FORMAT_SYNONYM:
            SetFontAndCols(appContext, &(displayPrefs->synonym));
            break;
        case (char) FORMAT_LIST:
           SetFontAndCols(appContext, &(displayPrefs->defList));
           break;
        case (char) FORMAT_BIG_LIST:
           SetFontAndCols(appContext, &(displayPrefs->posList));
           break;
        case (char) FORMAT_PRONUNCIATION:
           SetFontAndCols(appContext, &(displayPrefs->pronunciation));
           break;
        default:
            FntSetFont((FontID) 0x00);
            SetBackColorRGB(appContext, WHITE_Packed);
            SetTextColorRGB(appContext, BLACK_Packed);
            break;
    }
}


static DisplayElementPrefs *GetDEPForTag(DisplayPrefs *dp, ActualTag tag)
{
    switch(tag)
    {
        case actTagPos:
            return &dp->pos;
        case actTagWord:
            return &dp->word;
        case actTagDefinition:
            return &dp->definition;
        case actTagExample:
            return &dp->example;
        case actTagSynonym:
            return &dp->synonym;
        case actTagDefList:
            return &dp->defList;
        case actTagPosList:
            return &dp->posList;
        case actTagPronunciation:
            return &dp->pronunciation;
        default:
            Assert(0);
            return NULL;
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

// devnote: return the same color if colors are not supported or the user didn't
// canceled the color selection process
static PackedRGB RunColorSetForm(PackedRGB rgb)
{
    RGBColorType rgb_color;

    if( !IsColorSupported(GetAppContext()))
        return rgb;

    rgb_color.index = 0;
    rgb_color.r = RGBGetR(rgb);
    rgb_color.g = RGBGetG(rgb);
    rgb_color.b = RGBGetB(rgb);

    if ( UIPickColor (&rgb_color.index, &rgb_color, UIPickColorStartRGB,"Choose color", NULL) )
        return PackRGB(rgb_color.r,rgb_color.g,rgb_color.b);
    else
        return rgb;
}

static void RunColorSetFormForTag(DisplayPrefs *dp, ActualTag tag, Boolean bgCol)
{
    DisplayElementPrefs *dep;
    dep = GetDEPForTag(dp, tag);
    if (bgCol)
        dep->bgCol = RunColorSetForm(dep->bgCol);
    else
        dep->color = RunColorSetForm(dep->color);
}

static PackedRGB GetRGBForTag(DisplayPrefs *dp, ActualTag tag, Boolean fBgCol)
{
    DisplayElementPrefs *dep;

    dep = GetDEPForTag(dp, tag);
    if ( fBgCol )
        return dep->bgCol;
    else
        return dep->color;
}

/* Set color to draw buttons */
static void SetColorButton(AppContext * appContext, ActualTag actTag, Boolean fBgCol, DisplayPrefs *displayPrefs)
{
    SetTextColorRGB(appContext, GetRGBForTag(displayPrefs, actTag, fBgCol));
}

/* Draw color rectangles over the buttons */
static void RedrawFormElements( AppContext *appContext, ActualTag actTag, DisplayPrefs *displayPrefs)
{
/* Now we dont use Font button draw in current font... but we can do it here!*/
//    FontID prev_font;
    RectangleType r;
    UInt16 index=0;
    FormType* frm = FrmGetActiveForm();
//    prev_font = FntGetFont();
//    WinDrawChars("Aa",2, 70, 30);
    
    SetTextColorRGB(appContext,appContext->prefs.displayPrefs.bgCol);
    r.topLeft.x = 160 - 22;
    r.topLeft.y = 16;
    r.extent.x  = 20;
    r.extent.y  = 10;
    WinDrawRectangle(&r, 4);

    SetColorButton(appContext,actTag,false,displayPrefs);
    r.topLeft.x = 95;
    r.topLeft.y = 29;
    r.extent.x  = 20;
    r.extent.y  = 12;
    WinDrawRectangle(&r, 4);

    SetColorButton(appContext,actTag,true,displayPrefs);
    r.topLeft.x = 160 - 22;
    r.topLeft.y = 29;
    r.extent.x  = 20;
    r.extent.y  = 12;
    WinDrawRectangle(&r, 4);

    SetTextColorRGB(appContext, BLACK_Packed);
//    FntSetFont(prev_font);
#ifdef NOAH_PRO
    if(actTag == actTagPronunciation)
    {
        index=FrmGetObjectIndex(frm, buttonFpos);
        Assert(index!=frmInvalidObjectId);
        FrmHideObject(frm, index);
        FrmGetObjectBounds(frm, index, &r);
        r.topLeft.x=28;
        FrmSetObjectBounds(frm, index, &r);
        FrmShowObject(frm, index);
        index=FrmGetObjectIndex(frm, checkEnablePron);
        FrmSetControlValue (frm, index, appContext->prefs.displayPrefs.enablePronunciation);
        FrmShowObject(frm, index);
    }
    else    
    {
        FrmHideObject(frm, FrmGetObjectIndex(frm, checkEnablePron));
        index=FrmGetObjectIndex(frm, buttonFpos);
        Assert(index!=frmInvalidObjectId);
        FrmHideObject(frm, index);
        FrmGetObjectBounds(frm, index, &r);
        r.topLeft.x=10;
        FrmSetObjectBounds(frm, index, &r);
        FrmShowObject(frm, index);
    }
#endif
}

/* Makes definition and draw it! */
static void RedrawExampleDefinition(AppContext* appContext)
{
    ExtensibleBuffer *Buf;
    char *rawTxt;
#ifdef NOAH_PRO
    char *pron;
    unsigned char decompresed[10];
#endif  
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
#ifdef NOAH_PRO
        if(appContext->prefs.displayPrefs.enablePronunciation)
        if(appContext->pronData.isPronInUsedDictionary)
        {
            ebufAddChar(Buf, FORMAT_TAG); 
            ebufAddChar(Buf, FORMAT_PRONUNCIATION); 
            ebufAddStr(Buf, "["); 
            decompresed[0] = 36;
            decompresed[1] = 12;
            decompresed[2] = 9;
            decompresed[3] = 0;
            pron = pronTranslateDecompresed(appContext, decompresed);
            ebufAddStr(Buf, pron); 
            new_free(pron);
            ebufAddStr(Buf, "]\n"); 
        }      
#endif  
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
    ebufWrapBigLines(Buf,true);

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
    SetBackColorRGB(appContext, WHITE_Packed);
    DrawDisplayInfo(appContext->currDispInfo, 0, DRAW_DI_X, 30+12, appContext->dispLinesCount);
}

static Boolean DisplPrefFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(frm);

    FrmSetObjectPosByID(frm, buttonOk,      -1, appContext->screenHeight-14);
    FrmSetObjectPosByID(frm, buttonCancel,  -1, appContext->screenHeight-14);
    FrmSetObjectPosByID(frm, buttonDefault, -1, appContext->screenHeight-14);

    FrmDrawForm(frm);        
    return true;
}

static void InitOldDisplayPrefs(AppContext *appContext)
{
    appContext->ptrOldDisplayPrefs = new_malloc(sizeof(DisplayPrefs));
}

static void CopyParamsFromTo(DisplayPrefs *src, DisplayPrefs *dst)
{
    Assert(src);
    Assert(dst);
    dst->listStyle = src->listStyle;
    dst->bgCol = src->bgCol;
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
    int         setColor = 0;
    ActualTag   actTag = actTagWord;
    FormType *  frm;
    ListType *  list;
    char *      listTxt;
    char        txt[20];
    AppContext* appContext;
    DisplayElementPrefs *dep;
#ifdef NOAH_PRO            
    char *tab[10];
#endif

    frm = FrmGetActiveForm();
    appContext=GetAppContext();
    AppPrefs *prefs = &(appContext->prefs);
    DisplayPrefs *displayPrefs = &(prefs->displayPrefs);

    switch (event->eType)
    {
        case winDisplayChangedEvent:
            DisplPrefFormDisplayChanged(appContext, frm);
            actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
            SetPopupLabel(frm, listListStyle, popupListStyle, 2 - displayPrefs->listStyle);
            SetPopupLabel(frm, listActTag, popupActTag, actTag);
            RedrawFormElements(appContext,actTag,displayPrefs);
            RedrawExampleDefinition(appContext);
            return true;

        case frmOpenEvent:
            cbNoSelection(appContext);
            InitOldDisplayPrefs(appContext);
            CopyParamsFromTo(displayPrefs, (DisplayPrefs*)appContext->ptrOldDisplayPrefs);    
            FrmDrawForm(FrmGetActiveForm());
            actTag = actTagWord;
            SetPopupLabel(frm, listListStyle, popupListStyle, 2 - displayPrefs->listStyle);
            SetPopupLabel(frm, listActTag, popupActTag, actTag);
            RedrawExampleDefinition(appContext);
            RedrawFormElements(appContext,actTag,displayPrefs);
            
#ifdef NOAH_PRO            
            if(appContext->pronData.isPronInUsedDictionary)
            {
                list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag));
                for(setColor = 0; setColor < 7; setColor++)
                    tab[setColor] = LstGetSelectionText(list, setColor); 
                tab[setColor] = "pronunciation";
                LstSetListChoices (list, tab , 8);
            }
#endif            
            
            return true;

        case popSelectEvent:
            list =(ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, event->data.popSelect.listID));
            listTxt = LstGetSelectionText(list, event->data.popSelect.selection);
            switch (event->data.popSelect.listID)
            {
                case listListStyle:
                    displayPrefs->listStyle = 2 - (LayoutType) event->data.popSelect.selection;
                    SafeStrNCopy(txt, sizeof(txt), listTxt, -1);
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupListStyle)),txt);
                    SetDefaultDisplayParam(displayPrefs,false,false);
                    RedrawExampleDefinition(appContext);
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    RedrawFormElements(appContext,actTag,displayPrefs);
                    break;
                case listActTag:
                    actTag = (ActualTag) event->data.popSelect.selection;
                    SafeStrNCopy(txt, sizeof(txt), listTxt, -1);
                    CtlSetLabel((ControlType *)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,popupActTag)),txt);
                    RedrawFormElements(appContext,actTag,displayPrefs);
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

                    if ( actTagPronunciation == actTag )
                    {
                        if(! displayPrefs->enablePronunciationSpecialFonts)
                            displayPrefs->pronunciation.font = FontSelect(displayPrefs->pronunciation.font);
                    } else {
                        dep = GetDEPForTag(displayPrefs, actTag);
                        dep->font = FontSelect(dep->font);
                    }
                    RedrawFormElements(appContext,actTag,displayPrefs);
                    RedrawExampleDefinition(appContext);
                    break;
                case buttonCOLpos:
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag)));
                    RunColorSetFormForTag(displayPrefs, actTag, false);

                    RedrawFormElements(appContext,actTag,displayPrefs);
                    RedrawExampleDefinition(appContext);
                    setColor = 2;
                    break;
                case buttonBGpos:
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag)));
                    RunColorSetFormForTag(displayPrefs, actTag, true);

                    RedrawFormElements(appContext,actTag,displayPrefs);
                    RedrawExampleDefinition(appContext);
                    setColor = 2;
                    break;
                case buttonGlobalBGcol:
                    displayPrefs->bgCol = RunColorSetForm(displayPrefs->bgCol);
                    SetAllBackGroundLikeGlobal(displayPrefs, displayPrefs->bgCol);
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    RedrawFormElements(appContext,actTag,displayPrefs);
                    RedrawExampleDefinition(appContext);
                    setColor = 2;
                    break;
#ifdef NOAH_PRO
                case checkEnablePron:
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    if(FrmGetControlValue (frm, FrmGetObjectIndex(frm, checkEnablePron)))
                         displayPrefs->enablePronunciation = true;
                    else
                         displayPrefs->enablePronunciation = false;
                    RedrawFormElements(appContext,actTag,displayPrefs);
                    RedrawExampleDefinition(appContext);
                    break;
#endif                    
                case buttonDefault:
                    SetDefaultDisplayParam(displayPrefs,false,false);
                    actTag = (ActualTag)LstGetSelection ((ListType *) FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, listActTag))); 
                    RedrawFormElements(appContext,actTag,displayPrefs);
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
                    CopyParamsFromTo((DisplayPrefs*)appContext->ptrOldDisplayPrefs, displayPrefs);    
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
                SetPopupLabel(frm, listListStyle, popupListStyle, 2 - displayPrefs->listStyle);
                SetPopupLabel(frm, listActTag, popupActTag, actTag);
                RedrawFormElements(appContext,actTag,displayPrefs);
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

// return true if a,b represents a tag
// inline version to make it faster!
static inline Boolean IsTagInLine(char a, char b)
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
        if(IsTagInLine(buf->data[i],buf->data[i+1]))
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
int  CmpPos(char *pos1, char *pos2)
{
    int i,len1,len2;
    
    i = 1;
    while( !IsTagInLine(pos1[i],pos1[i+1]) && pos1[i+1]!='\0')
        i++;
    len1 = i;
    
    i = 1;
    while( !IsTagInLine(pos2[i],pos2[i+1]) && pos2[i+1]!='\0')
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

/* //old version - in place
int XchgInBuffer(char *txt, int offset1, int offset2, int end2)
{
    int  i;
    char z;
    char *txt1;
    char *txt2;
    //reverse all
    txt1 = txt + offset1;
    txt2 = txt + end2 - 1;
    for(i = 0; i < (end2-offset1)/2 ; i++)
    {
        z       = txt1[0];
        txt1[0] = txt2[0];
        txt2[0] = z;
        txt1++;
        txt2--;
    }
    //mirror offset2
    i = offset2 - offset1;
    offset2 = end2 - i;
    //reverse 1st
    txt1 = txt + offset1;
    txt2 = txt + offset2 - 1;
    for(i = 0; i < (offset2-offset1)/2 ; i++)
    {
        z       = txt1[0];
        txt1[0] = txt2[0];
        txt2[0] = z;
        txt1++;
        txt2--;
    }
    //reverse 2nd
    txt1 = txt + offset2;
    txt2 = txt + end2 - 1;
    for(i = 0; i < (end2-offset2)/2 ; i++)
    {
        z       = txt1[0];
        txt1[0] = txt2[0];
        txt2[0] = z;
        txt1++;
        txt2--;
    }
    return offset2;
}
*/
//faster but with malloc!
int XchgInBuffer(char *txt, int offset1, int offset2, int end2)
{
    char *txt1;
    int  i = end2 - (offset2 - offset1);
    txt1 = (char *) new_malloc(offset2 - offset1 + 1);
    MemMove(txt1,&txt[offset1],offset2 - offset1);
    MemMove(&txt[offset1],&txt[offset2],end2-offset2);
    MemMove(&txt[i],txt1,offset2 - offset1);
    new_free(txt1);
    return i;
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
    //free array!!!!!!!!!
    new_free((int *)array);
    return ret;
}

//delete all text until tag or EOB is reached
static void ebufDeletePos(ExtensibleBuffer *buf, int pos)
{
    while( !IsTagInLine(buf->data[pos],buf->data[pos+1]) && pos < buf->used)
        ebufDeleteChar(buf, pos);
}

//word is marked as synonym! and synonyms are marked as words!
//we need to change it... and sort synonyms after definition and examples
//also add "synonyms:" text (but not in thes.).
static void XchgWordsWithSynonyms(ExtensibleBuffer *buf)
{
    int  i, j, k;
    int  used;
    char *data;
    char *dataCurr;
    char *dataEnd;

    i = 0;
    if(buf->data[1] == (char)FORMAT_SYNONYM /*|| buf->data[1] == (char)FORMAT_WORD*/)
    {
        buf->data[1] = FORMAT_WORD;
        i = 2;
    }
    else
        return; //we dont have "word" at the begining. This is not Format1 or Format2...

    used = buf->used;
    data = buf->data;
    while(i < used)
    {
        //set i on word tag
        while(i < used && !(data[i]==(char)FORMAT_TAG && data[i+1]==(char)FORMAT_WORD))
            i++;
            
        if(i < used)    
        {
            ebufReplaceChar(buf, FORMAT_SYNONYM, i + 1);
#ifndef THESAURUS   //why? think :)
            ebufInsertStringOnPos(buf, "Synonyms: ", i + 2);
            used = buf->used;
            data = buf->data;
#endif
        }
        //set j on next tag
        j = i+2;
        dataCurr = data+j;
        dataEnd = data+used;
        while(dataCurr < dataEnd && !IsTagInLine(dataCurr[0],dataCurr[1]))
            dataCurr++;
        j = dataCurr-data;
    
        //some problems with unformated data
        if(!(j+1 < used))
            return; 
        k = j;
        //set k on pos //but not if its reached
        if(data[j+1] != (char)FORMAT_POS)
        {
            k = j+2;
            if(!(k+1 < used))
                return; 
        }    

        dataCurr = data+k;
        while(dataCurr < dataEnd && dataCurr[0]!='\0' && !(dataCurr[0]==(char)FORMAT_TAG && dataCurr[1]==(char)FORMAT_POS))
            dataCurr++;
            
        k = dataCurr - data;    
        //(i,j)(j,k) ---> (j,k)(i,j)
        if(j < used && j!=k)
            XchgInBuffer(data, i, j, k);
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
                while( !IsTagInLine(buf->data[first_pos+j],buf->data[first_pos+j+1]) )
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
    while( !IsTagInLine(buf->data[first_pos+j],buf->data[first_pos+j+1]) )
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
    char *data;
    char *dataTest;

    if(format_id != 2)  //as it was without formatting
        return;
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

    XchgWordsWithSynonyms(buf);

    i = 0;
    while((buf->data[i] != (char)FORMAT_TAG || buf->data[i+1] != (char)FORMAT_POS) && i+1 < buf->used)
        i++;

    first_pos = i + 2;
    if(first_pos > buf->used)
        return;   
    i = first_pos;
    number = 1;
    bignumber = 1;
    
    data = buf->data;
    data += i;
    dataTest = (buf->data) + (buf->used) - 1;
    
    while(data < dataTest)
    {
        if(data[0] == (char)FORMAT_TAG)
        {
            if(data[1] == (char)FORMAT_POS)
            {
                i = buf->used - (dataTest+1-data);
                
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
    
                    j = first_pos;

                    while( !IsTagInLine(buf->data[j],buf->data[j+1]) )
                    {
                        if(buf->data[j] == ')' || buf->data[j]== '(')
                        {
                            ebufDeleteChar(buf, j);
                            j--;
                            i--;
                        }
                        j++;
                    }
    
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
        
                data = buf->data + i;
                dataTest = (buf->data) + (buf->used) - 1;
            }
            else
                data++;            
        }
        else
            data++;
    } 

    //put 1) in first_pos (if its not a single pos type)
    if(buf->data[first_pos] == (char)149)
    {
        ebufDeleteChar(buf, first_pos);
        ebufDeleteChar(buf, first_pos);
    }

    j = first_pos;
    while( !IsTagInLine(buf->data[j],buf->data[j+1]) )
    {
        if(buf->data[j] == ')' || buf->data[j]== '(')
        {
            ebufDeleteChar(buf, j);
            j--;
        }
        j++;                
    }

    //goto end of pos
    j = first_pos + 1;
    while( !IsTagInLine(buf->data[j], buf->data[j+1]) )
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

//from "extensible_buffer.c" cut lines to smaller ones
int  ebufWrapLine(ExtensibleBuffer *buf, int line_start, int line_len, int spaces_at_start, AppContext* appContext)
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

    txt = buf->data;
    if (NULL == txt)
        return 0;

    string_dx = 152;
    pos_dx = 152;
    string_len = line_len;
    txt += line_start;
    tagOffset = 0;

    if( IsTagInLine(txt[0],txt[1]) )
    {
        SetOnlyFont(txt[1],&appContext->prefs.displayPrefs);    //we dont reset it to default before return!
        string_len -= 2;
        tagOffset += 2;
    }
    
    //we need to put as many as we can in one line
    fits = false;
    
    //is there more than one tag in his line???
    j = 1;

    while(j < string_len)
        if(txt[j] != (char)FORMAT_TAG)
            j++;
        else
            if(IsTagInLine(txt[j],txt[j+1]))
                break;
            else
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
                if( IsTagInLine(txt[j],txt[j+1]) )
                {
                    SetOnlyFont(txt[j+1],&appContext->prefs.displayPrefs);
                    tagOffset = j+2;
                    j += 2;
                    while(j < string_len && !IsTagInLine(txt[j],txt[j+1]))
                        j++;
                }
                else
                {
                    //j = string_len
                    //and it fits perfect
                    return string_len-1;
                }
            }
        };//end while
    }
    else
    {
        //only one(at the begining) or zero tags in line
        FntCharsInWidth(&txt[tagOffset], &string_dx, &string_len, &fits);
        if (fits)
            return string_len-1;
        pos = string_len - 1 + tagOffset;
        breakLine = string_len - 1 + tagOffset;
    }        
        
    /* doesn't fit, so we have to cut it */
    /* find last space or some other charactar that is save
       to wrap around */

    while (pos > 0)
    {
        switch (txt[pos])
        {
        case ' ':
            txt[pos] = '\n';
            for (i = 0; i < spaces_at_start; i++)
            {
                ebufInsertChar(buf, ' ', line_start + pos + i + 1);
            }
            return pos-1; 
        case ';':
        case ',':
            ebufInsertChar(buf, '\n', line_start + pos + 1);
            for (i = 0; i < spaces_at_start; i++)
            {
                ebufInsertChar(buf, ' ', line_start + pos + i + 2);
            }
            return pos-1;
        }
        --pos;
    }

    /* didn't find so just brutally insert a newline */
    ebufInsertChar(buf, '\n', line_start + breakLine);
    return breakLine-1;
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
        if( !IsTagInLine(line[0],line[1]) )   //no tag at the begining
        {
            i = firstLine - 1;
            while(i >= 0)
            {
                line = diGetLine(di, i);
                for(j = strlen(line) - 2; j >= 0; j--)
                    if( IsTagInLine(line[j],line[j+1]) )
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
  
        if( IsTagInLine(line[0],line[1]) )
        {
            SetDrawParam(line[1],&appContext->prefs.displayPrefs,appContext);
            tagOffset += 2; 
            fontDY = FntCharHeight();
        }

        //if there are more tags in one line!
        j = tagOffset;
        while(line[j] != '\0')
        {
            while( !IsTagInLine(line[j],line[j+1]) && line[j] != '\0')
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
    SetBackColorRGB(appContext, WHITE_Packed);
    SetTextColorRGB(appContext, BLACK_Packed);

//#ifndef I_NOAH
    cbInvertSelection(appContext);
//#endif    
}

