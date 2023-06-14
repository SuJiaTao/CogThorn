//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_draw.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"

typedef struct __CTDrawData {
	PCTFB			frameBuffer;
	PCTPrimitive	primList;
	UINT32			primCount;
	PCTShader		shader;
	PVOID			shaderInput;
} __CTDrawData, *P__CTDrawData;

static void		__HCTSetPixel(P__CTDrawData drawData, CTPoint screenPt) {

	if (screenPt.x < 0 ||
		screenPt.y < 0 ||
		screenPt.x >= drawData->frameBuffer->width ||
		screenPt.y >= drawData->frameBuffer->height)
		return;

	CTPixel drawPix = {
		.color = 
	};

}

static void		__HCTDrawPoints(P__CTDrawData drawData) {

	/// SUMMARY:
	/// loop (all primitives)
	///		convert vertex position to screen coordinate
	///		early cull if out of bounds
	///		do early depth test
	///		setup pixel ctx
	///		if (pixel shader exists)
	///			call pixel shader
	///		set framebuffer pixel
	
	for (UINT32 pixID = 0; pixID < drawData->primCount; pixID++) {

		PCTPrimitive pPrim	= drawData->primList + pixID;
		CTPoint	screenPt	= CTPointFromVector(pPrim->vertex);
		__HCTSetPixel(drawData, screenPt);

	}

}

CTCALL	BOOL		CTDraw(
	UINT32		drawMethod,
	PCTFB		frameBuffer,
	PCTMesh		mesh,
	PCTShader	shader,
	PVOID		shaderInput
) {

	if (frameBuffer == NULL) {
		CTErrorSetBadObject("CTDraw failed: frameBuffer was NULL");
		return FALSE;
	}
	if (mesh == NULL) {
		CTErrorSetBadObject("CTDraw failed: mesh was NULL");
		return FALSE;
	}
	if (shader == NULL) {
		CTErrorSetBadObject("CTDraw failed: shader was NULL");
		return FALSE;
	}
	if (shader->shaderInputSizeBytes != 0 && shaderInput == NULL) {
		CTErrorSetBadObject("CTDraw failed: shaderInput was NULL when shader requires input");
		return FALSE;
	}

	/// SUMMARY:
	/// 
	/// copy shader input
	/// make copy of primitives
	/// process primitives using primitive shader
	/// 
	/// switch (drawMethod):
	///		DRAW_POINTS:
	///		draw each point
	/// 
	///		DRAW_LINES:
	///		draw line strip from each primitive to the next in a closed shape
	///	
	///		DRAW_FILL:
	///		triangulate and fill polygon
	/// 
	///		UNKNOWN:
	///		raise error, return FALSE
	/// 
	/// return TRUE

	PVOID shaderInputCopy = CTGFXAlloc(shader->shaderInputSizeBytes);
	__movsb(shaderInputCopy, shaderInput, shader->shaderInputSizeBytes);

	PCTPrimitive primivites = CTGFXAlloc(sizeof(CTPrimitive) * mesh->primCount);
	__movsb(primivites, mesh->primList, sizeof(CTPrimitive) * mesh->primCount);

	for (UINT32 primID = 0; primID < mesh->primCount; primID++) {

		CTPrimCtx ctx = {
			.primID = primID,
			.mesh	= mesh
		};

		if (shader->primitiveShader) {
			shader->primitiveShader(
				ctx,
				primivites + primID,
				shaderInputCopy
			);
		}

	}

	switch (drawMethod)
	{
	case CT_DRAW_METHOD_POINTS:
		break;

	case CT_DRAW_METHOD_LINES:
		break;

	case CT_DRAW_METHOD_FILL:
		break;

	default:
		CTErrorSetParamValue("CTDraw failed: invalid draw method");
		return FALSE;
	}

	return TRUE;
}
