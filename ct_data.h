//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_data.h>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#ifndef _CT_DATA_INCLUDE_
#define _CT_DATA_INCLUDE_

#include "ct_base.h"
#include "ct_logging.h"
#include "ct_thread.h"

CTCALL	BOOL	CogThornInit(void);
CTCALL	BOOL	CogThornTerminate(void);

struct {
	
	BOOL	initialized;
	DWORD	initThreadID;

	struct {
		HANDLE					heap;
		SIZE_T					heapAllocCount;
		SIZE_T					heapAllocBytes;
		CRITICAL_SECTION		errorLock;
		CTErrMsg				lastError;
		PCTErrMsgCallbackNode	errorCallbackList;
	} base;

	struct {
		HANDLE		gfxHeap;
	} gfx;

	struct {
		INT64		startTimeMsecs;
		PCTLock		lock;
		HANDLE		logWriteThread;
		PCTDynList	logWriteQueue;
		HANDLE		killSignal;
	} logging;

	struct {

		struct {
			PCTLock			lock;
			PCTThread		thread;
			PCTLogStream	logStream;
			PCTDynList		objList;
			PCTDynList		cameraList;
		} rendering;

	} sys;

} __ctdata;	// INSTANCE

#endif
