//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_base_error.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_data.h"
#include "ct_base.h"

#include <stdio.h>
#include <intrin.h>

CTCALL	void		CTErrorSet(PCHAR message, DWORD type) {

	EnterCriticalSection(&__ctdata.base.errorLock);

	/// SUMMARY:
	/// copies message and severity to global last error struct
	/// then loops through list of error callbacks and calls them

	PCTErrMsg perr = &__ctdata.base.lastError;
	__stosb(
		perr->message,
		0,
		sizeof(perr->message)
	);
	__movsb(
		perr->message, 
		message, 
		strnlen_s(message, CT_ERRMSG_MESSAGE_MAX_SIZE)
	);
	perr->type		= type;
	perr->win32err	= GetLastError();

	PCTErrMsgCallbackNode node = __ctdata.base.errorCallbackList;
	while (node != NULL) {

		if (node->func != NULL)
			node->func(__ctdata.base.lastError);

		node = node->next;

	}

	LeaveCriticalSection(&__ctdata.base.errorLock);
}

CTCALL	CTErrMsg	CTErrorGet(void) {
	EnterCriticalSection(&__ctdata.base.errorLock);
	CTErrMsg msg = __ctdata.base.lastError;
	LeaveCriticalSection(&__ctdata.base.errorLock);
	return msg;
}

CTCALL	void		CTErrorAddCallback(PCTFUNCERRORCALLBACK pfErrCallback) {
	EnterCriticalSection(&__ctdata.base.errorLock);

	/// SUMMARY:
	/// inserts new callback node as first element of callback list
	
	PCTErrMsgCallbackNode oldFirstNode = __ctdata.base.errorCallbackList;
	PCTErrMsgCallbackNode newFirstNode = CTAlloc(sizeof(*newFirstNode));

	__ctdata.base.errorCallbackList	= newFirstNode;
	newFirstNode->func			= pfErrCallback;
	newFirstNode->next			= oldFirstNode;

	LeaveCriticalSection(&__ctdata.base.errorLock);
}
