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

static __forceinline void __HCTCallObjectGProc(PCTGO obj, UINT32 reason, PVOID input) {
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
	BOOL		disableGAlpha,
	BOOL		disableGOutline
) {
	PCTSubShader shader = CTGFXAlloc(sizeof(*shader));
	shader->subPrimShader		= primShader;
	shader->subPixShader		= pixShader;
	shader->disableGTransform	= disableGTransform;
	shader->disableGAlpha		= disableGAlpha;
	shader->disableGOutline		= disableGOutline;

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
	PCTFB			texture,
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
	obj->age				= 0;
	obj->lock				= CTLockCreate();
	obj->gData				= CTAlloc(max(4, gDataSizeBytes));
	obj->gDataSizeBytes		= gDataSizeBytes;
	obj->mesh				= mesh;
	obj->texture			= texture;
	obj->outlineColor		= CTColorCreate(0, 0, 0, 255);
	obj->outlineSizePixels	= 1;
	obj->subShader			= subShader;
	obj->alpha				= 255;
	obj->transform.pos		= position;
	obj->transform.rot		= rotation;
	obj->transform.scl		= scale;
	obj->transform.depth	= layer;
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

typedef struct __CTSurfCreateDat {
	PCHAR		title;
	UINT32		winType;
	UINT32		width, height, resX, resY;
	PCTSurface	outSurf;
} __CTSurfCreateDat, *P__CTSurfCreateDat;

static void __HCTSurfaceCreateFunc(PCTThread thread, PVOID threadData, P__CTSurfCreateDat dat) {
	PCTSurface surface		= CTDynListAdd(__ctdata.sys.rendering.surfaceList);
	surface->lock			= CTLockCreate();
	surface->destroySignal	= FALSE;
	surface->window			= CTWindowCreate(
		dat->winType,
		dat->title,
		dat->width,
		dat->height
	);
	surface->frameBuffer = CTFrameBufferCreate(
		dat->resX,
		dat->resY
	);
	CTWindowSetFrameBuffer(
		surface->window,
		surface->frameBuffer
	);

	dat->outSurf = surface;
}

CTCALL	PCTSurface	CTSurfaceCreate(
	PCHAR	title,
	UINT32	windowType,
	UINT32	width,
	UINT32	height,
	UINT32	resX,
	UINT32	resY
) {
	if (width == 0 || height == 0 || resX == 0 || resY == 0) {
		CTErrorSetParamValue("CTSurfaceCreate failed: dimensions or resolution was invalid");
		return FALSE;
	}

	__CTSurfCreateDat dat = {
		.title		= title,
		.winType	= windowType,
		.width		= width,
		.height		= height,
		.resX		= resX,
		.resY		= resY,
		.outSurf	= NULL
	};

	CTThreadTask(
		__ctdata.sys.rendering.thread,
		__HCTSurfaceCreateFunc,
		&dat,
		TRUE
	);

	return dat.outSurf;
}

CTCALL	BOOL		CTSurfaceDestroy(PCTSurface* pSurface) {

	CTLockEnter(__ctdata.sys.rendering.lock);

	if (pSurface == NULL) {
		CTErrorSetParamValue("CTSurfaceDestroy failed: pSurface was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}
	PCTSurface surface = *pSurface;
	if (surface == NULL) {
		CTErrorSetParamValue("CTSurfaceDestroy failed: surface was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTLockEnter(surface->lock);
	surface->destroySignal = TRUE;
	*pSurface = NULL;
	CTLockLeave(surface->lock);
	
	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;
}

typedef struct __CTCamCreateData {
	PCTCamera	outCam;
	CTVect		pos;
	CTVect		scl;
	FLOAT		rot;
} __CTCamCreateData, *P__CTCamCreateData;

static void __HCTCameraCreateTask(PCTThread thread, PVOID threadData, P__CTCamCreateData dat) {
	PCTCamera cam		= CTDynListAdd(__ctdata.sys.rendering.cameraList);
	cam->destroySignal	= FALSE;
	cam->lock			= CTLockCreate();
	cam->transform.pos	= dat->pos;
	cam->transform.rot	= dat->rot;
	cam->transform.scl	= dat->scl;
	cam->targetType		= CT_CAMERA_TARGET_NONE;
	cam->targetTexture	= NULL;
	cam->targetSurface	= NULL;
	dat->outCam			= cam;
}

CTCALL	PCTCamera	CTCameraCreate(
	CTVect	position,
	CTVect	scale,
	FLOAT	rotation
) {
	
	__CTCamCreateData dat = {
		.outCam = NULL,
		.pos	= position,
		.scl	= scale,
		.rot	= rotation
	};

	CTThreadTask(
		__ctdata.sys.rendering.thread,
		__HCTCameraCreateTask,
		&dat,
		TRUE
	);

	return dat.outCam;
}

CTCALL	BOOL		CTCameraClearTarget(PCTCamera camera) {

	CTLockEnter(__ctdata.sys.rendering.lock);

	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraClearTarget failed: camera was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTCameraLock(camera);

	camera->targetType		= CT_CAMERA_TARGET_NONE;
	camera->targetTexture	= NULL;
	camera->targetSurface	= NULL;

	CTCameraUnlock(camera);

	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;

}

CTCALL	BOOL		CTCameraSetTargetTexture(PCTCamera camera, PCTFB texture) {

	CTLockEnter(__ctdata.sys.rendering.lock);
	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraSetTargetTexture failed: camera was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}
	if (texture == NULL) {
		CTErrorSetBadObject("CTCameraSetTargetTexture failed: texture was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTCameraLock(camera);
	CTFrameBufferLock(texture);

	camera->targetType		= CT_CAMERA_TARGET_TEXTURE;
	camera->targetTexture	= texture;
	camera->targetSurface	= NULL;

	CTFrameBufferUnlock(texture);
	CTCameraUnlock(camera);

	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;

}

CTCALL	BOOL		CTCameraSetTargetSurface(PCTCamera camera, PCTSurface surface) {

	CTLockEnter(__ctdata.sys.rendering.lock);
	if (camera == NULL) {
		CTErrorSetBadObject("CTCameraSetTargetSurface failed: camera was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}
	if (surface == NULL) {
		CTErrorSetBadObject("CTCameraSetTargetSurface failed: surface was NULL");
		CTLockLeave(__ctdata.sys.rendering.lock);
		return FALSE;
	}

	CTCameraLock(camera);
	CTLockEnter(surface->lock);

	camera->targetType		= CT_CAMERA_TARGET_SURFACE;
	camera->targetTexture	= NULL;
	camera->targetSurface	= surface;

	CTLockLeave(surface->lock);
	CTCameraUnlock(camera);

	CTLockLeave(__ctdata.sys.rendering.lock);
	return TRUE;
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
	PCTFB		renderTarget;
} __CTRTShaderData, *P__CTRTShaderData;

static void __HCTRenderThreadPrimShader(
	CTPrimCtx			ctx,
	PCTPrimitive		prim,
	P__CTRTShaderData	data
) {

	BOOL applyTransform = TRUE;

	if (data->object->subShader != NULL) {
		applyTransform = !(data->object->subShader->disableGTransform);
	}

	if (data->object->subShader != NULL) {
		if (data->object->subShader->subPrimShader != NULL) {
			data->object->subShader->subPrimShader(
				ctx,
				prim,
				data->object
			);
		}
	}

	if (applyTransform == TRUE) {
		CTMatrix tform = CTMatrixTransform(
			CTMatrixIdentity(),
			data->object->transform.pos,
			data->object->transform.scl,
			data->object->transform.rot
		);

		prim->vertex = CTMatrixApply(
			tform,
			prim->vertex
		);
	}

	CTMatrix camTform = CTMatrixIdentity();
	camTform = CTMatrixTranslate(
		camTform, 
		CTVectCreate(
			data->camera->transform.pos.x * -1.0f,
			data->camera->transform.pos.y * -1.0f
		)
	);
	camTform = CTMatrixRotate(
		camTform,
		data->camera->transform.rot * -1.0f
	);
	camTform = CTMatrixScale(
		camTform,
		data->camera->transform.scl
	);

	prim->vertex = CTMatrixApply(
		camTform,
		prim->vertex
	);
}

static BOOL __HCTRenderThreadPixShader(
	CTPixCtx			ctx,
	PCTPixel			pixel,
	P__CTRTShaderData	data
) {
	BOOL applyOutline	= TRUE;
	BOOL applyAlpha		= TRUE;
	
	if (data->object->subShader != NULL) {
		applyOutline	= !(data->object->subShader->disableGOutline);
		applyAlpha		= !(data->object->subShader->disableGAlpha);
	}

	CTColor pixColor;

	switch (ctx.drawMethod)
	{
	case CT_DRAW_METHOD_LINES_CLOSED:

		if (applyOutline == FALSE)
			return FALSE;
		pixColor = data->object->outlineColor;
		break;

	default:

		if (data->object->texture == NULL) {
			pixColor = CTColorCreate(255, 255, 255, 255);
			
		} else {
			pixColor = CTSSample(
				data->object->texture,
				ctx.UV,
				CTS_SAMPLE_METHOD_CUTOFF
			);
		}

		if (applyAlpha == TRUE && data->object->alpha != 255) {
			pixColor.a = (pixColor.a * data->object->alpha) >> 8;
		}

		break;
	}

	pixel->color = pixColor;

	BOOL keepPixel = TRUE;
	if (data->object->subShader != NULL) {
		if (data->object->subShader->subPixShader != NULL) {
			keepPixel = data->object->subShader->subPixShader(
				ctx,
				pixel,
				data->object
			);
		}
	}

	return keepPixel;
}

static __forceinline void __HCTDrawGraphicsObject(PCTGO object, PCTCamera camera) {

	if (object->mesh == NULL)
		return;

	PCTFB renderTarget = NULL;
	if (camera->targetType == CT_CAMERA_TARGET_TEXTURE) {
		renderTarget = camera->targetTexture;
	} else {
		renderTarget = camera->targetSurface->frameBuffer;
	}

	__CTRTShaderData shaderData = {
		.object			= object,
		.camera			= camera,
		.renderTarget	= renderTarget
	};

	/// DRAW OBJECT OUTLINE
	if (object->outlineSizePixels != 0) {

		__ctdata.sys.rendering.shader->lineSizePixels = 
			max(
				CT_SHADER_LINESIZE_MIN, 
				min(
					CT_SHADER_LINESIZE_MAX, 
					object->outlineSizePixels
				)
			);

		CTDraw(
			CT_DRAW_METHOD_LINES_CLOSED,
			renderTarget,
			shaderData.object->mesh,
			__ctdata.sys.rendering.shader,
			&shaderData,
			shaderData.object->transform.depth
		);

	}

	/// DRAW OBJECT
	CTDraw(
		CT_DRAW_METHOD_FILL,
		renderTarget,
		shaderData.object->mesh,
		__ctdata.sys.rendering.shader,
		&shaderData,
		shaderData.object->transform.depth
	);
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
		__ctdata.sys.rendering.surfaceList = CTDynListCreate(
			sizeof(CTSurface),
			CT_RTHREAD_SURFACE_NODE_SIZE
		);

		__ctdata.sys.rendering.shader = CTShaderCreate(
			__HCTRenderThreadPrimShader,
			__HCTRenderThreadPixShader,
			sizeof(CTGObject),
			0,
			0,
			TRUE
		);

		SetThreadPriority(
			thread->hThread,
			THREAD_PRIORITY_TIME_CRITICAL
		);

		CTLogImportant(
			__ctdata.sys.rendering.logStream,
			"Rendering System Startup Complete..."
		);

		break;

	case CT_THREADPROC_REASON_SPIN:

		/// SUMMARY:
		/// if (cycles % clean interval == 0)
		///		clean camera/object buffers
		/// loop (all cameras)
		///		if (camera is SIGNALED TO BE DESTROYED)
		///			destroy camera
		///			skip
		///		if (camera has NO TARGET)
		///			skip
		///		get framebuffer
		///		LOCK FRAMEBUFFER
		///		CLEAR FRAMEBUFFER
		///		loop (all objects)	
		///			if (object is NOT VISIBLE) 
		///				skip
		///			if (object is SIGNALED TO BE DESTROYED)
		///				CALL DESTROY
		///				destroy object
		///				skip
		///			CALL PRE-RENDER
		///			setup shader parameters
		///			setup shader inputs
		///			draw renderObject
		///			CALL POST-RENDER
		///			increment object age
		///		loop (all surfaces)
		///			if (surface signaled destroy)
		///				remove surface
		///				skip
		///			update surface window
		///		UNLOCK FRAMEBUFFER

		CTLockEnter(__ctdata.sys.rendering.lock);

		if ((thread->threadSpinCount % CT_RTHREAD_CLEAN_INTERVAL) == 0) {

			UINT32 objCountBefore = __ctdata.sys.rendering.objList->elementsUsedCount;
			UINT32 camCountBefore = __ctdata.sys.rendering.cameraList->elementsUsedCount;
			UINT32 surfCountBefore = __ctdata.sys.rendering.surfaceList->elementsUsedCount;

			CTDynListClean(__ctdata.sys.rendering.objList);
			CTDynListClean(__ctdata.sys.rendering.cameraList);
			CTDynListClean(__ctdata.sys.rendering.surfaceList);

			UINT32 objCountAfter = __ctdata.sys.rendering.objList->elementsUsedCount;
			UINT32 camCountAfter = __ctdata.sys.rendering.cameraList->elementsUsedCount;
			UINT32 surfCountAfter = __ctdata.sys.rendering.surfaceList->elementsUsedCount;

			puts("CTRT: cleaning...");

			CTLogInfo(
				__ctdata.sys.rendering.logStream,
				"Cleaned %d Graphics Objects and %d Cameras",
				objCountBefore - objCountAfter,
				camCountBefore - camCountAfter,
				surfCountBefore - surfCountAfter
			);

		}

		PCTIterator camIter = CTIteratorCreate(__ctdata.sys.rendering.cameraList);
		PCTCamera	camera  = NULL;

		while ((camera = CTIteratorIterate(camIter)) != NULL) {

			CTCameraLock(camera);

			if (camera->destroySignal == TRUE) {
				CTLockDestroy(&camera->lock);
				CTDynListRemove(
					__ctdata.sys.rendering.cameraList,
					camera
				);
				continue;
			}

			if (camera->targetType == CT_CAMERA_TARGET_NONE) {
				CTCameraUnlock(camera);
				continue;
			}

			PCTFrameBuffer renderTarget = NULL;
			if (camera->targetType == CT_CAMERA_TARGET_TEXTURE) {
				renderTarget = camera->targetTexture;
			}
			if (camera->targetType == CT_CAMERA_TARGET_SURFACE) {
				renderTarget = camera->targetSurface->frameBuffer;
			}

			CTFrameBufferLock(renderTarget);
			CTFrameBufferClear(renderTarget, TRUE, TRUE);

			PCTIterator gObjIter = CTIteratorCreate(__ctdata.sys.rendering.objList);
			PCTGO		object	 = NULL;
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

				object->age += 1.0f;

				CTGraphicsObjectUnlock(object);

			} // END OBJECT LOOP

			CTIteratorDestroy(&gObjIter);
			CTFrameBufferUnlock(renderTarget);

			CTCameraUnlock(camera);

		} // END CAMERA LOOP

		CTIteratorDestroy(&camIter);

		PCTIterator surfIter = CTIteratorCreate(__ctdata.sys.rendering.surfaceList);
		PCTSurface	surface	 = NULL;
		while ((surface = CTIteratorIterate(surfIter)) != NULL) {

			CTLockEnter(surface->lock);

			if (surface->destroySignal == TRUE) {
				CTLockDestroy(&surface->lock);
				CTWindowDestroy(&surface->window);
				CTDynListRemove(__ctdata.sys.rendering.surfaceList, surface);
				continue;
			}

			CTWindowUpdate(surface->window);

			CTLockLeave(surface->lock);

		}

		CTIteratorDestroy(&surfIter);

		CTLockLeave(__ctdata.sys.rendering.lock);

		printf(
			"CTRT: objs: %d | %d msec taken... | %f FPS\n", 
			__ctdata.sys.rendering.objList->elementsUsedCount,
			thread->threadSpinLastIntervalMsec,
			1000.0f / (FLOAT)thread->threadSpinLastIntervalMsec
		);

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
		CTDynListDestroy(&__ctdata.sys.rendering.surfaceList);
		CTLogStreamDestroy(&__ctdata.sys.rendering.logStream);

		break;

	default:
		break;

	}

}
