//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_base_lock.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"

CTCALL	PCTLock		CTLockCreate(void) {
	PCTLock lock = CTAlloc(sizeof(*lock));
	InitializeCriticalSection(&lock->lock);
	return lock;
}

CTCALL	BOOL		CTLockDestroy(PCTLock* pLock) {
	if (pLock == NULL) {
		CTErrorSetBadObject("CTLockDestroy failed: pLock was NULL");
		return FALSE;
	}

	PCTLock lock = *pLock;

	if (lock == NULL) {
		CTErrorSetBadObject("CTLockDestroy failed: lock was NULL");
		return FALSE;
	}

	EnterCriticalSection(&lock->lock);
	DeleteCriticalSection(&lock->lock);
	CTFree(lock);

	*pLock = NULL;
	return TRUE;
}

CTCALL	BOOL		CTLockEnter(PCTLock lock) {
	if (lock == NULL) {
		CTErrorSetBadObject("CTLockEnter failed: lock was NULL");
		return FALSE;
	}

	EnterCriticalSection(&lock->lock);

	return TRUE;
}

CTCALL	BOOL		CTLockLeave(PCTLock lock) {
	if (lock == NULL) {
		CTErrorSetBadObject("CTLockLeave failed: lock was NULL");
		return FALSE;
	}

	LeaveCriticalSection(&lock->lock);

	return TRUE;
}
