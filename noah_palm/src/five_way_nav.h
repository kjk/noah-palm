#ifndef __FIVE_WAY_NAV_H__
#define __FIVE_WAY_NAV_H__

#include "common.h"
#include <PalmNavigator.h>
#include <68K/Hs.h>

void InitFiveWay(AppContext* appContext);

#define HaveFiveWay(appContext) AppTestFlag(appContext, appHasFiveWayNav)
#define HaveHsNav(appContext) AppTestFlag(appContext, appHasHsNav)

#define HsNavCenterPressed(appContext, eventP) \
( \
  IsHsFiveWayNavEvent(appContext, eventP) && \
  ((eventP)->data.keyDown.chr == vchrRockerCenter) && \
  (((eventP)->data.keyDown.modifiers & commandKeyMask) != 0) \
)

#define HsNavDirectionPressed(appContext, eventP, nav) \
( \
  IsHsFiveWayNavEvent(appContext, eventP) && \
  ( vchrRocker ## nav == vchrRockerUp) ? \
   (((eventP)->data.keyDown.chr == vchrPageUp) || \
    ((eventP)->data.keyDown.chr == vchrRocker ## nav)) : \
   (vchrRocker ## nav == vchrRockerDown) ? \
   (((eventP)->data.keyDown.chr == vchrPageDown) || \
    ((eventP)->data.keyDown.chr == vchrRocker ## nav)) : \
    ((eventP)->data.keyDown.chr == vchrRocker ## nav) \
)

#define HsNavKeyPressed(appContext, eventP, nav) \
( \
  ( vchrRocker ## nav == vchrRockerCenter ) ? \
  HsNavCenterPressed(appContext, eventP) : \
  HsNavDirectionPressed(appContext, eventP, nav) \
)

#define IsHsFiveWayNavEvent(appContext, eventP) \
( \
    HaveHsNav(appContext) && ((eventP)->eType == keyDownEvent) && \
    ( \
        ((((eventP)->data.keyDown.chr == vchrPageUp) || \
          ((eventP)->data.keyDown.chr == vchrPageDown)) && \
         (((eventP)->data.keyDown.modifiers & commandKeyMask) != 0)) \
        || \
        (TxtCharIsRockerKey((eventP)->data.keyDown.modifiers, \
                            (eventP)->data.keyDown.chr)) \
    ) \
)

#define FiveWayCenterPressed(appContext, eventP) \
( \
  NavSelectPressed(eventP) || \
  HsNavCenterPressed(appContext, eventP) \
)

#define FiveWayDirectionPressed(appContext, eventP, nav) \
( \
  NavDirectionPressed(eventP, nav) || \
  HsNavDirectionPressed(appContext, eventP, nav) \
)

/* only to be used with Left, Right, Up, and Down; use
   FiveWayCenterPressed for Select/Center */
#define FiveWayKeyPressed(appContext, eventP, nav) \
( \
  NavKeyPressed(eventP, nav) || \
  HsNavKeyPressed(appContext, eventP, nav) \
)

#define IsFiveWayEvent(appContext, eventP) \
( \
  HaveHsNav(appContext) ? IsHsFiveWayNavEvent(appContext, eventP) : IsFiveWayNavEvent(eventP) \
)

#endif