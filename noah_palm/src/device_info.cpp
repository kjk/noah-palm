/**
 * @file device_info.cpp
 *
 *
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 *
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#include "device_info.h"
#include "common.h"
#include <DLServer.h>
#include <68K/Hs.h>

Err GetDeviceSerialNumber(ExtensibleBuffer* out)
{
    Char* data=0;
    UInt16 length=0;
    Err error=SysGetROMToken(0, sysROMTokenSnum, reinterpret_cast<UInt8**>(&data), &length);
    if (!error)
    {
        if (NULL==data || 0xff==*data)
        {
            error=sysErrNotAllowed;
#ifdef NEVER
            CreateNewMemo( "SysGetROMToken() didn't return valid data",-1);
#endif
        }
        else
        {
            ebufAddStrN(out, data, length);
            ebufAddChar(out, chrNull);
#ifdef NEVER
            CreateNewMemo(data, length);
#endif
        }
    }
#ifdef NEVER
    else
        LogErrorToMemo_("SysGetROMToken() return", error);
#endif

    return error;
}

Err GetHotSyncName(ExtensibleBuffer* out)
{
    Err error=errNone;
    Char nameBuffer[dlkUserNameBufSize];
    Assert(out);
    error=DlkGetSyncInfo(NULL, NULL, NULL, nameBuffer, NULL, NULL);
    if (!error)
    {
        if (StrLen(nameBuffer))
            ebufAddStr(out, nameBuffer);
        else
            error=sysErrNotAllowed;            
    }
    return error;
}

Err GetPhoneNumber(ExtensibleBuffer* out)
{
    Boolean libLoaded=false;
    UInt16 refNum=sysInvalidRefNum ;
    Err error=SysLibFind(phnLibCDMAName, &refNum);
    if (error)
        error=SysLibFind(phnLibGSMName, &refNum);
    if (error)
    {
        error=SysLibLoad(phnLibDbType, phnLibCDMADbCreator, &refNum);
        if (error)
            error=SysLibLoad(phnLibDbType, phnLibGSMDbCreator, &refNum);
        if (!error)
            libLoaded=true;
    }
    if (!error)
    {
        Assert(sysInvalidRefNum!=refNum);
        PhnAddressList addressList=NULL;
        error=PhnLibGetOwnNumbers(refNum, &addressList);
        if (!error)
        {
            PhnAddressHandle address=NULL;
            error=PhnLibAPGetNth(refNum, addressList, 1, &address);
            if (!error && address)
            {
                Char* number=PhnLibAPGetField(refNum, address, phnAddrFldPhone);
                MemHandleFree(address);
                if (number)
                {
                    ebufAddStr(out, number);
                    MemPtrFree(number);
                }
                else 
                    error=sysErrNotAllowed;
            }
            MemHandleFree(addressList);
        }
        if (libLoaded)
            SysLibRemove(refNum);
    }
    return error;
}

static void HexBinEncode(ExtensibleBuffer* inOut)
{
    const Char* digits="0123456789ABSCDEF";
    Char* data=ebufGetDataPointer(inOut);
    UInt16 dataLength=StrLen(data);
    ExtensibleBuffer outBuffer;
    ebufInit(&outBuffer, 2*dataLength + 1);
    Char* out=ebufGetDataPointer(&outBuffer);
    while (*data)
    {
        UInt8 b=*(data++);
        *(out++)=digits[b/16];
        *(out++)=digits[b%16];
    }
    *out=chrNull;
    ebufSwap(inOut, &outBuffer);
    ebufFreeData(&outBuffer);
}

typedef Err (TokenGetter)(ExtensibleBuffer*);

static void RenderDeviceIdentifierToken(ExtensibleBuffer* out, const Char* prefix, TokenGetter* getter)
{
    Boolean empty=(ebufGetDataSize(out)==0);
    ExtensibleBuffer token;
    ebufInit(&token, 0);
    Err error=(*getter)(&token);
    if (!error)
    {
        if (!empty)
            ebufAddChar(out, ':');
        ebufAddStr(out, const_cast<Char*>(prefix));
        HexBinEncode(&token);
        ebufAddStr(out, ebufGetDataPointer(&token));
    }
    ebufFreeData(&token);
}

void RenderDeviceIdentifier(ExtensibleBuffer* out)
{
    Assert(out);
    RenderDeviceIdentifierToken(out, "HS", GetHotSyncName);
    RenderDeviceIdentifierToken(out, "SN", GetDeviceSerialNumber);
    RenderDeviceIdentifierToken(out, "PN", GetPhoneNumber);
}
