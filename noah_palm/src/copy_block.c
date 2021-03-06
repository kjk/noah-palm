/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Szymon Knitter (szknitter@wp.pl) 
*/


/**
 * @file copy_block.c
 * Implementation of select part of definition (use penEvents) 
 * and copy it to clippboard.
 * @author Szymon Knitter (szknitter@wp.pl) 
 */

#define ABS(x) ((x)<0?-(x):(x))

#include "common.h"

#ifdef I_NOAH
#include "main_form.h"
#endif

/**
 *  Remove '//' if you want to work without selection
 */

#ifdef  NOAH_LITE
#define DONT_DO_COPY_WORD_DEFINITION 1
#endif

/**
 *  Set no selection! Use it if you change word, or display settings!
 */
void cbNoSelection(struct _AppContext *appContext)
{
    appContext->copyBlock.state = cbNothingSelected;
    appContext->copyBlock.startClick.dx = 0;
    appContext->copyBlock.startClick.lineNo = 0;
    appContext->copyBlock.startClick.charNo = 0;
    appContext->copyBlock.left = appContext->copyBlock.startClick;
    appContext->copyBlock.right = appContext->copyBlock.startClick;
    appContext->copyBlock.oldLeft = appContext->copyBlock.left;
    appContext->copyBlock.oldRight = appContext->copyBlock.right;
}

/**
 * If sth. is selected copy it else return false
 */
Boolean cbCopyToClippboard(AppContext *appContext)
{
    ExtensibleBuffer clipboardBuf;
    int              i, j, m, n, k;
    char *           defTxt;
    int              defTxtLen;

#ifdef DONT_DO_COPY_WORD_DEFINITION
    return false;
#endif
    if(appContext->copyBlock.state == cbNothingSelected || appContext->copyBlock.state == cbWasPenDown)
        return false;

    Assert(appContext->currDispInfo);
    ebufInit(&clipboardBuf,256);

    m = appContext->copyBlock.left.lineNo;
    n = appContext->copyBlock.right.lineNo;

    while(m <= n)
    {
        defTxt = diGetLine(appContext->currDispInfo, m);

        if(m == appContext->copyBlock.left.lineNo)
            i = appContext->copyBlock.left.charNo;
        else
            i = 0;    

        if(m == n)
            j = appContext->copyBlock.right.charNo;
        else
            j = StrLen(defTxt);
            
        for(k = i; k < j; k++)
            ebufAddChar(&clipboardBuf, defTxt[k]);
        
        if(m != n)
            ebufAddChar(&clipboardBuf, '\n');
            
        m++;
    }

    //remove all display formating tags
    bfStripBufferFromTags(&clipboardBuf);
        
    defTxt = ebufGetDataPointer(&clipboardBuf);
    defTxtLen = StrLen(defTxt);
    ClipboardAddItem(clipboardText, defTxt, defTxtLen);
    ebufFreeData(&clipboardBuf);
    return true;    
}

/**
 *  Try to translate word.
 */
static Boolean cbTryWord(AppContext* appContext, char *wordInput)
{
    char        txt[WORD_MAX_LEN+1];
#ifndef I_NOAH
    char *      word;
    UInt32      wordNo;
#endif

    SafeStrNCopy( txt, sizeof(txt), wordInput, -1);

#ifdef NOAH_PRO
#ifndef DONT_DO_PRONUNCIATION
    if((char)(appContext->copyBlock.actTag) == (char)FORMAT_PRONUNCIATION)
    {
        pronDisplayHelp(appContext, wordInput);
        return true;
    }
#endif
#endif

#ifndef I_NOAH
#ifndef NOAH_LITE
    StringStrip( txt );
    strtolower(txt);
#endif
#endif

#ifndef I_NOAH
    wordNo = dictGetFirstMatching(GetCurrentFile(appContext), txt);
    word = dictGetWord(GetCurrentFile(appContext), wordNo);

    if (0 == StrNCaselessCompare(txt, word, StrLen(word) <  StrLen(txt) ? StrLen(word) : StrLen(txt)))
    {
        appContext->currentWord = wordNo;
        SendEvtWithType(evtNewWordSelected);
        return true;
    }
    else
    {
        SafeStrNCopy( appContext->lastWord, sizeof(appContext->lastWord), txt, -1);
        FrmPopupForm(formDictFind);
        return true;
    }
#else
    //translate word in I_NOAH
    FormType* form=FrmGetFormPtr(formDictMain);
    //set new word in edit window
    FldClearInsert(form, fieldWordInput, txt);

    //click go button
    MainFormPressFindButton(form);
    return true;
#endif
    return false;
}

