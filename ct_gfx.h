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
///								POINT
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTPoint {
	UINT32 x, y;
} CTPoint, * PCTPoint;

CTCALL	CTPoint			CTPointCreate(UINT32 x, UINT32 y);
CTCALL	CTPoint			CTPointAdd(CTPoint p1, CTPoint p2);
CTCALL	CTPoint			CTPointMultiply(CTPoint p, UINT32 factor);
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
} CTFrameBuffer, *PCTFrameBuffer;

CTCALL	PCTFrameBuffer	CTFrameBufferCreate(UINT32 width, UINT32 height);
CTCALL	BOOL			CTFrameBufferDestroy(PCTFrameBuffer fb);
CTCALL	BOOL			CTFrameBufferSet(PCTFrameBuffer fb, CTPoint pt, CTColor col, FLOAT depth);
CTCALL	BOOL			CTFrameBufferDepthTest(PCTFrameBuffer fb, CTPoint pt, FLOAT depth);
CTCALL	BOOL			CTFrameBufferGet(PCTFrameBuffer fb, CTPoint pt, PCTColor pCol, PFLOAT pDepth);
CTCALL	BOOL			CTFrameBufferLock(PCTFrameBuffer fb);
CTCALL	BOOL			CTFrameBufferUnlock(PCTFrameBuffer fb);
CTCALL	BOOL			CTFrameBufferClear(PCTFrameBuffer fb, BOOL color, BOOL depth);

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
