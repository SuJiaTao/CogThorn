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

//////////////////////////////////////////////////////////////////////////////
///
///							LOGGING THREAD PROC
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_LOGGING_SLEEP_INTERVAL_MSECS		500
#define CT_LOGGING_MAX_WRITE_SIZE			512
DWORD __stdcall __CTLoggingThreadProc(PVOID input);

//////////////////////////////////////////////////////////////////////////////
///
///							LOGGING STREAMS
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_LOGSTREAM_NAME_SIZE				0x80
typedef struct CTLogStream {
	CHAR	streamName [CT_LOGSTREAM_NAME_SIZE];
	UINT64	logCount;
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

CTCALL	PCTLogStream		CTLogStreamCreate(PCHAR streamName);
CTCALL	BOOL				CTLogStreamDestroy(PCTLogStream stream);
CTCALL	BOOL				CTLog(PCTLogStream stream, UINT32 logType, PCHAR message);
CTCALL	BOOL				CTLogFormatted(PCTLogStream stream, UINT32 logType, PCHAR message, ...);

//////////////////////////////////////////////////////////////////////////////
///
///							LOGGING MODULE INSTANCE
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_LOGGING_QUEUE_NODE_SIZE	1024
typedef struct CTLogging {

	INT64		startTimeMsecs;
	PCTLock		lock;
	HANDLE		logWriteThread;
	PCTDynList	logWriteQueue;
	BOOL		killSignal;

} CTLogging, *PCTLogging;
PCTLogging __ctlog;			/// INSTANCE ///

#endif
