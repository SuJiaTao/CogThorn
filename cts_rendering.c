///////////////////////////////////////////////////////////////////////////
///	
/// 							<cts_rendering.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "cts_rendering.h"
#include "ct_data.h"

#include <stdio.h>

CTCALL	PCTSubShader	CTSubShaderCreateEx(
	PCTSUBSPRIM primShader,
	PCTSUBSPIX	pixShader,
	BOOL		disableGTransform,
	BOOL		disableGTint,
	BOOL		disableGOutline,
	BOOL		disableGDither
) {
	PCTSubShader shader = CTGFXAlloc(sizeof(*shader));
	shader->subPrimShader		= primShader;
	shader->subPixShader		= pixShader;
	shader->disableGTransform	= disableGTransform;
	shader->disableGTint		= disableGTint;
	shader->disableGOutline		= disableGOutline;
	shader->disableGDither		= disableGDither;

	return shader;
}

CTCALL	BOOL			CTSubShaderDestroy(PCTSubShader* pSubShader) {
	if (pSubShader == NULL) {
		CTErrorSetBadObject("CTSubShaderDestroy failed: pSubShader was NULL");
		return FALSE;
	}
	PCTSubShader subShader = *pSubShader;
	
	CTLockEnter(__ctdata.sys.rendering.lock);
	if (subShader == NULL) {
		CTErrorSetBadObject("CTSubShaderDestroy failed: subShader was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTGFXFree(pSubShader);
	*pSubShader = FALSE;
	CTLockLeave(__ctdata.sys.rendering.lock);

	return TRUE;
}

CTCALL	PCTGO	CTGraphicsObjectCreate(
	CTVect			position,
	CTVect			scale,
	FLOAT			rotation,
	FLOAT			layer,
	PCTMesh			mesh,
	PCTSubShader	subShader,
	PCTFUNCGOPROC	gProc,
	SIZE_T			gDataSizeBytes,
	PVOID			initInput
) {
	
	CTLockEnter(__ctdata.sys.rendering.lock);
	CTDynListLock(__ctdata.sys.rendering.objList);

	PCTGO obj	= CTDynListAdd(__ctdata.sys.rendering.objList);
	ZeroMemory(obj, sizeof(*obj));
	obj->lock				= CTLockCreate();
	obj->gData				= CTAlloc(max(4, gDataSizeBytes));
	obj->gDataSizeBytes		= gDataSizeBytes;
	obj->mesh				= mesh;
	obj->outlineColor		= CTColorCreate(0, 0, 0, 255);
	obj->outlineSizePixels	= 1;
	obj->subShader			= subShader;
	obj->tintColor			= CTColorCreate(255, 255, 255, 255);
	obj->transform.pos		= position;
	obj->transform.rot		= rotation;
	obj->transform.scl		= scale;
	obj->transform.layer	= layer;
	obj->visible			= TRUE;
	obj->gProc				= gProc;

	obj->gProc(
		CT_GPROC_REASON_INIT,
		obj,
		initInput
	);

	CTDynListUnlock(__ctdata.sys.rendering.objList);
	CTLockLeave(__ctdata.sys.rendering.lock);

	return obj;
}

CTCALL	BOOL	CTGraphicsObjectDestroy(PCTGO* pGObject) {
	if (pGObject == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectDestroy failed: pGObject was NULL");
		return FALSE;
	}

	PCTGO object = *pGObject;
	if (object == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectDestroy failed: object was NULL");
		return FALSE;
	}

	CTGraphicsObjectLock(object);
	object->destroySignal = TRUE;
	CTGraphicsObjectUnlock(object);

	*pGObject = NULL;
	return TRUE;
}

CTCALL	BOOL	CTGraphicsObjectLock(PCTGO gObject) {
	if (gObject == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectLock failed: gObject was NULL");
		return FALSE;
	}

	CTLockEnter(gObject->lock);
	return TRUE;
}

CTCALL	BOOL	CTGraphicsObjectUnlock(PCTGO gObject) {
	if (gObject == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectUnlock failed: gObject was NULL");
		return FALSE;
	}

	CTLockLeave(gObject->lock);
	return TRUE;
}

CTCALL	PCTCamera	CTCameraCreate(
	CTVect	position,
	CTVect	scale,
	FLOAT	rotation,
	PCTFB	renderTarget
) {
	CTLockEnter(__ctdata.sys.rendering.lock);
	CTDynListLock(__ctdata.sys.rendering.cameraList);

	PCTCamera cam		= CTDynListAdd(__ctdata.sys.rendering.cameraList);
	cam->destroySignal	= FALSE;
	cam->lock			= CTLockCreate();
	cam->renderTarget	= renderTarget;
	cam->transform.pos	= position;
	cam->transform.rot	= rotation;
	cam->transform.scl	= scale;

	CTDynListUnlock(__ctdata.sys.rendering.cameraList);
	CTLockLeave(__ctdata.sys.rendering.lock);

	return cam;
}

CTCALL	BOOL		CTCameraDestroy(PCTCamera* pCamera) {
	if (pCamera == NULL) {
		CTErrorSetBadObject("CTCameraDestroy failed: pCamera was NULL");
		return FALSE;
	}

	PCTCamera camera = *pCamera;
	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraDestroy failed: camera was NULL");
		return FALSE;
	}

	CTCameraLock(camera);
	camera->destroySignal = TRUE;
	CTCameraUnlock(camera);

	*pCamera = NULL;
	return TRUE;
}

CTCALL	BOOL		CTCameraLock(PCTCamera camera) {
	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraLock failed: camera was NULL");
		return FALSE;
	}

	CTLockEnter(camera->lock);
	return TRUE;
}

