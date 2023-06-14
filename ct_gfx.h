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
///								FRAMEBUFFER
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTColor {
	BYTE a, r, g, b;
} CTColor, *PCTColor;

typedef struct CTFrameBuffer {
	PCTLock		lock;
	UINT32		width;
	UINT32		height;
	PCTColor	color;
	PFLOAT		depth;
} CTFrameBuffer, *PCTFrameBuffer, CTFB, *PCTFB;

CTCALL	PCTFB	CTFrameBufferCreate(UINT32 width, UINT32 height);
CTCALL	BOOL	CTFrameBufferDestroy(PCTFrameBuffer fb);
CTCALL	BOOL	CTFrameBufferSet(PCTFrameBuffer fb, CTPoint pt, CTColor col, FLOAT depth);
CTCALL	BOOL	CTFrameBufferDepthTest(PCTFrameBuffer fb, CTPoint pt, FLOAT depth);
CTCALL	BOOL	CTFrameBufferGet(PCTFrameBuffer fb, CTPoint pt, PCTColor pCol, PFLOAT pDepth);
CTCALL	BOOL	CTFrameBufferLock(PCTFrameBuffer fb);
CTCALL	BOOL	CTFrameBufferUnlock(PCTFrameBuffer fb);
CTCALL	BOOL	CTFrameBufferClear(PCTFrameBuffer fb, BOOL color, BOOL depth);

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
CTCALL	BOOL		CTMeshDestroy(PCTMesh mesh);

//////////////////////////////////////////////////////////////////////////////
///
///								SHADER
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTPixel {
	CTColor color;
	FLOAT	depth;
} PCTPixle, *PCTPixel;

typedef struct CTPrimitiveContext {
	UINT32	primID;
	PCTMesh	mesh;
} CTPrimitiveContext, *PCTPrimitiveContext, CTPrimCtx, *PCTPrimCtx;

typedef struct CTPixelContext {
	UINT32	pixID;
	CTPoint screenCoord;
	CTVect	UV;
	PCTFB	frameBuffer;
} CTPixelContext, *PCTPixelContext, CTPixCtx, *PCTPixCtx;

typedef void (*PCTSPRIMITIVE)(CTPrimCtx ctx, PCTPrimitive prim, PVOID input);
typedef void (*PCTSPIXEL	)(CTPixCtx ctx, PCTPixel pxl, PVOID input);

typedef struct CTShader {
	SIZE_T			shaderInputSizeBytes;
	PCTSPRIMITIVE	primitiveShader;
	PCTSPIXEL		pixelShader;
} CTShader, *PCTShader;

CTCALL	PCTShader	CTShaderCreate(PCTSPRIMITIVE sPrim, PCTSPIXEL sPix, SIZE_T shaderInputSize);
CTCALL	BOOL		CTShaderDestroy(PCTShader shader);

//////////////////////////////////////////////////////////////////////////////
///
///								DRAWING
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_DRAW_METHOD_POINTS	1
#define CT_DRAW_METHOD_LINES	2
#define CT_DRAW_METHOD_FILL		3
CTCALL	BOOL		CTDraw(
	UINT32		drawMethod, 
	PCTFB		frameBuffer, 
	PCTMesh		mesh, 
	PCTShader	shader, 
	PVOID		shaderInput
);

//////////////////////////////////////////////////////////////////////////////
///
///							GRAPHICS MODULE INSTANCE
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTGfx {
	HANDLE heap;
} CTGfx, *PCTGfx;
PCTGfx __ctgfx;		/// INSTANCE ///

#endif
