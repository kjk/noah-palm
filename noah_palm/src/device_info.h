/**
 * @file device_info.h
 * Declarations of functions that gather various information about device that allows to identify it.
 *
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 *
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 */
#ifndef __DEVICE_INFO_H__
#define __DEVICE_INFO_H__
 
#include <PalmOS.h>
#include "extensible_buffer.h"

/**
 * Queries system for device serial number.
 * @param out pointer to @c ExtensibleBuffer that will be filled with data on successful return.
 * @return standard PalmOS error code (@c errNone on success). Contrary to SysGetROMToken() system call
 * data are guaranteed to be valid if success is indicated.
 */
extern Err GetDeviceSerialNumber(ExtensibleBuffer* out);

/**
 * Queries system for actual HotSync user name.
 * @param out pointer to @c ExtensibleBuffer that will be filled with data on successful return.
 * @return standard PalmOS error code (@c errNone on success).
 */
extern Err GetHotSyncName(ExtensibleBuffer* out);

/**
 * Queries available phone libraries for actual phone number (works probably only on smart phones like Treo, Zire etc.).
 * @param out pointer to @c ExtensibleBuffer that will be filled with data on successful return.
 * @return standard PalmOS error code (@c errNone on success).
 */
extern Err GetPhoneNumber(ExtensibleBuffer* out);

/**
 * Renders device identifier formatted according to @link http://dev.arslexis.com/cgi/cvstrac.cgi/noah_palm/wiki?p=InoahProtocol
 * InoahProtocol @endlink into provided buffer.
 * @param out pointer to @c ExtensibleBuffer that will contain device identifier on successful return.
 * @note It is possible (and highly probable) that returned string will contain ':' character that is forbidden as a part of
 * URI and should be URL-Encoded before transmitting (using StrUrlEncode() or the like).
 */
extern void RenderDeviceIdentifier(ExtensibleBuffer* out);

#endif