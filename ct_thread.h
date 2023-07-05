///////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_thread.h>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"

#ifndef _CT_THREAD_INCLUDE_
#define _CT_THREAD_INCLUDE_ 

//////////////////////////////////////////////////////////////////////////////
///
///								THREAD HANDLING
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_THREADPROC_REASON_INIT		0
#define CT_THREADPROC_REASON_SPIN		1
#define CT_THREADPROC_REASON_EXIT		2
typedef void (*PCTFUNCTHREADPROC)(
	UINT32	reason, 
	PVOID	thread, 
	PVOID	threadData,
	PVOID	input
);

typedef void (*PCTFUNCTHREADTASK)(
	PVOID	thread,
	PVOID	threadData,
	PVOID	input
);

#define CT_THREAD_TASK_QUEUE_NODE_SIZE	100
typedef struct CTThread {
	HANDLE				hThread;
	PCTLock				threadLock;
	PVOID				threadData;
	PCTFUNCTHREADPROC	threadProc;
	UINT64				threadSpinCount;
	UINT64				threadSpinIntervalMsec;
	PCTDynList			threadTaskQueue;
	BOOL				killSignal;
} CTThread, *PCTThread;

CTCALL	PCTThread	CTThreadCreate(
	PCTFUNCTHREADPROC	threadProc,
	SIZE_T				threadDataSizeBytes,
	PVOID				threadInitInput,
	UINT64				spinIntervalMsec
);
CTCALL	BOOL		CTThreadDestroy(PCTThread thread);
CTCALL	BOOL		CTThreadTask(PCTThread thread, PCTFUNCTHREADTASK pfTask, PVOID userInput);
CTCALL	BOOL		CTThreadLock(PCTThread thread);
CTCALL	BOOL		CTThreadUnlock(PCTThread thread);

#endif
