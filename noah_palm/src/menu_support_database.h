#ifndef __MENU_SUPPORT_DATABASE_H__
#define __MENU_SUPPORT_DATABASE_H__

#include "common.h"

extern Err PrepareSupportDatabase(const Char* name, UInt16 bitmapId);
extern Err DisposeSupportDatabase(const Char* name);

#endif