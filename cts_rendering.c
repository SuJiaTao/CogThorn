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

static void __HCTCallObjectGProc(PCTGO obj, UINT32 reason, PVOID input) {
	obj->gProc(
		reason,
		obj,
		input
	);
}

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

	CTGFXFree(subShader);
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
	
	if (gProc == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectCreate failed: gProc was NULL");
		return NULL;
	}

	CTLockEnter(__ctdata.sys.rendering.lock);
	CTDynListLock(__ctdata.sys.rendering.objList);

	PCTGO obj	= CTDynListAdd(__ctdata.sys.rendering.objList);
	ZeroMemory(obj, sizeof(*obj));
	obj->destroySignal		= FALSE;
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

	__HCTCallObjectGProc(
		obj,
		CT_GPROC_REASON_INIT,
		initInput
	);

	CTDynListUnlock(__ctdata.sys.rendering.objList);
	CTLockLeave(__ctdata.sys.rendering.lock);

	return obj;
}

CTCALL	BOOL	CTGraphicsObjectDestroy(PCTGO* pGObject) {

	CTLockEnter(__ctdata.sys.rendering.lock);

	if (pGObject == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectDestroy failed: pGObject was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	PCTGO object = *pGObject;
	if (object == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectDestroy failed: object was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTGraphicsObjectLock(object);
	object->destroySignal = TRUE;
	CTGraphicsObjectUnlock(object);

	*pGObject = NULL;

	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;
}

CTCALL	BOOL	CTGraphicsObjectLock(PCTGO gObject) {
	
	CTLockEnter(__ctdata.sys.rendering.lock);

	if (gObject == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectLock failed: gObject was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;

}

CTCALL	BOOL	CTGraphicsObjectUnlock(PCTGO gObject) {

	CTLockEnter(__ctdata.sys.rendering.lock);

	if (gObject == NULL) {
		CTErrorSetBadObject("CTGraphicsObjectUnlock failed: gObject was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTLockLeave(__ctdata.sys.rendering.lock);
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

	CTLockEnter(__ctdata.sys.rendering.lock);

	PCTCamera camera = *pCamera;
	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraDestroy failed: camera was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTCameraLock(camera);
	camera->destroySignal = TRUE;
	CTCameraUnlock(camera);

	*pCamera = NULL;

	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;
}

CTCALL	BOOL		CTCameraLock(PCTCamera camera) {

	CTLockEnter(__ctdata.sys.rendering.lock);

	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraLock failed: camera was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTLockEnter(camera->lock);
	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;

}

CTCALL	BOOL		CTCameraUnlock(PCTCamera camera) {
	CTLockEnter(__ctdata.sys.rendering.lock);

	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraUnlock failed: camera was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTLockLeave(camera->lock);
	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;
}

typedef struct __CTRTShaderData {
	PCTGO		object;
	PCTCamera	camera;
} __CTRTShaderData, *P__CTRTShaderData;

static void __HCTRenderThreadPrimShader(
	CTPrimCtx			ctx,
	PCTPrimitive		prim,
	P__CTRTShaderData	data
) {



}

static void __HCTRenderThreadPixShader(
	CTPixCtx			ctx,
	PCTPixel			pixel,
	P__CTRTShaderData	data
) {

}

static void __HCTDrawGraphicsObject(PCTGO object, PCTCamera camera) {

	__CTRTShaderData shaderData = {
					.object = object,
					.camera = camera
	};


}

void __CTRenderThreadProc(
	UINT32		reason,
	PCTThread	thread,
	PVOID		threadData,
	PVOID		input
) {

	switch (reason)
	{

	case CT_THREADPROC_REASON_INIT:

		CTLogImportant(
			__ctdata.sys.rendering.logStream,
			"Rendering System Starting Up..."
		);

		__ctdata.sys.rendering.lock			= CTLockCreate();
		__ctdata.sys.rendering.logStream	= CTLogStreamCreate("$renderlog.txt", NULL, NULL);
		__ctdata.sys.rendering.objList		= CTDynListCreate(
			sizeof(CTGO),
			CT_RTHREAD_GOBJ_NODE_SIZE
		);
		__ctdata.sys.rendering.cameraList = CTDynListCreate(
			sizeof(CTCamera),
			CT_RTHREAD_CAMERA_NODE_SIZE
		);

		__ctdata.sys.rendering.shader = CTShaderCreate(
			__HCTRenderThreadPrimShader,
			__HCTRenderThreadPixShader,
			sizeof(CTGObject),
			0,
			0,
			TRUE
		);

		CTLogImportant(
			__ctdata.sys.rendering.logStream,
			"Rendering System Startup Complete..."
		);

		break;

	case CT_THREADPROC_REASON_SPIN:
		
		CTLockEnter(__ctdata.sys.rendering.lock);

		/// SUMMARY:
		/// loop (all graphics objects)
		///		if (object is NOT VISIBLE) 
		///			skip
		///		if (object is SIGNALED TO BE DESTROYED)
		///			CALL DESTROY
		///			destroy object
		///			skip
		///		loop (all cameras)
		///			if (camera is SIGNALED TO BE DESTROYED)
		///				destroy camera
		///				skip
		///			if (camera's target is NULL)
		///				skip
		///			CALL PRE-RENDER
		///			setup shader parameters
		///			setup shader inputs
		///			draw renderObject
		///			CALL POST-RENDER

		CTLockLeave(__ctdata.sys.rendering.lock);

		PCTIterator gObjIter = CTIteratorCreate(__ctdata.sys.rendering.objList);
		PCTGO object		 = NULL;
		while ((object = CTIteratorIterate(gObjIter)) != NULL) {

			CTGraphicsObjectLock(object);

			if (object->visible == FALSE) {
				CTGraphicsObjectUnlock(object);
				continue;
			}

			if (object->destroySignal == TRUE) {
				__HCTCallObjectGProc(
					object,
					CT_GPROC_REASON_DESTROY,
					NULL
				);
				CTGraphicsObjectUnlock(object);

				CTLockDestroy(&object->lock);
				CTFree(object->gData);

				CTDynListRemove(
					__ctdata.sys.rendering.objList,
					object
				);
				continue;
			}

			PCTIterator camIter = CTIteratorCreate(__ctdata.sys.rendering.cameraList);
			PCTCamera camera	= NULL;
			while ((camera = CTIteratorIterate(camIter)) != NULL) {
				
				CTCameraLock(camera);

				if (camera->destroySignal == TRUE) {
					CTLockDestroy(camera->lock);
					CTDynListRemove(
						__ctdata.sys.rendering.cameraList,
						camera
					);
					continue;
				}

				if (camera->renderTarget == NULL) {
					CTCameraUnlock(camera);
					continue;
				}

				__HCTCallObjectGProc(
					object,
					CT_GPROC_REASON_PRE_RENDER,
					NULL
				);

				__HCTDrawGraphicsObject(
					object,
					camera
				);

				__HCTCallObjectGProc(
					object,
					CT_GPROC_REASON_POST_RENDER,
					NULL
				);

			}

			CTIteratorDestroy(&camIter);

			CTGraphicsObjectUnlock(object);

		}

		CTIteratorDestroy(gObjIter);

		break;

	case CT_THREADPROC_REASON_EXIT:

		CTLogImportant(
			__ctdata.sys.rendering.logStream,
			"Rendering System Shutting Down..."
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
