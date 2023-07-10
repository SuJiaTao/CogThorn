//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_data.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_data.h"
#include "ct_logging.h"
#include "cts_rendering.h"

#include <stdio.h>

CTCALL	BOOL	CogThornInit(void) {

	if (__ctdata.initialized == TRUE)
		return FALSE;

	__ctdata.initialized	= TRUE;
	__ctdata.initThreadID	= GetCurrentThreadId();

	//////////////////////////////////////////////////////////////////////////////
	///								INITIALIZE CORE
	//////////////////////////////////////////////////////////////////////////////

	ZeroMemory(&__ctdata.base, sizeof(__ctdata.base));
	__ctdata.base.heap				= HeapCreate(0, 0, 0);
	__ctdata.base.errorCallbackList	= NULL;
	InitializeCriticalSection(&__ctdata.base.errorLock);

	//////////////////////////////////////////////////////////////////////////////
	///							  INITIALIZE GRAPHICS
	//////////////////////////////////////////////////////////////////////////////

	ZeroMemory(&__ctdata.gfx, sizeof(__ctdata.gfx));
	__ctdata.gfx.gfxHeap				= HeapCreate(0, 0, 0);
	__ctdata.gfx.allTasksCompleteEvent  = CreateEventA(
		NULL,
		TRUE,
		FALSE,
		NULL
	);
	__ctdata.gfx.threadTerminateEvent = CreateEventA(
		NULL,
		TRUE,
		FALSE,
		NULL
	);
	__ctdata.gfx.taskList = CTDynListCreate(
		sizeof(CTGFXDrawTask),
		CT_GFX_DRAW_TASK_LIST_NODE_SIZE
	);
	InitializeSRWLock(&__ctdata.gfx.threadLock);
	for (UINT32 i = 0; i < CT_GFX_THREADCOUNT; i++) {
		__ctdata.gfx.threads[i] = CreateThread(
			NULL,
			CT_GFX_THREAD_STACKSIZE,
			__CTDrawThreadProc,
			NULL,
			NULL,
			NULL
		);
	}

	SetProcessDPIAware();

	//////////////////////////////////////////////////////////////////////////////
	///							  INITIALIZE LOGGING
	//////////////////////////////////////////////////////////////////////////////

	ZeroMemory(&__ctdata.logging, sizeof(__ctdata.logging));

	__ctdata.logging.startTimeMsecs = GetTickCount64();
	__ctdata.logging.lock			= CTLockCreate();
	__ctdata.logging.killSignal = CreateEventA(
		NULL,
		FALSE,
		FALSE,
		NULL
	);
	__ctdata.logging.logWriteQueue	= CTDynListCreate(
		sizeof(CTLogEntry),
		CT_LOGGING_QUEUE_NODE_SIZE
	);
	__ctdata.logging.logWriteThread	= CreateThread(
		NULL,
		NULL,
		__CTLoggingThreadProc,
		NULL,
		NULL,
		NULL
	);

	//////////////////////////////////////////////////////////////////////////////
	///							  INITIALIZE THREADDING
	//////////////////////////////////////////////////////////////////////////////

	TIMECAPS timeCaps;
	timeGetDevCaps(&timeCaps, sizeof(TIMECAPS));
	timeBeginPeriod(timeCaps.wPeriodMin);

	//////////////////////////////////////////////////////////////////////////////
	///						   INITIALIZE RENDERING SYSTEM
	//////////////////////////////////////////////////////////////////////////////

	ZeroMemory(&__ctdata.sys.rendering, sizeof(__ctdata.sys.rendering));

	__ctdata.sys.rendering.thread = CTThreadCreate(
		__CTRenderThreadProc,
		NULL,
		NULL,
		CT_RTHREAD_SPINTIME_MSEC,
		TRUE
	);

	return TRUE;

}

CTCALL	BOOL	CogThornTerminate(void) {

	if (__ctdata.initialized == FALSE)
		return FALSE;

	if (GetCurrentThreadId() != __ctdata.initThreadID)
		return FALSE;

	//////////////////////////////////////////////////////////////////////////////
	///						   CLEANUP RENDERING SYSTEM
	//////////////////////////////////////////////////////////////////////////////

	CTThreadDestroy(
		&__ctdata.sys.rendering.thread
	);

	//////////////////////////////////////////////////////////////////////////////
	///							  CLEANUP LOGGING
	//////////////////////////////////////////////////////////////////////////////

	SetEvent(__ctdata.logging.killSignal);
	WaitForSingleObject(__ctdata.logging.logWriteThread, INFINITE);

	CTDynListDestroy(&__ctdata.logging.logWriteQueue);
	CTLockDestroy(&__ctdata.logging.lock);
	CloseHandle(__ctdata.logging.killSignal);

	//////////////////////////////////////////////////////////////////////////////
	///							  CLEANUP GRAPHICS
	//////////////////////////////////////////////////////////////////////////////

	HeapDestroy(__ctdata.gfx.gfxHeap);

	//////////////////////////////////////////////////////////////////////////////
	///							  CLEANUP BASE
	//////////////////////////////////////////////////////////////////////////////

	EnterCriticalSection(&__ctdata.base.errorLock);

	PCTErrMsgCallbackNode node = __ctdata.base.errorCallbackList;
	while (node != NULL) {
		PCTErrMsgCallbackNode nextNode = node->next;
		CTFree(node);
		node = nextNode;
	}

	DeleteCriticalSection(&__ctdata.base.errorLock);
	HeapDestroy(__ctdata.base.heap);

	ZeroMemory(&__ctdata, sizeof(__ctdata));

	return TRUE;
}