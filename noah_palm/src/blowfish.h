/**
 * @file blowfish.h
 * Interface to Blowfish cipher algorithm using single class @c BlowfishCipher.
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 * @see http://www.schneier.com/blowfish.html
 */

#ifndef __BLOWFISH_H__
#define __BLOWFISH_H__

#ifndef _MSC_VER // so that I can easily test it with MSVC
#include <PalmOS.h>
#else 

typedef unsigned __int32 UInt32;
typedef unsigned __int16 UInt16; 
typedef unsigned __int8  UInt8;

#endif

/**
 * Blowfish cipher implementation.
 * Be aware that single object of class @c BlowfishCipher is slightly larger than 1kB, 
 * so don't try to allocate it on stack as it may easily lead to overflow.
 */
class BlowfishCipher {
    
    enum {
        numberOfRounds=16,
    };
    
    static const UInt32 originalP_[numberOfRounds+2];
    static const UInt32 originalS_[4][256];
    
    BlowfishCipher(const BlowfishCipher&);
    BlowfishCipher& operator=(const BlowfishCipher&);
    
public:

    enum {
        /**
         * Maximum key length in bytes.
         */
        maxKeyBytes=56,
    };
    
private:    
    UInt32 p_[numberOfRounds+2];
    UInt32 s_[4][256];
    
    UInt32 f(UInt32 x);
    
    void encrypt(UInt32& l, UInt32& r);
    void decrypt(UInt32& l, UInt32& r);
    
public:

    /**
     * Constructs @c BlowfishCipher object initializing it with specified @c key.
     * @param key byte array containing key bytes.
     * @param keyLength length of array passed through @c key.
     */
    explicit BlowfishCipher(const UInt8* key, UInt16 keyLength);

    /**
     * Encrypts sequence of bytes with current key.
     * @param in pointer to start of input sequence.
     * @param out pointer to start of output buffer that should provide space for at least @c length characters. 
     * @param length length of sequence pointed by @c in.
     * @note @c length should be integer multiply of 8, otherwise undefined behaviour is expected. 
     * If your @c in sequence is shorter, you should zero-padd it prior to encoding.
     */
    void encrypt(const UInt8* in, UInt8* out, UInt16 length);
    
    /**
     * Encrypts sequence of bytes with current key in place.
     * @param inOut pointer to start of input sequence.
     * @param length length of sequence pointed by @c inOut.
     * @note @c length should be integer multiply of 8, otherwise undefined behaviour is expected. 
     * If your @c in sequence is shorter, you should zero-padd it prior to encoding.
     */
    void encrypt(UInt8* inOut, UInt16 length);
    
    /**
     * Decrypts sequence of bytes with current key.
     * @param in pointer to start of input sequence.
     * @param out pointer to start of output buffer that should provide space for at least @c length characters. 
     * @param length length of sequence pointed by @c in.
     * @note @c length is expected to be integer multiply of 8 (that is encryption invariant).
     */
    void decrypt(const UInt8* in, UInt8* out, UInt16 length);

    /**
     * Decrypts sequence of bytes with current key in place.
     * @param inOut pointer to start of input sequence.
     * @param length length of sequence pointed by @c inOut.
     * @note @c length is expected to be integer multiply of 8 (that is encryption invariant).
     */
    void decrypt(UInt8* inOut, UInt16 length);

#ifdef _MSC_VER
    friend int main(int, char*[]);
#endif

};

#endif