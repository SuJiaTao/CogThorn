///////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_logging.h>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#ifndef _CT_LOGGING_INCLUDE_
#define _CT_LOGGING_INCLUDE_ 

#include "ct_base.h"
#include <varargs.h>

#define CT_LOGGING_QUEUE_NODE_SIZE 1024

//////////////////////////////////////////////////////////////////////////////
///
///							LOGGING THREAD PROC
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_LOGGING_SLEEP_INTERVAL_MSECS		250
#define CT_LOGGING_MAX_WRITE_SIZE			512
DWORD __stdcall __CTLoggingThreadProc(PVOID input);

//////////////////////////////////////////////////////////////////////////////
///
///							LOGGING STREAMS
/// 
//////////////////////////////////////////////////////////////////////////////

typedef void (*PCTFUNCLOGHOOK)(PVOID entry, PVOID hookInput);

#define CT_LOGSTREAM_NAME_SIZE				0x80
#define CT_LOGSTREAM_DESTROY_SPIN_DELAY		1
typedef struct CTLogStream {
	BOOL			destroySignal;
	PCTFUNCLOGHOOK	logHook;
	PVOID			hookInput;
	CHAR			streamName [CT_LOGSTREAM_NAME_SIZE];
	UINT64			logCount;
	UINT64			logsOutstanding;
} CTLogStream, *PCTLogStream;

#define CT_LOG_ENTRY_TYPE_INFO				0
#define CT_LOG_ENTRY_TYPE_INFO_IMPORTANT	1
#define CT_LOG_ENTRY_TYPE_WARNING			2
#define CT_LOG_ENTRY_TYPE_FAILURE			3
#define CT_LOG_MESSAGE_SIZE					0xFF
typedef struct CTLogEntry {
	UINT64			logNumber;
	UINT64			logTimeMsecs;
	DWORD			logThreadID;
	PCTLogStream	logStream;
	UINT32			logType;
	CHAR			message[CT_LOG_MESSAGE_SIZE];
} CTLogEntry, * PCTLogEntry;

CTCALL	PCTLogStream		CTLogStreamCreate(PCHAR streamName, PCTFUNCLOGHOOK logHook, PVOID hookInput);
CTCALL	BOOL				CTLogStreamDestroy(PCTLogStream* pStream);
CTCALL	BOOL				CTLog(PCTLogStream stream, UINT32 logType, PCHAR message);
CTCALL	BOOL				CTLogFormatted(PCTLogStream stream, UINT32 logType, PCHAR message, ...);

#define CTLogInfo(stream, msg, ...)			\
	CTLogFormatted(stream, CT_LOG_ENTRY_TYPE_INFO, msg, __VA_ARGS__ )
#define CTLogImportant(stream, msg, ...)	\
	CTLogFormatted(stream, CT_LOG_ENTRY_TYPE_INFO_IMPORTANT, msg, __VA_ARGS__ )
#define CTLogWarning(stream, msg, ...)		\
	CTLogFormatted(stream, CT_LOG_ENTRY_TYPE_WARNING, msg, __VA_ARGS__ )
#define CTLogFailure(stream, msg, ...)		\
	CTLogFormatted(stream, CT_LOG_ENTRY_TYPE_FAILURE, msg, __VA_ARGS__ )

#endif
