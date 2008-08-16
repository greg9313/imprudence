/** 
 * @file llimagej2c.h
 * @brief Image implmenation for jpeg2000.
 *
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLIMAGEJ2C_H
#define LL_LLIMAGEJ2C_H

#include "llimage.h"
#include "llassettype.h"

class LLImageJ2CImpl;
class LLImageJ2C : public LLImageFormatted
{
protected:
	virtual ~LLImageJ2C();

public:
	LLImageJ2C();

	// Base class overrides
	/*virtual*/ BOOL updateData();
	/*virtual*/ BOOL decode(LLImageRaw *raw_imagep, F32 decode_time=0.0);
	/*virtual*/ BOOL decode(LLImageRaw *raw_imagep, F32 decode_time, S32 first_channel, S32 max_channel_count);
	/*virtual*/ BOOL encode(const LLImageRaw *raw_imagep, F32 encode_time=0.0);
	/*virtual*/ S32 calcHeaderSize();
	/*virtual*/ S32 calcDataSize(S32 discard_level = 0);
	/*virtual*/ S32 calcDiscardLevelBytes(S32 bytes);
	/*virtual*/ S8  getRawDiscardLevel();
	
	// Encode with comment text 
	BOOL encode(const LLImageRaw *raw_imagep, const char* comment_text, F32 encode_time=0.0);

	BOOL validate(U8 *data, U32 file_size);
	BOOL loadAndValidate(const LLString &filename);

	// Encode accessors
	void setReversible(const BOOL reversible); // Use non-lossy?
	void setRate(F32 rate);
	void setMaxBytes(S32 max_bytes);
	S32 getMaxBytes() const { return mMaxBytes; }

	static S32 calcHeaderSizeJ2C();
	static S32 calcDataSizeJ2C(S32 w, S32 h, S32 comp, S32 discard_level, F32 rate = 0.f);

	static void openDSO();
	static void closeDSO();
	
protected:
	friend class LLImageJ2CImpl;
	friend class LLImageJ2COJ;
	friend class LLImageJ2CKDU;
	void setDecodingDone(BOOL complete = TRUE);
	void updateRawDiscardLevel();

	S32 mMaxBytes; // Maximum number of bytes of data to use...
	S8  mRawDiscardLevel;
	F32 mRate;
	LLImageJ2CImpl *mImpl;
};

// Derive from this class to implement JPEG2000 decoding
class LLImageJ2CImpl
{
public:
	virtual ~LLImageJ2CImpl();
protected:
	virtual BOOL getMetadata(LLImageJ2C &base) = 0;
	virtual BOOL decodeImpl(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count) = 0;
	virtual BOOL encodeImpl(LLImageJ2C &base, const LLImageRaw &raw_image, const char* comment_text, F32 encode_time=0.0) = 0;

	friend class LLImageJ2C;
};

#define LINDEN_J2C_COMMENT_PREFIX "LL_"

#endif