/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Szymon Knitter
*/

/**
 * @file copy_block.h
 * Interface of copy_block.c
 * @author Szymon Knitter (szknitter@wp.pl) 
 */
#ifndef _COPY_BLOCK_H_
#define _COPY_BLOCK_H_

/**
 * Max no of text lines displayed at once (set it!) 
 * (in sony it will be 20 but safety set it 30)
 */
#define CB_MAX_DISPLAY_LINES  30    
/**
 * No of pixels. 
 * When screenY < CB_SCROLL_UP_DY+DRAW_DI_Y 
 *     then scrollUp screen (only if pen is selecting)
 */
#define CB_SCROLL_UP_DY       2

Boolean cbCopyToClippboard(struct _AppContext *appContext);
Boolean cbPenDownEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);
Boolean cbPenUpEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);
Boolean cbPenMoveEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);

void cbInvertSelection(struct _AppContext *appContext);
void cbNoSelection(struct _AppContext *appContext);

/**
 * States of selection
 */
typedef enum {
    cbNothingSelected = 0,
    cbWasPenDown,
    cbIsSelection,
    cbOnceAgain    
} cbState;

/**
 * Point in display info
 */
typedef struct {
    
    int     lineNo; //no of line
    int     charNo; //no of char in that line
    Int16   dx;     //offset of that char
} cbSelectionPoint;

/**
 * Include all important informations
 * 
 */
typedef struct _CopyBlock{
    cbState     state;
    
    cbSelectionPoint    startClick;     //first Click
    cbSelectionPoint    actPosition;    
    cbSelectionPoint    left;           //left end of selection
    cbSelectionPoint    right;          //right end of selection
    cbSelectionPoint    oldLeft;        //used to detect double click
    cbSelectionPoint    oldRight;       //used to detect double click
   
    Int16   maxDx[CB_MAX_DISPLAY_LINES];        
    Int16   nextDy[CB_MAX_DISPLAY_LINES + 1];   
    char    firstTag;                   //to store format of first displayed line
    char    actTag;                     //format of actPosition char
    
#ifndef I_NOAH
    long    wordNoOld;    
#else
    char    wordTxtOld[WORD_MAX_LEN + 1];   //in I_Noah words have no No!
#endif
} CopyBlock;

#endif