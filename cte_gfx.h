///////////////////////////////////////////////////////////////////////////
///	
/// 							<cte_gfx.h>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#ifndef _CTE_GFX_INCLUDE_
#define _CTE_GFX_INCLUDE_ 

#include "ct_base.h"
#include "ct_math.h"
#include "ct_gfx.h"
#include "ct_window.h"
#include "ct_thread.h"

//////////////////////////////////////////////////////////////////////////////
///
///								SUBSHADERS
/// 
//////////////////////////////////////////////////////////////////////////////

typedef void (*PCTSUBSPRIM)(
	CTPrimCtx		primCtx, 
	PCTPrimitive	primitive,
	PVOID			gData
);

typedef void (*PCTSUBSPIX)(
	CTPixCtx	pixCtx,
	PCTPixel	pix,
	PVOID		gData
);

typedef struct CTSubShader {
	BOOL		disableGTransform;
	BOOL		disableGTint;
	BOOL		disableGOutline;
	BOOL		disableGDither;
	PCTSUBSPRIM	subPrimShader;
	PCTSUBSPIX	subPixShader;
} CTSubSHader, *PCTSubShader;

CTCALL	PCTSubShader	CTSubShaderCreateEx(
	PCTSUBSPRIM primShader,
	PCTSUBSPIX	pixShader,
	BOOL		disableGTransform,
	BOOL		disableGTint,
	BOOL		disableGOutline,
	BOOL		disableGDither
);
#define CTSubShaderCreate(prim, pix) \
	CTSubShaderCreateEx(prim, pix, FALSE, FALSE, FALSE, FALSE)
CTCALL	BOOL			CTSubShaderDestroy(PCTSubShader shader);

//////////////////////////////////////////////////////////////////////////////
///
///								GRAPHICS OBJECT
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_GPROC_REASON_INIT	0
#define CT_GPROC_REASON_UPDATE	1
#define CT_GPROC_REASON_DESTROY	2
typedef (*PCTFUNCGOPROC)(
	UINT32	reason,
	PVOID	object,
	PVOID	userInput
);

typedef struct CTTransform {
	CTVect	pos;
	CTVect	scl;
	FLOAT	rot;
	FLOAT	layer;
} CTTransform, *PCTTransform, CTTForm, *PCTTForm;

typedef struct CTGObject {
	PCTLock			objLock;
	BOOL			visible;
	BOOL			destroySignal;
	UINT32			outlineSizePixels;
	PCTSubShader	subShader;
	CTColor			tintColor;
	CTColor			outlineColor;
	CTTForm			transform;
	SIZE_T			gDataSizeBytes;
	PVOID			gData;
} CTGObject, *PCTGObject, CTGO, *PCTGO;

CTCALL	PCTGO	CTGraphicsObjectCreate(
	CTVect			position,
	CTVect			scale,
	FLOAT			rotation,
	FLOAT			layer,
	PCTSubShader	subShader,
	SIZE_T			gDataSizeBytes,
	PVOID			initInput
);

CTCALL	BOOL	CTGraphicsObjectDestroy(PCTGO gObject);

#endif
