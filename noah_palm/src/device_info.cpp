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

#define NON_PORTABLE
#include <HwrMiscFlags.h>  // For hwrOEMCompanyIDHandspring
#undef NON_PORTABLE

static Boolean fIsTreo()
{
    UInt32 rom = 0, hal = 0, company = 0, device = 0, hsFtrVersion;

    UInt32 requiredVersion = sysMakeROMVersion(5,0,0,sysROMStageRelease,0);
    if (FtrGet (hsFtrCreator, hsFtrIDVersion, &hsFtrVersion) != 0)
        return false;

    FtrGet (sysFtrCreator, sysFtrNumOEMHALID, &hal);
    FtrGet (sysFtrCreator, sysFtrNumOEMCompanyID, &company);
    FtrGet (sysFtrCreator, sysFtrNumOEMDeviceID, &device);
    FtrGet (sysFtrCreator, sysFtrNumROMVersion, &rom);
    if (device == hsDeviceIDOs5Device1Sim)
    {
        // doesn't work on simulator
        return false;
    }

    if (    rom       >= requiredVersion
        &&  company   == hwrOEMCompanyIDHandspring     
        &&  device    == hsDeviceIDOs5Device1
        &&  (   hal   == hsHALIDHandspringOs5Rev1
             || hal   == hsHALIDHandspringOs5Rev1Sim) )
    {
        return true;
    }

    return false;
}


Err GetDeviceSerialNumber(ExtensibleBuffer* out)
{
    char*   data=0;
    UInt16  length;
    Err error=SysGetROMToken(0, sysROMTokenSnum, reinterpret_cast<UInt8**>(&data), &length);
    if (error)
        return error;

    if (NULL==data)
        return sysErrNotAllowed;

    unsigned char firstChar = ((unsigned char*)data)[0];
    if (0xff==firstChar)
        return sysErrNotAllowed;

    ebufAddStrN(out, data, length);
    return errNone;
}

Err GetOEMCompanyId(ExtensibleBuffer* out)
{
    Err     error;
    UInt32  id;
    char *  idAsChar;

    error = FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &id);
    if (error)
        return error;

    idAsChar = (char*) &id;
    ebufAddStrN(out, idAsChar, sizeof(UInt32));
    return errNone;
}

Err GetOEMDeviceId(ExtensibleBuffer* out)
{
    Err     error;
    UInt32  id;
    char *  idAsChar;

    error = FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, &id);
    if (error)
        return error;
    idAsChar = (char*) &id;
    ebufAddStrN(out, idAsChar, sizeof(UInt32));
    return errNone;
}

Err GetHotSyncName(ExtensibleBuffer* out)
{
    Err   error;
    char  nameBuffer[dlkUserNameBufSize];

    Assert(out);
    error=DlkGetSyncInfo(NULL, NULL, NULL, nameBuffer, NULL, NULL);
    if (error)
        return error;

    if (StrLen(nameBuffer)>0)
        ebufAddStr(out, nameBuffer);
    else
        error=sysErrNotAllowed;            
    return error;
}

Err GetHsSerialNum(ExtensibleBuffer* out)
{
    char  versionStr[hsVersionStringSize];

    if (!fIsTreo())
        return sysErrNotAllowed;

    MemSet (versionStr, hsVersionStringSize, 0);
    if (HsGetVersionString (hsVerStrSerialNo, versionStr, NULL) != errNone)
        return sysErrNotAllowed;

    if (0==versionStr[0])
        return sysErrNotAllowed;

    ebufAddStr(out, (char*)versionStr);
    return errNone;
}

static UInt32 GetPhoneType (void)
{

  UInt32 phnType = hsAttrPhoneTypeGSM; 
  HsAttrGet (hsAttrPhoneType, 0, &phnType);
  return phnType;
}

