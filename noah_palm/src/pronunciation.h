/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Szymon Knitter
*/


/**
 * @file pronunciation.h
 * Interface of pronunciation.c
 * @author Szymon Knitter (szknitter@wp.pl) 
 */
#ifndef _PRONUNCIATION_H_
#define _PRONUNCIATION_H_

//WORD_MAX_LEN
#define PRON_COMPRESED_MAX_LEN      40
#define PRON_DECOMPRESED_MAX_LEN    42

Boolean pronAddPronunciationToBuffer(struct _AppContext *appContext, ExtensibleBuffer *buf, long wordNo);
char* pronTranslateDecompresed(struct _AppContext *appContext, unsigned char *decompresed);

#endif