/**
 *  Gets word from selection. Then run cbTryWord().
 */
static void cbGetWordFromSelectionAndTranslate(AppContext *appContext,cbSelectionPoint *start,cbSelectionPoint *stop)
{
    ExtensibleBuffer eBuf;
    int              i, j, m, n, k;
    char *           defTxt;
    char             txt[WORD_MAX_LEN+1];

    Assert(appContext->currDispInfo);
    ebufInit(&eBuf,256);

    m = start->lineNo;
    n = stop->lineNo;

    while(m <= n)
    {
        defTxt = diGetLine(appContext->currDispInfo, m);

        if(m == start->lineNo)
            i = start->charNo;
        else
            i = 0;    

        if(m == n)
            j = stop->charNo;
        else
            j = StrLen(defTxt);
            
        for(k = i; k < j; k++)
            ebufAddChar(&eBuf, defTxt[k]);
        
        if(m != n)
            ebufAddChar(&eBuf, '\n');
            
        m++;
    }
    ebufAddChar(&eBuf, '\0');

    //remove all display formating tags
    bfStripBufferFromTags(&eBuf);
    defTxt = ebufGetDataPointer(&eBuf);

    MemSet(txt, sizeof(txt), 0);
    i = 0;
    do
    {
        txt[i] = defTxt[i];
        i++;
    }while(defTxt[i]!='\0' && (i+1)<sizeof(txt));
    ebufFreeData(&eBuf);
    defTxt = txt;
    cbTryWord(appContext, defTxt);
    return;    
}

/**
 *  Return true if X,Y is in txt. Set actPosition to X,Y.
 */
static Boolean cbIsTextHit(AppContext *appContext, Int16 screenX, Int16 screenY)
{
    int i,j;
    char *line;
    Boolean endNow;
    FontID prev_font;
    Int16  dxL, dxR, dx;

    //set j on selected line
    for(i = appContext->firstDispLine, j = appContext->firstDispLine; i <= appContext->lastDispLine; i++)
        if(appContext->copyBlock.nextDy[i - appContext->firstDispLine] <= screenY &&
           appContext->copyBlock.nextDy[i - appContext->firstDispLine + 1] >= screenY)
        {
            j = i;
            i = appContext->lastDispLine + 2;
        } 

    //it passed all lines and dont stop!        
    if(i == appContext->lastDispLine + 1)
    {
        j = appContext->lastDispLine;
        appContext->copyBlock.actPosition.lineNo = j;
    
        appContext->copyBlock.actPosition.dx = appContext->copyBlock.maxDx[j - appContext->firstDispLine];
        line = diGetLine(appContext->currDispInfo, j);
        appContext->copyBlock.actPosition.charNo = StrLen(line);
        return false;
    }
    
    //now we have lineNo
    appContext->copyBlock.actPosition.lineNo = j;
    if(screenX < DRAW_DI_X || screenX > appContext->copyBlock.maxDx[j - appContext->firstDispLine])
    {
        appContext->copyBlock.actPosition.dx = appContext->copyBlock.maxDx[j - appContext->firstDispLine];
        line = diGetLine(appContext->currDispInfo, j);
        appContext->copyBlock.actPosition.charNo = StrLen(line);
        return false;
    }
    else
    {
        line = diGetLine(appContext->currDispInfo, j);
        prev_font = FntGetFont();
    
    
        //find and set
        //previous TAG
        if(!IsTag(line[0],line[1]))   //no tag at the begining
        {
            i = j - 1;
            while(i >= appContext->firstDispLine)
            {
                line = diGetLine(appContext->currDispInfo, i);
                for(j = strlen(line) - 2; j >= 0; j--)
                    if(IsTag(line[j],line[j+1]))
                    { 
                        SetOnlyFont(line[j+1],&appContext->prefs.displayPrefs);
                        appContext->copyBlock.actTag = line[j+1];
                        j = -1;
                        i = -1;
                    }
                    i--;
            }
            if(i != -2)
            {
                SetOnlyFont(appContext->copyBlock.firstTag,&appContext->prefs.displayPrefs);
                appContext->copyBlock.actTag = appContext->copyBlock.firstTag;
            }            
        }    
        
        line = diGetLine(appContext->currDispInfo, appContext->copyBlock.actPosition.lineNo);
        //fonts are set
        i = 0;
        dx = DRAW_DI_X;
        endNow = false;
        while(!endNow)
        {
            if(IsTag(line[i],line[i+1]))
            {
                SetOnlyFont(line[i+1],&appContext->prefs.displayPrefs);
                appContext->copyBlock.actTag = line[i+1];
                i += 2;
            }
            else
            {
                dxL = dx;
                dx += FntCharWidth (line[i]);
                dxR = dx;
                
                if(screenX <= dxR)
                {
                    if((screenX - dxL) < (dxR - screenX))
                    {
                        appContext->copyBlock.actPosition.dx = dxL;
                        appContext->copyBlock.actPosition.charNo = i;
                    }
                    else
                    {
                        appContext->copyBlock.actPosition.dx = dxR;
                        appContext->copyBlock.actPosition.charNo = i+1;
                    }
                    endNow = true;                
                }
                else
                    i++;            
            }   
        }
        FntSetFont(prev_font);
        return true;
    }
}

