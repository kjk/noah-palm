/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
 */
#ifndef _CW_DEFS_H_
#define _CW_DEFS_H_ 1

/* For using from within Metrowerks CodeWarrior I need to use this hack
to provide per-target defines. In gcc version they're set via Makefile.
*/

#ifdef __MWERKS__

#define FS_VFS 1

#if __ide_target("NP Debug")
#define NOAH_PRO 1
#define WNLEX_DICT 1
#define WN_PRO_DICT 1
#define SIMPLE_DICT 1
#define EP_DICT 1
#define MEM_LEAKS 1
#define DEBUG 1
#endif

#if __ide_target("NP Release")
#define NOAH_PRO 1
#define WNLEX_DICT 1
#define WN_PRO_DICT 1
#define SIMPLE_DICT 1
#define EP_DICT 1
#endif

#if __ide_target("NP Handango Demo")
#define NOAH_PRO 1
#define WN_PRO_DICT 1
#define DEMO 1
#define DEMO_HANDANGO 1
#endif

#if __ide_target("NP Palmgear Demo")
#define NOAH_PRO 1
#define WN_PRO_DICT 1
#define DEMO 1
#define DEMO_PALMGEAR 1
#endif

#if __ide_target("NL Debug")
#define NOAH_LITE 1
#define WNLEX_DICT 1
#define MEM_LEAKS 1
#define DEBUG 1
#endif

#if __ide_target("NL Release")
#define NOAH_LITE 1
#define WNLEX_DICT 1
#endif

#if __ide_target("Thes Debug")
#define THESAURUS 1
#define MEM_LEAKS 1
#define DEBUG 1
#endif

#if __ide_target("Thes Release")
#define THESAURUS 1
#endif

#if __ide_target("Thes handango demo")
#define THESAURUS 1
#define DEMO 1
#define DEMO_HANDANGO 1
#endif

#if __ide_target("Thes palmgear demo")
#define THESAURUS 1
#define DEMO 1
#define DEMO_PALMGEAR 1
#endif

#if __ide_target("iNoah Debug")
#define DEBUG 1
#define MEM_LEAKS 1
#define I_NOAH
#endif

#if __ide_target("iNoah Release")
#define I_NOAH
#endif

#endif /* __MWERKS__ */

#endif /* _CW_DEFS_H_ */
