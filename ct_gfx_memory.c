//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_memory.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_data.h"
#include "ct_gfx.h"

#include <intrin.h>

CTCALL	PVOID		CTGFXAlloc(SIZE_T size) {
	PVOID ptr = HeapAlloc(__ctdata.gfx.gfxHeap, 0, size);

	if (ptr == NULL) {
		CTErrorSetFunction("CTGFXAlloc failed: heap error");
		return NULL;
	}
	__stosb(ptr, 0, size);
	return ptr;
}

CTCALL	BOOL		CTGFXFree(PVOID block) {
	if (block == NULL) {
		CTErrorSetParamValue("CTGFXFree failed: block was NULL");
		return FALSE;
	}

	HeapFree(__ctdata.gfx.gfxHeap, 0, block);
	return TRUE;
}