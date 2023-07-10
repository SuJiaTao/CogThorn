///////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx.h>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#ifndef _CT_GRAPHICS_INCLUDE_
#define _CT_GRAPHICS_INCLUDE_

#include "ct_base.h"
#include "ct_math.h"

#include <math.h>

//////////////////////////////////////////////////////////////////////////////
///
///								MEMORY
/// 
//////////////////////////////////////////////////////////////////////////////

CTCALL	PVOID		CTGFXAlloc(SIZE_T size);
CTCALL	BOOL		CTGFXFree(PVOID block);

//////////////////////////////////////////////////////////////////////////////
///
///								POINT
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTPoint {
	INT32 x, y;
} CTPoint, * PCTPoint;

CTCALL	CTPoint			CTPointCreate(INT32 x, INT32 y);
CTCALL	CTPoint			CTPointAdd(CTPoint p1, CTPoint p2);
CTCALL	CTPoint			CTPointMultiply(CTPoint p, INT32 factor);
CTCALL	CTPoint			CTPointFromVector(CTVect vect);
CTCALL	CTVect			CTPointToVector(CTPoint p);

//////////////////////////////////////////////////////////////////////////////
///
///								COLOR
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTColor {
	BYTE b, g, r, a;
} CTColor, * PCTColor;

CTCALL	CTColor			CTColorCreate(INT r, INT g, INT b, INT a);
CTCALL	CTColor			CTColorScale(CTColor col, FLOAT factor);
CTCALL	CTColor			CTColorMultiply(CTColor c1, CTColor c2);
CTCALL	CTColor			CTColorAdd(CTColor c1, CTColor c2);
CTCALL	CTColor			CTColorBlend(CTColor bottom, CTColor top);
CTCALL	CTColor			CTColorBlendWeighted(CTColor c1, CTColor c2, FLOAT factor);

//////////////////////////////////////////////////////////////////////////////
///
///								FRAMEBUFFER
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTFrameBuffer {
	PCTLock		lock;
	UINT32		width;
	UINT32		height;
	PCTColor	color;
	PFLOAT		depth;
} CTFrameBuffer, *PCTFrameBuffer, CTFB, *PCTFB;

CTCALL	PCTFB	CTFrameBufferCreate(UINT32 width, UINT32 height);
CTCALL	BOOL	CTFrameBufferDestroy(PCTFrameBuffer* pfb);
CTCALL	BOOL	CTFrameBufferSetEx(PCTFrameBuffer fb, CTPoint pt, CTColor col, FLOAT depth, BOOL safe);
CTCALL	BOOL	CTFrameBufferDepthTestEx(PCTFrameBuffer fb, CTPoint pt, FLOAT depth, BOOL safe);
CTCALL	BOOL	CTFrameBufferGetEx(PCTFrameBuffer fb, CTPoint pt, PCTColor pCol, PFLOAT pDepth, BOOL safe);
CTCALL	BOOL	CTFrameBufferLock(PCTFrameBuffer fb);
CTCALL	BOOL	CTFrameBufferUnlock(PCTFrameBuffer fb);
CTCALL	BOOL	CTFrameBufferClear(PCTFrameBuffer fb, BOOL color, BOOL depth);

#define CTFrameBufferSet(fb, pt, col, depth)	\
	CTFrameBufferSetEx(fb, pt, col, depth, TRUE)
#define CTFrameBufferDepthTest(fb, pt, depth)	\
	CTFrameBufferDepthTestEx(fb, pt, depth, TRUE)
#define CTFrameBufferGet(fb, pt, pCol, pDepth)	\
	CTFrameBufferGetEx(fb, pt, pCol, pDepth, TRUE);

//////////////////////////////////////////////////////////////////////////////
///
///								MESH
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTPrimitive {
	CTVect	vertex;
	CTVect	UV;
} CTPrimitive, *PCTPrimitive;

typedef struct CTMesh {
	UINT32			primCount;
	PCTPrimitive	primList;
} CTMesh, *PCTMesh;

CTCALL	PCTMesh		CTMeshCreate(PFLOAT verts, PFLOAT uvs, UINT32 primCount);
CTCALL	BOOL		CTMeshDestroy(PCTMesh* pMesh);

//////////////////////////////////////////////////////////////////////////////
///
///								SHADER
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTPixel {
	CTColor color;
	CTPoint	screenCoord;
} CTPixel, *PCTPixel;