/**
 *  Return true if actPosition > left and actPosition < right
 */
static Boolean cbIsActBetweenLR(AppContext *appContext)
{
    if(appContext->copyBlock.actPosition.lineNo < appContext->copyBlock.left.lineNo ||
        appContext->copyBlock.actPosition.lineNo > appContext->copyBlock.right.lineNo)
        return false;
        
    if(appContext->copyBlock.actPosition.lineNo > appContext->copyBlock.left.lineNo &&
        appContext->copyBlock.actPosition.lineNo < appContext->copyBlock.right.lineNo)
        return true;
        
    if(appContext->copyBlock.actPosition.lineNo == appContext->copyBlock.left.lineNo)
        if(appContext->copyBlock.actPosition.dx < appContext->copyBlock.left.dx)
            return false;

    if(appContext->copyBlock.actPosition.lineNo == appContext->copyBlock.right.lineNo)
        if(appContext->copyBlock.actPosition.dx > appContext->copyBlock.right.dx)
            return false;

    return true;
}

/**
 *  Return true if char is not alfa
 */
static Boolean cbIsCharNotAlfa(char a)
{
    if(TxtCharIsAlpha((unsigned char) a))
        return false; 
    if(TxtCharIsDigit((unsigned char) a))
        return false; 
    return true;
} 

/**
 *  Return true if left is difrent than right
 */
static Boolean cbIsLNotEqualR(AppContext *appContext)
{
    if(appContext->copyBlock.left.lineNo != appContext->copyBlock.right.lineNo)
        return true;
    if(appContext->copyBlock.left.charNo != appContext->copyBlock.right.charNo)
        return true;
    return false;
}

/**
 *  Return true if actPosition is not equal startClick
 */
static Boolean cbIsActNotEqualStart(AppContext *appContext)
{
    if(appContext->copyBlock.startClick.lineNo != appContext->copyBlock.actPosition.lineNo)
        return true;
    if(appContext->copyBlock.startClick.charNo != appContext->copyBlock.actPosition.charNo)
        return true;
    return false;
}

/**
 *  Sets left and right on the selected word
 */
