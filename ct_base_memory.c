//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_base_memory.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_data.h"
#include "ct_base.h"

#include <intrin.h>

CTCALL	PVOID	CTAlloc(SIZE_T sizeBytes) {

	__ctdata.base.heapAllocBytes += sizeBytes;
	__ctdata.base.heapAllocCount += 1;

	PVOID ptr = HeapAlloc(__ctdata.base.heap, 0, sizeBytes);
	__stosb(ptr, 0, sizeBytes);

	return ptr;
}

CTCALL	void	CTFree(PVOID ptr) {

	__ctdata.base.heapAllocBytes -= HeapSize(__ctdata.base.heap, 0, ptr);
	__ctdata.base.heapAllocCount -= 1;

	HeapFree(__ctdata.base.heap, 0, ptr);
}

CTCALL	SIZE_T	CTAllocCount(void) {
	return __ctdata.base.heapAllocCount;
}

CTCALL	SIZE_T	CTAllocSizeBytes(void) {
	return __ctdata.base.heapAllocBytes;
}