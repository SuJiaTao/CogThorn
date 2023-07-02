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

	PCTWin ctwin = GetWindowLongPtrA(window, GWLP_USERDATA);

	if (message == WM_CLOSE) {
		ctwin->shouldClose = TRUE;
		return TRUE;
	}

	if (message == CT_WINDOW_CLOSEMESSAGE) {
		UnregisterClassA(ctwin->wndClassName, NULL);
		CTLockDestroy(ctwin->lock);
		CTGFXFree(ctwin);

		message = WM_CLOSE;

		goto __CTWinProcEnd;
	}

	if (message == WM_ERASEBKGND) {
		return TRUE;
	}

	if (message == WM_PAINT) {

		if (ctwin == NULL) {
			CTErrorSetFunction("__HCTWindowProc failed because GWLP_USERDATA was overwritten or corrupted.");
			goto __CTWinProcEnd;
		}

		CTWindowLock(ctwin);

		if (ctwin->frameBuffer == NULL) {
			goto __CTWinProcEnd;
		}

		PAINTSTRUCT paintObj;
		HDC paintDC = BeginPaint(ctwin->hwnd, &paintObj);

		PCTFB frameBuffer = ctwin->frameBuffer;

		BITMAP rbBitmap;
		rbBitmap.bmType			= 0;
		rbBitmap.bmWidth		= frameBuffer->width;
		rbBitmap.bmHeight		= frameBuffer->height;
		rbBitmap.bmWidthBytes	= frameBuffer->width * sizeof(CTColor);
		rbBitmap.bmPlanes		= 1;
		rbBitmap.bmBitsPixel	= 32;
		rbBitmap.bmBits			= frameBuffer->color;

		HBITMAP hBitMap = CreateBitmapIndirect(&rbBitmap);

		HDC bitmapDC = CreateCompatibleDC(paintDC);
		SelectObject(bitmapDC, hBitMap);

		BLENDFUNCTION blendFunc;
		ZeroMemory(&blendFunc, sizeof(blendFunc));
		blendFunc.BlendOp = AC_SRC_OVER;
		blendFunc.BlendFlags = NULL;
		blendFunc.SourceConstantAlpha = 255;
		blendFunc.BlendFlags = NULL;

		RECT drawRect;
		GetClientRect(ctwin->hwnd, &drawRect);
		DWORD drawAreaWidth		= drawRect.right - drawRect.left;
		DWORD drawAreaHeight	= drawRect.bottom - drawRect.top;

		BOOL drawResult = AlphaBlend(
			paintDC, 0, 0,
			drawAreaWidth, 
			drawAreaHeight,
			bitmapDC, 0, 0, 
			frameBuffer->width, 
			frameBuffer->height,
			blendFunc
		);

		DeleteObject(hBitMap);
		DeleteObject(bitmapDC);

		EndPaint(ctwin->hwnd, &paintObj);

		CTWindowUnlock(ctwin);
	}

__CTWinProcEnd:
	return DefWindowProcA(window, message, wParam, lParam);
}

CTCALL	PCTWin	CTWindowCreate(PCHAR title, UINT32 width, UINT32 height) {

	PCTWin window = CTGFXAlloc(sizeof(*window));
	window->frameBuffer = NULL;
	window->lock		= CTLockCreate();
	window->shouldClose	= FALSE;

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

	InvalidateRect(window->hwnd, NULL, FALSE);
	UpdateWindow(window->hwnd);

	MSG messageBuff;
	PeekMessageA(&messageBuff, window->hwnd, 0, 0, PM_REMOVE);
	DispatchMessageA(&messageBuff);

	CTWindowUnlock(window);

	return TRUE;
}

CTCALL	BOOL	CTWindowShouldClose(PCTWindow window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowShouldClose failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);
	BOOL value = window->shouldClose;
	CTWindowUnlock(window);

	return value;
}

CTCALL	BOOL	CTWindowSetShouldClose(PCTWindow window, BOOL state) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowShouldClose failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);
	window->shouldClose = state;
	CTWindowUnlock(window);

	return TRUE;
}

CTCALL	BOOL	CTWindowDestroy(PCTWin window) {
	if (window == NULL) {
		CTErrorSetBadObject("CTWindowDestroy failed because window was NULL");
		return FALSE;
	}

	CTWindowLock(window);

	SendMessageA(window->hwnd, CT_WINDOW_CLOSEMESSAGE, NULL, NULL);

	return TRUE;
}
