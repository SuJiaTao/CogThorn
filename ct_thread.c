///////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_thread.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"
#include "ct_thread.h"

#include <stdio.h>

typedef struct __CTThreadInput {
	PCTThread	thread;
	PVOID		initUserInput;
} __CTThreadInput, *P__CTThreadInput;

typedef struct __CTThreadTaskData {
	PCTFUNCTHREADTASK	taskFunc;
	PVOID				userInput;
} __CTThreadTaskData, *P__CTThreadTaskData;

static DWORD __HCTThreadProc(P__CTThreadInput threadInput) {

	/// SUMMARY:
	/// call thread init
	/// loop (forever)
	///		record start time
	///		call thread spin
	///		loop (all threadtasks)
	///			execute threadtask
	///		if (killsignal)
	///			call thread exit
	///			exit
	///		record end time
	///		sleep(spin_time - elasped time)

	PCTThread thread	= threadInput->thread;
	PVOID userInit		= threadInput->initUserInput;
	CTFree(threadInput);

	CTLockEnter(thread->threadLock);
	thread->threadProc(
		CT_THREADPROC_REASON_INIT,
		thread,
		thread->threadData,
		userInit
	);
	CTLockLeave(thread->threadLock);

	UINT64 CLOCK_FREQUENCY_MSEC;
	QueryPerformanceFrequency(&CLOCK_FREQUENCY_MSEC);
	CLOCK_FREQUENCY_MSEC /= 1000;

	while (TRUE) {

		INT64 SPIN_START;
		QueryPerformanceCounter(&SPIN_START);
		SPIN_START /= CLOCK_FREQUENCY_MSEC;

		CTLockEnter(thread->threadLock);
		thread->threadProc(
			CT_THREADPROC_REASON_SPIN,
			thread,
			thread->threadData,
			NULL
		);
		thread->threadSpinCount++;
		CTLockLeave(thread->threadLock);

		PCTIterator taskIter		= CTIteratorCreate(thread->threadTaskQueue);
		P__CTThreadTaskData task	= NULL;
		while ((task = CTIteratorIterate(taskIter)) != NULL) {

			CTLockEnter(thread->threadLock);
			task->taskFunc(
				thread,
				thread->threadData,
				task->userInput
			);
			CTLockLeave(thread->threadLock);

		}

		CTIteratorDestroy(taskIter);
		CTDynListClear(thread->threadTaskQueue);

		CTLockEnter(thread->threadLock);
		if (thread->killSignal == TRUE) {

			thread->threadProc(
				CT_THREADPROC_REASON_EXIT,
				thread,
				thread->threadData,
				NULL
			);

			CTDynListDestroy(thread->threadTaskQueue);
			CTLockDestroy(thread->threadLock);
			CTFree(thread->threadData);
			CTFree(thread);

			ExitThread(ERROR_SUCCESS);

		}
		CTLockLeave(thread->threadLock);

		INT64 SPIN_END;
		QueryPerformanceCounter(&SPIN_END);
		SPIN_END /= CLOCK_FREQUENCY_MSEC;

		Sleep(max(0, thread->threadSpinIntervalMsec - (SPIN_END - SPIN_START)));

	}

}

CTCALL	PCTThread	CTThreadCreate(
	PCTFUNCTHREADPROC	threadProc,
	SIZE_T				threadDataSizeBytes,
	PVOID				threadInitInput,
	UINT64				spinIntervalMsec
) {

	if (threadProc == NULL) {
		CTErrorSetBadObject("CTThreadCreate failed: threadProc was NULL");
		return NULL;
	}

	PCTThread thread	= CTAlloc(sizeof(*thread));
	thread->killSignal	= FALSE;
	thread->threadData	= CTAlloc(min(4, threadDataSizeBytes));
	thread->threadLock	= CTLockCreate();
	thread->threadProc	= threadProc;
	thread->threadSpinCount			= 0;
	thread->threadSpinIntervalMsec	= spinIntervalMsec;
	thread->threadTaskQueue			= CTDynListCreate(
		sizeof(__CTThreadTaskData), 
		CT_THREAD_TASK_QUEUE_NODE_SIZE
	);

	P__CTThreadInput threadInput	= CTAlloc(sizeof(*threadInput));
	threadInput->thread				= thread;
	threadInput->initUserInput		= threadInitInput;

	thread->hThread = CreateThread(
		NULL,
		NULL,
		__HCTThreadProc,
		threadInput,
		NULL,
		NULL
	);

	return thread;
}

CTCALL	BOOL		CTThreadDestroy(PCTThread thread) {
	if (thread == NULL) {
		CTErrorSetBadObject("CTThreadDestroy failed: thread was NULL");
		return FALSE;
	}
	if (thread->killSignal == TRUE) {
		CTErrorSetBadObject("CTThreadDestroy failed: thread is already being destroyed");
		return FALSE;
	}

	CTLockEnter(thread->threadLock);
	thread->killSignal = TRUE;
	CTLockLeave(thread->threadLock);

	WaitForSingleObject(thread->hThread, INFINITE);
	return TRUE;
}

CTCALL	BOOL		CTThreadTask(PCTThread thread, PCTFUNCTHREADTASK pfTask, PVOID userInput) {
	if (thread == NULL) {
		CTErrorSetBadObject("CTThreadTask failed: thread was NULL");
		return FALSE;
	}
	if (pfTask == NULL) {
		CTErrorSetBadObject("CTThreadTask failed: pfTask was NULL");
		return FALSE;
	}
	if (thread->killSignal == TRUE) {
		CTErrorSetFunction("CTThreadTask failed: thread is being destroyed");
		return FALSE;
	}

	CTLockEnter(thread->threadLock);
	CTDynListLock(thread->threadTaskQueue);

	P__CTThreadTaskData task = CTDynListAdd(thread->threadTaskQueue);
	task->userInput	= userInput;
	task->taskFunc	= pfTask;

	CTDynListUnlock(thread->threadTaskQueue);
	CTLockLeave(thread->threadLock);

	return TRUE;
}
