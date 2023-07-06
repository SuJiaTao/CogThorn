//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_base_file.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"

#include <stdio.h>
#include <Shlwapi.h>

CTCALL	BOOL		CTFileExists(PCHAR path) {
	return PathFileExistsA(path);
}

CTCALL	PCTFile		CTFileCreate(PCHAR path) {
	
	if (path == NULL) {
		CTErrorSetBadObject("CTFileCreate failed: path was NULL");
		return NULL;
	}

	PCTFile file	= CTAlloc(sizeof(*file));
	file->hFile = CreateFileA(
		path,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	DWORD lastErr = GetLastError();
	if (lastErr != 0 &&
		lastErr != ERROR_ALREADY_EXISTS) {
		CTErrorSetFunction("CTFileCreate failed");
		CTFree(file);
		return NULL;
	}

	strcpy_s(
		file->fileName,
		CT_FILENAME_MAX_LENGTH - 1,
		path
	);

	return file;
}

CTCALL	BOOL		CTFileDelete(PCHAR path) {

	if (path == NULL) {
		CTErrorSetBadObject("CTFileDelete failed: path was NULL");
		return FALSE;
	}

	BOOL rslt = DeleteFileA(path);

	if (rslt == FALSE) {
		CTErrorSetFunction("CTFileDelete failed");
	}

	return rslt;
}

CTCALL	PCTFile		CTFileOpen(PCHAR path) {

	if (path == NULL) {
		CTErrorSetBadObject("CTFileOpen failed: path was NULL");
		return NULL;
	}

	PCTFile file = CTAlloc(sizeof(*file));
	file->hFile = CreateFileA(
		path,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	DWORD lastErr = GetLastError();
	if (lastErr != 0) {
		CTErrorSetFunction("CTFileOpen failed");
		CTFree(file);
		return NULL;
	}

	strcpy_s(
		file->fileName,
		CT_FILENAME_MAX_LENGTH - 1,
		path
	);

	return file;
}

CTCALL	BOOL		CTFileClose(PCTFile* pFile) {
	if (pFile == NULL) {
		CTErrorSetBadObject("CTFileClose failed: pFile was NULL");
		return FALSE;
	}

	PCTFile file = *pFile;

	if (file == NULL) {
		CTErrorSetBadObject("CTFileClose failed: file was NULL");
		return FALSE;
	}

	CloseHandle(file->hFile);
	CTFree(file);

	*pFile = NULL;
	return TRUE;
}

CTCALL	SIZE_T		CTFileSize(PCTFile file) {
	if (file == NULL) {
		CTErrorSetBadObject("CTFileSize failed: file was NULL");
		return 0;
	}

	return GetFileSize(file->hFile, NULL);
}

CTCALL	BOOL		CTFileRead(PCTFile file, PVOID buffer, SIZE_T offset, SIZE_T sizeBytes) {
	if (file == NULL) {
		CTErrorSetBadObject("CTFileRead failed: file was NULL");
		return FALSE;
	}
	if (buffer == NULL) {
		CTErrorSetParamValue("CTFileRead failed: buffer was NULL");
		return FALSE;
	}

	OVERLAPPED readOffset	= { 0 };
	readOffset.Offset		= offset;
	BOOL rslt = ReadFile(
		file->hFile,
		buffer,
		sizeBytes,
		NULL,
		&readOffset
	);

	if (rslt == FALSE) {
		CTErrorSetFunction("CTFileRead failed");
		return FALSE;
	}

	return TRUE;
}

CTCALL	BOOL		CTFileWrite(PCTFile file, PVOID buffer, SIZE_T offset, SIZE_T sizeBytes) {
	if (file == NULL) {
		CTErrorSetBadObject("CTFileWrite failed: file was NULL");
		return FALSE;
	}
	if (buffer == NULL) {
		CTErrorSetParamValue("CTFileWrite failed: buffer was NULL");
		return FALSE;
	}

	OVERLAPPED readOffset	= { 0 };
	readOffset.Offset		= offset;

	BOOL rslt = WriteFile(
		file->hFile,
		buffer,
		sizeBytes,
		NULL,
		&readOffset
	);

	if (rslt == FALSE) {
		CTErrorSetFunction("CTFileWrite failed");
		return FALSE;
	}

	return TRUE;
}