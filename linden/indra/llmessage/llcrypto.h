/** 
 * @file llcrypto.h
 * @brief llcrypto library module header. For now, all crypto classes
 * are here, and as we grow the library, we can split these things
 * out.
 *
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#ifndef LL_LLCRYPTO_H
#define LL_LLCRYPTO_H

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLCipher
//
// Abstract base class for a cipher object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLCipher
{
public:
	virtual ~LLCipher() {}

	// encrypt src and place result into dst. returns TRUE if
	// encryption was successful, otherwise FALSE.
	virtual BOOL encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len) = 0;

	// decrypt src and place result into dst. returns TRUE if
	// encryption was successful, otherwise FALSE. 
	virtual BOOL decrpyt(const U8* src, U32 src_len, U8* dst, U32 dst_len) = 0;

	// returns the space required to encrypt for a unencrypted source
	// buffer of length len.
	virtual U32 requiredEncryptionSpace(U32 src_len) const = 0 ;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLNullCipher
//
// A class which implements LLCipher, but does not transform src
// during encryption.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLNullCipher
{
public:
	LLNullCipher() {}
	virtual ~LLNullCipher() {}
	virtual BOOL encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
	virtual BOOL decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
	virtual U32 requiredEncryptionSpace(U32 src_len);
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLXORCipher
//
// Implementation of LLCipher which encrypts using a XOR pad.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLXORCipher
{
public:
	LLXORCipher(const U8* pad, U32 pad_len);
	LLXORCipher(const LLXORCipher& cipher);
	virtual ~LLXORCipher();
	LLXORCipher& operator=(const LLXORCipher& cipher);

	// Cipher functions
	virtual BOOL encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
	virtual BOOL decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
	virtual U32 requiredEncryptionSpace(U32 src_len);

	// special syntactic-sugar since xor can be performed in place.
	BOOL encrypt(U8* buf, U32 len) { return encrypt((const U8*)buf, len, buf, len); }
	BOOL decrypt(U8* buf, U32 len) { return decrypt((const U8*)buf, len, buf, len); }

#ifdef _DEBUG
	// This function runs tests to make sure the crc is
	// working. Returns TRUE if it is.
	static BOOL testHarness();
#endif

protected:
	void init(const U8* pad, U32 pad_len);
	U8* mPad;
	U8* mHead;
	U32 mPadLen;
};

#endif // LL_LLCRYPTO_H