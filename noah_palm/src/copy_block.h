/**
 * 
 * 
 * 
 */
#ifndef _COPY_BLOCK_H_
#define _COPY_BLOCK_H_


Boolean cbCopyToClippboard(struct _AppContext *appContext);
Boolean cbPenDownEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);
Boolean cbPenUpEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);
Boolean cbPenMoveEvent(struct _AppContext *appContext, Int16 screenX, Int16 screenY);


typedef struct _CopyBlock{

    int k;

} CopyBlock;

#endif