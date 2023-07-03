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
	/// update point position so that (0, 0) is centered in screen
	/// 
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
	/// if (pixelShader has changed screencoord)
	///		if (point is out of bounds again)
	///			return
	///		if (depth test failed again)
	///			return
	/// 
	/// get below color
	/// generate blended color
	/// set frameBuffer pixel to blended color
	
	screenCoord.x += (drawInfo->frameBuffer->width)  >> 1;
	screenCoord.y += (drawInfo->frameBuffer->height) >> 1;

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

	if (screenCoord.x != pixel.screenCoord.x || screenCoord.y != pixel.screenCoord.y) {

		if (pixel.screenCoord.x >= drawInfo->frameBuffer->width ||
			pixel.screenCoord.y >= drawInfo->frameBuffer->height ||
			pixel.screenCoord.x < 0 ||
			pixel.screenCoord.y < 0) return;

		if (CTFrameBufferDepthTest(drawInfo->frameBuffer, screenCoord, drawInfo->depth) == FALSE &&
			drawInfo->shader->depthTest == TRUE) return;

	}

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
	/// loop (all prims in prim list)
	///		drawPixel
	
	for (UINT32 pixID = 0; pixID < primCount; pixID++) {

		const CTPoint pixPt		= CTPointFromVector(primList[pixID].vertex);

		__HCTDrawPoint(
			drawInfo,
			pixID,
			pixPt,
			primList[pixID].UV,
			drawInfo->shader->pointSizePixels
		);
	}

}

static void __HCTDrawLine(PCTPrimitive prim1, PCTPrimitive prim2, P__CTDrawInfo drawInfo) {
	
	FLOAT dx = prim2->vertex.x - prim1->vertex.x;
	FLOAT dy = prim2->vertex.y - prim1->vertex.y;
	FLOAT dt = CTVectMagnitudeFast(CTVectCreate(dx, dy));

	dx /= dt;
	dy /= dt;

	FLOAT drawX = prim1->vertex.x;
	FLOAT drawY = prim1->vertex.y;
	for (UINT32 pixID = 0; pixID < dt; pixID++) {

		FLOAT uvFactor	= (FLOAT)pixID / dt;
		FLOAT uvx		= prim1->UV.x * (1.0f - uvFactor) + prim2->UV.x * (uvFactor);
		FLOAT uvy		= prim1->UV.y * (1.0f - uvFactor) + prim2->UV.y * (uvFactor);

		__HCTDrawPoint(
			drawInfo,
			pixID,
			CTPointCreate(
				drawX,
				drawY
			),
			CTVectCreate(
				uvx,
				uvy
			),
			drawInfo->shader->lineSizePixels
		);

		drawX += dx;
		drawY += dy;
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

	case CT_DRAW_METHOD_LINES_CLOSED:
		__HCTDrawLine(
			processedPrimList + 0,
			processedPrimList + mesh->primCount - 1,
			&drawInfo
		);

	case CT_DRAW_METHOD_LINES_OPEN:
		for (UINT32 primIndex = 0; primIndex < mesh->primCount - 1; primIndex++) {
			__HCTDrawLine(
				processedPrimList + primIndex + 0,
				processedPrimList + primIndex + 1,
				&drawInfo
			);
		}
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
