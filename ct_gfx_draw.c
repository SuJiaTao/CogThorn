//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_draw.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"

typedef struct __CTDrawInfo {
	PCTFB		frameBuffer;
	PCTShader	shader;
	PVOID		shaderInput;
	FLOAT		depth;
} __CTDrawInfo, *P__CTDrawInfo;


static void __HCTProcessAndDrawPixel(
	P__CTDrawInfo	drawInfo, 
	UINT32			pixID, 
	CTPoint			screenCoord, 
	CTVect			UV
) {

	/// SUMMARY:
	/// if (point is out of bounds)
	///		return
	/// 
	/// if (depth test failed)
	///		return
	/// 
	/// setup pixelCtx
	/// setup pixel
	/// 
	/// if (pixelShader != NULL)
	///		process pixel with shader
	///		if (should discard pixel)
	///			return
	/// 
	/// if (point is out of bounds again)
	///		return
	/// 
	/// get below color
	/// generate blended color
	/// set frameBuffer pixel to blended color
	
	if (screenCoord.x >= drawInfo->frameBuffer->width	||
		screenCoord.y >= drawInfo->frameBuffer->height	||
		screenCoord.x < 0 ||
		screenCoord.y < 0) return;

	if (CTFrameBufferDepthTest(drawInfo->frameBuffer, screenCoord, drawInfo->depth) == FALSE &&
		drawInfo->shader->depthTest == TRUE) return;

	CTPixCtx pixCtx = {
		.frameBuffer	= drawInfo->frameBuffer,
		.pixID			= pixID,
		.UV				= UV
	};

	CTPixel pixel = {
		.screenCoord	= screenCoord,
		.color			= CTColorCreate(0, 0, 0, 0)
	};
	
	CTColor belowColor;
	CTFrameBufferGet(
		drawInfo->frameBuffer, 
		screenCoord, 
		&belowColor, 
		NULL
	);

	if (drawInfo->shader->pixelShader != NULL) {

		BOOL keepPixel = drawInfo->shader->pixelShader(
			pixCtx,
			&pixel,
			drawInfo->shaderInput
		);

		if (keepPixel == FALSE)
			return;

	}

	if (pixel.screenCoord.x >= drawInfo->frameBuffer->width ||
		pixel.screenCoord.y >= drawInfo->frameBuffer->height ||
		pixel.screenCoord.x < 0 ||
		pixel.screenCoord.y < 0) return;

	CTColor newColor = CTColorBlend(belowColor, pixel.color);
	CTFrameBufferSet(
		drawInfo->frameBuffer,
		pixel.screenCoord,
		newColor,
		drawInfo->depth
	);

}

