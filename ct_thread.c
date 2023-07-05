///////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_thread.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"
#include "ct_thread.h"

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

	CTThreadLock(thread);
	thread->threadProc(
		CT_THREADPROC_REASON_INIT,
		thread,
		thread->threadData,
		userInit
	);
	CTThreadUnlock(thread);

	while (TRUE) {

		INT64 SPIN_START = GetTickCount64();

		CTThreadLock(thread);
		thread->threadProc(
			CT_THREADPROC_REASON_SPIN,
			thread,
			thread->threadData,
			NULL
		);
		CTThreadUnlock(thread);

		PCTIterator taskIter		= CTIteratorCreate(thread->threadTaskQueue);
		P__CTThreadTaskData task	= NULL;
		while ((task = CTIteratorIterate(taskIter)) != NULL) {

			CTThreadLock(thread);
			task->taskFunc(
				thread,
				thread->threadData,
				task->userInput
			);
			CTThreadUnlock(thread);

		}

		CTDynListClear(thread->threadTaskQueue);

		CTThreadLock(thread);
		if (thread->killSignal == TRUE) {

			thread->threadProc(
				CT_THREADPROC_REASON_EXIT,
				thread,
				thread->threadData,
				NULL
			);

			CTFree(thread->threadData);
			CTDynListDestroy(thread->threadTaskQueue);
			CTLockDestroy(thread->threadLock);

			ExitThread(ERROR_SUCCESS);

		}
		CTThreadUnlock(thread);

		INT64 SPIN_END = GetTickCount64();

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

	CTThreadLock(thread);
	thread->killSignal = TRUE;
	CTThreadUnlock(thread);

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

	CTThreadLock(thread);
	CTDynListLock(thread->threadTaskQueue);

	P__CTThreadTaskData task = CTDynListAdd(thread->threadTaskQueue);
	task->userInput	= userInput;
	task->taskFunc	= pfTask;

	CTDynListUnlock(thread->threadTaskQueue);
	CTThreadUnlock(thread);

	return TRUE;
}

CTCALL	BOOL		CTThreadLock(PCTThread thread) {
	if (thread == NULL) {
		CTErrorSetBadObject("CTThreadLock failed: thread was NULL");
		return FALSE;
	}
	if (thread->killSignal == TRUE) {
		CTErrorSetFunction("CTThreadLock failed: thread is being destroyed");
		return FALSE;
	}

	CTLockEnter(thread->threadLock);
	return TRUE;
}

CTCALL	BOOL		CTThreadUnlock(PCTThread thread) {
	if (thread == NULL) {
		CTErrorSetBadObject("CTThreadUnlock failed: thread was NULL");
		return FALSE;
	}
	if (thread->killSignal == TRUE) {
		CTErrorSetFunction("CTThreadUnlock failed: thread is being destroyed");
		return FALSE;
	}

	CTLockLeave(thread->threadLock);
	return TRUE;
}