CTCALL	BOOL		CTCameraUnlock(PCTCamera camera) {
	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraUnlock failed: camera was NULL");
		return FALSE;
	}

	CTLockLeave(camera->lock);
	return TRUE;
}

void __CTRenderThreadProc(
	UINT32		reason,
	PCTThread	thread,
	PVOID		threadData,
	PVOID		input
) {

	printf("GFXtpreason: %d\n", reason);

	switch (reason)
	{

	case CT_THREADPROC_REASON_INIT:

		__ctdata.sys.rendering.lock			= CTLockCreate();
		__ctdata.sys.rendering.logStream	= CTLogStreamCreate("$gfxlog.log", NULL, NULL);
		__ctdata.sys.rendering.objList		= CTDynListCreate(
			sizeof(CTGO),
			CT_RTHREAD_GOBJ_NODE_SIZE
		);
		__ctdata.sys.rendering.cameraList = CTDynListCreate(
			sizeof(CTCamera),
			CT_RTHREAD_CAMERA_NODE_SIZE
		);

		CTLogImportant(
			__ctdata.sys.rendering.logStream,
			"GFX Handler Starting Up..."
		);

		break;

	case CT_THREADPROC_REASON_SPIN:
		
		printf("GFX spin %d\n",
			thread->threadSpinCount);
		CTLogInfo(
			__ctdata.sys.rendering.logStream,
			"GFX spin %d",
			thread->threadSpinCount
		);

		break;

	case CT_THREADPROC_REASON_EXIT:

		puts("GFX: exiting...");

		CTLogImportant(
			__ctdata.sys.rendering.logStream,
			"GFX Handler Shutting Down..."
		);

		CTLockEnter(__ctdata.sys.rendering.lock);
		CTLockDestroy(&__ctdata.sys.rendering.lock);
		CTDynListDestroy(&__ctdata.sys.rendering.objList);
		CTDynListDestroy(&__ctdata.sys.rendering.cameraList);
		CTLogStreamDestroy(&__ctdata.sys.rendering.logStream);

		break;

	default:
		break;

	}

}
