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
	PVOID			object
);

typedef BOOL (*PCTSUBSPIX)(
	CTPixCtx	pixCtx,
	PCTPixel	pix,
	PVOID		object
);

typedef struct CTSubShader {
	BOOL		disableGTransform;
	BOOL		disableGAlpha;
	BOOL		disableGOutline;
	PCTSUBSPRIM	subPrimShader;
	PCTSUBSPIX	subPixShader;
} CTSubShader, *PCTSubShader;

CTCALL	PCTSubShader	CTSubShaderCreateEx(
	PCTSUBSPRIM primShader,
	PCTSUBSPIX	pixShader,
	BOOL		disableGTransform,
	BOOL		disableGAlpha,
	BOOL		disableGOutline
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
typedef void (*PCTFUNCGOPROC)(
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
	FLOAT			age;
	UINT32			outlineSizePixels;
	PCTFB			texture;
	PCTMesh			mesh;
	PCTSubShader	subShader;
	BYTE			alpha;
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

//////////////////////////////////////////////////////////////////////////////
///
///								SURFACE OBJECT
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTSurface {
	BOOL		destroySignal;
	PCTWindow	window;
	PCTFB		frameBuffer;
} CTSurface, *PCTSurface;

CTCALL	PCTSurface	CTSurfaceCreate(
	PCHAR	title,
	UINT32	windowType,
	UINT32	width,
	UINT32	height,
	UINT32	resX,
	UINT32	resY
);
CTCALL	BOOL		CTSurfaceDestroy(PCTSurface* pSurface);

//////////////////////////////////////////////////////////////////////////////
///
///								CAMERA OBJECT
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_CAMERA_TARGET_NONE		0
#define CT_CAMERA_TARGET_TEXTURE	1
#define CT_CAMERA_TARGET_SURFACE	2
typedef struct CTCamera {
	CTTransform	transform;
	BOOL		destroySignal;
	UINT32		targetType;
	PCTFB		targetTexture;
	PCTSurface	targetSurface;
} CTCamera, *PCTCamera;

CTCALL	PCTCamera	CTCameraCreate(
	CTVect	position,
	CTVect	scale,
	FLOAT	rotation
);
CTCALL	BOOL		CTCameraClearTarget(PCTCamera camera);
CTCALL	BOOL		CTCameraSetTargetTexture(PCTCamera camera, PCTFB texture);
CTCALL	BOOL		CTCameraSetTargetSurface(PCTCamera camera, PCTSurface surface);
CTCALL	BOOL		CTCameraDestroy(PCTCamera* pCamera);

//////////////////////////////////////////////////////////////////////////////
///
///						   GRAPHICS HANDLER THREAD LOGIC
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_RTHREAD_SPINTIME_MSEC		(1000 / 30)
#define CT_RTHREAD_GOBJ_NODE_SIZE		2048
#define CT_RTHREAD_CAMERA_NODE_SIZE		32
#define CT_RTHREAD_SURFACE_NODE_SIZE	32
#define CT_RTHREAD_CLEAN_INTERVAL		(1250 / (CT_RTHREAD_SPINTIME_MSEC))
#define CT_RTHREAD_DITHER_MAX_ALPHA		247
#define CT_RTHREAD_DITHER_MIN_ALPHA		7

void __CTRenderThreadProc(
	UINT32		reason, 
	PCTThread	thread, 
	PVOID		threadData, 
	PVOID		input
);

#endif
