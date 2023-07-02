//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_window.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_window.h"

#include <stdio.h>

static LRESULT CALLBACK __HCTWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {

	if (message == WM_CREATE) {
		PCTWindow userWindow = (PCTWindow)((LPCREATESTRUCTA)lParam)->lpCreateParams;
		SetWindowLongPtrA(window, GWLP_USERDATA, userWindow);
	}

	PVOID ptr = GetWindowLongPtrA(window, GWLP_USERDATA);

	return DefWindowProcA(window, message, wParam, lParam);
}

CTCALL	PCTWin	CTWindowCreate(PCHAR title, UINT32 width, UINT32 height) {

	PCTWin window = CTGFXAlloc(sizeof(*window));
	window->frameBuffer = NULL;
	window->lock = CTLockCreate();

	sprintf_s(
		window->wndClassName,
		sizeof(window->wndClassName) - 1,
		"CTWindow%p\0",
		window
	);

	WNDCLASSA windowClass;
	ZeroMemory(&windowClass, sizeof(windowClass));
	windowClass.lpszClassName = window->wndClassName;
	windowClass.lpfnWndProc = __HCTWindowProc;

	ATOM classRegRslt = RegisterClassA(&windowClass);
	if (classRegRslt == NULL) {
		CTErrorSetFunction("CTWindowCreated failed: RegisterClassA failed");
		return NULL;
	}

	RECT clientRect = {
		.bottom = height,
		.top = 0,
		.left = 0,
		.right = width
	};

	AdjustWindowRect(&clientRect, CT_WINDOW_STYLE, TRUE);

	window->hwnd = CreateWindowExA(
		NULL,
		window->wndClassName,
		title,
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		clientRect.right - clientRect.left,
		clientRect.bottom - clientRect.top,
		NULL,
		NULL,
		NULL,
		window
	);

	if (window->hwnd == NULL) {
		CTErrorSetFunction("CTWindowCreated failed: CreateWindowExA failed");
		return NULL;
	}

	SetWindowLongA(window->hwnd, GWL_STYLE, CT_WINDOW_STYLE);

	return window;
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

	BOOL rslt = SetWindowTextA(window->hwnd, title);
	if (rslt == FALSE) {
		CTErrorSetFunction("CTWindowSetTitle failed: SetWindowTextA failed");
	}

	CTWindowUnlock(window);

	return rslt;
}

CTCALL	BOOL	CTWindowSetSize(PCTWin window, UINT32 width, UINT height) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowSetSize failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);
	BOOL rslt = SetWindowPos(
		window->hwnd, 
		NULL, 
		0, 0, 
		width, height, 
		SWP_NOMOVE
	);
	if (rslt == FALSE) {
		CTErrorSetFunction("CTWindowSetSize failed: SetWindowPos failed");
	}

	CTWindowUnlock(window);

	return rslt;
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
