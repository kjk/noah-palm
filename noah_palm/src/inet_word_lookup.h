#ifndef __INET_WORD_LOOKUP_H__
#define __INET_WORD_LOOKUP_H__

#include "common.h"
#include <NetMgr.h>

#define serverAddress                "www.arslexis.com"
#define serverPort                      80
#define serverRelativeURL           "/dict-raw.php?word=^0"
#define maxResponseLength        8192               // reasonable limit so malicious response won't use all available memory
#define responseBufferSize          256                 // size of single chunk used to retrieve server response
#define addressResolveTimeout   20000              // timeouts in milliseconds
#define socketOpenTimeout         1000
#define socketConnectTimeout     10000
#define transmitTimeout              5000

extern Err INetWordLookup(const Char* word, NetIPAddr* serverIpAddress, Char** response, UInt16* responseLength);

#endif