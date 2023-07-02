//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_window.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_window.h"

#include <stdio.h>

CTCALL	PCTWin	CTWindowCreate(PCHAR title, UINT32 width, UINT32 height) {

}

CTCALL	BOOL	CTWindowLock(PCTWin window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowLock failed because window was NULL");
		return FALSE;
	}
	return CTLockEnter(window->lock);
}

CTCALL	BOOL	CTWindowUnlock(PCTWin window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowUnlock failed because window was NULL");
		return FALSE;
	}
	return CTLockLeave(window->lock);
}

CTCALL	BOOL	CTWindowSetTitle(PCTWin window, PCHAR title) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowSetTitle failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);
	SetWindowTextA(window->hwnd, title);
	CTWindowUnlock(window);

	return TRUE;
}

CTCALL	BOOL	CTWindowSetSize(PCTWin window, UINT32 width, UINT height) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowSetSize failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);
	SetWindowPos(
		window->hwnd, 
		NULL, 
		0, 0, 
		width, height, 
		SWP_NOMOVE
	);
	CTWindowUnlock(window);

	return TRUE;
}

CTCALL	BOOL	CTWindowSetFrameBuffer(PCTWin window, PCTFB frameBuffer) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowSetFrameBuffer failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);
	window->frameBuffer = frameBuffer;
	CTWindowUnlock(window);

	return TRUE;
}

CTCALL	BOOL	CTWindowUpdate(PCTWindow window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowUpdate failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);

	MSG messageBuff;
	PeekMessageA(&messageBuff, window->hwnd, 0, 0, PM_REMOVE);
	DispatchMessageA(&messageBuff);

	CTWindowUnlock(window);

	return TRUE;
}

CTCALL	BOOL	CTWindowDestroy(PCTWin window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowDestroy failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);
}
