//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_data.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_data.h"
#include "ct_logging.h"

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
	__ctdata.gfx.gfxHeap	= HeapCreate(0, 0, 0);
	SetProcessDPIAware();

	//////////////////////////////////////////////////////////////////////////////
	///							  INITIALIZE LOGGING
	//////////////////////////////////////////////////////////////////////////////

	ZeroMemory(&__ctdata.logging, sizeof(__ctdata.logging));

	__ctdata.logging.startTimeMsecs = GetTickCount64();
	__ctdata.logging.lock			= CTLockCreate();
	__ctdata.logging.killSignal		= FALSE;
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
	timeGetDevCaps(&timeCaps, sizeof(timeCaps));
	timeBeginPeriod(timeCaps.wPeriodMin);

	return TRUE;

}

CTCALL	BOOL	CogThornTerminate(void) {

	if (__ctdata.initialized == FALSE)
		return FALSE;

	if (GetCurrentThreadId() != __ctdata.initThreadID)
		return FALSE;

	//////////////////////////////////////////////////////////////////////////////
	///							  CLEANUP LOGGING
	//////////////////////////////////////////////////////////////////////////////

	CTLockEnter(__ctdata.logging.lock);
	__ctdata.logging.killSignal = TRUE;
	CTLockLeave(__ctdata.logging.lock);
	WaitForSingleObject(__ctdata.logging.logWriteThread, INFINITE);

	CTDynListDestroy(&__ctdata.logging.logWriteQueue);
	CTLockDestroy(&__ctdata.logging.lock);

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

	return TRUE;
}