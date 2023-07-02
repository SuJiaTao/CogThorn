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

#define CT_WINDOW_NAME_SIZE			0xFF
#define CT_WINDOW_STYLE				WS_VISIBLE | WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME) 
typedef struct CTWindow {
	PCTLock	lock;
	HWND	hwnd;
	PCTFB	frameBuffer;
	PVOID	userData;
	CHAR	wndClassName[CT_WINDOW_NAME_SIZE];
} CTWindow, *PCTWindow, CTWin, *PCTWin;

CTCALL	PCTWin	CTWindowCreate(PCHAR title, UINT32 width, UINT32 height);
CTCALL	BOOL	CTWindowLock(PCTWin window);
CTCALL	BOOL	CTWindowUnlock(PCTWin window);
CTCALL	BOOL	CTWindowSetTitle(PCTWin window, PCHAR title);
CTCALL	BOOL	CTWindowSetSize(PCTWin window, UINT32 width, UINT height);
CTCALL	BOOL	CTWindowSetFrameBuffer(PCTWin window, PCTFB frameBuffer);
CTCALL	BOOL	CTWindowUpdate(PCTWindow window);
CTCALL	BOOL	CTWindowDestroy(PCTWin window);

#endif
