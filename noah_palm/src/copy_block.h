/**
 * @file copy_block.h
 * Interface of copy_block.c
 * @author Szymon Knitter (szknitter@wp.pl) 
 */
#ifndef _COPY_BLOCK_H_
#define _COPY_BLOCK_H_

/**
 * Max no of text lines displayed at once (set it)
 */
#define CB_MAX_DISPLAY_LINES  40

Boolean cbCopyToClippboard(struct _AppContext *appContext);
Boolean cbPenDownEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);
Boolean cbPenUpEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);
Boolean cbPenMoveEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);

void cbInvertSelection(struct _AppContext *appContext);
void cdNoSelection(struct _AppContext *appContext);

/**
 * States of selection
 */
typedef enum {
    noSelection = 0,
    wasPenDown,
    isSelection,
    onceAgain    
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
        
} CopyBlock;

#endif