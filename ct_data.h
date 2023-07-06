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

struct {
	
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
		BOOL		killSignal;
	} logging;

} __ctdata;	// INSTANCE

#endif
