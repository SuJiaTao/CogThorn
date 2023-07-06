//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_dllmain.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"
#include "ct_gfx.h"
#include "ct_logging.h"
#include "ct_g_handler.h"

#include <stdio.h>

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason, 
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        
        /// INITIALIZE BASE MODULE

        __ctbase = LocalAlloc(0, sizeof(*__ctbase));
        if (__ctbase == NULL) return FALSE;

        ZeroMemory(__ctbase, sizeof(*__ctbase));

        __ctbase->heap = HeapCreate(0, 0, 0);
        __ctbase->errorCallbackList = NULL;
        InitializeCriticalSection(&__ctbase->errorLock);

        /// INITIALIZE GFX MODULE

        __ctgfx = LocalAlloc(0, sizeof(*__ctgfx));
        if (__ctgfx == NULL) return FALSE;

        ZeroMemory(__ctgfx, sizeof(*__ctgfx));

        __ctgfx->heap = HeapCreate(0, 0, 0);

        SetProcessDPIAware();

        /// INITIALIZE LOGGING MODULE

        __ctlog = LocalAlloc(0, sizeof(*__ctlog));
        if (__ctlog == NULL) return FALSE;

        ZeroMemory(__ctlog, sizeof(*__ctlog));

        __ctlog->startTimeMsecs = GetTickCount64();
        __ctlog->lock           = CTLockCreate();
        __ctlog->killSignal     = FALSE;
        __ctlog->logWriteQueue  = CTDynListCreate(
            sizeof(CTLogEntry), 
            CT_LOGGING_QUEUE_NODE_SIZE
        );
        __ctlog->logWriteThread = CreateThread(
            NULL,
            NULL,
            __CTLoggingThreadProc,
            NULL,
            NULL,
            NULL
        );

        /// INITIALIZE THREAD MODULE
        {
            TIMECAPS timeCaps;
            timeGetDevCaps(&timeCaps, sizeof(timeCaps));
            timeBeginPeriod(timeCaps.wPeriodMin);
        }

        /// INITIALIZE GRAPHICS HANDLER

        __ctghandler = LocalAlloc(0, sizeof(*__ctghandler));
        if (__ctghandler == NULL) return FALSE;

        ZeroMemory(__ctghandler, sizeof(*__ctghandler));

        printf("initing ctghandler\n");
        __ctghandler->thread = CTThreadCreate(
            __CTGFXHandlerThreadProc,
            NULL,
            NULL,
            CT_G_HANDLER_SPINTIME_MSEC
        );

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:

        /// CLEANUP GRAPHICS HANDLER
        


        /// CLEANUP LOGGING MODULE
         
        __ctlog->killSignal = TRUE;
        WaitForSingleObject(__ctlog->logWriteThread, INFINITE);

        CTDynListDestroy(&__ctlog->logWriteQueue);
        CTLockDestroy(&__ctlog->lock);

        LocalFree(__ctlog);
        
        /// CLEANUP GFX MODULE

        HeapDestroy(__ctgfx->heap);
        LocalFree(__ctgfx);

        /// CLEANUP BASE MODULE

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