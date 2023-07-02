//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_framebuffer.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"
#include <intrin.h>
#include <float.h>

CTCALL	PCTFB	CTFrameBufferCreate(UINT32 width, UINT32 height) {
	if (width == 0 || height == 0) {
		CTErrorSetParamValue("CTFrameBufferCreate failed: width/height was invalid");
		return NULL;
	}

	PCTFrameBuffer rfb = CTGFXAlloc(sizeof(*rfb));

	rfb->width	= width;
	rfb->height	= height;
	rfb->color	= CTGFXAlloc(sizeof(*rfb->color) * width * height);
	rfb->depth	= CTGFXAlloc(sizeof(*rfb->depth) * width * height);
	rfb->lock	= CTLockCreate();

	CTFrameBufferClear(rfb, TRUE, TRUE);

	return rfb;
}

CTCALL	BOOL	CTFrameBufferDestroy(PCTFrameBuffer fb) {
	if (fb == NULL) {
		CTErrorSetBadObject("CTFrameBufferDestroy failed: fb was NULL");
		return FALSE;
	}

	CTLockEnter(fb->lock);
	CTGFXFree(fb->color);
	CTGFXFree(fb->depth);
	CTLockDestroy(fb->lock);
	CTGFXFree(fb);

	return TRUE;
}

CTCALL	BOOL	CTFrameBufferSet(PCTFrameBuffer fb, CTPoint pt, CTColor col, FLOAT depth) {
	if (fb == NULL) {
		CTErrorSetBadObject("CTFrameBufferSet failed: fb was NULL");
		return FALSE;
	}
	if (pt.x > fb->width - 1 || pt.y > fb->height - 1) {
		CTErrorSetParamValue("CTFrameBufferSet failed: pt was out of bounds");
		return FALSE;
	}

	CTLockEnter(fb->lock);

	UINT32 index		= pt.x + (pt.y * fb->width);
	fb->color[index]	= col;
	fb->depth[index]	= depth;

	CTLockLeave(fb->lock);

	return TRUE;
}

CTCALL	BOOL	CTFrameBufferDepthTest(PCTFrameBuffer fb, CTPoint pt, FLOAT depth) {
	if (fb == NULL) {
		CTErrorSetBadObject("CTFrameBufferDepthTest failed: fb was NULL");
		return FALSE;
	}
	if (pt.x > fb->width - 1 || pt.y > fb->height - 1) {
		CTErrorSetParamValue("CTFrameBufferDepthTest failed: pt was out of bounds");
		return FALSE;
	}

	UINT32 index = pt.x + (pt.y * fb->width);

	CTLockEnter(fb->lock);
	BOOL depthTest = fb->depth[index] > depth;
	CTLockLeave(fb->lock);

	return depthTest;
}

CTCALL	BOOL	CTFrameBufferGet(PCTFrameBuffer fb, CTPoint pt, PCTColor pCol, PFLOAT pDepth) {
	if (fb == NULL) {
		CTErrorSetBadObject("CTFrameBufferGet failed: fb was NULL");
		return FALSE;
	}
	if (pt.x > fb->width - 1 || pt.y > fb->height - 1) {
		CTErrorSetParamValue("CTFrameBufferGet failed: pt was out of bounds");
		return FALSE;
	}

	CTLockEnter(fb->lock);

	UINT32 index = pt.x + (pt.y * fb->width);

	if (pCol != NULL)
		*pCol = fb->color[index];
	if (pDepth != NULL)
		*pDepth = fb->depth[index];

	CTLockLeave(fb->lock);

	return TRUE;
}

CTCALL	BOOL	CTFrameBufferLock(PCTFrameBuffer fb) {
	if (fb == NULL) {
		CTErrorSetBadObject("CTFrameBufferLock failed: fb was NULL");
		return FALSE;
	}
	CTLockEnter(fb->lock);
	return TRUE;
}

CTCALL	BOOL	CTFrameBufferUnlock(PCTFrameBuffer fb) {
	if (fb == NULL) {
		CTErrorSetBadObject("CTFrameBufferUnlock failed: fb was NULL");
		return FALSE;
	}
	CTLockLeave(fb->lock);
	return TRUE;
}

CTCALL	BOOL	CTFrameBufferClear(PCTFrameBuffer fb, BOOL color, BOOL depth) {
	if (fb == NULL) {
		CTErrorSetBadObject("CTFrameBufferClear failed: fb was NULL");
		return FALSE;
	}

	CTLockEnter(fb->lock);

	const FLOAT		CLEAR_DEPTH_VALUE	= FLT_MAX;
	const UINT32	FB_ELEMENT_COUNT	= fb->width * fb->height;
	if (color == TRUE)
		__stosd(fb->color, 0, FB_ELEMENT_COUNT);
	if (depth == TRUE)
		__stosd(fb->depth, *(PDWORD)&CLEAR_DEPTH_VALUE, FB_ELEMENT_COUNT);

	CTLockLeave(fb->lock);

	return TRUE;
}
