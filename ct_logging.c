///////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_logging.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_data.h"
#include "ct_logging.h"

#include <stdio.h>
#include <interlockedapi.h>

DWORD __stdcall __CTLoggingThreadProc(PVOID input) {

	/// SUMMARY:
	/// loop (forever)
	/// 
	///		if (no queued elements)
	///			sleep to satisfy sleep interval
	/// 
	///		record spin start time
	///		
	///		ENTER LOCK
	///		
	///		for (all queued log entries)
	///			copy to write list
	/// 
	///		CLEAR QUEUE
	///		
	///		if (NOT kill signal)
	///			LEAVE LOCK
	/// 
	///		for (all in write list)
	///			if (file is NULL)
	///				open specified file
	///			if (file has changed)
	///				close file
	///				open specified file
	///			if (log hook exists)
	///				call log hook
	///			format log
	///			write to file
	///			decrement stream's outstanding log count
	///			
	///		if (file is unclosed)
	///			close file	
	/// 
	///		if (recieived kill signal)
	///			terminate thread
	/// 
	///		record spin end time

	INT64 SPIN_TIME_START		= 0;
	INT64 SPIN_TIME_END			= CT_LOGGING_SLEEP_INTERVAL_MSECS;
	PCTLogEntry LOG_ENTRY		= NULL;
	PCTDynList logWriteBuffer	= CTDynListCreate(sizeof(CTLogEntry), CT_LOGGING_QUEUE_NODE_SIZE);

	while (TRUE) {

		INT64 SPIN_TIME_TOTAL = SPIN_TIME_END - SPIN_TIME_START;

		if (__ctdata.logging.logWriteQueue->elementsUsedCount == 0) {
			Sleep(max(0, CT_LOGGING_SLEEP_INTERVAL_MSECS - SPIN_TIME_TOTAL));
		}

		SPIN_TIME_START = GetTickCount64();

		CTLockEnter(__ctdata.logging.lock);

		PCTIterator logQueueIter = CTIteratorCreate(__ctdata.logging.logWriteQueue);
		while ((LOG_ENTRY = CTIteratorIterate(logQueueIter)) != NULL) {
			PCTLogEntry entry = CTDynListAdd(logWriteBuffer);
			*entry = *LOG_ENTRY;
		}
		CTIteratorDestroy(&logQueueIter);
		CTDynListClear(__ctdata.logging.logWriteQueue);

		if (__ctdata.logging.killSignal == FALSE) {
			CTLockLeave(__ctdata.logging.lock);
		}

		PCTIterator writeBufferIter = CTIteratorCreate(logWriteBuffer);
		PCTFile		logFile			= NULL;
		while ( (LOG_ENTRY = CTIteratorIterate(writeBufferIter)) != NULL) {

			if (logFile == NULL) {
				logFile = CTFileOpen(LOG_ENTRY->logStream->streamName);
				if (logFile == NULL) {
					CTErrorSetFunction("__CTLoggingThreadProc encountered fatal error: could not open file");
					continue;
				}
			}

			if (strcmp(logFile->fileName, LOG_ENTRY->logStream->streamName) != 0) {
				CTFileClose(&logFile);
				logFile = CTFileOpen(LOG_ENTRY->logStream->streamName);
			}

			if (LOG_ENTRY->logStream->logHook != NULL) {
				LOG_ENTRY->logStream->logHook(
					LOG_ENTRY,
					LOG_ENTRY->logStream->hookInput
				);
			}

			PCHAR	logFmtBuffer	= CTAlloc(CT_LOGGING_MAX_WRITE_SIZE);
			PCHAR	logTypeString	= "Unknown";
			switch (LOG_ENTRY->logType)
			{

			case CT_LOG_ENTRY_TYPE_INFO:
				logTypeString = "Info";
				break;

			case CT_LOG_ENTRY_TYPE_INFO_IMPORTANT:
				logTypeString = "Imporant Info";
				break;

			case CT_LOG_ENTRY_TYPE_WARNING:
				logTypeString = "Warning";
				break;

			case CT_LOG_ENTRY_TYPE_FAILURE:
				logTypeString = "Failure";
				break;

			default:
				break;

			}

			INT64 totalTimeSecs = LOG_ENTRY->logTimeMsecs / 1000;
			INT32 timeHours		= totalTimeSecs  / 3600;
			INT32 timeMins		= (totalTimeSecs / 60) % 60;
			INT32 timeSecs		= (totalTimeSecs) % 60;
			INT32 timeMsecs		= LOG_ENTRY->logTimeMsecs % 1000;

			sprintf_s(
				logFmtBuffer,
				CT_LOGGING_MAX_WRITE_SIZE - 1,
				"\n<%08d> ( %02dh : %02dm : %02ds : %03dms ) [ THREAD ID: %X ] [ %s ] \n%s\n",
				(INT32)LOG_ENTRY->logNumber,
				timeHours,
				timeMins,
				timeSecs,
				timeMsecs,
				LOG_ENTRY->logThreadID,
				logTypeString,
				LOG_ENTRY->message
			);

			CTFileWrite(
				logFile,
				logFmtBuffer,
				CTFileSize(logFile),
				strnlen_s(logFmtBuffer, CT_LOGGING_MAX_WRITE_SIZE)
			);

			CTFree(logFmtBuffer);

			InterlockedAdd64(&LOG_ENTRY->logStream->logsOutstanding, -1);

		}

		if (logFile != NULL)
			CTFileClose(&logFile);

		CTIteratorDestroy(&writeBufferIter);
		CTDynListClear(logWriteBuffer);

		SPIN_TIME_END = GetTickCount64();

		if (__ctdata.logging.killSignal == TRUE) {
			CTDynListDestroy(&logWriteBuffer);
			ExitThread(ERROR_SUCCESS);
		}
		
	}

}