static void cbSetLRonActWord(AppContext *appContext)
{
    int i;
    Int16 dx, oldDx;
    FontID prev_font;
    Boolean endNow;
    char *line;
   
    prev_font = FntGetFont();
    line = diGetLine(appContext->currDispInfo, appContext->copyBlock.actPosition.lineNo);
        
    appContext->copyBlock.left.lineNo  = appContext->copyBlock.actPosition.lineNo;
    appContext->copyBlock.right.lineNo = appContext->copyBlock.actPosition.lineNo;
    
    dx = appContext->copyBlock.actPosition.dx;
    i = appContext->copyBlock.actPosition.charNo;
    endNow = false;
    oldDx = dx;

    //we didn't hit the word. we hitted ' ' or ',' or...
    if(cbIsCharNotAlfa(line[i]))
    {
        appContext->copyBlock.left.dx = dx;
        appContext->copyBlock.left.charNo = i;
        appContext->copyBlock.right.dx = dx;
        appContext->copyBlock.right.charNo = i;
        return;
    }
    
    SetOnlyFont(appContext->copyBlock.actTag,&appContext->prefs.displayPrefs);
    //find left end of word
    while(!endNow)
    {
        if(cbIsCharNotAlfa(line[i]))
        {
            appContext->copyBlock.left.dx = oldDx;
            appContext->copyBlock.left.charNo = i + 1;
            endNow = true;
        }
        else
        if(i == 0)
        {
            appContext->copyBlock.left.dx = dx;
            appContext->copyBlock.left.charNo = i;
            endNow = true;
        }
        else
        if(i > 1)
            if(IsTag(line[i-2],line[i-1]))
            {
                appContext->copyBlock.left.dx = dx;
                appContext->copyBlock.left.charNo = i;
                endNow = true;
            }
        
        if(!endNow)
        {
            i--;
            oldDx = dx;
            dx -= FntCharWidth(line[i]);
        }    
    }   
    //now find right end of word
    dx = appContext->copyBlock.actPosition.dx;
    i = appContext->copyBlock.actPosition.charNo;
    endNow = false;
    SetOnlyFont(appContext->copyBlock.actTag,&appContext->prefs.displayPrefs);
    while(!endNow)
    {
        if(cbIsCharNotAlfa(line[i]))
        {
            appContext->copyBlock.right.dx = dx;
            appContext->copyBlock.right.charNo = i;
            endNow = true;
        }
        else
        if(i == StrLen(line))
        {
            appContext->copyBlock.right.dx = dx;
            appContext->copyBlock.right.charNo = i;
            endNow = true;
        }
        else
        if(IsTag(line[i],line[i+1]))
        {
            appContext->copyBlock.right.dx = dx;
            appContext->copyBlock.right.charNo = i;
            endNow = true;
        }
        
        if(!endNow)
        {
            dx += FntCharWidth(line[i]);
            i++;
        }    
    }   
    FntSetFont(prev_font);
}

/**
 *  Return true if left == oldLeft && right == oldRight && state == cbIsSelection
 */
static Boolean cbIsThatAgainThatWord(AppContext *appContext)
{
    if(appContext->copyBlock.state != cbIsSelection)
        return false;
    if(appContext->copyBlock.left.lineNo != appContext->copyBlock.oldLeft.lineNo)
        return false;
    if(appContext->copyBlock.left.charNo != appContext->copyBlock.oldLeft.charNo)
        return false;
    if(appContext->copyBlock.right.lineNo != appContext->copyBlock.oldRight.lineNo)
        return false;
    if(appContext->copyBlock.right.charNo != appContext->copyBlock.oldRight.charNo)
        return false;
    return true;
}

/**
 *  Return: 
 *  r = 0 if sp1 = sp2
 *  r < 0 if sp1 < sp2
 *  r > 0 if sp1 > sp2
 */
static int cbCompareSelectionPoints(cbSelectionPoint *sp1, cbSelectionPoint *sp2)
{
    int i;
    i = sp1->lineNo - sp2->lineNo;
    if(i != 0)
        return i;
    i = sp1->charNo - sp2->charNo;
    return i;
}

/**
 *  Sets: (left, right) <- (actPosition, startClick)
 */
static void cbSetLRonActAndStart(AppContext *appContext)
{
    int actFirst = 0;   //0- undefined; 1- first; 2- 2nd
    
    if(appContext->copyBlock.actPosition.lineNo < appContext->copyBlock.startClick.lineNo)
        actFirst = 1;
    if(appContext->copyBlock.actPosition.lineNo > appContext->copyBlock.startClick.lineNo)
        actFirst = 2;
        
    if(actFirst == 0)
        if(appContext->copyBlock.actPosition.charNo < appContext->copyBlock.startClick.charNo)
            actFirst = 1;
        else
            actFirst = 2;
    
    if(actFirst == 1)
    {
        appContext->copyBlock.left = appContext->copyBlock.actPosition;
        appContext->copyBlock.right = appContext->copyBlock.startClick;
    }
    else
    {
        appContext->copyBlock.left = appContext->copyBlock.startClick;
        appContext->copyBlock.right = appContext->copyBlock.actPosition;
    }
}

/**
 * Handle penDownEvent
 * return true if handled
 * return false if define DONT_DO_COPY_WORD_DEFINITION
 */
