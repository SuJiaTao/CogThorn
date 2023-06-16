//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_window.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_window.h"

#include <stdio.h>

static LRESULT CALLBACK __HCTWindowProc(HWND win, UINT msg, WPARAM wP, LPARAM lP) {


	return DefWindowProcA(win, msg, wP, lP);
}

CTCALL	PCTWin	CTWindowCreate(PCHAR title, UINT32 width, UINT32 height) {
	/// SUMMARY:
	/// allocate window and setup members
	/// generate unique window class name based on window
	/// create window class
	/// create window rect from client rect
	/// register window class
	/// create window
	/// set window user data to CTWindow ptr
	
	if (width == 0 || height == 0) {
		CTErrorSetParamValue("CTWindowCreate failed because width or height was invalid");
		return NULL;
	}

	PCTWin rWindow = CTGFXAlloc(sizeof(*rWindow));

	rWindow->lock					= CTLockCreate();
	rWindow->shouldClose			= FALSE;
	rWindow->frameBuffer			= NULL;
	rWindow->frameBufferFitMethod	= CT_WINDOW_FB_FIT_MAX_DIMENSION;
	
	CHAR wndNameBuffer[MAX_PATH] = { 0 };
	sprintf_s(
		wndNameBuffer,
		sizeof(wndNameBuffer),
		"CTWindow %p",
		rWindow
	);

	rWindow->wndClassName = CTGFXAlloc(strlen(wndNameBuffer) + 1);
	strcpy(rWindow->wndClassName, wndNameBuffer);

	WNDCLASSA windowClass;
	ZeroMemory(&windowClass, sizeof(windowClass));
	windowClass.lpszClassName	= rWindow->wndClassName;
	windowClass.lpfnWndProc		= __HCTWindowProc;
	RegisterClassA(&windowClass);

	DWORD windowFlags	= WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
	RECT clientRect		= {
		.bottom = 0,
		.left	= 0,
		.top	= height,
		.right	= width
	};

	AdjustWindowRectExForDpi(&clientRect, windowFlags, TRUE, windowFlags,
		GetDpiForSystem());

	DWORD realWinWidth	= clientRect.right - clientRect.left;
	DWORD realWinHeight = clientRect.bottom - clientRect.top;

	rWindow->wndHandle = CreateWindowExA(
		0L, 
		rWindow->wndClassName, 
		title,
		windowFlags, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT,
		realWinWidth, 
		realWinHeight, 
		NULL, 
		NULL, 
		NULL, 
		NULL
	);

	SetWindowLongPtrA(
		rWindow->wndHandle,
		GWLP_USERDATA,
		rWindow
	);

	ShowWindow(rWindow->wndHandle, SW_SHOW);
	
	return rWindow;
}

CTCALL	BOOL	CTWindowSetTitle(PCTWin window, PCHAR title) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowSetTitle failed: window was NULL");
		return FALSE;
	}

	CTLockEnter(window->lock);

	SetWindowTextA(window->wndHandle, title);

	CTLockLeave(window->lock);
	return TRUE;
}

CTCALL	BOOL	CTWindowSetSize(PCTWin window, UINT32 width, UINT height) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowSetSize failed: window was NULL");
		return FALSE;
	}
	if (width == 0 || height == 0) {
		CTErrorSetBadObject("CTWindowSetSize failed: width or height was invalid");
		return FALSE;
	}

	CTLockEnter(window->lock);
	SetWindowPos(window->wndHandle, NULL, 0, 0, width, height, SWP_NOMOVE);
	CTLockLeave(window->lock);

	return TRUE;
}

CTCALL	BOOL	CTWindowShouldClose(PCTWindow window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowShouldClose failed: window was NULL");
		return FALSE;
	}

	CTLockEnter(window->lock);
	BOOL state = window->shouldClose;
	CTLockLeave(window->lock);

	return state;
}

CTCALL	BOOL	CTWindowSetShouldClose(PCTWindow window, BOOL state) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowSetShouldClose failed: window was NULL");
		return FALSE;
	}

	CTLockEnter(window->lock);
	window->shouldClose = state;
	CTLockLeave(window->lock);

	return TRUE;
}

CTCALL	BOOL	CTWindowUpdate(PCTWindow window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowUpdate failed: window was NULL");
		return FALSE;
	}

	CTLockEnter(window->lock);

	InvalidateRect(window->wndHandle, NULL, FALSE);
	UpdateWindow(window->wndHandle);

	MSG messageBuff;
	PeekMessageA(&messageBuff, window->wndHandle, 0, 0, PM_REMOVE);
	DispatchMessageA(&messageBuff);

	CTLockLeave(window->lock);
	return TRUE;
}

CTCALL	BOOL	CTWindowDestroy(PCTWin window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowDestroy failed: window was NULL");
		return FALSE;
	}

	CTLockEnter(window->lock);
	CTLockDestroy(window->lock);

	CTGFXFree(window->wndClassName);
	CTGFXFree(window);
	
	return TRUE;
}
