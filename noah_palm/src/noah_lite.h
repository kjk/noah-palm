/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _NOAH_LITE_H_
#define _NOAH_LITE_H_

#include "noah_lite_rcp.h"

#define  NOAH_LITE_CREATOR     'NoaH'
#define APP_CREATOR NOAH_LITE_CREATOR


typedef struct
{
    char                lastWord[WORD_MAX_LEN];
    DisplayPrefs        displayPrefs;
} NoahLitePrefs;

typedef NoahLitePrefs AppPrefs;

#define APP_NAME "Noah Lite"

#endif
