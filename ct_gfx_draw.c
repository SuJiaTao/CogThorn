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

		for (INT drawX = pixPt.x - halfPtSize; drawX < pixPt.x + halfPtSize; drawX++) {

			for (INT drawY = pixPt.y - halfPtSize; drawY < pixPt.y + halfPtSize; drawY++) {

				__HCTProcessAndDrawPixel(
					drawInfo,
					pixID,
					CTPointCreate(drawX, drawY),
					primList[pixIndex].UV
				);

				pixID++;
			}

		}

	}

}

static void __HCTDrawLine(PCTPrimitive prim1, PCTPrimitive prim2, P__CTDrawInfo drawInfo) {

	/// SUMMARY:
	/// generate normalized direction vector from prim1 to prim2 as "dn"
	/// centerX = p1.x
	/// centerY = p1.y
	/// setup pixID
	/// loop (each pixel in hypotenuses (roughly))
	///		for (centerX - lineSize / 2 to centerX + lineSize / 2)
	///			for (centerY - lineSize / 2 to centerY + lineSize / 2)
	///				calculate UV as weighted average of draw position's distance to each primitive
	///				draw pixel
	///				increment pixID
	///		centerX += dn.x
	///		centerY += dn.y
	///		
	
	const CTVect delta				= CTVectSubtract(prim2->vertex, prim1->vertex);
	const FLOAT  deltaMag			= CTVectMagnitudeFast(delta);
	const CTVect deltaNormalized	= CTVectMultiply(delta, 1.0f / deltaMag);

	FLOAT drawX		= prim1->vertex.x;
	FLOAT drawY		= prim1->vertex.y;
	UINT32 pixID	= 0;
	for (UINT32 walk = 0; walk < (deltaMag); walk++) {

		const INT halfPtSize	= drawInfo->shader->lineSizePixels / 2;
		const CTPoint pixPt		= CTPointCreate(drawX, drawY);

		for (INT drawX = pixPt.x - halfPtSize; drawX < pixPt.x + halfPtSize; drawX++) {

			for (INT drawY = pixPt.y - halfPtSize; drawY < pixPt.y + halfPtSize; drawY++) {

				FLOAT p1Dist = CTVectMagnitudeFast(
					CTVectSubtract(
						CTVectCreate(drawX, drawY),
						prim1->vertex
					)
				);
				FLOAT p2Dist = CTVectMagnitudeFast(
					CTVectSubtract(
						CTVectCreate(drawX, drawY),
						prim2->vertex
					)
				);

				FLOAT p1UVFactor = p1Dist / (p1Dist + p2Dist);
				FLOAT p2UVFactor = 1.0f - p1UVFactor;

				CTVect pointUV = CTVectCreate(
					prim1->UV.x * p1UVFactor + prim2->UV.x * p2UVFactor,
					prim1->UV.y * p1UVFactor + prim2->UV.y * p2UVFactor
				);

				__HCTProcessAndDrawPixel(
					drawInfo,
					pixID,
					CTPointCreate(drawX, drawY),
					pointUV
				);

				pixID++;
			}

		}

		drawX += deltaNormalized.x;
		drawY += deltaNormalized.y;

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

	return TRUE;
}
