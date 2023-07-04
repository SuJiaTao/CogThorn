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
	///		LEAVE LOCK
	/// 
	///		for (all in write list)
	///			open specified file
	///			format log
	///			write to file
	///			close file
	///		
	///		if (recieived kill signal)
	///			terminate thread
	/// 
	///		record spin end time

	INT64 SPIN_TIME_START	= 0;
	INT64 SPIN_TIME_END		= 0;
	PCTLogEntry LOG_ENTRY	= NULL;
	PCTDynList logWriteBuffer	= CTDynListCreate(sizeof(CTLogEntry), CT_LOGGING_QUEUE_NODE_SIZE);

	while (TRUE) {

		INT64 SPIN_TIME_TOTAL = SPIN_TIME_END - SPIN_TIME_START;

		if (__ctlog->logWriteQueue->elementsUsedCount == 0) {
			Sleep(max(0, CT_LOGGING_SLEEP_INTERVAL_MSECS - SPIN_TIME_TOTAL));
		}

		CTLockEnter(__ctlog->lock);
		SPIN_TIME_START = GetTickCount64();

		PCTIterator logQueueIter = CTIteratorCreate(__ctlog->logWriteQueue);
		while ((LOG_ENTRY = CTIteratorIterate(logQueueIter)) != NULL) {
			PCTLogEntry entry = CTDynListAdd(logWriteBuffer);
			*entry = *LOG_ENTRY;
		}
		CTIteratorDestroy(logQueueIter);
		CTDynListClear(__ctlog->logWriteQueue);

		CTLockLeave(__ctlog->lock);

		PCTIterator writeBufferIter = CTIteratorCreate(logWriteBuffer);
		PCTFile		logFile			= NULL;
		while ( (LOG_ENTRY = CTIteratorIterate(writeBufferIter)) != NULL) {

			if (logFile == NULL) {
				logFile = CTFileOpen(LOG_ENTRY->logStream->streamName);
			}

			if (strcmp(logFile->fileName, LOG_ENTRY->logStream->streamName) != 0) {
				CTFileClose(logFile);
				logFile = CTFileOpen(LOG_ENTRY->logStream->streamName);
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
				logTypeString = "Failue";
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
				"<%08d> ( %02dh : %02dm : %02ds : %03dms ) [ THREADID: %X ] [ %s ] \n\t%s\n",
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

		}

		if (logFile != NULL)
			CTFileClose(logFile);

		CTIteratorDestroy(writeBufferIter);
		CTDynListClear(logWriteBuffer);

		SPIN_TIME_END = GetTickCount64();

		if (__ctlog->killSignal == TRUE) {
			CTDynListDestroy(logWriteBuffer);
			ExitThread(ERROR_SUCCESS);
		}
			

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
	ls->logCount	= 0;
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
		"< %s >\n[ %dh : %dm : %ds : %dms ]\n",
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

	CTFileClose(logFile);

	return ls;

}

CTCALL	BOOL				CTLogStreamDestroy(PCTLogStream stream) {

	CTLockEnter(__ctlog->lock);

	if (stream == NULL) {

		CTErrorSetBadObject("CTLogStreamDestroy failed: stream was NULL");
		CTLockLeave(__ctlog->lock);

		return FALSE;

	}

	CTFree(stream);

	CTLockLeave(__ctlog->lock);
	return TRUE;

}

CTCALL	BOOL				CTLog(PCTLogStream stream, UINT32 logType, PCHAR message) {

	CTLockEnter(__ctlog->lock);

	if (stream == NULL) {

		CTErrorSetBadObject("CTLog failed: stream was NULL");
		CTLockLeave(__ctlog->lock);

		return FALSE;

	}

	if (message == NULL) {

		CTErrorSetBadObject("CTLog failed: message was NULL");
		CTLockLeave(__ctlog->lock);

		return FALSE;

	}

	CTDynListLock(__ctlog->logWriteQueue);

	PCTLogEntry pentry		= CTDynListAdd(__ctlog->logWriteQueue);
	pentry->logStream		= stream;
	pentry->logThreadID		= GetThreadId(GetCurrentThread());
	pentry->logType			= logType;
	pentry->logNumber		= stream->logCount++;
	pentry->logTimeMsecs	= GetTickCount64() - __ctlog->startTimeMsecs;

	strcpy_s(
		pentry->message,
		CT_LOG_MESSAGE_SIZE - 1,
		message
	);

	CTDynListUnlock(__ctlog->logWriteQueue);

	CTLockLeave(__ctlog->lock);

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

	va_end(args, message);

	return logFuncRslt;

}