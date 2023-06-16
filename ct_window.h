///////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_window.h>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#ifndef _CT_WINDOW_INCLUDE_
#define _CT_WINDOW_INCLUDE_ 

#include "ct_gfx.h"

//////////////////////////////////////////////////////////////////////////////
///
///								WINDOW
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTWindow {
	PCTLock	lock;
	PCHAR	wndClassName;
	HWND	wndHandle;
	BOOL	shouldClose;
	PCTFB	frameBuffer;
} CTWindow, *PCTWindow, CTWin, *PCTWin;

CTCALL	PCTWin	CTWindowCreate(PCHAR title, UINT32 width, UINT32 height);
CTCALL	BOOL	CTWindowSetTitle(PCTWin window, PCHAR title);
CTCALL	BOOL	CTWindowSetSize(PCTWin window, UINT32 width, UINT height);
CTCALL	BOOL	CTWindowSetFrameBuffer(PCTWin window, PCTFB frameBuffer);
CTCALL	BOOL	CTWindowShouldClose(PCTWindow window);
CTCALL	BOOL	CTWindowSetShouldClose(PCTWindow window, BOOL state);
CTCALL	BOOL	CTWindowUpdate(PCTWindow window);
CTCALL	BOOL	CTWindowDestroy(PCTWin window);

#endif
