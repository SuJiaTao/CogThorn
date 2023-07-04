///////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_logging.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_logging.h"

#include <stdio.h>
#include <interlockedapi.h>

DWORD __stdcall __CTLoggingThreadProc(PVOID input) {

	UINT64 spinTimeStart = 0;
	UINT64 spinTimeEnd   = 0;

	puts("thread enter");

	while (TRUE) {

		Sleep(
			max(
				0,
				CT_LOGGING_SLEEP_INTERVAL_MSECS - (spinTimeEnd - spinTimeStart)
			)
		);

		printf("running log cycle\n");

		spinTimeStart = GetTickCount64();
		
		CTLockEnter(__ctlog->lock);
		PCTIterator queueIter = CTIteratorCreate(__ctlog->logWriteQueue);

		PCTLogEntry entry;
		while ((entry = CTIteratorIterate(queueIter)) != NULL) {

			CTLockEnter(entry->logStream->lock);
			PCTFile writeFile = CTFileOpen(entry->logStream->streamName);

			// FORMAT MESSAGE
			// (mins.seconds) <THREADID> [MESSAGE TYPE]:
			//		message
			
			const UINT32 CURRENT_TIME_SECS = GetTickCount64() / 1000;
			const UINT32 CURRENT_TIME_MINS = CURRENT_TIME_SECS / 60;

			PCHAR writeBuffer = CTAlloc(CT_LOGGING_MAX_WRITE_SIZE);

			PCHAR messageTypeName = "Unkown";
			switch (entry->logType)
			{
			case CT_LOG_ENTRY_TYPE_INFO:
				messageTypeName = "Info";
				break;

			case CT_LOG_ENTRY_TYPE_INFO_IMPORTANT:
				messageTypeName = "Important Info";
				break;

			case CT_LOG_ENTRY_TYPE_WARNING:
				messageTypeName = "Warning";
				break;

			case CT_LOG_ENTRY_TYPE_FAILURE:
				messageTypeName = "Failure";
				break;

			default:
				break;
			}

			sprintf_s(
				writeBuffer,
				CT_LOGGING_MAX_WRITE_SIZE - 1,
				"(%d.%d) <%d> [%s]:\n\t%s\n",
				CURRENT_TIME_SECS % 60,
				CURRENT_TIME_MINS,
				entry->logThreadID,
				messageTypeName,
				entry->message
			);

			CTFileWrite(
				writeFile,
				writeBuffer,
				CTFileSize(writeFile),
				strnlen_s(writeBuffer, CT_LOGGING_MAX_WRITE_SIZE)
			);

			entry->logStream->logCount += 1;

			CTFree(writeBuffer);
			CTFileClose(writeFile);
			CTLockLeave(entry->logStream->lock);

		}

		CTIteratorDestroy(queueIter);
		CTDynListClean(__ctlog->logWriteQueue);

		CTLockLeave(__ctlog->lock);

		spinTimeEnd = GetTickCount64();

		if (__ctlog->killSignal == TRUE)
			ExitThread(ERROR_SUCCESS);

		printf("log cycle complete\n");
	}

}

CTCALL	PCTLogStream		CTLogStreamCreate(PCHAR streamName) {

	if (streamName == NULL) {
		CTErrorSetBadObject("CTLogStreamCreate failed: streamName was NULL");
		return NULL;
	}

	PCTFile logFile = CTFileCreate(streamName);
	if (logFile == NULL) {
		CTErrorSetFunction("CTLogStreamCreate failed: could not create log file");
		return NULL;
	}

	PCTLogStream ls = CTAlloc(sizeof(*ls));
	ls->lock		= CTLockCreate();
	ls->logCount	= 0;
	strcpy_s(
		ls->streamName,
		CT_LOGSTREAM_NAME_SIZE - 1,
		streamName
	);

	CTFileClose(logFile);

	return ls;
}

CTCALL	BOOL				CTLogStreamDestroy(PCTLogStream stream) {

	if (stream == NULL) {
		CTErrorSetBadObject("CTLogStreamDestroy failed: stream was NULL");
		return FALSE;
	}

	CTLockEnter(stream->lock);
	CTLockDestroy(stream->lock);
	CTFree(stream);

	return TRUE;

}

CTCALL	BOOL				CTLog(PCTLogStream stream, UINT32 logType, PCHAR message) {

	if (stream == NULL) {
		CTErrorSetBadObject("CTLog failed: stream was NULL");
		return FALSE;
	}

	if (message == NULL) {
		CTErrorSetBadObject("CTLog failed: message was NULL");
		return FALSE;
	}

	CTLockEnter(__ctlog->lock);
	CTLockEnter(stream->lock);

	CTDynListLock(__ctlog->logWriteQueue);

	PCTLogEntry pentry = CTDynListAdd(__ctlog->logWriteQueue);
	pentry->logStream = stream;
	pentry->logThreadID = GetThreadId(GetCurrentThread());
	pentry->logType		= logType;
	strcpy_s(
		pentry->message,
		CT_LOG_MESSAGE_SIZE - 1,
		message
	);

	CTDynListUnlock(__ctlog->logWriteQueue);

	CTLockLeave(stream->lock);
	CTLockLeave(__ctlog->lock);

}

CTCALL	BOOL				CTLogFormatted(PCTLogStream stream, UINT32 logTYpe, PCHAR message, ...) {

}