static void __HCTDrawPoint(
	P__CTDrawInfo	drawInfo,
	UINT32			pixID,
	CTPoint			screenCoord,
	CTVect			UV,
	UINT32			ptSize) {
	
	/// SUMMARY:
	/// incrementally adds pixels based on size
	/// shape of point looks like this:
	///   4 4 4 
	/// 4 3 2 3 4
	/// 4 2 1 2 4
	/// 4 3 2 3 4
	///   4 4 4
	
	switch (ptSize)
	{
	case 4:

		// draw outer 9 pixels
		for (int i = -1; i <= 1; i++) {

			__HCTProcessAndDrawPixel(
				drawInfo,
				pixID,
				CTPointAdd(
					screenCoord,
					CTPointCreate(i, 2)
				),
				UV
			);

			__HCTProcessAndDrawPixel(
				drawInfo,
				pixID,
				CTPointAdd(
					screenCoord,
					CTPointCreate(i, -2)
				),
				UV
			);

			__HCTProcessAndDrawPixel(
				drawInfo,
				pixID,
				CTPointAdd(
					screenCoord,
					CTPointCreate(2, i)
				),
				UV
			);

			__HCTProcessAndDrawPixel(
				drawInfo,
				pixID,
				CTPointAdd(
					screenCoord,
					CTPointCreate(-2, i)
				),
				UV
			);

		}
		

	case 3:

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			CTPointAdd(
				screenCoord,
				CTPointCreate(-1, -1)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			CTPointAdd(
				screenCoord,
				CTPointCreate(-1,  1)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			CTPointAdd(
				screenCoord,
				CTPointCreate( 1, 1)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			CTPointAdd(
				screenCoord,
				CTPointCreate(1, -1)
			),
			UV
		);

	case 2:

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			CTPointAdd(
				screenCoord,
				CTPointCreate(-1, 0)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			CTPointAdd(
				screenCoord,
				CTPointCreate(0, 1)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			CTPointAdd(
				screenCoord,
				CTPointCreate(1, 0)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			CTPointAdd(
				screenCoord,
				CTPointCreate(0, -1)
			),
			UV
		);

	case 1:

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID,
			screenCoord,
			UV
		);

	default:
		break;
	}

}

static void __HCTDrawPoints(PCTPrimitive primList, UINT32 primCount, P__CTDrawInfo drawInfo) {

	/// SUMMARY:
	/// initialize pixID
	/// loop (all prims in prim list)
	///		calculate ptPos
	///		loop (ptPos.x - ptSize / 2, ptPos.x + ptSize / 2)
	///			loop (ptPos.y - ptSize / 2, ptPos.y + ptSize / 2)
	///				drawPixel
	///				increment pixID
	
	UINT32 pixID = 0;

	for (UINT32 pixIndex = 0; pixIndex < primCount; pixIndex++) {

		const INT halfPtSize	= drawInfo->shader->pointSizePixels / 2;
		const CTPoint pixPt		= CTPointFromVector(primList[pixIndex].vertex);

		__HCTDrawPoint(
			drawInfo,
			pixID,
			pixPt,
			primList[pixIndex].UV,
			drawInfo->shader->pointSizePixels
		);

		pixID++;

	}

}

CTCALL	BOOL		CTDraw(
	UINT32		drawMethod,
	PCTFB		frameBuffer,
	PCTMesh		mesh,
	PCTShader	shader,
	PVOID		shaderInput,
	FLOAT		depth
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
	/// create copy of shader input
	/// create copy of mesh primitives
	/// 
	/// loop(all primitives in copy)
	///		if (primitive shader != NULL)
	///			process primitive with shader
	/// 
	/// setup drawInfo object
	/// 
	/// switch (drawMethod);
	///		POINTS:
	///		draw points
	///		LINES_OPEN:
	///		line array of all verts in mesh
	///		LINES_CLOSED:
	///		closed polygon outline of mesh
	///		FILLED:
	///		filled mesh
	///		UNKNOWN:
	///		raise error
	///		return FALSE
	/// 
	/// return TRUE

	PVOID shaderInputCopy = CTGFXAlloc(shader->shaderInputSizeBytes);
	__movsb(
		shaderInputCopy, 
		shaderInput, 
		shader->shaderInputSizeBytes
	);

	PCTPrimitive processedPrimList = CTGFXAlloc(sizeof(CTPrimitive) * mesh->primCount);
	__movsb(
		processedPrimList, 
		mesh->primList, 
		sizeof(CTPrimitive) * mesh->primCount
	);

	for (UINT32 primID = 0; primID < mesh->primCount; primID++) {

		CTPrimCtx primtCtx = {
			.primID = primID,
			.mesh	= mesh
		};

		if (shader->primitiveShader != NULL) {
			shader->primitiveShader(
				primtCtx,
				processedPrimList + primID,
				shaderInputCopy
			);
		}

	}

	__CTDrawInfo drawInfo = {
		.depth			= depth,
		.frameBuffer	= frameBuffer,
		.shader			= shader,
		.shaderInput	= shaderInputCopy
	};

	switch (drawMethod)
	{
	case CT_DRAW_METHOD_POINTS:
		__HCTDrawPoints(processedPrimList, mesh->primCount, &drawInfo);
		break;

	case CT_DRAW_METHOD_LINES_OPEN:
		break;

	case CT_DRAW_METHOD_LINES_CLOSED:
		break;

	case CT_DRAW_METHOD_FILL:
		break;

	default:
		CTErrorSetParamValue("CTDraw failed: invalid draw method");
		return FALSE;
	}

	CTGFXFree(shaderInputCopy);
	CTGFXFree(processedPrimList);

	return TRUE;
}
