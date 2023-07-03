//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_draw.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"

#include <stdio.h>

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
	/// if (pixelShader has changed screencoord)
	///		if (point is out of bounds again)
	///			return
	///		if (depth test failed again)
	///			return
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
				pixID++,
				CTPointAdd(
					screenCoord,
					CTPointCreate(i, 2)
				),
				UV
			);

			__HCTProcessAndDrawPixel(
				drawInfo,
				pixID++,
				CTPointAdd(
					screenCoord,
					CTPointCreate(i, -2)
				),
				UV
			);

			__HCTProcessAndDrawPixel(
				drawInfo,
				pixID++,
				CTPointAdd(
					screenCoord,
					CTPointCreate(2, i)
				),
				UV
			);

			__HCTProcessAndDrawPixel(
				drawInfo,
				pixID++,
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
			pixID++,
			CTPointAdd(
				screenCoord,
				CTPointCreate(-1, -1)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID++,
			CTPointAdd(
				screenCoord,
				CTPointCreate(-1,  1)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID++,
			CTPointAdd(
				screenCoord,
				CTPointCreate( 1, 1)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID++,
			CTPointAdd(
				screenCoord,
				CTPointCreate(1, -1)
			),
			UV
		);

	case 2:

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID++,
			CTPointAdd(
				screenCoord,
				CTPointCreate(-1, 0)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID++,
			CTPointAdd(
				screenCoord,
				CTPointCreate(0, 1)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID++,
			CTPointAdd(
				screenCoord,
				CTPointCreate(1, 0)
			),
			UV
		);

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID++,
			CTPointAdd(
				screenCoord,
				CTPointCreate(0, -1)
			),
			UV
		);

	case 1:

		__HCTProcessAndDrawPixel(
			drawInfo,
			pixID++,
			screenCoord,
			UV
		);

	default:
		break;
	}

}

static UINT32 __HCTRealPixelsPerPointSize(UINT32 pointSize) {

	switch (pointSize) {

	case 1:
		return 1;
	case 2:
		return 5;
	case 3:
		return 9;
	case 4:
		return 21;

	}

	return 1;
}

static void __HCTDrawPoints(PCTPrimitive primList, UINT32 primCount, P__CTDrawInfo drawInfo) {

	/// SUMMARY:
	/// loop (all prims in prim list)
	///		drawPixel
	
	for (UINT32 pixID = 0; pixID < primCount; pixID++) {

		const CTPoint pixPt		= CTPointFromVector(primList[pixID].vertex);

		__HCTDrawPoint(
			drawInfo,
			pixID * __HCTRealPixelsPerPointSize(drawInfo->shader->pointSizePixels),
			pixPt,
			primList[pixID].UV,
			drawInfo->shader->pointSizePixels
		);

	}

}

static void __HCTDrawLine(PCTPrimitive prim1, PCTPrimitive prim2, P__CTDrawInfo drawInfo) {
	
	FLOAT dx = prim2->vertex.x - prim1->vertex.x;
	FLOAT dy = prim2->vertex.y - prim1->vertex.y;
	const FLOAT dt = CTVectMagnitudeFast(CTVectCreate(dx, dy));

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
			pixID * __HCTRealPixelsPerPointSize(drawInfo->shader->lineSizePixels),
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

static UINT32 __HCTDrawTriangleTop(UINT32 startPixID, PCTPrimitive prims, P__CTDrawInfo drawInfo) {

	const FLOAT invSlopep2p1 =
		(prims[1].vertex.x - prims[0].vertex.x) /
		(prims[1].vertex.y - prims[0].vertex.y);

	const FLOAT invSlopep4p1 =
		(prims[3].vertex.x - prims[0].vertex.x) /
		(prims[3].vertex.y - prims[0].vertex.y);

	const INT32 DRAW_Y_START = 
		max(
			prims[1].vertex.y,
			0
		);
	const INT32 DRAW_Y_END =
		min(
			prims[0].vertex.y,
			drawInfo->frameBuffer->height - 1
		);

	UINT32 pixID = startPixID;

	for (INT32 drawY = DRAW_Y_START; drawY <= DRAW_Y_END; drawY++) {

		const FLOAT Y_DISTANCE_WALKED = 
			max(
				drawY - prims[1].vertex.y,
				0
			);

		const INT32 DRAW_X_START = 
			max(
				prims[1].vertex.x + (invSlopep2p1) * Y_DISTANCE_WALKED,
				0
			);
		const INT32 DRAW_X_END = 
			min(
				prims[3].vertex.x + (invSlopep4p1) * Y_DISTANCE_WALKED,
				drawInfo->frameBuffer->width - 1	
			);

		for (INT32 drawX = DRAW_X_START; drawX <= DRAW_X_END; drawX++) {
			__HCTDrawPoint(
				drawInfo,
				pixID,
				CTPointCreate(
					drawX,
					drawY
				),
				CTVectCreate(
					0,
					0
				),
				1
			);
			
			pixID++;
		}

	}

	return pixID;
}

static UINT32 __HCTDrawTriangleBottom(UINT32 startPixID, PCTPrimitive prims, P__CTDrawInfo drawInfo) {

	const FLOAT invSlopep3p2 =
		(prims[2].vertex.x - prims[1].vertex.x) /
		(prims[2].vertex.y - prims[1].vertex.y);

	const FLOAT invSlopep3p4 =
		(prims[2].vertex.x - prims[3].vertex.x) /
		(prims[2].vertex.y - prims[3].vertex.y);

	const INT32 DRAW_Y_START =
		max(
			prims[2].vertex.y,
			0
		);
	const INT32 DRAW_Y_END =
		min(
			prims[1].vertex.y,
			drawInfo->frameBuffer->height - 1
		);

	UINT32 pixID = startPixID;

	for (INT32 drawY = DRAW_Y_START; drawY <= DRAW_Y_END; drawY++) {

		const FLOAT Y_DISTANCE_WALKED =
			max(
				drawY - prims[2].vertex.y,
				0
			);

		const INT32 DRAW_X_START =
			max(
				prims[2].vertex.x + (invSlopep3p2) * Y_DISTANCE_WALKED,
				0
			);
		const INT32 DRAW_X_END =
			min(
				prims[2].vertex.x + (invSlopep3p4) * Y_DISTANCE_WALKED,
				drawInfo->frameBuffer->width - 1
			);

		for (INT32 drawX = DRAW_X_START; drawX <= DRAW_X_END; drawX++) {
			__HCTDrawPoint(
				drawInfo,
				pixID,
				CTPointCreate(
					drawX,
					drawY
				),
				CTVectCreate(
					0,
					0
				),
				1
			);

			pixID++;
		}

	}

	return pixID;
}

static void __HCTDrawTriangle(PCTPrimitive p1, PCTPrimitive p2, PCTPrimitive p3, P__CTDrawInfo drawInfo) {

	/// SUMMARY:
	/// sort primitives by height (p1 highest, p3 lowest)
	/// create p4 which splits traingle into 2, both with flat bases
	/// create list of all 4 verticies
	/// ensure p2 is left and p4 is right (swap if needed)
	/// draw top triangle (p2 p1 p4)
	/// draw bottom triangle (p3 p2 p4)

	if (p2->vertex.y > p1->vertex.y) {
		PCTPrimitive temp = p1;
		p1 = p2;
		p2 = temp;
	}

	if (p3->vertex.y > p1->vertex.y) {
		PCTPrimitive temp = p1;
		p1 = p3;
		p3 = temp;
	}

	if (p3->vertex.y > p2->vertex.y) {
		PCTPrimitive temp = p2;
		p2 = p3;
		p3 = temp;
	}

	const FLOAT invSlopep3p1 = 
		(p1->vertex.x - p3->vertex.x) /
		(p1->vertex.y - p3->vertex.y);

	CTPrimitive p4 = {
		.vertex.x = p3->vertex.x + (invSlopep3p1 * (p2->vertex.y - p3->vertex.y)),
		.vertex.y = p2->vertex.y,
	};

	CTPrimitive primList[] = {
		*p1,
		*p2,
		*p3,
		p4
	};

	if (primList[1].vertex.x > primList[3].vertex.x) {
		CTVect temp = primList[3].vertex;
		primList[3].vertex = primList[1].vertex;
		primList[1].vertex = temp;
	}

	UINT32 lastPixID = 0;
	lastPixID = __HCTDrawTriangleTop(lastPixID, primList, drawInfo);
	lastPixID = __HCTDrawTriangleBottom(lastPixID, primList, drawInfo);
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
	///		update primitive so that it is centered in screenspace
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

		PCTPrimitive prim = processedPrimList + primID;

		CTPrimCtx primtCtx = {
			.primID = primID,
			.mesh	= mesh
		};

		if (shader->primitiveShader != NULL) {
			shader->primitiveShader(
				primtCtx,
				prim,
				shaderInputCopy
			);
		}

		prim->vertex.x += (frameBuffer->width  >> 1);
		prim->vertex.y += (frameBuffer->height >> 1);
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

		if (mesh->primCount <= 2) {
			CTErrorSetFunction("CTDraw failed: cannot draw a filled polygon with only 2 verticies");
			goto DrawFuncFailure;
		}

		for (UINT32 primIndex = 0; primIndex < mesh->primCount - 2; primIndex++) {
			__HCTDrawTriangle(
				processedPrimList + 0,
				processedPrimList + primIndex + 1,
				processedPrimList + primIndex + 2,
				&drawInfo
			);
		}

		break;

	default:

		CTErrorSetParamValue("CTDraw failed: invalid draw method");
		goto DrawFuncFailure;

	}

DrawFuncSucess:

	CTGFXFree(shaderInputCopy);
	CTGFXFree(processedPrimList);
	return TRUE;

DrawFuncFailure:

	CTGFXFree(shaderInputCopy);
	CTGFXFree(processedPrimList);
	return FALSE;
}