Boolean cbPenDownEvent(AppContext *appContext, Int16 screenX, Int16 screenY)
{

#ifdef DONT_DO_COPY_WORD_DEFINITION
    return false;
#endif

    appContext->copyBlock.penDownX = screenX;
    appContext->copyBlock.penDownY = screenY;

    cbInvertSelection(appContext);

    if(cbIsTextHit(appContext, screenX, screenY))
    {
        appContext->copyBlock.startClick = appContext->copyBlock.actPosition;
        if(cbIsActBetweenLR(appContext))
        {
            cbSetLRonActWord(appContext);
            if(cbIsThatAgainThatWord(appContext))
            {
                if(cbIsLNotEqualR(appContext))
                {
                    appContext->copyBlock.state = cbOnceAgain;
                    cbInvertSelection(appContext);
                }
                else
                    appContext->copyBlock.state = cbWasPenDown;
            }
            else
            {
                appContext->copyBlock.left = appContext->copyBlock.actPosition;
                appContext->copyBlock.right = appContext->copyBlock.actPosition;
                appContext->copyBlock.oldLeft = appContext->copyBlock.actPosition;
                appContext->copyBlock.oldRight = appContext->copyBlock.actPosition;
                appContext->copyBlock.state = cbWasPenDown;
            }             
        }   
        else
        {
            cbSetLRonActWord(appContext);
            if(cbIsLNotEqualR(appContext))
            {
                appContext->copyBlock.state = cbIsSelection;
                cbInvertSelection(appContext);
            }    
            else
                appContext->copyBlock.state = cbWasPenDown;
        } 
    }
    else
    {
        appContext->copyBlock.left = appContext->copyBlock.actPosition;
        appContext->copyBlock.right = appContext->copyBlock.actPosition;
        appContext->copyBlock.startClick = appContext->copyBlock.actPosition;
        appContext->copyBlock.state = cbWasPenDown;
    }
    return true;    
}

/**
 * Handle penUpEvent
 * return true if handled
 * return false if define DONT_DO_COPY_WORD_DEFINITION
 */
Boolean cbPenUpEvent(AppContext *appContext, Int16 screenX, Int16 screenY)
{
    cbSelectionPoint start,stop;

#ifdef DONT_DO_COPY_WORD_DEFINITION
    return false;
#endif

    if(appContext->copyBlock.state == cbNothingSelected)
        return true;

    if(appContext->copyBlock.state == cbOnceAgain)
    {
        cbInvertSelection(appContext);
        start = appContext->copyBlock.left;
        stop = appContext->copyBlock.right;

        cbNoSelection(appContext);
        
        cbGetWordFromSelectionAndTranslate(appContext, &start, &stop);

        return true;
    }
    
    if(appContext->copyBlock.state == cbIsSelection)
    {
        appContext->copyBlock.oldLeft = appContext->copyBlock.left;
        appContext->copyBlock.oldRight = appContext->copyBlock.right;
    }
    else
    {
        cbInvertSelection(appContext);
        cbNoSelection(appContext);
    }
    return true;    
}

#define PEN_MOVE_IGNORE_THRESH_X 1
#define PEN_MOVE_IGNORE_THRESH_Y 1

/**
 * Handle penMoveEvent
 * return true if handled
 * return false if define DONT_DO_COPY_WORD_DEFINITION
 */
