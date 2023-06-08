//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_base.h>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#ifndef _CT_BASE_INCLUDE_
#define _CT_BASE_INCLUDE_ 

#ifndef COGTHORN_EXPORTS
#define CTCALL __declspec(dllimport)
#else
#define CTCALL __declspec(dllexport)
#endif

#include <Windows.h>

//////////////////////////////////////////////////////////////////////////////
///
///								MEMORY MANAGEMENT
/// 
//////////////////////////////////////////////////////////////////////////////

CTCALL	PVOID	CTAlloc(SIZE_T sizeBytes);
CTCALL	void	CTFree(PVOID ptr);
CTCALL	SIZE_T	CTAllocCount(void);
CTCALL	SIZE_T	CTAllocSizeBytes(void);

//////////////////////////////////////////////////////////////////////////////
///
///								ERROR HANDLING
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_ERRMSG_TYPE_FUNCTION		0
#define CT_ERRMSG_TYPE_PARAM_VALUE	1
#define CT_ERRMSG_TYPE_BAD_OBJECT	2
#define CT_ERRMSG_MESSAGE_MAX_SIZE	0xFF

typedef struct CTErrMsg {
	DWORD win32err;
	DWORD type;
	CHAR  message	[CT_ERRMSG_MESSAGE_MAX_SIZE];
} CTErrMsg, *PCTErrMsg;

typedef void (*PCTFUNCERRORCALLBACK)(CTErrMsg msg);

typedef struct CTErrMsgCallbackNode {
	PCTFUNCERRORCALLBACK	func;
	PVOID					next;
} CTErrMsgCallbackNode, * PCTErrMsgCallbackNode;

CTCALL	void		CTErrorSet(PCHAR message, DWORD type);
CTCALL	CTErrMsg	CTErrorGet(void);
CTCALL	void		CTErrorAddCallback(PCTFUNCERRORCALLBACK pfErrCallback);

#define CTErrorSetFunction(msg)		CTErrorSet(msg, CT_ERRMSG_TYPE_FUNCTION)
#define CTErrorSetParamValue(msg)	CTErrorSet(msg, CT_ERRMSG_TYPE_PARAM_VALUE)
#define CTErrorSetBadObject(msg)	CTErrorSet(msg, CT_ERRMSG_TYPE_BAD_OBJECT)

//////////////////////////////////////////////////////////////////////////////
///
///								FILE I/O
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTFile {
	HANDLE hFile;
} CTFile, *PCTFile;

CTCALL	BOOL		CTFileExists(PCHAR path);
CTCALL	PCTFile		CTFileCreate(PCHAR path);
CTCALL	BOOL		CTFileDelete(PCHAR path);
CTCALL	PCTFile		CTFileOpen(PCHAR path);
CTCALL	BOOL		CTFileClose(PCTFile file);
CTCALL	SIZE_T		CTFileSize(PCTFile file);
CTCALL	BOOL		CTFileRead(PCTFile file, PVOID buffer, SIZE_T offset, SIZE_T sizeBytes);
CTCALL	BOOL		CTFileWrite(PCTFile file, PVOID buffer, SIZE_T offset, SIZE_T sizeBytes);

//////////////////////////////////////////////////////////////////////////////
///
///								THREAD SYNCHRONIZATION
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTLock {
	CRITICAL_SECTION lock;
} CTLock, *PCTLock;

CTCALL	PCTLock		CTLockCreate(void);
CTCALL	BOOL		CTLockDestroy(PCTLock lock);
CTCALL	BOOL		CTLockEnter(PCTLock lock);
CTCALL	BOOL		CTLockLeave(PCTLock lock);

//////////////////////////////////////////////////////////////////////////////
///
///								DYNAMIC LIST
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTDynListNode {
	PBYTE	elements;
	UINT32	elementUseCount;
	PBYTE	useField;
	PVOID	nextNode;
} CTDynListNode, *PCTDynListNode;

typedef struct CTDynList {
	CRITICAL_SECTION	lock;
	PCTDynListNode		nodeFirst;
	PCTDynListNode		nodeLast;
	UINT32				nodeCount;
	SIZE_T				elementSizeBytes;
	UINT32				elementsPerNode;
	UINT32				elementsUsedCount;
	UINT32				elementsTotalCount;
} CTDynList, *PCTDynList;

typedef struct CTIterator {
	PCTDynList		parent;
	PCTDynListNode	currentNode;
	UINT32			currentNodeIndex;
} CTIterator, *PCTIterator;

CTCALL	PCTDynList	CTDynListCreate(SIZE_T elemSize, UINT32 elemsPerNode);
CTCALL	BOOL		CTDynListDestroy(PCTDynList list);
CTCALL	BOOL		CTDynListClear(PCTDynList list);
CTCALL	BOOL		CTDynListLock(PCTDynList list);
CTCALL	BOOL		CTDynListUnlock(PCTDynList list);
CTCALL	PVOID		CTDynListAdd(PCTDynList list);
CTCALL	BOOL		CTDynListRemove(PCTDynList list, PVOID element);
CTCALL	PCTIterator	CTIteratorCreate(PCTDynList list);
CTCALL	BOOL		CTIteratorDestroy(PCTIterator iterator);
CTCALL	PVOID		CTIteratorIterate(PCTIterator iterator);

//////////////////////////////////////////////////////////////////////////////
///
///								BASE MODULE INSTANCE
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTBase {
	HANDLE					heap;
	SIZE_T					heapAllocCount;
	SIZE_T					heapAllocBytes;
	CRITICAL_SECTION		errorLock;
	CTErrMsg				lastError;
	PCTErrMsgCallbackNode	errorCallbackList;
} CTBase, *PCTBase;
PCTBase __ctbase;	/// INSTANCE ///

#endif
