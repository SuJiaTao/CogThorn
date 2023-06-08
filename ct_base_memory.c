//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_base_memory.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"

#include <intrin.h>

CTCALL	PVOID	CTAlloc(SIZE_T sizeBytes) {

	__ctbase->heapAllocBytes += sizeBytes;
	__ctbase->heapAllocCount += 1;

	PVOID ptr = HeapAlloc(__ctbase->heap, 0, sizeBytes);
	__stosb(ptr, 0, sizeBytes);

	return ptr;
}

CTCALL	void	CTFree(PVOID ptr) {

	__ctbase->heapAllocBytes -= HeapSize(__ctbase->heap, 0, ptr);
	__ctbase->heapAllocCount -= 1;

	HeapFree(__ctbase->heap, 0, ptr);
}

CTCALL	SIZE_T	CTAllocCount(void) {
	return __ctbase->heapAllocCount;
}

CTCALL	SIZE_T	CTAllocSizeBytes(void) {
	return __ctbase->heapAllocBytes;
}