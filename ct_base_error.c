//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_base_error.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"

#include <stdio.h>
#include <intrin.h>

CTCALL	void		CTErrorSet(PCHAR message, DWORD type) {

	EnterCriticalSection(&__ctbase->errorLock);

	/// SUMMARY:
	/// copies message and severity to global last error struct
	/// then loops through list of error callbacks and calls them

	PCTErrMsg perr = &__ctbase->lastError;
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

	PCTErrMsgCallbackNode node = __ctbase->errorCallbackList;
	while (node != NULL) {

		if (node->func != NULL)
			node->func(__ctbase->lastError);

		node = node->next;

	}

	LeaveCriticalSection(&__ctbase->errorLock);
}

CTCALL	CTErrMsg	CTErrorGet(void) {
	EnterCriticalSection(&__ctbase->errorLock);
	CTErrMsg msg = __ctbase->lastError;
	LeaveCriticalSection(&__ctbase->errorLock);
	return msg;
}

CTCALL	void		CTErrorAddCallback(PCTFUNCERRORCALLBACK pfErrCallback) {
	EnterCriticalSection(&__ctbase->errorLock);

	/// SUMMARY:
	/// inserts new callback node as first element of callback list
	
	PCTErrMsgCallbackNode oldFirstNode = __ctbase->errorCallbackList;
	PCTErrMsgCallbackNode newFirstNode = CTAlloc(sizeof(*newFirstNode));

	__ctbase->errorCallbackList	= newFirstNode;
	newFirstNode->func			= pfErrCallback;
	newFirstNode->next			= oldFirstNode;

	LeaveCriticalSection(&__ctbase->errorLock);
}
