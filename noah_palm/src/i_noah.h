#ifndef _I_NOAH_H_
#define _I_NOAH_H_

#include "i_noah_rcp.h"

/**
 * @todo Tweak iNoah settings.
 */
#define  I_NOAH_CREATOR      'STRT'
#define  I_NOAH_PREF_TYPE    'thpr'

#define APP_CREATOR I_NOAH_CREATOR
#define APP_PREF_TYPE I_NOAH_PREF_TYPE

#define AppPrefId 0


/* structure of the general preferences record */
typedef struct
{
    DisplayPrefs displayPrefs;
} iNoahPrefs;

typedef iNoahPrefs AppPrefs;

#define PREF_REC_MIN_SIZE 4

#define SUPPORT_DATABASE_NAME "iNoah_Temp"
#define APP_NAME "iNoah"

//extern Err AppPerformResidentLookup(Char* term);

#endif