typedef struct CTPrimitiveContext {
	UINT32	drawMethod;
	UINT32	primID;
	PCTMesh	mesh;
} CTPrimitiveContext, *PCTPrimitiveContext, CTPrimCtx, *PCTPrimCtx;

typedef struct CTPixelContext {
	UINT32	drawMethod;
	UINT32	pixID;
	PCTFB	frameBuffer;
	CTVect	UV;
} CTPixelContext, *PCTPixelContext, CTPixCtx, *PCTPixCtx;

typedef void (*PCTSPRIMITIVE)(CTPrimCtx ctx, PCTPrimitive prim, PVOID input);
typedef BOOL (*PCTSPIXEL	)(CTPixCtx ctx, PCTPixel pxl, PVOID input);

typedef struct CTShader {
	SIZE_T			shaderInputSizeBytes;
	PCTSPRIMITIVE	primitiveShader;
	PCTSPIXEL		pixelShader;
	UINT32			pointSizePixels;
	UINT32			lineSizePixels;
	BOOL			depthTest;
} CTShader, *PCTShader;

#define CT_SHADER_POINTSIZE_MIN		1
#define CT_SHADER_POINTSIZE_MAX		4
#define CT_SHADER_LINESIZE_MIN		1
#define CT_SHADER_LINESIZE_MAX		4
CTCALL	PCTShader	CTShaderCreate(
	PCTSPRIMITIVE	sPrim, 
	PCTSPIXEL		sPix, 
	SIZE_T			shaderInputSize,
	UINT32			pointSize,
	UINT32			lineSize,
	BOOL			depthTest
);
CTCALL	BOOL		CTShaderDestroy(PCTShader* pShader);

//////////////////////////////////////////////////////////////////////////////
///
///								DRAWING
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_DRAW_METHOD_POINTS		0
#define CT_DRAW_METHOD_LINES_OPEN	1
#define CT_DRAW_METHOD_LINES_CLOSED	2
#define CT_DRAW_METHOD_FILL			3
#define CT_DRAW_METHOD_WIREFRAME	4
CTCALL	BOOL		CTDraw(
	UINT32		drawMethod, 
	PCTFB		frameBuffer, 
	PCTMesh		mesh, 
	PCTShader	shader, 
	PVOID		shaderInput,
	FLOAT		depth
);

//////////////////////////////////////////////////////////////////////////////
///
///								SHADER FUNCTIONS
/// 
//////////////////////////////////////////////////////////////////////////////

#define CTS_SAMPLE_METHOD_CLAMP_TO_EDGE	0
#define CTS_SAMPLE_METHOD_CUTOFF		1
#define CTS_SAMPLE_METHOD_REPEAT		2
#define CTS_SAMPLE_EPSILON				0.001f
CTCALL __forceinline CTColor CTSSample(PCTFB texture, CTVect UV, UINT32 sampleMethod) {

	CTColor retColor = {
		.r = 0,
		.g = 0,
		.b = 0,
		.a = 0
	};

	if (texture == NULL)
		goto __CTSSampleComplete;

	switch (sampleMethod)
	{

	case CTS_SAMPLE_METHOD_CUTOFF:

		if (UV.x < 0.0f || UV.x > 1.0f || UV.y < 0.0f || UV.y > 1.0f)
			goto __CTSSampleComplete;

	case CTS_SAMPLE_METHOD_CLAMP_TO_EDGE:

		UV.x = min(1.0f, max(UV.x, 0.0f));
		UV.y = min(1.0f, max(UV.y, 0.0f));
		
		break;

	case CTS_SAMPLE_METHOD_REPEAT:

		UV.x = fmodf(UV.x, 1.0f);
		UV.y = fmodf(UV.y, 1.0f);

		if (UV.x < 0.0f)
			UV.x += 1.0f;

		if (UV.y < 0.0f)
			UV.y += 1.0f;

		break;

	default:

		goto __CTSSampleComplete;

	}

	UINT32 samplex = (UINT32)(UV.x * ((FLOAT)texture->width  - 1 - CTS_SAMPLE_EPSILON));
	UINT32 sampley = (UINT32)(UV.y * ((FLOAT)texture->height - 1 - CTS_SAMPLE_EPSILON));

	CTFrameBufferGetEx(
		texture,
		CTPointCreate(
			samplex,
			sampley
		),
		&retColor,
		NULL,
		FALSE
	);

__CTSSampleComplete:
	return retColor;
}

#endif
