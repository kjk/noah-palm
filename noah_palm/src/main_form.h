#ifndef __MAIN_FORM_H__
#define __MAIN_FORM_H__

#include "common.h"

extern const UInt16 lookupStatusBarHeight;

extern Err MainFormLoad(AppContext* appContext);
extern void MainFormPressFindButton(const FormType* form);

#endif