Boolean cbPenMoveEvent(AppContext *appContext, Int16 screenX, Int16 screenY)
{
    Boolean wasScroll = false;
    cbSelectionPoint sp1,sp2,sp3,sp4;
//    cbCompareSelectionPoints(cbSelectionPoint *sp1, cbSelectionPoint *sp2)

#ifdef DONT_DO_COPY_WORD_DEFINITION
    return false;
#endif

    if(appContext->copyBlock.state == cbNothingSelected)
        return true;

    // depending on the device/emulator penMove migth be generated right after
    // penDown with the same/close coordinates. We just ignore penMove events
    // if they fall within very close range of penDown that started this
    // processing. without this double-click on word is totally broken
    
    //old version - bug when just moving horizontal or vertical
    //if (ABS(appContext->copyBlock.penDownX - screenX) <= PEN_MOVE_IGNORE_THRESH_X)
    //    return true;
    //if (ABS(appContext->copyBlock.penDownY - screenY) <= PEN_MOVE_IGNORE_THRESH_Y)
    //    return true;
    
    //new version - works good
    if (ABS(appContext->copyBlock.penDownX - screenX) <= PEN_MOVE_IGNORE_THRESH_X
        && ABS(appContext->copyBlock.penDownY - screenY) <= PEN_MOVE_IGNORE_THRESH_Y)
        return true;

    if(screenY > appContext->copyBlock.nextDy[appContext->lastDispLine - appContext->firstDispLine + 1]
        &&  appContext->lastDispLine+1 < appContext->currDispInfo->linesCount)
    {
        DefScrollDown(appContext, scrollLine);
        cbInvertSelection(appContext);
        wasScroll = true;
    }
    
    if(screenY < appContext->copyBlock.nextDy[0] + CB_SCROLL_UP_DY 
        &&  appContext->firstDispLine > 0)
    {
        DefScrollUp(appContext, scrollLine);
        cbInvertSelection(appContext);
        wasScroll = true;
    }    

    if(!wasScroll)
        if(cbCompareSelectionPoints(&appContext->copyBlock.left, &appContext->copyBlock.startClick) != 0 &&
            cbCompareSelectionPoints(&appContext->copyBlock.right, &appContext->copyBlock.startClick) != 0)
        {
            cbInvertSelection(appContext);
            wasScroll = true;
        }    
        else
        {   
            //set left and right of old selection
            sp1 = appContext->copyBlock.left;
            sp2 = appContext->copyBlock.right;    
        }    
    
    cbIsTextHit(appContext, screenX, screenY);
    
    cbSetLRonActAndStart(appContext);
    if(cbIsLNotEqualR(appContext))
    {
        appContext->copyBlock.state = cbIsSelection;
        if(wasScroll)
        {
            cbInvertSelection(appContext);
        }
        else
        {
            if(cbCompareSelectionPoints(&appContext->copyBlock.left, &sp1) == 0 ||
                cbCompareSelectionPoints(&appContext->copyBlock.right, &sp2) == 0)
            {
                if(cbCompareSelectionPoints(&appContext->copyBlock.left, &sp1) != 0)
                {
                    if(cbCompareSelectionPoints(&appContext->copyBlock.left, &sp1) < 0)
                    {   //invert from left to sp1
                        sp2 = appContext->copyBlock.right;
                        appContext->copyBlock.right = sp1;
                        cbInvertSelection(appContext);
                        appContext->copyBlock.right = sp2;
                    }
                    else
                    {   //invert from sp1 to left
                        sp2 = appContext->copyBlock.right;
                        sp3 = appContext->copyBlock.left;
                        appContext->copyBlock.right = sp3;
                        appContext->copyBlock.left  = sp1;
                        cbInvertSelection(appContext);
                        appContext->copyBlock.left  = sp3;
                        appContext->copyBlock.right = sp2;
                    }
                }
                else
                {
                    if(cbCompareSelectionPoints(&appContext->copyBlock.right, &sp2) > 0)
                    {   //invert from sp2 to right
                        sp1 = appContext->copyBlock.left;
                        appContext->copyBlock.left = sp2;
                        cbInvertSelection(appContext);
                        appContext->copyBlock.left = sp1;
                    }
                    else
                    {   //invert from right to sp2
                        sp3 = appContext->copyBlock.left;
                        sp1 = appContext->copyBlock.right;
                        appContext->copyBlock.left  = sp1;
                        appContext->copyBlock.right = sp2;
                        cbInvertSelection(appContext);
                        appContext->copyBlock.left  = sp3;
                        appContext->copyBlock.right = sp1;
                    }
                }
            }
            else
            {   //now we are at the another side of startClick
                sp3 = appContext->copyBlock.left;
                sp4 = appContext->copyBlock.right;
                appContext->copyBlock.left  = sp1;
                appContext->copyBlock.right = sp2;
                cbInvertSelection(appContext);
                appContext->copyBlock.left  = sp3;
                appContext->copyBlock.right = sp4;
                cbInvertSelection(appContext);            
            }
        }    
    }
    else
    {
        if(!wasScroll)
        {
            sp3 = appContext->copyBlock.left;
            appContext->copyBlock.left  = sp1;
            appContext->copyBlock.right = sp2;
            cbInvertSelection(appContext);
            appContext->copyBlock.left  = sp3;
            appContext->copyBlock.right = sp3;
        }    
        appContext->copyBlock.state = cbWasPenDown;
    }
    return true;    
}

