/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: szymon knitter (szknitter@wp.pl)
 */

/* Dont want armlet? - just uncomment*/
//#define DONT_DO_ARMLET 1 

/**
 * run armlet or not? - thats the question!
 * we can set minimum settings like this:
 */
//minimum buffer->used for format1/2
#define ARMTORUN_MIN_BUF_USED       50
//minimum (target_entry - current_entry)
#define ARMTORUN_MIN_ENTRIES_TO_GO  30 

#include "extensible_buffer.h"

Boolean armTestArmLet();
Boolean armFormatonSortedBuffer(ExtensibleBuffer *buf, int format_nr);
Boolean armGetDefsRecordWhile(long *current_entry, long *offset, long *curr_len,
                unsigned char **def_lens_fast, unsigned char *def_lens_fast_end, long synsetNoFast);

