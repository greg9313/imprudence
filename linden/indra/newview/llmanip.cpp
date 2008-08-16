/** 
 * @file llmanip.cpp
 * @brief LLManip class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llmanip.h"

#include "llmath.h"
#include "v3math.h"
//#include "llquaternion.h"
#include "llgl.h"
#include "llprimitive.h"
#include "llview.h"
#include "llviewerimagelist.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llfontgl.h"
#include "llhudrender.h"
#include "llselectmgr.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerjoint.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llworld.h"		// for gWorldPointer
#include "llresmgr.h"
#include "viewer.h"			// for gFPS
#include "pipeline.h"
#include "llglheaders.h"

// Local constants...
const S32 VERTICAL_OFFSET = 50;

F32		LLManip::sHelpTextVisibleTime = 2.f;
F32		LLManip::sHelpTextFadeTime = 2.f;
S32		LLManip::sNumTimesHelpTextShown = 0;
S32		LLManip::sMaxTimesShowHelpText = 5;
F32		LLManip::sGridMaxSubdivisionLevel = 32.f;
F32		LLManip::sGridMinSubdivisionLevel = 1.f;
LLVector2 LLManip::sTickLabelSpacing(60.f, 25.f);


//static
void LLManip::rebuild(LLViewerObject* vobj)
{
	LLDrawable* drawablep = vobj->mDrawable;
	if (drawablep && drawablep->getVOVolume())
	{
		
		gPipeline.markRebuild(drawablep,LLDrawable::REBUILD_VOLUME, TRUE);
		//gPipeline.markMoved(drawablep, FALSE);
		//gPipeline.updateMoveNormalAsync(vobj->mDrawable);

		drawablep->setState(LLDrawable::MOVE_UNDAMPED); // force to UNDAMPED
		drawablep->updateMove();
	}
}

//////////////////////////////////////////////////////////////////////////////
// LLManip


LLManip::LLManip( const LLString& name, LLToolComposite* composite )
	:
	LLTool( name, composite ),
	mInSnapRegime(FALSE)
{}


void LLManip::getManipNormal(LLViewerObject* object, EManipPart manip, LLVector3 &normal)
{
	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;

	gSelectMgr->getGrid(grid_origin, grid_rotation, grid_scale);

	if (manip >= LL_X_ARROW && manip <= LL_Z_ARROW)
	{
		LLVector3 arrow_axis;
		getManipAxis(object, manip, arrow_axis);

		LLVector3 cross = arrow_axis % gCamera->getAtAxis();
		normal = cross % arrow_axis;
		normal.normVec();
	}
	else if (manip >= LL_YZ_PLANE && manip <= LL_XY_PLANE)
	{
		switch (manip)
		{
		case LL_YZ_PLANE:
			normal = LLVector3::x_axis;
			break;
		case LL_XZ_PLANE:
			normal = LLVector3::y_axis;
			break;
		case LL_XY_PLANE:
			normal = LLVector3::z_axis;
			break;
		default:
			break;
		}
		normal.rotVec(grid_rotation);
	}
	else
	{
		normal.clearVec();
	}
}


BOOL LLManip::getManipAxis(LLViewerObject* object, EManipPart manip, LLVector3 &axis)
{
	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;

	gSelectMgr->getGrid(grid_origin, grid_rotation, grid_scale);

	if (manip == LL_X_ARROW)
	{
		axis = LLVector3::x_axis;
	}
	else if (manip == LL_Y_ARROW)
	{
		axis = LLVector3::y_axis;
	}
	else if (manip == LL_Z_ARROW)
	{
		axis = LLVector3::z_axis;
	}
	else
	{
		return FALSE;
	}

	axis.rotVec( grid_rotation );
	return TRUE;
}

F32 LLManip::getSubdivisionLevel(const LLVector3 &reference_point, const LLVector3 &translate_axis, F32 grid_scale, S32 min_pixel_spacing)
{
	//update current snap subdivision level
	LLVector3 cam_to_reference;
	if (gSelectMgr->getSelectType() == SELECT_TYPE_HUD)
	{
		cam_to_reference = LLVector3(1.f / gAgent.getAvatarObject()->mHUDCurZoom, 0.f, 0.f);
	}
	else
	{
		cam_to_reference = reference_point - gCamera->getOrigin();
	}
	F32 current_range = cam_to_reference.normVec();

	F32 projected_translation_axis_length = (translate_axis % cam_to_reference).magVec();
	F32 subdivisions = llmax(projected_translation_axis_length * grid_scale / (current_range / gCamera->getPixelMeterRatio() * min_pixel_spacing), 0.f);
	subdivisions = llclamp((F32)pow(2.f, llfloor(log(subdivisions) / log(2.f))), 1.f / 32.f, 32.f);

	return subdivisions;
}

BOOL LLManip::handleHover(S32 x, S32 y, MASK mask)
{
	// We only handle the event if mousedown started with us
	if( hasMouseCapture() )
	{
		if( gSelectMgr->isEmpty() )
		{
			// Somehow the object got deselected while we were dragging it.
			// Release the mouse
			setMouseCapture( FALSE );
		}

		lldebugst(LLERR_USER_INPUT) << "hover handled by LLManip (active)" << llendl;
	}
	else
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLManip (inactive)" << llendl;
	}
	gViewerWindow->setCursor(UI_CURSOR_ARROW);
	return TRUE;
}


BOOL LLManip::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;
	if( hasMouseCapture() )
	{
		handled = TRUE;
		setMouseCapture( FALSE );
	}
	return handled;
}

void LLManip::updateGridSettings()
{
	sGridMaxSubdivisionLevel = gSavedSettings.getBOOL("GridSubUnit") ? (F32)gSavedSettings.getS32("GridSubdivision") : 1.f;
}

BOOL LLManip::getMousePointOnPlaneAgent(LLVector3& point, S32 x, S32 y, LLVector3 origin, LLVector3 normal)
{
	LLVector3d origin_double = gAgent.getPosGlobalFromAgent(origin);
	LLVector3d global_point;
	BOOL result = getMousePointOnPlaneGlobal(global_point, x, y, origin_double, normal);
	point = gAgent.getPosAgentFromGlobal(global_point);
	return result;
}

BOOL LLManip::getMousePointOnPlaneGlobal(LLVector3d& point, S32 x, S32 y, LLVector3d origin, LLVector3 normal)
{
	if (gSelectMgr->getSelectType() == SELECT_TYPE_HUD)
	{
		BOOL result = FALSE;
		F32 mouse_x = ((F32)x / gViewerWindow->getWindowWidth() - 0.5f) * gCamera->getAspect() / gAgent.getAvatarObject()->mHUDCurZoom;
		F32 mouse_y = ((F32)y / gViewerWindow->getWindowHeight() - 0.5f) / gAgent.getAvatarObject()->mHUDCurZoom;

		LLVector3 origin_agent = gAgent.getPosAgentFromGlobal(origin);
		LLVector3 mouse_pos = LLVector3(0.f, -mouse_x, mouse_y);
		if (llabs(normal.mV[VX]) < 0.001f)
		{
			// use largish value that should be outside HUD manipulation range
			mouse_pos.mV[VX] = 10.f;
		}
		else
		{
			mouse_pos.mV[VX] = (normal * (origin_agent - mouse_pos))
								/ (normal.mV[VX]);
			result = TRUE;
		}

		point = gAgent.getPosGlobalFromAgent(mouse_pos);
		return result;
	}
	else
	{
		return gViewerWindow->mousePointOnPlaneGlobal(
										point, x, y, origin, normal );
	}

	//return FALSE;
}

// Given the line defined by mouse cursor (a1 + a_param*(a2-a1)) and the line defined by b1 + b_param*(b2-b1),
// returns a_param and b_param for the points where lines are closest to each other.
// Returns false if the two lines are parallel.
BOOL LLManip::nearestPointOnLineFromMouse( S32 x, S32 y, const LLVector3& b1, const LLVector3& b2, F32 &a_param, F32 &b_param ) const
{
	LLVector3 a1;
	LLVector3 a2;

	if (gSelectMgr->getSelectType() == SELECT_TYPE_HUD)
	{
		F32 mouse_x = (((F32)x / gViewerWindow->getWindowWidth()) - 0.5f) * gCamera->getAspect() / gAgent.getAvatarObject()->mHUDCurZoom;
		F32 mouse_y = (((F32)y / gViewerWindow->getWindowHeight()) - 0.5f) / gAgent.getAvatarObject()->mHUDCurZoom;
		a1 = LLVector3(llmin(b1.mV[VX] - 0.1f, b2.mV[VX] - 0.1f, 0.f), -mouse_x, mouse_y);
		a2 = a1 + LLVector3(1.f, 0.f, 0.f);
	}
	else
	{
		a1 = gAgent.getCameraPositionAgent();
		a2 = gAgent.getCameraPositionAgent() + LLVector3(gViewerWindow->mouseDirectionGlobal(x, y));
	}

	BOOL parallel = TRUE;
	LLVector3 a = a2 - a1;
	LLVector3 b = b2 - b1;

	LLVector3 normal;
	F32 dist, denom;
	normal = (b % a) % b;	// normal to plane (P) through b and (shortest line between a and b)
	normal.normVec();
	dist = b1 * normal;			// distance from origin to P

	denom = normal * a; 
	if( (denom < -F_APPROXIMATELY_ZERO) || (F_APPROXIMATELY_ZERO < denom) )
	{
		a_param = (dist - normal * a1) / denom;
		parallel = FALSE;
	}

	normal = (a % b) % a;	// normal to plane (P) through a and (shortest line between a and b)
	normal.normVec();
	dist = a1 * normal;			// distance from origin to P
	denom = normal * b; 
	if( (denom < -F_APPROXIMATELY_ZERO) || (F_APPROXIMATELY_ZERO < denom) )
	{
		b_param = (dist - normal * b1) / denom;
		parallel = FALSE;
	}

	return parallel;
}

LLVector3 LLManip::getSavedPivotPoint() const
{
	return gSelectMgr->getSavedBBoxOfSelection().getCenterAgent();
}

LLVector3 LLManip::getPivotPoint() const
{
	if (gSelectMgr->getFirstObject() && gSelectMgr->getObjectCount() == 1 && gSelectMgr->getSelectType() != SELECT_TYPE_HUD)
	{
		return gSelectMgr->getFirstObject()->getPivotPositionAgent();
	}
	return gSelectMgr->getBBoxOfSelection().getCenterAgent();
}


void LLManip::renderGuidelines(BOOL draw_x, BOOL draw_y, BOOL draw_z)
{
	LLVector3 grid_origin;
	LLQuaternion grid_rot;
	LLVector3 grid_scale;
	gSelectMgr->getGrid(grid_origin, grid_rot, grid_scale);

	LLViewerObject* object = gSelectMgr->getFirstRootObject();
	if (!object)
	{
		object = gSelectMgr->getFirstObject();
		if (!object)
		{
			return;
		}
	}

	//LLVector3  center_agent  = gSelectMgr->getBBoxOfSelection().getCenterAgent();
	LLVector3  center_agent  = getPivotPoint();

	glPushMatrix();
	{
		glTranslatef(center_agent.mV[VX], center_agent.mV[VY], center_agent.mV[VZ]);

		F32 angle_radians, x, y, z;

		grid_rot.getAngleAxis(&angle_radians, &x, &y, &z);
		glRotatef(angle_radians * RAD_TO_DEG, x, y, z);

		F32 region_size = gWorldPointer->getRegionWidthInMeters();

		const F32 LINE_ALPHA = 0.33f;

		LLGLSNoTexture gls_no_texture;
		LLUI::setLineWidth(1.5f);

		if (draw_x)
		{
			glColor4f(1.f, 0.f, 0.f, LINE_ALPHA);
			glBegin(GL_LINES);
			glVertex3f( -region_size, 0.f, 0.f );
			glVertex3f(  region_size, 0.f, 0.f );
			glEnd();
		}

		if (draw_y)
		{
			glColor4f(0.f, 1.f, 0.f, LINE_ALPHA);
			glBegin(GL_LINES);
			glVertex3f( 0.f, -region_size, 0.f );
			glVertex3f( 0.f,  region_size, 0.f );
			glEnd();
		}

		if (draw_z)
		{
			glColor4f(0.f, 0.f, 1.f, LINE_ALPHA);
			glBegin(GL_LINES);
			glVertex3f( 0.f, 0.f, -region_size );
			glVertex3f( 0.f, 0.f,  region_size );
			glEnd();
		}
		LLUI::setLineWidth(1.0f);
	}
	glPopMatrix();
}

void LLManip::renderXYZ(const LLVector3 &vec) 
{
	const S32 PAD = 10;
	char feedback_string[128];
	LLVector3 camera_pos = gCamera->getOrigin() + gCamera->getAtAxis();
	S32 vertical_offset = gViewerWindow->getWindowHeight() / 2 - VERTICAL_OFFSET;
	S32 window_center_x = gViewerWindow->getWindowWidth() / 2;
	S32 window_center_y = gViewerWindow->getWindowHeight() / 2;

	LLUUID image_id;
	image_id.set(gViewerArt.getString("rounded_square.tga"));
	LLViewerImage* imagep = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	glPushMatrix();
	{
		gViewerWindow->setup2DRender();
		const LLVector2& display_scale = gViewerWindow->getDisplayScale();
		glScalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);
		glColor4f(0.f, 0.f, 0.f, 0.7f);

		gl_draw_scaled_image_with_border(window_center_x - 115, 
			window_center_y + vertical_offset - PAD, 
			16, 
			16, 
			235,
			PAD * 2 + 10, 
			imagep, 
			LLColor4(0.f, 0.f, 0.f, 0.7f) );
	}
	glPopMatrix();

	gViewerWindow->setup3DRender();

	{
		LLLocale locale(LLLocale::USER_LOCALE);
		LLGLDepthTest gls_depth(GL_FALSE);
		LLGLEnable tex(GL_TEXTURE_2D);
		// render drop shadowed text
		sprintf(feedback_string, "X: %.3f", vec.mV[VX]);
		hud_render_text(utf8str_to_wstring(feedback_string), camera_pos, *gResMgr->getRes( LLFONT_SANSSERIF ), LLFontGL::NORMAL, -102.f + 1.f, (F32)vertical_offset - 1.f, LLColor4::black, FALSE);

		sprintf(feedback_string, "Y: %.3f", vec.mV[VY]);
		hud_render_text(utf8str_to_wstring(feedback_string), camera_pos, *gResMgr->getRes( LLFONT_SANSSERIF ), LLFontGL::NORMAL, -27.f + 1.f, (F32)vertical_offset - 1.f, LLColor4::black, FALSE);
		
		sprintf(feedback_string, "Z: %.3f", vec.mV[VZ]);
		hud_render_text(utf8str_to_wstring(feedback_string), camera_pos, *gResMgr->getRes( LLFONT_SANSSERIF ), LLFontGL::NORMAL, 48.f + 1.f, (F32)vertical_offset - 1.f, LLColor4::black, FALSE);

		// render text on top
		sprintf(feedback_string, "X: %.3f", vec.mV[VX]);
		hud_render_text(utf8str_to_wstring(feedback_string), camera_pos, *gResMgr->getRes( LLFONT_SANSSERIF ), LLFontGL::NORMAL, -102.f, (F32)vertical_offset, LLColor4(1.f, 0.5f, 0.5f, 1.f), FALSE);

		glColor3f(0.5f, 1.f, 0.5f);
		sprintf(feedback_string, "Y: %.3f", vec.mV[VY]);
		hud_render_text(utf8str_to_wstring(feedback_string), camera_pos, *gResMgr->getRes( LLFONT_SANSSERIF ), LLFontGL::NORMAL, -27.f, (F32)vertical_offset, LLColor4(0.5f, 1.f, 0.5f, 1.f), FALSE);
		
		glColor3f(0.5f, 0.5f, 1.f);
		sprintf(feedback_string, "Z: %.3f", vec.mV[VZ]);
		hud_render_text(utf8str_to_wstring(feedback_string), camera_pos, *gResMgr->getRes( LLFONT_SANSSERIF ), LLFontGL::NORMAL, 48.f, (F32)vertical_offset, LLColor4(0.5f, 0.5f, 1.f, 1.f), FALSE);
	}
}

void LLManip::renderTickText(const LLVector3& pos, const char* text, const LLColor4 &color)
{
	const LLFontGL* big_fontp = gResMgr->getRes( LLFONT_SANSSERIF );

	BOOL hud_selection = gSelectMgr->getSelectType() == SELECT_TYPE_HUD;
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	LLVector3 render_pos = pos;
	if (hud_selection)
	{
		F32 zoom_amt = gAgent.getAvatarObject()->mHUDCurZoom;
		F32 inv_zoom_amt = 1.f / zoom_amt;
		// scale text back up to counter-act zoom level
		render_pos = pos * zoom_amt;
		glScalef(inv_zoom_amt, inv_zoom_amt, inv_zoom_amt);
	}

	// render shadow first
	LLColor4 shadow_color = LLColor4::black;
	LLGLEnable tex(GL_TEXTURE_2D);
	shadow_color.mV[VALPHA] = color.mV[VALPHA] * 0.5f;
	gViewerWindow->setupViewport(1, -1);
	hud_render_utf8text(text, render_pos, *big_fontp, LLFontGL::NORMAL, -0.5f * big_fontp->getWidthF32(text), 3.f, shadow_color, gSelectMgr->getSelectType() == SELECT_TYPE_HUD);
	gViewerWindow->setupViewport();
	hud_render_utf8text(text, render_pos, *big_fontp, LLFontGL::NORMAL, -0.5f * big_fontp->getWidthF32(text), 3.f, color, gSelectMgr->getSelectType() == SELECT_TYPE_HUD);

	glPopMatrix();
}

void LLManip::renderTickValue(const LLVector3& pos, F32 value, const char* suffix, const LLColor4 &color)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	const LLFontGL* big_fontp = gResMgr->getRes( LLFONT_SANSSERIF );
	const LLFontGL* small_fontp = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );

	char val_string[128];
	char fraction_string[128];
	F32 val_to_print = llround(value, 0.001f);
	S32 fractional_portion = llround(fmodf(llabs(val_to_print), 1.f) * 100.f);
	if (val_to_print < 0.f)
	{
		if (fractional_portion == 0)
		{
			sprintf(val_string, "-%d%s", lltrunc(llabs(val_to_print)), suffix);
		}
		else
		{
			sprintf(val_string, "-%d", lltrunc(llabs(val_to_print)));
		}
	}
	else
	{
		if (fractional_portion == 0)
		{
			sprintf(val_string, "%d%s", lltrunc(llabs(val_to_print)), suffix);
		}
		else
		{
			sprintf(val_string, "%d", lltrunc(val_to_print));
		}
	}

	BOOL hud_selection = gSelectMgr->getSelectType() == SELECT_TYPE_HUD;
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	LLVector3 render_pos = pos;
	if (hud_selection)
	{
		F32 zoom_amt = gAgent.getAvatarObject()->mHUDCurZoom;
		F32 inv_zoom_amt = 1.f / zoom_amt;
		// scale text back up to counter-act zoom level
		render_pos = pos * zoom_amt;
		glScalef(inv_zoom_amt, inv_zoom_amt, inv_zoom_amt);
	}

	LLColor4 shadow_color = LLColor4::black;
	shadow_color.mV[VALPHA] = color.mV[VALPHA] * 0.5f;

	LLGLEnable tex(GL_TEXTURE_2D);
	if (fractional_portion != 0)
	{
		sprintf(fraction_string, "%c%d%s", gResMgr->getDecimalPoint(), fractional_portion, suffix);

		gViewerWindow->setupViewport(1, -1);
		hud_render_utf8text(val_string, render_pos, *big_fontp, LLFontGL::NORMAL, -1.f * big_fontp->getWidthF32(val_string), 3.f, shadow_color, hud_selection);
		hud_render_utf8text(fraction_string, render_pos, *small_fontp, LLFontGL::NORMAL, 1.f, 3.f, shadow_color, hud_selection);

		gViewerWindow->setupViewport();
		hud_render_utf8text(val_string, render_pos, *big_fontp, LLFontGL::NORMAL, -1.f * big_fontp->getWidthF32(val_string), 3.f, color, hud_selection);
		hud_render_utf8text(fraction_string, render_pos, *small_fontp, LLFontGL::NORMAL, 1.f, 3.f, color, hud_selection);
	}
	else
	{
		gViewerWindow->setupViewport(1, -1);
		hud_render_utf8text(val_string, render_pos, *big_fontp, LLFontGL::NORMAL, -0.5f * big_fontp->getWidthF32(val_string), 3.f, shadow_color, hud_selection);
		gViewerWindow->setupViewport();
		hud_render_utf8text(val_string, render_pos, *big_fontp, LLFontGL::NORMAL, -0.5f * big_fontp->getWidthF32(val_string), 3.f, color, hud_selection);
	}
	glPopMatrix();
}

LLColor4 LLManip::setupSnapGuideRenderPass(S32 pass)
{
	static LLColor4 grid_color_fg = gColors.getColor("GridlineColor");
	static LLColor4 grid_color_bg = gColors.getColor("GridlineBGColor");
	static LLColor4 grid_color_shadow = gColors.getColor("GridlineShadowColor");

	LLColor4 line_color;
	F32 line_alpha = gSavedSettings.getF32("GridOpacity");

	switch(pass)
	{
	case 0:
		// shadow
		gViewerWindow->setupViewport(1, -1);
		line_color = grid_color_shadow;
		line_color.mV[VALPHA] *= line_alpha;
		LLUI::setLineWidth(2.f);
		break;
	case 1:
		// hidden lines
		gViewerWindow->setupViewport();
		line_color = grid_color_bg;
		line_color.mV[VALPHA] *= line_alpha;
		LLUI::setLineWidth(1.f);
		break;
	case 2:
		// visible lines
		line_color = grid_color_fg;
		line_color.mV[VALPHA] *= line_alpha;
		break;
	}

	return line_color;
}