/**
 * Inverts selection from left to right
 */
void cbInvertSelection(AppContext *appContext)
{
    int  i; 
    RectangleType r;

#ifdef DONT_DO_COPY_WORD_DEFINITION
    return;
#endif
    
    //if nothing in currDispInfo
    if(appContext->currDispInfo==NULL)
    {
        cbNoSelection(appContext);
        return;
    }    

#ifndef I_NOAH
    if(appContext->currentWord != appContext->copyBlock.wordNoOld)
    {
        appContext->copyBlock.wordNoOld = appContext->currentWord;
        cbNoSelection(appContext);
        return;
    }
#else
    //no word in buffer?
    if(appContext->currentWordBuf.used <= 0)
    {
        cbNoSelection(appContext);
        return;
    }    
    //in I_noah. detect when word was changed
    if(StrNCaselessCompare(appContext->copyBlock.wordTxtOld,ebufGetDataPointer(&appContext->currentWordBuf),WORD_MAX_LEN-1)!=0)
    {
        StrNCopy(appContext->copyBlock.wordTxtOld,ebufGetDataPointer(&appContext->currentWordBuf),WORD_MAX_LEN-1);
        cbNoSelection(appContext);
        return;
    }
#endif

    //nothing selected 
    if(appContext->copyBlock.state == cbNothingSelected || appContext->copyBlock.state == cbWasPenDown)
        return;

    //if not in display area
    if(appContext->copyBlock.left.lineNo > appContext->lastDispLine ||
       appContext->copyBlock.right.lineNo < appContext->firstDispLine)
       return;
    
    /*only one line (left and right are in the same line)*/    
    if(appContext->copyBlock.left.lineNo == appContext->copyBlock.right.lineNo)    
    {
        r.topLeft.x = appContext->copyBlock.left.dx; 
        r.topLeft.y = appContext->copyBlock.nextDy[appContext->copyBlock.left.lineNo - appContext->firstDispLine];
        r.extent.x = appContext->copyBlock.right.dx - appContext->copyBlock.left.dx;
        r.extent.y = appContext->copyBlock.nextDy[appContext->copyBlock.left.lineNo - appContext->firstDispLine + 1] - r.topLeft.y;

        WinInvertRectangle(&r,0);
    }
    else
    {
        //from left to end
        if(appContext->copyBlock.left.lineNo >= appContext->firstDispLine)
        {
            r.topLeft.x = appContext->copyBlock.left.dx; 
            r.topLeft.y = appContext->copyBlock.nextDy[appContext->copyBlock.left.lineNo - appContext->firstDispLine];
            r.extent.x = appContext->copyBlock.maxDx[appContext->copyBlock.left.lineNo - appContext->firstDispLine] - appContext->copyBlock.left.dx;
            r.extent.y = appContext->copyBlock.nextDy[appContext->copyBlock.left.lineNo - appContext->firstDispLine + 1] - r.topLeft.y;
    
            WinInvertRectangle(&r,0);
        }        
        //all full lines between left and right
        for(i = appContext->copyBlock.left.lineNo + 1; i < appContext->copyBlock.right.lineNo; i++)
            if( i >= appContext->firstDispLine && i <= appContext->lastDispLine)    
            {
                r.topLeft.x = DRAW_DI_X; 
                r.topLeft.y = appContext->copyBlock.nextDy[i - appContext->firstDispLine];
                r.extent.x = appContext->copyBlock.maxDx[i - appContext->firstDispLine] - DRAW_DI_X;
                r.extent.y = appContext->copyBlock.nextDy[i - appContext->firstDispLine + 1] - r.topLeft.y;
        
                WinInvertRectangle(&r,0);
            }
        //from begining to right
        if(appContext->copyBlock.right.lineNo <= appContext->lastDispLine)
        {
            r.topLeft.x = DRAW_DI_X; 
            r.topLeft.y = appContext->copyBlock.nextDy[appContext->copyBlock.right.lineNo - appContext->firstDispLine];
            r.extent.x = appContext->copyBlock.right.dx - DRAW_DI_X;
            r.extent.y = appContext->copyBlock.nextDy[appContext->copyBlock.right.lineNo - appContext->firstDispLine + 1] - r.topLeft.y;
    
            WinInvertRectangle(&r,0);
        }        
    }
}
