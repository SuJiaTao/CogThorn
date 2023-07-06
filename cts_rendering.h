///////////////////////////////////////////////////////////////////////////
///	
/// 							<cts_rendering.h>
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
#include "ct_logging.h"
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
CTCALL	BOOL			CTSubShaderDestroy(PCTSubShader* pSubShader);

//////////////////////////////////////////////////////////////////////////////
///
///								GRAPHICS OBJECT
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_GPROC_REASON_INIT		0
#define CT_GPROC_REASON_DESTROY		1
#define CT_GPROC_REASON_PRE_RENDER	2
#define CT_GPROC_REASON_POST_RENDER	3
typedef (*PCTFUNCGOPROC)(
	UINT32	reason,
	PVOID	object,
	PVOID	userInput
);

typedef struct CTTransform {
	CTVect	pos;
	CTVect	scl;
	FLOAT	rot;
	FLOAT	depth;
} CTTransform, *PCTTransform, CTTForm, *PCTTForm;

typedef struct CTGObject {
	BOOL			visible;
	BOOL			destroySignal;
	PCTLock			lock;
	UINT32			outlineSizePixels;
	PCTFB			texture;
	PCTMesh			mesh;
	PCTSubShader	subShader;
	CTColor			tintColor;
	CTColor			outlineColor;
	CTTForm			transform;
	SIZE_T			gDataSizeBytes;
	PVOID			gData;
	PCTFUNCGOPROC	gProc;
} CTGObject, *PCTGObject, CTGO, *PCTGO;

CTCALL	PCTGO	CTGraphicsObjectCreate(
	CTVect			position,
	CTVect			scale,
	FLOAT			rotation,
	FLOAT			layer,
	PCTFB			texture,
	PCTMesh			mesh,
	PCTSubShader	subShader,
	PCTFUNCGOPROC	gProc,
	SIZE_T			gDataSizeBytes,
	PVOID			initInput
);
CTCALL	BOOL	CTGraphicsObjectDestroy(PCTGO* pGObject);
CTCALL	BOOL	CTGraphicsObjectLock(PCTGO gObject);
CTCALL	BOOL	CTGraphicsObjectUnlock(PCTGO gObject);

//////////////////////////////////////////////////////////////////////////////
///
///								CAMERA OBJECT
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTCamera {
	PCTLock		lock;
	CTTransform	transform;
	PCTFB		renderTarget;
	BOOL		destroySignal;
} CTCamera, *PCTCamera;

CTCALL	PCTCamera	CTCameraCreate(
	CTVect	position,
	CTVect	scale,
	FLOAT	rotation,
	PCTFB	renderTarget
);
CTCALL	BOOL		CTCameraDestroy(PCTCamera* pCamera);
CTCALL	BOOL		CTCameraLock(PCTCamera camera);
CTCALL	BOOL		CTCameraUnlock(PCTCamera camera);

//////////////////////////////////////////////////////////////////////////////
///
///						   GRAPHICS HANDLER THREAD LOGIC
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_RTHREAD_SPINTIME_MSEC		(1000 / 40)
#define CT_RTHREAD_GOBJ_NODE_SIZE		1024
#define CT_RTHREAD_CAMERA_NODE_SIZE		32
#define CT_RTHREAD_CLEAN_INTERVAL		200

void __CTRenderThreadProc(
	UINT32		reason, 
	PCTThread	thread, 
	PVOID		threadData, 
	PVOID		input
);

#endif