static Boolean CheckPhonePower(UInt16 PhoneLibRefNum)
{
    UInt16 isSimReady = false;

    // Make sure that the phone is powered...
    if (PhnLibModulePowered (PhoneLibRefNum) != phnPowerOn )
    {
        if (resAlertPhoneNotReadyTurnOn == FrmAlert(alertPhoneNotReady))
            PhnLibSetModulePower (PhoneLibRefNum, 1);
        else
            return false;
    }

    //Check if radio has initiliazed the SIM on GSM phone
    //if not, will ask the user if he wants to check again or cancel out
    while (!isSimReady)  
    {
        if ((PhnLibGetSIMStatus(PhoneLibRefNum) != simReady) && (GetPhoneType() == hsAttrPhoneTypeGSM))
        {
            if (resAlertSimNotReadyCheck == FrmAlert(alertSimNotReady))
                isSimReady = false;
            else
                return false;
        }
        else
            isSimReady = true;
    }
    return true;
}

static Err GetPhoneLib(UInt16* refNum, Boolean* fNeedToRemove)
{
    *fNeedToRemove = false;
    Err error=SysLibFind(phnLibCDMAName, refNum);
    if (error)
        error=SysLibFind(phnLibGSMName, refNum);
    if (error)
    {
        error=SysLibLoad(phnLibDbType, phnLibCDMADbCreator, refNum);
        if (error)
            error=SysLibLoad(phnLibDbType, phnLibGSMDbCreator, refNum);
        if (!error)
            *fNeedToRemove=true;
    }
    return error;
}

Err GetPhoneNumber(ExtensibleBuffer* out)
{
    Boolean fNeedToRemove=false;
    UInt16 refNum;

    Err error = GetPhoneLib(&refNum,&fNeedToRemove);
    if (error)
        return error;

    if ( !CheckPhonePower(refNum) )
        return sysErrNotAllowed;

    Assert(sysInvalidRefNum!=refNum);

    PhnAddressList addressList=NULL;
    error=PhnLibGetOwnNumbers(refNum, &addressList);
    if (error)
        goto Exit;

    PhnAddressHandle address=NULL;
    error=PhnLibAPGetNth(refNum, addressList, 1, &address);
    if (!error && address)
    {
        char* number=PhnLibAPGetField(refNum, address, phnAddrFldPhone);
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
Exit:
    if (fNeedToRemove)
        SysLibRemove(refNum);
    return error;
}

#define HEX_DIGITS "0123456789ABCDEF"

static void HexBinEncode(ExtensibleBuffer* inOut)
{
    char*            data=ebufGetDataPointer(inOut);
    UInt16           dataLength=ebufGetDataSize(inOut);
    ExtensibleBuffer outBuffer;

    ebufInit(&outBuffer, 2*dataLength + 1);
    while (dataLength>0)
    {
        UInt8 b=*(data++);
        dataLength--;
        ebufAddChar(&outBuffer, HEX_DIGITS[b/16]);
        ebufAddChar(&outBuffer, HEX_DIGITS[b%16]);
    }
    ebufAddChar(&outBuffer, chrNull);
    ebufSwap(inOut, &outBuffer);
    ebufFreeData(&outBuffer);
}

typedef Err (TokenGetter)(ExtensibleBuffer*);

static void RenderDeviceIdentifierToken(ExtensibleBuffer* out, const char* prefix, TokenGetter* getter)
{
    Boolean empty=(ebufGetDataSize(out)==0);
    ExtensibleBuffer token;
    ebufInit(&token, 0);
    Err error=(*getter)(&token);
    if (!error)
    {
        if (!empty)
            ebufAddChar(out, ':');
        ebufAddStr(out, const_cast<char*>(prefix));
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
    // TODO: uncomment later
    // RenderDeviceIdentifierToken(out, "HN", GetHsSerialNum);
    RenderDeviceIdentifierToken(out, "PN", GetPhoneNumber);
    RenderDeviceIdentifierToken(out, "OC", GetOEMCompanyId);
    RenderDeviceIdentifierToken(out, "OD", GetOEMDeviceId);
}
