/** 
 * @file llframestats.h
 * @brief LLFrameStats class definition
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

#ifndef LL_LLFRAMESTATS_H
#define LL_LLFRAMESTATS_H

#include "stdtypes.h"
#include "llstat.h"
#include "llstring.h"
#include "lldarray.h"
#include "v4color.h"
#include "lltimer.h"

class LLFrameStats
{
public:
	// Affects sStatLabels and sStatColors in llframestats.cpp
	enum EStat
	{
		IDLE_NETWORK,
		AGENT_MISC,
		OBJECT_UPDATE,
		CLEAN_DEAD,
		UPDATE_EFFECTS,
		IMAGE_UPDATE,
		UPDATE_MOVE,
		UPDATE_PARTICLES,
		UPDATE_CULL,
		UPDATE_GEOM,
		AUDIO,
		UPDATE_TEX_STATS,
		STATE_SORT,
		REBUILD,
		RENDER_SYNC,
		RENDER_GEOM,
		RENDER_UI,
		MISC_END,
		NUM_STATS
	};

	LLFrameStats();
	virtual ~LLFrameStats();

	void setFilename(const std::string& filename)			{ mFilename = filename; }
	void setSummaryFilename(const std::string& filename)	{ mSummaryFilename = filename; }

	void setTrackStats(const BOOL track_stats);

	void start(const EStat stat_name);

	void addFrameData();

	void dump();

	const char *getCurStatName() const;

	LLStat &getStat(const S32 i)						{ return mStats[i]; }
	const char *getStatLabel(const S32 i) const			{ return sStatLabels[i]; }
	LLColor4 getStatColor(const S32 i) const			{ return sStatColors[i]; }

	static void startLogging(void *);
	static void stopLogging(void *);
	static void timedLogging60(void *);
	static void timedLogging30(void *);
	static void timedLogging10(void *);
private:
	class FrameData
	{
	public:
		FrameData() {};
		F32 mDuration[NUM_STATS];
		F32 mTotalDuration;
		S32 mNumTriangles;
		S32 mNumObjects;
		S32 mNumVisibleObjects;
		S32 mNumFullUpdates;
		S32 mNumTerseUpdates;
		F32 mTotalVorbisTime;
		F32 mLongVorbisTime;
		S32 mNumVorbisDecodes;
	};

	LLString			mFilename;
	LLString			mSummaryFilename;

	BOOL				mTrackStats;
	LLTimer				mTimer;
	BOOL				mUseTimer;
	F32					mStopTime;

	EStat				mCurrentStat;

	LLStat				mStats[NUM_STATS];
	static char			*sStatLabels[NUM_STATS];
	static LLColor4		sStatColors[NUM_STATS];
	LLDynamicArray<FrameData> mFrameData;
	void stop();
};

extern LLFrameStats gFrameStats;

#endif // LL_LLFRAMESTATS_H