CTCALL	PCTLogStream		CTLogStreamCreate(PCHAR streamName, PCTFUNCLOGHOOK logHook, PVOID hookInput) {

	if (streamName == NULL) {
		CTErrorSetBadObject("CTLogStreamCreate failed: streamName was NULL");
		return NULL;
	}

	PCTFile logFile = CTFileCreate(streamName);
	if (logFile == NULL) {
		CTErrorSetFunction("CTLogStreamCreate failed: could not create log file");
		return NULL;
	}

	PCTLogStream ls		= CTAlloc(sizeof(*ls));
	ls->logCount		= 0;
	ls->logsOutstanding	= 0;
	ls->logHook			= logHook;
	ls->hookInput		= hookInput;
	ls->destroySignal	= FALSE;
	strcpy_s(
		ls->streamName,
		CT_LOGSTREAM_NAME_SIZE - 1,
		streamName
	);

	SYSTEMTIME streamInitTime;
	GetLocalTime(&streamInitTime);

	PCHAR fileHeaderFmtBuffer = CTAlloc(CT_LOGGING_MAX_WRITE_SIZE);
	sprintf_s(
		fileHeaderFmtBuffer,
		CT_LOGGING_MAX_WRITE_SIZE - 1,
		"Filename: < %s >\nCreated at: [ %dh : %dm : %ds : %dms ]\n",
		streamName,
		streamInitTime.wHour,
		streamInitTime.wMinute,
		streamInitTime.wSecond,
		streamInitTime.wMilliseconds
	);

	CTFileWrite(
		logFile,
		fileHeaderFmtBuffer,
		CTFileSize(logFile),
		strnlen_s(fileHeaderFmtBuffer, CT_LOGGING_MAX_WRITE_SIZE)
	);
	CTFree(fileHeaderFmtBuffer);

	CTFileClose(&logFile);

	return ls;

}

CTCALL	BOOL				CTLogStreamDestroy(PCTLogStream* pStream) {

	if (pStream == NULL) {
		CTErrorSetBadObject("CTLogStreamDestroy failed: pStream was NULL");
		return FALSE;
	}

	CTLockEnter(__ctdata.logging.lock);

	PCTLogStream stream = *pStream;

	if (stream == NULL) {

		CTErrorSetBadObject("CTLogStreamDestroy failed: stream was NULL");
		CTLockLeave(__ctdata.logging.lock);

		return FALSE;

	}

	if (stream->destroySignal == TRUE) {

		CTErrorSetBadObject("CTLogStreamDestroy failed: stream is already being destroyed");
		CTLockLeave(__ctdata.logging.lock);

		return FALSE;

	}

	stream->destroySignal = TRUE;

	CTLockLeave(__ctdata.logging.lock);

	// wait for outstanding count to become 0
	while (stream->logsOutstanding > 0) {
		Sleep(CT_LOGSTREAM_DESTROY_SPIN_DELAY);
	}

	CTLockEnter(__ctdata.logging.lock);
	CTFree(stream);
	CTLockLeave(__ctdata.logging.lock);

	*pStream = NULL;
	return TRUE;

}

CTCALL	BOOL				CTLog(PCTLogStream stream, UINT32 logType, PCHAR message) {

	CTLockEnter(__ctdata.logging.lock);

	if (stream == NULL) {

		CTErrorSetBadObject("CTLog failed: stream was NULL");
		CTLockLeave(__ctdata.logging.lock);

		return FALSE;

	}

	if (message == NULL) {

		CTErrorSetBadObject("CTLog failed: message was NULL");
		CTLockLeave(__ctdata.logging.lock);

		return FALSE;

	}

	if (stream->destroySignal == TRUE) {

		CTErrorSetBadObject("CTLogStreamDestroy failed: stream is being destroyed");
		CTLockLeave(__ctdata.logging.lock);

		return FALSE;

	}

	CTDynListLock(__ctdata.logging.logWriteQueue);

	PCTLogEntry pentry		= CTDynListAdd(__ctdata.logging.logWriteQueue);
	pentry->logStream		= stream;
	pentry->logThreadID		= GetThreadId(GetCurrentThread());
	pentry->logType			= logType;
	pentry->logNumber		= stream->logCount++;
	pentry->logTimeMsecs	= GetTickCount64() - __ctdata.logging.startTimeMsecs;

	InterlockedAdd64(&stream->logsOutstanding, 1);

	strcpy_s(
		pentry->message,
		CT_LOG_MESSAGE_SIZE - 1,
		message
	);

	CTDynListUnlock(__ctdata.logging.logWriteQueue);

	CTLockLeave(__ctdata.logging.lock);

	return TRUE;

}

CTCALL	BOOL				CTLogFormatted(PCTLogStream stream, UINT32 logType, PCHAR message, ...) {

	va_list args;
	va_start(args, message);

	CHAR fmtBuffer [CT_ERRMSG_MESSAGE_MAX_SIZE];
	ZeroMemory(&fmtBuffer, sizeof(fmtBuffer));

	vsprintf_s(
		fmtBuffer,
		sizeof(fmtBuffer) - 1,
		message,
		args
	);

	BOOL logFuncRslt = CTLog(
		stream,
		logType,
		fmtBuffer
	);
	
	va_end(args);

	return logFuncRslt;

}