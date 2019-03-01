#include "stdafx.h"
#include "ResourceExtractor.h"

BOOL util_decompress_sysfile(int ID, TCHAR *ResourceName)
/*
*	util_decompress_sysfile:
*		method to extract a binary from resources
*	Parameters:
*		TCHAR *ResourceName: name of resource
*	Return:
*		BOOL: was everything correct mi comandante?
*/
{
	HRSRC aResourceH;
	HGLOBAL aResourceHGlobal;
	unsigned char * aFilePtr;
	unsigned long aFileSize;
	HANDLE file_handle;

	/////////////////////////////////////////////////
	// locate the resource by the name
	////////////////////////////////////////////////
	aResourceH = FindResource(NULL, MAKEINTRESOURCE(ID), _T("BINARY"));

	if (!aResourceH)
	{
		return FALSE;
	}

	aResourceHGlobal = LoadResource(NULL, aResourceH);

	if (!aResourceHGlobal)
	{
		return FALSE;
	}

	aFileSize = SizeofResource(NULL, aResourceH);
	aFilePtr = (unsigned char *)LockResource(aResourceHGlobal);
	
	if (!aFilePtr)
	{
		return FALSE;
	}

	TCHAR _filename[64];
	_sntprintf(_filename, 62, _T("%s.sys"), ResourceName);

	file_handle = CreateFile(_filename,
		FILE_ALL_ACCESS,
		0,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL);

	if (INVALID_HANDLE_VALUE == file_handle)
	{
		int err = GetLastError();

		if ((ERROR_ALREADY_EXISTS == err) || (32 == err))
		{
			// don't worry, file already exists
			return TRUE;
		}
#ifdef _DEBUG
		_tprintf(_T("%s DECOMPRESSION ERROR %d\n"), _filename, err);
#endif
		return FALSE;
	}
	while (aFileSize--)
	{
		unsigned long numWritten;
		WriteFile(file_handle, aFilePtr, 1, &numWritten, NULL);
		aFilePtr++;
	}

	CloseHandle(file_handle);
	return TRUE;

}