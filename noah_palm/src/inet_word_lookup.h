#ifndef __INET_WORD_LOOKUP_H__
#define __INET_WORD_LOOKUP_H__

#include "common.h"
#include <NetMgr.h>

#define serverAddress                "www.arslexis.com"
#define serverPort                      80
#define maxResponseLength        8192
#define responseBufferSize          256
#define addressResolveTimeout   evtWaitForever
#define socketOpenTimeout         addressResolveTimeout
#define socketConnectTimeout     socketOpenTimeout
#define serverRelativeURL           "/dict-raw.php?word=^0"
#define transmitTimeout              evtWaitForever

extern Err INetWordLookup(const Char* word, NetIPAddr* serverIpAddress, Char** response, UInt16* responseLength);

#endif