#ifndef __INET_WORD_LOOKUP_H__
#define __INET_WORD_LOOKUP_H__

#include "common.h"
#include <NetMgr.h>

#define serverAddress                "www.arslexis.com"
#define serverPort                      80
#define maxResponseLength        8192
#define addressResolveTimeout   evtWaitForever

extern Err INetWordLookup(const Char* word, NetIPAddr* serverIpAddress, Char** response, UInt16* responseLength);

#endif