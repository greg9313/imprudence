/** 
 * @file lltut.h
 * @author Phoenix
 * @date 2005-09-26
 * @brief helper tut methods
 *
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

/** 
 *
 * THOROUGH_DESCRIPTION
 *
 */

#ifndef LL_LLTUT_H
#define LL_LLTUT_H

#include <tut/tut.h>

#include "lldate.h"
#include "lluri.h"

class LLSD;

namespace tut
{
	template <class T,class Q>
	void ensure_not_equals(const char* msg,const Q& actual,const T& expected)
	{
		if( expected == actual )
		{
			std::stringstream ss;
			ss << (msg?msg:"") << (msg?": ":"") << "both equal " << expected;
			throw tut::failure(ss.str().c_str());
		}
	}

	template <class T,class Q>
	void ensure_not_equals(const Q& actual,const T& expected)
	{
		ensure_not_equals(NULL, actual, expected);
	}
	
	
	template <class T,class Q>
	void ensure_equals(const std::string& msg,
		const Q& actual,const T& expected)
		{ ensure_equals(msg.c_str(), actual, expected); }

	template<>
	void ensure_equals(const char* msg,
		const LLDate& actual, const LLDate& expected);

	template<>
	void ensure_equals(const char* msg,
		const LLURI& actual, const LLURI& expected);
		
	template<>
	void ensure_equals(const char* msg,
		const std::vector<U8>& actual, const std::vector<U8>& expected);

	template<>
	void ensure_equals(const char* msg,
		const LLSD& actual, const LLSD& expected);
	
	void ensure_starts_with(const std::string& msg,
		const std::string& actual, const std::string& expectedStart);

	void ensure_contains(const std::string& msg,
		const std::string& actual, const std::string& expectedSubString);
}


#endif // LL_LLTUT_H