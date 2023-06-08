// <ct_dllmain.c>
// Bailey JT Brown
// 2023

#include "ct_base.h"

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason, 
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        __ctbase = LocalAlloc(0, sizeof(*__ctbase));
        if (__ctbase == NULL) return FALSE;

        ZeroMemory(__ctbase, sizeof(*__ctbase));

        __ctbase->heap = HeapCreate(0, 0, 0);
        __ctbase->errorCallbackList = NULL;
        InitializeCriticalSection(&__ctbase->errorLock);

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:

        EnterCriticalSection(&__ctbase->errorLock);

        PCTErrMsgCallbackNode node = __ctbase->errorCallbackList;
        while (node != NULL) {
            PCTErrMsgCallbackNode nextNode = node->next;
            CTFree(node);
            node = nextNode;
        }

        DeleteCriticalSection(&__ctbase->errorLock);
        HeapDestroy(__ctbase->heap);
        LocalFree(__ctbase);
        break;
    }

    return TRUE;
}